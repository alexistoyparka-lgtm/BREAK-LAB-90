#!/usr/bin/env python3
"""Add the OS 3.10.1 discovery descriptor and emit USB + Plugin Manager ZIPs.

The build/wrapper pin stays unchanged. Descriptor and SDK layout follow OpenMPC
release.py/sdk_zip.py at 52c1fb072966d7de77e97dd9eec0544548b042dc (build #79).
No installer is executed. Standard library plus binutils readelf only.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import zipfile

FOLDER = 'OpenBench - VST - BREAK LAB 90'
SO = 'break_lab_90.so'
SDK_SO = 'break_lab_90-arm32.so'
UID = '42726b39'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def encode(value):
    return (json.dumps(value, indent=2) + '\n').encode()


def read_zip(path):
    with zipfile.ZipFile(path) as z:
        files = {}
        for i in z.infolist():
            if i.is_dir():
                continue
            p = PurePosixPath(i.filename)
            if p.is_absolute() or '..' in p.parts or '\\' in i.filename or i.filename in files:
                raise ValueError('unsafe or duplicate archive member: ' + i.filename)
            if (i.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('symlinks are not supported')
            files[i.filename] = z.read(i)
        return files


def write_zip(path, files):
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = (0o100755 if name.endswith('.sh') else 0o100644) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data)


def check_binary(data):
    import tempfile
    if (data[:7] != b'\x7fELF\x01\x01\x01' or len(data) < 52 or
            struct.unpack_from('<HH', data, 16) != (3, 40) or
            struct.unpack_from('<I', data, 36)[0] & 0xff000600 != 0x05000400):
        raise ValueError('expected ELF32 ARM EABI5 hard-float shared library')
    with tempfile.NamedTemporaryFile(suffix='.so') as f:
        f.write(data)
        f.flush()
        versions = subprocess.check_output(['readelf', '--version-info', f.name], text=True)
        symbols = subprocess.check_output(['readelf', '--dyn-syms', '--wide', f.name], text=True)
        attributes = subprocess.check_output(['readelf', '-A', f.name], text=True)
    glibc = [tuple(map(int, v.split('.'))) for v in re.findall(r'Name: GLIBC_([0-9.]+)', versions)]
    if not glibc or max(glibc) > (2, 29):
        raise ValueError('required GLIBC exceeds 2.29 or could not be determined')
    if 'VSTPluginMain' not in symbols or 'Tag_CPU_arch: v7' not in attributes:
        raise ValueError('missing VST entry point or ARMv7 attributes')
    return '.'.join(map(str, max(glibc)))


def check_sums(files, top):
    expected = {}
    for line in files[top + '/SHA256SUMS'].decode().splitlines():
        sha, name = line.split('  ', 1)
        if name in expected:
            raise ValueError('duplicate checksum')
        expected[name] = sha
    actual = {n[len(top)+1:]: digest(d) for n, d in files.items()
              if n.startswith(top + '/') and n != top + '/SHA256SUMS'}
    if expected != actual:
        raise ValueError('archive checksum mismatch or incomplete checksum inventory')


def package(source, output):
    files = read_zip(source)
    manifests = [n for n in files if n.count('/') == 1 and n.endswith('/mpc-plugin.json')]
    if len(manifests) != 1:
        raise ValueError('expected one top-level manager manifest')
    top = manifests[0].split('/')[0]
    if any(not n.startswith(top + '/') for n in files):
        raise ValueError('unexpected top-level files')
    check_sums(files, top)
    manifest = json.loads(files[manifests[0]])
    if (manifest['name'], manifest['manufacturer'], manifest['uid'].lower(), manifest['arch'],
        manifest['so'], manifest['folder']) != ('BREAK LAB 90', 'OpenBench', UID, 'armv7', SO, 'portable/' + FOLDER):
        raise ValueError('unexpected plugin identity, architecture or layout')
    pre = top + '/portable/' + FOLDER + '/'
    max_glibc = check_binary(files[pre + SO])
    for name in ('Plugin Skins/TUI.json', 'Plugin Skins/Q-Links.json', 'engine/breaks/break01.wav', 'version.xml'):
        if not files.get(pre + name):
            raise ValueError('missing payload: ' + name)
    pj = dict(name='BREAK LAB 90', manufacturer='OpenBench', version=manifest['version'],
              format='vst2', kind='instrument', uid='0x' + UID, category='Synth',
              numInputs=0, numOutputs=2, file32=SO)
    for key, value in [('description', manifest.get('about')), ('license', manifest.get('license')),
                       ('url', 'https://github.com/' + manifest['source_repo'] if manifest.get('source_repo') else None)]:
        if value:
            pj[key] = value
    files[pre + 'plugin.json'] = encode(pj)
    manifest.update(vst2=True, max_glibc=max_glibc)
    files[manifests[0]] = files[pre + 'mpc-plugin.json'] = encode(manifest)
    files[top + '/SHA256SUMS'] = ''.join(
        digest(d) + '  ' + n[len(top)+1:] + '\n' for n, d in sorted(files.items())
        if n != top + '/SHA256SUMS').encode()
    sdk = {FOLDER + '/' + n[len(pre):]: d for n, d in files.items()
           if n.startswith(pre) and n[len(pre):] not in (SO, 'plugin-meta.xml', 'plugin.json', 'mpc-plugin.json')}
    pj['file32'] = SDK_SO
    sdk[FOLDER + '/plugin.json'] = encode(pj)
    sdk[FOLDER + '/' + SDK_SO] = files[pre + SO]
    universal = dict(manifest, arch='universal', builds={'armv7': {'max_glibc': max_glibc}})
    universal.pop('max_glibc')
    sdk[FOLDER + '/mpc-plugin.json'] = encode(universal)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    manager_path = output / (top + '-mpc-armv7.zip')
    sdk_path = output / (top + '-mpc-sdk.zip')
    if Path(source).resolve() in (manager_path.resolve(), sdk_path.resolve()):
        raise ValueError('output must not overwrite the input archive')
    write_zip(manager_path, files)
    write_zip(sdk_path, sdk)
    return manager_path, sdk_path


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source', type=Path)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    for path in package(args.source, args.output):
        print(digest(path.read_bytes()), path.name)
