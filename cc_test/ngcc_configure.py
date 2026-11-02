#!/usr/bin/env python3
"""Check configuration semantics without executing author algorithms."""
import hashlib
import json
import os
import platform
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    checks = []
    with tempfile.TemporaryDirectory(prefix='ngcc-configure-') as directory:
        root = Path(directory)
        (root/'src').mkdir()
        (root/'ngcc/metadata').mkdir(parents=True)
        for name in ('config', 'Makefile.in'):
            shutil.copy2(ROOT/name, root/name)
        for name in ('registered.json', 'tls-groups.json', 'sig-oids.json'):
            shutil.copy2(ROOT/'ngcc/metadata'/name, root/'ngcc/metadata'/name)
        shutil.copytree(ROOT/'make', root/'make')
        rows = json.loads((root/'ngcc/metadata/registered.json').read_text())
        all_names = {row['provider_name'] for row in rows}
        gmp_names = {row['provider_name'] for row in rows if '-lgmp' in row['LDLIBS'].split()}
        default_names = all_names - gmp_names
        assert len(gmp_names) == 19 and len(default_names) == 201
        x86_linux_names = {
            'kem-04-bag-piglet-128', 'kem-04-bag-piglet-256',
            'kem-06-bra-128', 'kem-06-bra-256',
            'kem-07-brqc-128', 'kem-07-brqc-256',
            'kem-10-cmultiurag-128', 'kem-10-cmultiurag-256',
            'sig-32-uvw-128', 'sig-32-uvw-256',
        }
        kem, sig = 'kem-08-bw-kem-128', 'sig-01-aigis-sig1'

        def configure(options, expected=None, fail=False, environment=None):
            paths = [root/p for p in ('Makefile', 'src/ngcc_config.h', 'configdata.json')]
            before = [p.read_bytes() if p.exists() else None for p in paths]
            env = {key: value for key, value in os.environ.items()
                   if not key.startswith('GMP_') and key != 'OPENSSL_PREFIX'}
            env.update(environment or {})
            run = subprocess.run([str(root/'config'), *options], cwd=root, env=env,
                                 text=True, capture_output=True)
            if fail:
                assert run.returncode != 0, options
                assert before == [p.read_bytes() if p.exists() else None for p in paths], options
            else:
                assert run.returncode == 0, run.stderr
                if expected is not None:
                    data = json.loads((root/'configdata.json').read_text())
                    assert set(data['enabled']) == expected, options
                    header = (root/'src/ngcc_config.h').read_text()
                    for row in rows:
                        macro = 'NGCC_NO_'+row['provider_name'].upper().replace('-', '_')
                        assert (macro in header.split()) == (row['provider_name'] not in expected)
                    listing = subprocess.check_output(['make', '-s', 'print-instances'], cwd=root, text=True)
                    assert set(listing.splitlines()) == {r['id']+'/'+r['label'] for r in rows
                                                       if r['provider_name'] in expected}
            checks.append({'options': options, 'environment': environment or {},
                           'expected_failure': fail, 'passed': True})
            return run

        x86_default = x86_linux_names - gmp_names
        native_linux_x86 = (platform.system() == 'Linux'
                            and platform.machine() in ('x86_64', 'amd64'))
        native_default = default_names if native_linux_x86 else default_names - x86_default
        native_all = all_names if native_linux_x86 else all_names - x86_linux_names
        configure([], native_default)
        configure(['linux-x86_64'], default_names)
        configure(['darwin64-arm64-cc'], default_names - x86_default)
        configure(['linux-aarch64'], default_names - x86_default)
        configure(['OPENSSL_PREFIX=/custom/openssl'], native_default)
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['OPENSSL_PREFIX'] == '/custom/openssl'
        configure([], native_default, environment={'OPENSSL_PREFIX': '/custom/tongsuo'})
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['OPENSSL_PREFIX'] == '/custom/tongsuo'
        configure(['OPENSSL_PREFIX=/custom/openssl'], native_default,
                  environment={'OPENSSL_PREFIX': '/custom/tongsuo'})
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['OPENSSL_PREFIX'] == '/custom/openssl'
        for option in ('OPENSSL_PREFIX=', 'OPENSSL_PREFIX=/path with spaces',
                       'TONGSUO_ROOT=/old/path', 'OPENSSL_ROOT=/old/path'):
            configure([option], fail=True)
        configure(['GMP_ROOT=/installed'], native_all)
        configure(['linux-x86_64', 'GMP_ROOT=/installed'], all_names)
        configure(['darwin64-arm64-cc', 'GMP_ROOT=/installed'], all_names - x86_linux_names)
        configure(['GMP_STATIC=/installed/lib/libgmp.a'], native_all)
        configure([], native_all, environment={'GMP_ROOT': '/installed'})
        configure([], native_default)
        for option in ('--help', '--list', '--list-algorithms'):
            files = [root/p for p in ('Makefile', 'src/ngcc_config.h', 'configdata.json')]
            before = [p.read_bytes() for p in files]
            configure([option])
            assert before == [p.read_bytes() for p in files]
        configure(['no-'+kem], native_default-{kem})
        configure(['no-kem-08-bw-kem-128', 'no-'+sig], native_default-{kem, sig})
        configure(['GMP_ROOT=/installed', 'no-'+kem], native_all-{kem})
        configure(['GMP_ROOT=/installed', *['no-'+name for name in sorted(gmp_names)]], native_default)
        for name in ('kem-31-ngcc-1', 'sig-25-sqisign2dsq-l1-eff-compressed',
                     'sig-26-sqisign2d-lvl1', 'sig-27-sqisigntriangle-lvl1'):
            configure(['enable-'+name], fail=True)
        configure(['enable-'+kem, 'enable-'+sig], {kem, sig})
        configure(['enable-KEM-08-BW-KEM-128', 'enable-'+kem], {kem})
        configure(['enable-'+sig, 'no-'+kem], {sig})
        for alias in ('sig-25-sqisign2dsq-l1-eff-compressed',):
            sigs = json.loads((root/'ngcc/metadata/sig-oids.json').read_text())['sigs']
            row = next(r for r in sigs if r['tls_name'] == alias)
            configure(['GMP_ROOT=/installed', 'enable-'+alias], {row['provider_name']})
        configure(['enable-chinith-ublockith-d3-256f'], {'sig-05-ublockith-d3-256f'})
        for name in sorted(all_names):
            if name.startswith(('sig-25-sqisign2dsq-', 'kem-08-bw-kem-',
                                'kem-25-nev-128', 'kem-25-nev-256',
                                'kem-34-rudraksh2-', 'kem-41-zen-')):
                options = ['GMP_ROOT=/installed'] if name in gmp_names else []
                configure(options+['enable-'+name], {name})
                configure(['no-'+name], native_default-{name})
        for options in (['enable-'+kem, 'no-kem-08-bw-kem-128'], ['enable-unknown'],
                        ['enable-chinith-ublockith-d3-256f', 'no-sig-05-ublockith-d3-256f'],
                        ['enable-kem-08-bw-kem-c128'],
                        ['enable-sig-25-sqisign2dsquare-level1-eff-compressed'],
                        ['enable-sqisign2dsq-l1-eff-compressed'],
                        ['no-unknown'], ['unsupported-target'], ['--unknown'],
                        ['linux-aarch64', 'linux-x86_64'],
                        ['no-'+name for name in sorted(all_names)], ['CFLAGS=$(bad)']):
            configure(options, fail=True)
        for target in ('darwin64-arm64-cc', 'darwin64-x86_64-cc', 'linux-aarch64', 'linux-x86_64'):
            configure([target, 'enable-'+kem], {kem})
            data = json.loads((root/'configdata.json').read_text())
            assert data['target'] == target
            default_prefix = ('/opt/homebrew/opt/openssl@3' if target == 'darwin64-arm64-cc'
                              else '/usr/local/opt/openssl@3' if target == 'darwin64-x86_64-cc'
                              else '/usr')
            assert data['variables']['OPENSSL_PREFIX'] == default_prefix
            makefile = (root/'Makefile').read_text()
            assert 'PLATFORM := '+('Darwin' if target.startswith('darwin') else 'Linux') in makefile
            probe = subprocess.run(['cc', '-fsyntax-only', '-x', 'c', '-include',
                                    str(root/'src/ngcc_config.h'), '/dev/null'], capture_output=True)
            native_arch = 'aarch64' if platform.machine() in ('arm64', 'aarch64') else platform.machine()
            target_arch = 'aarch64' if data['arch'] == 'arm64' else data['arch']
            native = data['platform'] == platform.system() and target_arch == native_arch
            assert (probe.returncode == 0) == native, (target, probe.stderr.decode())
        configure(['enable-'+kem, 'CC=clang', 'CFLAGS=-O0 -g'], {kem})
        stamps = [(root/p).stat().st_mtime_ns for p in ('Makefile', 'src/ngcc_config.h', 'configdata.json')]
        configure(['enable-'+kem, 'CC=clang', 'CFLAGS=-O0 -g'], {kem})
        assert stamps == [(root/p).stat().st_mtime_ns for p in ('Makefile', 'src/ngcc_config.h', 'configdata.json')]

        # Only validate configuration here; a real GMP build is tested separately.
        gmp_source = root/'gmp-source'
        (gmp_source/'mpz').mkdir(parents=True)
        (gmp_source/'gmp-h.in').touch()
        (gmp_source/'configure').write_text('#!/bin/sh\nexit 0\n')
        (gmp_source/'configure').chmod(0o755)
        configure(['--with-gmp-source=gmp-source'], native_all)
        configure(['--with-gmp-source=gmp-source', 'enable-'+kem], {kem})
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['GMP_SOURCE'] == str(gmp_source.resolve())
        database = subprocess.check_output(['make', '-pn', 'print-instances'], cwd=root, text=True)
        assert next(line.strip() for line in database.splitlines()
                    if line.startswith('GMP_LIBRARY :=')) == 'GMP_LIBRARY :='
        configure(['GMP_SOURCE=gmp-source', 'enable-kem-31-ngcc-1'], {'kem-31-ngcc-1'})
        database = subprocess.check_output(['make', '-pn', 'print-instances'], cwd=root, text=True)
        assert next(line.strip() for line in database.splitlines()
                    if line.startswith('GMP_LIBRARY :=')) == 'GMP_LIBRARY := '+str(root.resolve()/'build/gmp/.libs/libgmp.a')
        for options in (['--with-gmp-source='], ['--with-gmp-source=missing'],
                        ['--with-gmp-source=gmp-source', 'GMP_ROOT=/installed'],
                        ['--with-gmp-source=gmp-source', 'GMP_STATIC=/installed/libgmp.a'],
                        ['GMP_HOST=bad triplet']):
            configure(options, fail=True)
        (gmp_source/'config.status').touch()
        configure(['--with-gmp-source=gmp-source'], fail=True)
        (gmp_source/'config.status').unlink()
        (root/'build').mkdir(exist_ok=True)
        shutil.copytree(gmp_source, root/'build/gmp-source')
        configure(['--with-gmp-source=build/gmp-source'], fail=True)
        configure([], native_default)

        # A tiny pinned ZIP exercises the same importer without copying proposals.
        candidates=root/'ngcc/candidates'
        (candidates/'provenance').mkdir(parents=True)
        (candidates/'.gitignore').write_text('*\n!*/\n')
        archive=root/'author sources.zip'
        fixtures={'kem-01/fixture source.c': ('submission/优化/source.c', b'original\r\n\x00bytes\n'),
                  'kem-01/header.h': ('submission/header.h', b'#define AUTHOR 1\n')}
        fixtures['kem-01/alias.c']=fixtures['kem-01/fixture source.c']
        with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED) as z:
            z.writestr('unselected.txt',b'not part of the import')
            for member,data in dict.fromkeys(fixtures.values()):z.writestr(member,data)
        archive_bytes=archive.read_bytes()
        archive_sha=hashlib.sha256(archive_bytes).hexdigest()
        origins={'archives':{archive_sha:{'path':root.name+'/'+archive.name,
            'files':[[name,member] for name,(member,_) in fixtures.items()]}}}
        manifest=''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n'
                         for name,(_,data) in fixtures.items())
        (root/'make/optimized-origins.json').write_text(json.dumps(origins))
        (root/'make/optimized-sources.sha256').write_text(manifest)

        # The same EVP names select different author trees, with explicit fallback.
        for target in ('darwin64-arm64-cc', 'darwin64-x86_64-cc', 'linux-aarch64', 'linux-x86_64'):
            # Use the catalog's canonical BAG-Loong name rather than a spelling alias.
            names = {'kem-01-aigis-enc1', next(r['provider_name'] for r in rows if r['id'] == 'kem-03')}
            configure([target, '--implementation=optimized', *['enable-'+n for n in sorted(names)]], names)
            data = json.loads((root/'configdata.json').read_text())
            assert data['implementation'] == 'optimized'
            variant = 'optimized-aarch64' if 'arm64' in target or 'aarch64' in target else 'optimized-x86_64'
            assert data['implementations']['kem-01-aigis-enc1']['variant'] == variant
            fallback = data['implementations'][next(n for n in names if n.startswith('kem-03-'))]
            assert fallback['variant'] == 'ref' and fallback['reason']
            listing = subprocess.check_output(['make', '-s', 'REFERENCE_TEST=1', 'print-implementations'], cwd=root, text=True)
            assert all(line.endswith(':ref') for line in listing.splitlines())
            configure([target, '--implementation=ref', 'enable-kem-01-aigis-enc1'], {'kem-01-aigis-enc1'})
            data = json.loads((root/'configdata.json').read_text())
            assert data['implementations']['kem-01-aigis-enc1']['variant'] == 'ref'
        for name,(_,data) in fixtures.items():assert (candidates/name).read_bytes()==data

        # Cold import works with only Perl on PATH: no Python or external unzip.
        perl_bin=root/'perl-only-bin';perl_bin.mkdir()
        (perl_bin/'perl').symlink_to(shutil.which('perl'))
        optimized=['darwin64-arm64-cc','--implementation=optimized','enable-kem-01-aigis-enc1']
        for name in fixtures:(candidates/name).unlink()
        run=configure(optimized,environment={'PATH':str(perl_bin)})
        assert 'imported 3 from 1 pinned archives' in run.stdout
        for name,(_,data) in fixtures.items():assert (candidates/name).read_bytes()==data

        # Once imported, the archives are unnecessary; unchanged config is stable.
        archive.unlink()
        before=[(root/p).stat().st_mtime_ns for p in ('Makefile','configdata.json','src/ngcc_config.h')]
        configure(optimized,environment={'PATH':str(perl_bin)})
        assert before==[(root/p).stat().st_mtime_ns for p in ('Makefile','configdata.json','src/ngcc_config.h')]
        for name in fixtures:(candidates/name).unlink()
        run=configure(optimized,fail=True)
        assert 'Missing optimized source archive' in run.stderr
        # Reference-only or all-fallback selections do not import archives.
        configure(['--implementation=ref','enable-kem-01-aigis-enc1'])
        fallback_name=next(r['provider_name'] for r in rows if r['id']=='kem-03')
        configure(['--implementation=optimized','enable-'+fallback_name])
        archive.write_bytes(archive_bytes+b'changed')
        assert 'Archive checksum mismatch' in configure(optimized,fail=True).stderr
        archive.write_bytes(archive_bytes)
        # All members must validate before any missing source is installed.
        (root/'make/optimized-sources.sha256').write_text(manifest.replace(
            hashlib.sha256(fixtures['kem-01/header.h'][1]).hexdigest(),'0'*64))
        assert 'Author member checksum mismatch' in configure(optimized,fail=True).stderr
        assert not any((candidates/name).exists() for name in fixtures)
        (root/'make/optimized-sources.sha256').write_text(manifest)
        configure(optimized)
        source=candidates/'kem-01/fixture source.c'
        source.write_bytes(b'local edit')
        assert 'refusing to overwrite' in configure(optimized,fail=True).stderr
        assert source.read_bytes()==b'local edit'
        source.unlink()
        outside=root/'outside.c';outside.write_bytes(b'untouched')
        source.symlink_to(outside)
        assert 'symlink' in configure(optimized,fail=True).stderr
        assert outside.read_bytes()==b'untouched'
        source.unlink()
        source.symlink_to(root/'nonexistent.c')
        assert 'symlink' in configure(optimized,fail=True).stderr
        assert not (root/'nonexistent.c').exists()
        source.unlink()
        # Explicit archive root supports paths containing spaces.
        moved=root/'separate archives';moved.mkdir()
        (moved/root.name).mkdir()
        archive.rename(moved/root.name/archive.name)
        configure([*optimized,'--with-optimized-archives='+str(moved)])
        assert source.read_bytes()==fixtures['kem-01/fixture source.c'][1]
        configure(['--with-optimized-archives='],fail=True)
        configure(['--implementation=unknown'], fail=True)
    result = {'checks': checks, 'algorithm_execution': False,
              'configure_sha256': hashlib.sha256((ROOT/'config').read_bytes()).hexdigest()}
    (ROOT/'test-results/configure-options.json').write_text(json.dumps(result, indent=2)+'\n')
    print('PASS config:', len(checks), 'checks; no algorithm executions')


if __name__ == '__main__':
    main()
