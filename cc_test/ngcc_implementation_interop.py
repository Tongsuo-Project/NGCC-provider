#!/usr/bin/env python3
"""Check reference/optimized wire compatibility, using separately linked instances."""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--selection', type=Path, default=ROOT/'configdata.json')
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--timeout', type=int, default=600)
    ap.add_argument('--output', default='implementation-interop.json')
    ap.add_argument('--build-reference', action='store_true')
    args = ap.parse_args()
    config = json.loads(args.selection.read_text())
    rows = json.loads((ROOT/'ngcc/metadata/registered.json').read_text())
    rows = [r for r in rows if r['provider_name'] in config['enabled'] and
            config['implementations'][r['provider_name']]['variant'] != 'ref']
    if not rows:
        raise SystemExit('No optimized implementations selected; configure --implementation=optimized')
    if args.build_reference:
        current = json.loads((ROOT/'configdata.json').read_text())
        if current != config:
            raise SystemExit('--build-reference requires the current configdata.json selection')
        subprocess.run(['make', '-j'+str(args.jobs), 'REFERENCE_TEST=1',
                        *['build/ngcc/'+config['target']+'/'+r['id']+'/'+r['label']+'/ref/instance.o' for r in rows]],
                       cwd=ROOT, check=True)
    values = config['variables']
    cc = shlex.split(values.get('CC', 'cc'))
    prefix = Path(values['OPENSSL_PREFIX'])
    target_flags = shlex.split(config['target_flags'])
    darwin = config['platform'] == 'Darwin'
    work = ROOT/'build/implementation-interop'/config['target']
    work.mkdir(parents=True, exist_ok=True)
    (ROOT/'test-results').mkdir(exist_ok=True)
    binary = work/'check'
    subprocess.run([*cc, *target_flags, '-O2', '-Isrc', '-I'+str(prefix/'include'),
                    str(ROOT/'cc_test/ngcc_implementation_interop.c'), '-o', str(binary),
                    *([] if darwin else ['-ldl'])], cwd=ROOT, check=True)
    compat = work/'compat.o'
    subprocess.run([*cc, *target_flags, '-fPIC', '-c', 'src/ngcc_compat.c', '-o', str(compat)],
                   cwd=ROOT, check=True)
    libdirs = [p for p in (prefix/'lib', prefix/'lib64') if p.is_dir()]
    libs = [v for p in libdirs for v in ('-L'+str(p), '-Wl,-rpath,'+str(p))]+['-lcrypto']
    env = os.environ.copy()
    def check(row):
        name = row['provider_name']
        variant = config['implementations'][name]['variant']
        log = ROOT/'test-results'/('interop--'+config['target']+'--'+name+'.log')
        start = time.monotonic()
        result = {'name': name, 'target': config['target'], 'variant': variant,
                  'log': str(log.relative_to(ROOT))}
        with log.open('w') as output:
            try:
                modules = []
                for v in ('ref', variant):
                    obj = ROOT/'build/ngcc'/config['target']/row['id']/row['label']/v/'instance.o'
                    result[v+'_object_sha256'] = hashlib.sha256(obj.read_bytes()).hexdigest()
                    module = work/(name+'-'+v+'.so')
                    extra = []
                    if '-lgmp' in row['LDLIBS']:
                        gmp = values.get('GMP_STATIC')
                        extra = [gmp] if gmp else ['-L'+str(Path(values.get('GMP_ROOT', '/usr'))/'lib'), '-lgmp']
                    entry = config['implementations'][name]
                    if v != 'ref' and any(Path(s).suffix in ('.cpp', '.cc') for s in entry['SRCS'].split()):
                        extra += ['-lc++' if darwin else '-lstdc++']
                    subprocess.run([*cc, *target_flags, '-bundle' if darwin else '-shared',
                                    str(obj), str(compat), *libs, *extra, '-lm', '-lpthread', '-o', str(module)],
                                   cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True)
                    modules.append(str(module))
                run = subprocess.run([str(binary), *modules, row['symbol']], env=env,
                                     stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
                result['status'] = 'passed' if run.returncode == 0 else 'failed'
                result['exit_code'] = run.returncode
            except (subprocess.CalledProcessError, OSError) as exc:
                result.update(status='build-failed', error=str(exc))
            except subprocess.TimeoutExpired:
                result['status'] = 'timeout'
        result['seconds'] = round(time.monotonic()-start, 3)
        print(name, result['status'], flush=True)
        return result
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(check, rows):
            results.append(result)
            (ROOT/'test-results'/args.output).write_text(json.dumps(results, indent=2)+'\n')
    print(sum(r['status']=='passed' for r in results), '/', len(results), 'passed')
    raise SystemExit(0 if all(r['status']=='passed' for r in results) else 1)

if __name__ == '__main__':
    main()
