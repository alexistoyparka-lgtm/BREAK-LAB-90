#!/usr/bin/env python3
"""Validate the shipped bank inventory, provenance, format and preset bindings."""
import hashlib
import json
from pathlib import Path
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]


class BankAssets(unittest.TestCase):
    def test_wav_inventory_and_preset_bindings(self):
        folder = ROOT / 'engine/breaks'
        entries = json.loads((folder / 'bank.json').read_text())['selection']
        self.assertEqual([e['index'] for e in entries], list(range(1, 11)))
        self.assertEqual(len({e['name'] for e in entries}), 10)
        self.assertEqual(len({e['sha256'] for e in entries}), 10)
        programs = json.loads((ROOT / 'presets.json').read_text())['presets']
        self.assertEqual([p['name'] for p in programs[:5]],
                         ['Clean Break', 'Dusty 12-Bit', 'Dark Warehouse', 'Reverse Texture', 'Paula Jungle'])
        self.assertEqual(len(programs), 15)
        options = json.loads((ROOT / 'vst/params.json').read_text())['params'][18]['options']
        self.assertEqual(options, ['Original Break'] + [e['name'] for e in entries])
        decoded = 0
        for i, e in enumerate(entries):
            path = folder / e['file']
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), e['sha256'])
            with wave.open(str(path)) as wav:
                self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate()), (2, 2, 44100))
                self.assertEqual(wav.getnframes(), e['frames'])
                decoded += wav.getnframes() * 8
            preset = programs[i + 5]
            self.assertEqual((preset['name'], preset['values']['break_select']), (e['name'], e['index']))
            self.assertFalse(any(k.startswith('pad_') for k in preset['values']))
        with wave.open(str(folder / 'break01.wav')) as wav:
            decoded += wav.getnframes() * 8
        self.assertLessEqual(decoded, 64 * 1024 * 1024)
        for index in range(11):
            self.assertTrue((ROOT / f'skin/assets/wave_{index:02d}.svg').is_file())


if __name__ == '__main__':
    unittest.main()
