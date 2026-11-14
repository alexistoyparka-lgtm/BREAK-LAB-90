#!/usr/bin/env python3
"""Integration and rejection tests using the actual ARMv7 build archive."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('package_usb', ROOT / 'tools/package_usb.py')
pkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pkg)
SOURCE = Path(sys.argv.pop(1))


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.out = Path(self.tmp.name)
        self.original = pkg.read_zip(SOURCE)
        self.top = next(n.split('/')[0] for n in self.original)
        self.pre = self.top + '/portable/' + pkg.FOLDER + '/'

    def test_both_packages_preserve_every_payload_byte(self):
        manager, sdk = pkg.package(SOURCE, self.out)
        m, s = pkg.read_zip(manager), pkg.read_zip(sdk)
        pkg.check_sums(m, self.top)
        self.assertEqual({n.split('/')[0] for n in s}, {pkg.FOLDER})
        self.assertNotIn(pkg.FOLDER + '/plugin-meta.xml', s)
        for name, data in self.original.items():
            if name.startswith(self.pre) and not name.endswith(('/mpc-plugin.json', '/plugin.json')):
                self.assertEqual(m[name], data)
                relative = name[len(self.pre):]
                if relative == 'plugin-meta.xml':
                    continue
                if relative == pkg.SO:
                    relative = pkg.SDK_SO
                self.assertEqual(s[pkg.FOLDER + '/' + relative], data, relative)
        pj = json.loads(s[pkg.FOLDER + '/plugin.json'])
        for k, v in dict(name='BREAK LAB 90', manufacturer='OpenBench', format='vst2',
                         kind='instrument', uid='0x42726b39', category='Synth',
                         numInputs=0, numOutputs=2, file32=pkg.SDK_SO).items():
            self.assertEqual(pj[k], v)
        self.assertNotIn('file64', pj)
        self.assertEqual(pj['version'], json.loads(m[self.top + '/mpc-plugin.json'])['version'])
        self.assertEqual(m[self.top + '/mpc-plugin.json'], m[self.pre + 'mpc-plugin.json'])
        self.assertLessEqual(tuple(map(int, pkg.check_binary(s[pkg.FOLDER + '/' + pj['file32']]).split('.'))), (2, 29))
        # Deterministic ZIPs across repeated packaging.
        before = (manager.read_bytes(), sdk.read_bytes())
        pkg.package(SOURCE, self.out)
        self.assertEqual(before, (manager.read_bytes(), sdk.read_bytes()))

    def test_parameter_indices_and_identity_stay_locked(self):
        expected = ['slice', 'pitch', 'reverse', 'mode', 'bits', 'rate',
                    'cutoff', 'volume', 'status', 'interpolation',
                    'slice_start', 'slice_end', 'undo']
        self.assertEqual((ROOT / 'params.lock').read_text().splitlines(), expected)
        self.assertEqual([p['key'] for p in json.loads((ROOT / 'vst/params.json').read_text())['params']], expected)
        self.assertEqual(json.loads((ROOT / 'vst.json').read_text())['uid'], 'Brk9')

    def test_tampered_payload_rejected(self):
        data = dict(self.original)
        data[self.pre + pkg.SO] += b'corrupt'
        bad = self.out / 'bad.zip'
        pkg.write_zip(bad, data)
        with self.assertRaisesRegex(ValueError, 'checksum'):
            pkg.package(bad, self.out / 'result')

    def test_wrong_architecture_rejected(self):
        binary = bytearray(self.original[self.pre + pkg.SO])
        binary[4] = 2  # ELF64
        with self.assertRaisesRegex(ValueError, 'ELF32'):
            pkg.check_binary(binary)
        binary[4] = 1
        binary[37] = 2  # soft-float flag
        with self.assertRaisesRegex(ValueError, 'hard-float'):
            pkg.check_binary(binary)

    def test_path_traversal_rejected(self):
        bad = self.out / 'unsafe.zip'
        pkg.write_zip(bad, {'../outside': b'no'})
        with self.assertRaisesRegex(ValueError, 'unsafe'):
            pkg.read_zip(bad)

    def test_input_never_overwritten(self):
        with self.assertRaisesRegex(ValueError, 'overwrite'):
            pkg.package(SOURCE, SOURCE.parent)


if __name__ == '__main__':
    unittest.main()
