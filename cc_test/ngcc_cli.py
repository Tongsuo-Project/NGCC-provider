#!/usr/bin/env python3
"""Issue a proposal-signed chain and test TLS using the configured installation."""
import argparse
import hashlib
import json
import os
import subprocess
import re
import time
from pathlib import Path
from ngcc_test_policy import (configured_names, slow_parameters, test_hosts,
                              host_binary, host_environment)

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'test-results/cli-example'
ALGORITHM = 'sig-01-aigis-sig1'
GROUP = 'kem-08-bw-kem-128'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--full', action='store_true')
    args = parser.parse_args()
    os.umask(0o077)
    OUT.mkdir(exist_ok=True)
    module_hash = hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest()
    prefixes = test_hosts()
    pairs = [(host, host) for host in prefixes]
    required = {ALGORITHM, 'kem-08-bw-kem-128'}
    if not required <= configured_names() or (not args.full and required & slow_parameters().keys()):
        results = [{'server': server, 'client': client, 'module_sha256': module_hash,
                    'status': 'skipped', 'reason': 'test-parameters-not-enabled-or-slow'}
                   for server, client in pairs]
        (ROOT/'test-results/interop.json').write_text(json.dumps(results, indent=2)+'\n')
        print('CLI interop skipped: test parameters disabled or slow')
        return
    hosts = {}
    for host, prefix in prefixes.items():
        binary = host_binary(prefix)
        env = host_environment(prefix)
        hosts[host] = (binary, env)
    provider = ['-provider-path', str(ROOT), '-provider', 'default', '-provider', 'ngcc']
    extensions = OUT/'leaf.cnf'
    extensions.write_text('basicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature\nextendedKeyUsage=serverAuth\nsubjectAltName=DNS:localhost\n')

    def command(host, arguments):
        binary, env = hosts[host]
        return [str(binary), arguments[0], *provider, *arguments[1:]], env

    steps = [
        ['genpkey', '-algorithm', ALGORITHM, '-out', 'ca.key'],
        ['req', '-config', '/dev/null', '-new', '-x509', '-key', 'ca.key', '-subj', '/CN=NGCC Test CA', '-days', '1',
         '-addext', 'basicConstraints=critical,CA:TRUE', '-addext', 'keyUsage=critical,keyCertSign,cRLSign', '-out', 'ca.pem'],
        ['genpkey', '-algorithm', ALGORITHM, '-out', 'server.key'],
        ['req', '-config', '/dev/null', '-new', '-key', 'server.key', '-subj', '/CN=localhost', '-out', 'server.csr'],
        ['x509', '-req', '-in', 'server.csr', '-CA', 'ca.pem', '-CAkey', 'ca.key', '-set_serial', '2',
         '-days', '1', '-extfile', str(extensions), '-out', 'server.pem'],
    ]
    with (OUT/'issuance.log').open('w') as log:
        for step in steps:
            args, env = command(next(iter(hosts)), step)
            subprocess.run(args, env=env, cwd=OUT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=60)
        for host in hosts:
            args, env = command(host, ['verify', '-CAfile', 'ca.pem', '-check_ss_sig', '-purpose', 'sslserver',
                                       '-verify_hostname', 'localhost', 'server.pem'])
            subprocess.run(args, env=env, cwd=OUT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=60)
    results = []
    for server_host, client_host in pairs:
        name = server_host+'-server--'+client_host+'-client'
        server_args, server_env = command(server_host, [
            's_server', '-accept', '127.0.0.1:0', '-cert', str(OUT/'server.pem'), '-key', str(OUT/'server.key'),
            '-www', '-naccept', '1', '-tls1_3', '-groups', GROUP, '-sigalgs', ALGORITHM])
        client_args, client_env = command(client_host, [
            's_client', '-CAfile', str(OUT/'ca.pem'), '-verify_return_error',
            '-verify_hostname', 'localhost', '-tls1_3', '-groups', GROUP, '-sigalgs', ALGORITHM])
        server_log = OUT/(name+'.server.log')
        client_log = OUT/(name+'.client.log')
        with server_log.open('w') as output:
            server = subprocess.Popen(server_args, env=server_env, stdout=output, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic()+10
                address = None
                while server.poll() is None and time.monotonic()<deadline:
                    match = re.search(r'ACCEPT (127\.0\.0\.1:\d+)', server_log.read_text())
                    if match is not None:
                        address = match[1]
                        break
                    time.sleep(0.05)
                if address is None:
                    raise RuntimeError('Server did not listen; see '+str(server_log))
                client_args += ['-connect', address]
                client = subprocess.run(client_args, input='GET / HTTP/1.0\r\n\r\n', text=True,
                                        env=client_env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
                client_log.write_text(client.stdout)
                server.wait(timeout=30)
                passed = client.returncode == 0 and server.returncode == 0 and 'Verification: OK' in client.stdout
                passed = passed and 'TLSv1.3' in client.stdout and GROUP in client.stdout
                if not passed:
                    raise RuntimeError('TLS handshake failed; see '+str(client_log))
            finally:
                if server.poll() is None:
                    server.terminate()
                    server.wait(timeout=5)
        results.append({'server': server_host, 'client': client_host, 'group': GROUP,
                        'sig': ALGORITHM, 'status': 'passed', 'module_sha256': module_hash,
                        'client_log': str(client_log.relative_to(ROOT)), 'server_log': str(server_log.relative_to(ROOT))})
        print('PASS', name, GROUP, ALGORITHM, flush=True)
    assert hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest() == module_hash
    (ROOT/'test-results/interop.json').write_text(json.dumps(results, indent=2)+'\n')


if __name__ == '__main__':
    main()
