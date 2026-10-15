"""Host-only failure controls for the physical console/media interfaces."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock,patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'batch'))
import console_machine as cm
import jcl_fixture as jf
import forms_recovery_actor as recovery

class Controls(unittest.TestCase):
    def test_complete_fixture_valid_on_physical_line(self):
        cases=json.loads(Path('pdos/tests/batch/forms-complete-commands.json').read_text())
        cm.validate_responses(cases,True,True,True,True)
        with self.assertRaises(ValueError):cm.validate_responses(cases,True,False,True,True)

    def test_invalid_actions_fail_before_guest_setup(self):
        for text,source,forms,complete,line,monitor in (
                ('printer-offline','forms-device',True,False,False,True),
                ('detach 01B9','forms-device',True,True,False,True),
                ('Clear()','primary-reconnect',True,False,True,True),
                ('','primary-reconnect',True,False,False,False),
                ('PF(8)','key',True,True,True,True),
                ('String(secret)','key',True,True,False,True),
                ('','typo-device',True,True,False,True)):
            with self.subTest(source=source,text=text):
                with self.assertRaises(ValueError):cm.validate_responses([['BATCH','PCOMM END 1 RC=0',[['READY',text,source]]]],forms,complete,line,monitor)

    def test_recovery_fixture_is_screen_only(self):
        cases=json.loads(Path('pdos/tests/batch/forms-recovery-commands.json').read_text())
        cm.validate_responses(cases,True,False,False,True)
        with self.assertRaises(ValueError):cm.validate_responses(cases,True,False,True,True)

    def test_only_fresh_primary_connected_records_count(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'log';p.write_text('HHC01018I 0:0009 COMM: client old devtype3270: connected\nHHC01019I 0:0009 COMM: client old: disconnected\nHHC01018I 0:000A COMM: client monitor: connected\n')
            self.assertEqual(cm.primary_connections(p),1)
            with p.open('a') as f:f.write('HHC01018I 0:0009 COMM: client new devtype3270: connected\n')
            self.assertEqual(cm.primary_connections(p),2)

    def test_dead_script_endpoint_still_kills_and_reaps(self):
        proc=Mock();proc.wait.side_effect=[subprocess.TimeoutExpired('s3270',10),subprocess.TimeoutExpired('s3270',5),0]
        with patch.object(cm,'terminal_action',side_effect=OSError('script closed')):cm.close_terminal(proc,123)
        proc.terminate.assert_called_once();proc.kill.assert_called_once();self.assertEqual(proc.wait.call_count,3)

    def test_failed_new_terminal_setup_is_reaped(self):
        a=SimpleNamespace(colour=False,model=5,oversize=None,basic=False,stage3=None);child=Mock()
        with tempfile.TemporaryDirectory() as td,patch.object(cm.subprocess,'Popen',return_value=child) as popen,patch.object(cm,'until',side_effect=TimeoutError('no script')),patch.object(cm,'close_terminal') as close:
            with self.assertRaises(TimeoutError):cm.start_terminal(a,Path(td),Mock(),Path(td)/'log',123,124,'3270.trace')
            self.assertEqual(popen.call_args.args[0][2],'3278-5');close.assert_called_once_with(child,124)

    def test_full_observation_uses_ASCII_and_physical_attributes(self):
        values={'Ascii()':'BATCH / OUTPUT / REVISION 18\nRIGHT EDGE PD033\n','ReadBuffer(Ascii)':'SF(c0=f8) SA(42=f4)','Query(ScreenCurSize)':'24 80\n','Query(Cursor)':'18 4\n'}
        def query(port,action):return SimpleNamespace(returncode=0,stdout=values[action])
        with tempfile.TemporaryDirectory() as td,patch.object(cm,'terminal_action',side_effect=query):
            out=Path(td);v=cm.form_observation(out,123,'REVISION 18',1,2)
            self.assertTrue(v['has_colour']);self.assertEqual(v['geometry'],'24 80');self.assertEqual(v['epoch'],2)
            self.assertIn('RIGHT EDGE PD033',(out/v['screen']).read_text())
        with tempfile.TemporaryDirectory() as td,patch.object(cm,'terminal_action',return_value=SimpleNamespace(returncode=1,stdout='')):
            with self.assertRaises(RuntimeError):cm.form_observation(Path(td),123,'READY',1,0)
            self.assertEqual(list(Path(td).iterdir()),[])

    def test_owner_site_changes_only_derived_mapping(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td);fixtures=root/'fixtures';fixtures.mkdir();site=jf.fixed('PD033 SITE 1\nBTEST31 BTEST31\nBADDR31 BADDR31\n')
            (fixtures/'SITE.fb80').write_bytes(site);(fixtures/'SYSIN.fb80').write_bytes(jf.fixed('input\n'))
            for owner in (False,True):
                media=root/str(owner);media.mkdir();rows=['PDOS00 3390-1 150']+[f'PD4.{n} EMPTY TRK 1 1 0 PS VB 260 264' for n in ('OBJ','LINK','AOBJ','POBJ','BYTES','RESULT')]+['PDOS.STORE SEQ store.bin CYL 42 1 0 PS F 18452 18452']
                (media/'ctl.txt').write_text('\n'.join(rows)+'\n');jf.prepare(media,fixtures,Path('forms-owner') if owner else None)
                self.assertEqual((fixtures/'SITE.fb80').read_bytes(),site)
                expected=site.replace(jf.fixed('BTEST31 BTEST31\n'),jf.fixed('BTEST31 FOWNER\n')) if owner else site
                self.assertEqual((media/'SITE.fb80').read_bytes(),expected)
                self.assertEqual((media/'OWNER.fb80').exists(),owner)
                if owner:self.assertIn("PARM='BATCH'",jf.OWNER_JCL)

    def test_durable_spool_and_named_residue_controls(self):
        import hashlib
        with tempfile.TemporaryDirectory() as td:
            out=Path(td);(out/'job-spool.bin').write_bytes(b'actual stopped spool');good={'kind':0x50444a32,'erased':False,'sha256':hashlib.sha256(b'actual stopped spool').hexdigest()}
            self.assertTrue(all(recovery.stored(out,[good]).values()))
            self.assertFalse(recovery.stored(out,[dict(good,sha256='0'*64)])['recovery_durable_spool_matches'])
            self.assertFalse(recovery.stored(out,[good,{'kind':0x54534f,'erased':False}])['recovery_no_named_or_temporary_output'])
        self.assertEqual(len(recovery.EDGE),132)

if __name__=='__main__':unittest.main()
