#!/usr/bin/env python3
"""Check configuration semantics without executing author algorithms."""
import hashlib
import json
import os
import platform
import shutil
import subprocess
import tempfile
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
        assert len(gmp_names) == 19 and len(default_names) == 189
        kem, sig = 'kem-bw-kem-128', 'sig-01-aigis-sig1'

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

        configure([], default_names)
        configure(['OPENSSL_PREFIX=/custom/openssl'], default_names)
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['OPENSSL_PREFIX'] == '/custom/openssl'
        configure([], default_names, environment={'OPENSSL_PREFIX': '/custom/tongsuo'})
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['OPENSSL_PREFIX'] == '/custom/tongsuo'
        configure(['OPENSSL_PREFIX=/custom/openssl'], default_names,
                  environment={'OPENSSL_PREFIX': '/custom/tongsuo'})
        data = json.loads((root/'configdata.json').read_text())
        assert data['variables']['OPENSSL_PREFIX'] == '/custom/openssl'
        for option in ('OPENSSL_PREFIX=', 'OPENSSL_PREFIX=/path with spaces',
                       'TONGSUO_ROOT=/old/path', 'OPENSSL_ROOT=/old/path'):
            configure([option], fail=True)
        configure(['GMP_ROOT=/installed'], all_names)
        configure(['GMP_STATIC=/installed/lib/libgmp.a'], all_names)
        configure([], all_names, environment={'GMP_ROOT': '/installed'})
        configure([], default_names)
        for option in ('--help', '--list', '--list-algorithms'):
            files = [root/p for p in ('Makefile', 'src/ngcc_config.h', 'configdata.json')]
            before = [p.read_bytes() for p in files]
            configure([option])
            assert before == [p.read_bytes() for p in files]
        configure(['no-'+kem], default_names-{kem})
        configure(['no-kem-bw-kem-128', 'no-'+sig], default_names-{kem, sig})
        configure(['GMP_ROOT=/installed', 'no-'+kem], all_names-{kem})
        configure(['GMP_ROOT=/installed', *['no-'+name for name in sorted(gmp_names)]], default_names)
        for name in ('kem-31-ngcc-1', 'sig-sqisign2dsq-l1-eff-compressed',
                     'sig-26-sqisign2d-lvl1', 'sig-27-sqisigntriangle-lvl1'):
            configure(['enable-'+name], fail=True)
        configure(['enable-'+kem, 'enable-'+sig], {kem, sig})
        configure(['enable-KEM-BW-KEM-128', 'enable-'+kem], {kem})
        configure(['enable-'+sig, 'no-'+kem], {sig})
        for alias in ('sig-sqisign2dsq-l1-eff-compressed',):
            sigs = json.loads((root/'ngcc/metadata/sig-oids.json').read_text())['sigs']
            row = next(r for r in sigs if r['tls_name'] == alias)
            configure(['GMP_ROOT=/installed', 'enable-'+alias], {row['provider_name']})
        configure(['enable-chinith-ublockith-d3-256f'], {'sig-05-ublockith-d3-256f'})
        for name in sorted(all_names):
            if name.startswith(('sig-sqisign2dsq-', 'kem-bw-kem-', 'kem-nev-',
                                'kem-rudraksh2-', 'kem-zen-')):
                options = ['GMP_ROOT=/installed'] if name in gmp_names else []
                configure(options+['enable-'+name], {name})
                configure(['no-'+name], default_names-{name})
        for options in (['enable-'+kem, 'no-kem-bw-kem-128'], ['enable-unknown'],
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
        configure(['--with-gmp-source=gmp-source'], all_names)
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
        configure([], default_names)
    result = {'checks': checks, 'algorithm_execution': False,
              'configure_sha256': hashlib.sha256((ROOT/'config').read_bytes()).hexdigest()}
    (ROOT/'test-results/configure-options.json').write_text(json.dumps(result, indent=2)+'\n')
    print('PASS config:', len(checks), 'checks; no algorithm executions')


if __name__ == '__main__':
    main()
