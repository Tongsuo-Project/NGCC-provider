#!/usr/bin/env python3
"""Run TLS and X.509 checks using the configured OpenSSL installation."""
import argparse
import concurrent.futures
import hashlib
import json
import subprocess
import time
from pathlib import Path
from ngcc_test_policy import (configured_names, slow_parameters, test_hosts,
                              host_environment, library_dirs)

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--kind', choices=['kem', 'sig', 'both'], default='both')
    parser.add_argument('--id', action='append')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--timeout', type=int, default=1800)
    parser.add_argument('--output', default='tls.json')
    parser.add_argument('--full', action='store_true', help='Explicitly include parameters previously taking more than one second')
    args = parser.parse_args()
    slow = {} if args.full else slow_parameters()
    groups = json.loads((ROOT/'ngcc/metadata/tls-groups.json').read_text())['groups']
    sigs = json.loads((ROOT/'ngcc/metadata/sig-oids.json').read_text())['sigs']
    enabled = configured_names()
    groups = [r for r in groups if r['provider_name'] in enabled]
    sigs = [r for r in sigs if r['provider_name'] in enabled]
    kem_group = next((r['group_name'] for r in groups
                      if r['enabled'] and r['provider_name'] not in slow), None)
    hosts = test_hosts()
    available_hosts = set(hosts)
    module_hash = hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest()
    out = ROOT/'test-results'
    tasks = []
    capabilities = []
    for host, prefix in hosts.items():
        env = host_environment(prefix)
        for kind, driver, rows in [('kem', 'ngcc_tls', groups), ('sig', 'ngcc_cert', sigs)]:
            if args.kind not in (kind, 'both'):
                continue
            binary = ROOT/'build'/(driver+'_'+host)
            command = ['cc', '-O2', '-Wall', '-Wextra', '-I'+str(prefix/'include'),
                       str(ROOT/'cc_test'/(driver+'.c')),
                       *['-L'+str(p) for p in library_dirs(prefix)],
                       '-lssl', '-lcrypto', '-o', str(binary)]
            subprocess.run(command, check=True)
            listing = subprocess.check_output([str(binary), str(ROOT), '--list'], env=env, text=True)
            expected = {tuple(map(str, (r['group_name'], r['provider_name'], r['group_id'], r['security_bits'])))
                        for r in rows if r['enabled']} if kind == 'kem' else {
                            tuple(map(str, (r['tls_name'], r['tls_name'], r['oid'], r['code_point'], r['security_bits'])))
                            for r in rows if r['tls_enabled']}
            actual = [tuple(line.split()[1:]) for line in listing.splitlines()]
            assert set(actual) == expected and len(actual) == len(expected), (host, kind, 'capabilities mismatch')
            log = out/('capabilities-'+host+'-'+kind+'.log')
            log.write_text(listing)
            capabilities.append({'host': host, 'kind': kind, 'count': len(actual),
                                 'module_sha256': module_hash, 'log': str(log.relative_to(ROOT))})
            for row in rows:
                if args.id and row['id'] not in args.id:
                    continue
                modes = ['direct', 'hrr'] if kind == 'kem' and row['enabled'] else ['excluded']
                if kind == 'sig':
                    modes = ['certificate-tls' if row['tls_enabled'] else 'certificate-only']
                for mode in modes:
                    tasks.append((host, binary, env, kind, row, mode))
            if kind == 'kem' and not args.id and any(r['enabled'] for r in rows):
                tasks.append((host, binary, env, kind, next(r for r in rows if r['enabled']), 'tls12'))

    def run(task):
        host, binary, env, kind, row, mode = task
        name = row['provider_name']
        if name in slow:
            return {'host': host, 'kind': kind, 'id': row['id'], 'label': row['label'],
                    'provider_name': name, 'mode': mode, 'module_sha256': module_hash,
                    'status': 'skipped', 'reason': 'previous-case-over-1s',
                    'skip_evidence': slow[name]}
        log = out/(Path(args.output).stem+'--'+host+'--'+name+'--'+mode+'.log')
        if kind == 'kem':
            command = [str(binary), str(ROOT), row['group_name'], mode,
                       str(row['group_id']), str(row['public_key_bytes']), str(row['ciphertext_bytes'])]
        else:
            example = str(out/('certificate-example-'+host+'.pem')) if name == 'sig-01-aigis-sig1' else '-'
            command = [str(binary), str(ROOT), name, row['oid'], str(row['code_point']),
                       'enabled' if row['tls_enabled'] else 'excluded', example, row['tls_name'],
                       kem_group or '-']
        started = time.monotonic()
        with log.open('w') as output:
            try:
                process = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
                code = process.returncode
                status = 'passed' if code == 0 else 'crashed' if code < 0 else 'failed'
            except subprocess.TimeoutExpired:
                code, status = None, 'timeout'
        result = {'host': host, 'kind': kind, 'id': row['id'], 'label': row['label'],
                  'provider_name': name, 'mode': mode, 'status': status, 'exit_code': code,
                  'seconds': round(time.monotonic()-started, 3), 'module_sha256': module_hash,
                  'log': str(log.relative_to(ROOT))}
        if mode == 'certificate-tls':
            result['kem_group'] = kem_group
        print(host, kind, row['label'], mode, status, flush=True)
        return result

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(run, task) for task in tasks]
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())
            (out/(Path(args.output).stem+'.partial.json')).write_text(json.dumps(results, indent=2)+'\n')
    assert hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest() == module_hash, 'Module changed during tests'
    path = out/args.output
    key = lambda row: (row['host'], row['provider_name'], row['mode'])
    current = {key(row) for row in results}
    if path.exists():
        results.extend(row for row in json.loads(path.read_text())
                       if row['module_sha256'] == module_hash and key(row) not in current
                       and row['host'] in available_hosts
                       and row['provider_name'] in enabled)
    results.sort(key=key)
    path.write_text(json.dumps(results, ensure_ascii=False, indent=2)+'\n')
    cap_path = out/'tls-capabilities.json'
    current_cap = {(r['host'], r['kind']) for r in capabilities}
    if cap_path.exists():
        capabilities.extend(r for r in json.loads(cap_path.read_text())
                            if r['module_sha256'] == module_hash and r['host'] in available_hosts
                            and (r['host'], r['kind']) not in current_cap)
    cap_path.write_text(json.dumps(capabilities, indent=2)+'\n')
    for example in out.glob('certificate-example-*.pem'):
        example.chmod(0o600)
    print(sum(r['status'] == 'passed' for r in results), 'passed /', len(results), 'TLS/certificate cases')
    print(sum(r['status'] == 'skipped' for r in results), 'skipped by runtime policy')
    if any(r['status'] not in ('passed', 'skipped') for r in results):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
