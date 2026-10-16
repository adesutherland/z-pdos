"""Cheap fixture controls. No guest, media staging, lease or target build."""
import ast
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

from payload import BLOCK, ROOT, envelope, prepare, verify
from support import CODED, MARKER, SEED, SAVED, address, encode, decode, field_contract, named, native_records, seed_records, saved_records, write_frame


class Controls(unittest.TestCase):
    def refused(self, function, *args):
        with self.assertRaises((ValueError, KeyError)):
            function(*args)

    def test_python_syntax_and_import_safety(self):
        for source in Path(__file__).resolve().parent.glob('*.py'):
            ast.parse(source.read_text(), filename=str(source))
        self.refused(prepare, Path('/tmp'), Path('/tmp/not-an-editor'))
        self.refused(verify, Path('/tmp'), Path('/tmp'), Path('/tmp'))
        from run import expected_keys
        for cell in ('model2','model5','mono','restart','line'):
            self.assertIn('expected_check_keys',expected_keys(cell))
            self.assertIn('listener_sockets_closed',expected_keys(cell))

    def test_current_envelope(self):
        data = (ROOT / 'build/pdos/batch-sdk/apps/edit.bin').read_bytes()
        self.assertGreater(envelope(data)['raw_bytes'], 644)
        for offset in (0, 8, 12, 16, 20, 24, 65, len(data)-1):
            bad = bytearray(data); bad[offset] ^= 1
            self.refused(envelope, bad)
        self.refused(envelope, data[:-1])
        self.refused(envelope, data + bytes(BLOCK))

    def test_line_refusal_precedes_transport(self):
        from terminal import Line
        class UnusedClient:
            def text(self):
                raise AssertionError('invalid command reached transport')
            def enter(self, text):
                raise AssertionError('invalid command was sent')
        line=Line(UnusedClient(),None,'/tmp/unused-editor-control')
        for call in (line.command,line.shell):
            for text in ('café!','x'*149,'one\ntwo','tab\t','\x7f'):
                self.refused(call,text)
        for text in ('',' '*148,'PRINT 12 12'):
            self.assertIsNone(Line.validate(text))

    def test_exact_seed_and_changed_marker(self):
        source, saved = seed_records(), saved_records()
        self.assertEqual(len(source), 35)
        self.assertTrue(all(len(x)==80 for x in source+saved))
        self.assertEqual([i for i,(a,b) in enumerate(zip(source,saved),1) if a!=b], [12])
        self.assertIn('PD033 EDITOR LIBRARY', decode(saved[11]))
        self.assertEqual(MARKER, 'PD033 EDITOR LIBRARY new payload')
        self.assertEqual(len(b''.join(saved)), 2800)

    def test_fb_vb_boundaries(self):
        self.assertEqual(native_records({'native_blocks_hex': ['c1404040c2404040']},'FB',4,8),[bytes.fromhex('c1404040'),bytes.fromhex('c2404040')])
        parts = [b'', encode('  padded  '), encode('café!')]
        body = b''.join(struct.pack('>HH',len(p)+4,0)+p for p in parts)
        block = struct.pack('>HH',len(body)+4,0)+body
        self.assertEqual(native_records({'native_blocks_hex':[block.hex()]},'VB',256,18452),parts)
        for bad in (block[:-1], b'\x00\x08\x00\x00\x00\x03\x00\x00', b'\x00\x08\x00\x01\x00\x04\x00\x00'):
            self.refused(native_records, {'native_blocks_hex':[bad.hex()]},'VB',256,18452)
        self.refused(native_records, {'native_blocks_hex':['c14040']},'FB',4,8)
        binary=bytes(range(256));block=struct.pack('>HHHH',264,0,260,0)+binary
        self.assertEqual(native_records({'native_blocks_hex':[block.hex()]},'VB',260,264),[binary])
        self.refused(native_records,{'native_blocks_hex':[block.hex()]},'VB',256,264)

    def test_addresses_and_native_keys(self):
        for value in (0, 80, 1920, 3563):
            self.assertEqual(address(bytes((CODED[value//64],CODED[value%64]))),value)
            self.assertEqual(address(bytes((value//256,value%256))),value)
        self.refused(address,b'\xff\xff')
        key=encode('LABA01.B33.SRC'.ljust(44)+'EDSAVE'.ljust(8)).hex()
        item={'kind':0x54534f,'erased':False,'key_hex':key}
        self.assertEqual(set(named([item])),{SAVED})
        self.refused(named,[item,item])

    def test_actual_renderer_oracle_and_probe_c89(self):
        with tempfile.TemporaryDirectory(prefix='pdos-editor-control-') as scratch:
            exe=Path(scratch)/'paint'
            flags=['clang','-std=c89','-Wall','-Wextra','-Werror','-Wno-deprecated-declarations','-pedantic','-I',str(ROOT/'pdos/src')]
            subprocess.run(flags+['-fsyntax-only',str(ROOT/'pdos/tests/editor/probe.c')],check=True)
            sources=['pdos/tests/editor/paint.c','pdos/src/apps/edit_core.c','pdos/src/apps/edit_text.c',
                     'pdos/src/apps/edit_session.c','pdos/src/apps/edit_screen.c','pdos/src/twospace_3270.c']
            subprocess.run(flags+['-fsanitize=address,undefined']+[str(ROOT/p) for p in sources]+['-o',str(exe)],check=True)
            for model,rows,cols in ((2,24,80),(5,27,132)):
                record=subprocess.check_output([str(exe),str(model)])
                frame=write_frame(record,rows,cols)
                self.assertTrue(frame['unlocked'] and field_contract(frame,rows,cols))
                self.assertEqual(frame['cursor'],(rows-3)*cols+7)
                self.assertEqual(frame['cells'][2*cols+10],encode('A')[0])
                self.assertTrue(all(frame['cells'][3*cols+i]==0 for i in range(10,cols-1)))
                frame['fields'][2*cols+9]|=0x20
                self.assertFalse(field_contract(frame,rows,cols))
                self.refused(write_frame,record[:-1],rows,cols)


if __name__ == '__main__':
    unittest.main(verbosity=2)
