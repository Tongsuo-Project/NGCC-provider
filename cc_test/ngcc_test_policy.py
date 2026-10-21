"""Locate configured test hosts and select cases using recorded wall times."""
import json
import os
import platform
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def openssl_prefix():
    configuration = json.loads((ROOT/'configdata.json').read_text())
    return Path(os.environ.get('OPENSSL_PREFIX', configuration['variables']['OPENSSL_PREFIX']))


def library_dirs(prefix):
    return [path for path in (prefix/'lib', prefix/'lib64',
                             *sorted((prefix/'lib').glob('*-linux-gnu')), prefix)
            if path.is_dir()]


def host_environment(prefix):
    env = dict(os.environ)
    key = 'DYLD_LIBRARY_PATH' if platform.system() == 'Darwin' else 'LD_LIBRARY_PATH'
    env[key] = os.pathsep.join(map(str, library_dirs(prefix)))
    return env


def host_binary(prefix):
    return next(path for path in (prefix/'bin/openssl', prefix/'apps/openssl') if path.is_file())


def test_hosts():
    prefix = openssl_prefix()
    version = subprocess.check_output([str(host_binary(prefix)), 'version'],
                                      env=host_environment(prefix), text=True)
    name = 'tongsuo' if 'Tongsuo' in version else 'openssl'
    return {name: prefix}


def configured_names():
    return set(json.loads((ROOT/'configdata.json').read_text())['enabled'])


def slow_parameters():
    saved = ROOT/'test-results/slow-parameters.json'
    slow = json.loads(saved.read_text()) if saved.exists() else {}
    names = {(r['id'], r['label']): r['provider_name'] for r in
             json.loads((ROOT/'ngcc/metadata/registered.json').read_text())}
    for name in ('current/evp.json', 'current/tls.json',
                 'evp.json', 'evp.partial.json', 'tls.json', 'tls.partial.json'):
        path = ROOT/'test-results'/name
        if not path.exists():
            continue
        for row in json.loads(path.read_text()):
            evidence = row.get('skip_evidence', row)
            seconds = evidence.get('seconds', 0)
            key = names.get((row['id'], row['label']), row['provider_name'])
            if seconds > 1 and seconds > slow.get(key, {}).get('seconds', 0):
                slow[key] = {field: evidence[field] for field in
                             ('seconds', 'host', 'module_sha256', 'log')}
    saved.write_text(json.dumps(slow, indent=2)+'\n')
    return slow
