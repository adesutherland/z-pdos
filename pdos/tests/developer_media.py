"""Bounded package/encoding controls; no guest, disk or fleet operations."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('developer_media', Path(__file__).resolve().parents[1] / 'scripts/developer-media.py')
media = importlib.util.module_from_spec(spec)
spec.loader.exec_module(media)


class Controls(unittest.TestCase):
    def test_manifest_requires_one_exact_complete_package(self):
        with tempfile.TemporaryDirectory(prefix='pd035-package-') as scratch:
            package = Path(scratch)
            inputs = [str(Path(n) / (n + '.rdw')) for n in media.TOOLS] + ['shared/' + n for n in media.LIBS.values()]
            rows = []
            for i, name in enumerate(inputs):
                path = package / name
                path.parent.mkdir(exist_ok=True)
                path.write_bytes(bytes((i + 1,)))
                rows.append(media.digest(path.read_bytes()) + '  ' + str(path))
            manifest = package / 'batch-inputs.sha256'
            manifest.write_text('\n'.join(rows) + '\n')
            entry = package / 'batch-entry-check.json'
            entry.write_text(json.dumps({'amode': 31, 'profile_bits': 31, 'batch': True}))
            expected = media.digest(manifest.read_bytes())
            self.assertEqual(set(media.package_inputs(package, expected)), set(inputs))
            for wrong in ('', '0' * 64, expected.upper()):
                with self.assertRaises(ValueError):
                    media.package_inputs(package, wrong)
            (package / inputs[-1]).write_bytes(b'changed')
            with self.assertRaises(ValueError):
                media.package_inputs(package, expected)
            (package / inputs[-1]).write_bytes(bytes((len(inputs),)))
            entry.write_text(json.dumps({'amode': 64, 'profile_bits': 64, 'batch': True}))
            with self.assertRaises(ValueError):
                media.package_inputs(package, expected)
            # Existing user output is never opened or modified, even after
            # the package has passed validation.
            entry.write_text(json.dumps({'amode': 31, 'profile_bits': 31, 'batch': True}))
            with self.assertRaises(ValueError):
                media.prepare(package, package, package, package, expected, package)
            self.assertEqual(media.digest(manifest.read_bytes()), expected)

    def test_native_cards_and_source_width(self):
        self.assertEqual(len(media.fixed('say "café!"\n\nreturn 0\n')), 240)
        self.assertEqual(media.fixed('\n'), b'\x40' * 80)
        with self.assertRaises(ValueError):
            media.fixed('x' * 81)
        # Normal media has logical tool names and contains no qualification
        # probes, private Lab paths or LABA01 fixture collection names.
        site = (media.PROFILE / 'BATCH.SITE').read_text()
        for name in media.TOOLS:
            self.assertIn(name + ' ' + name + '\n', site)
        for marker in ('BTEST31', 'BADDR31', 'LABA01', '/Users/', 'BRXC'):
            self.assertNotIn(marker, site)
        self.assertIn('MAINSIZE 512', (media.PROFILE / 'hercules.cnf').read_text())


if __name__ == '__main__':
    unittest.main(verbosity=2)
