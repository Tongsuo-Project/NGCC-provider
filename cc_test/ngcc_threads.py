#!/usr/bin/env python3
"""Run concurrent EVP operations using the configured OpenSSL installation."""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path
from ngcc_test_policy import (configured_names, slow_parameters, test_hosts,
                              host_environment, library_dirs)

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--full',action='store_true')
    args=parser.parse_args()
    results=[]
    digest=hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest()
    enabled=configured_names()-(set() if args.full else slow_parameters().keys())
    kinds=[kind for kind,name in [('kem','kem-08-bw-kem-128'),('sig','sig-01-aigis-sig1')]
           if name in enabled]
    for host,prefix in test_hosts().items():
        env=host_environment(prefix)
        result={'host':host,'module_sha256':digest}
        if not kinds:
            result.update(status='skipped',reason='test-parameters-not-enabled-or-slow')
            results.append(result)
            continue
        result['kinds']=kinds
        for name,sources in [('threads',['ngcc_evp.c','ngcc_threads.c']),
                             ('lifecycle',['ngcc_lifecycle.c'])]:
            exe=ROOT/'build'/('ngcc_'+name+'_'+host)
            subprocess.run([os.environ.get('CC','cc'),'-O2','-Wall','-Wextra','-pthread','-DNGCC_TEST_NO_MAIN',
                '-I'+str(prefix/'include'),*[str(ROOT/'cc_test'/source) for source in sources],
                *['-L'+str(p) for p in library_dirs(prefix)],'-lcrypto','-o',str(exe)],check=True)
            log=ROOT/'test-results'/(name+'-'+host+'.log')
            with log.open('w') as f:
                r=subprocess.run([str(exe),str(ROOT),'both' if len(kinds)==2 else kinds[0]],
                                 env=env,stdout=f,stderr=subprocess.STDOUT,timeout=120)
            result[('' if name=='threads' else 'lifecycle_')+'exit_code']=r.returncode
            result[('' if name=='threads' else 'lifecycle_')+'log']=str(log.relative_to(ROOT))
            print(host,name+' test:',r.returncode)
        results.append(result)
    if hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest()!=digest:
        raise SystemExit('Module changed during concurrency tests')
    (ROOT/'test-results/threads.json').write_text(json.dumps(results,indent=2)+'\n')
    if any(r.get('exit_code',0) or r.get('lifecycle_exit_code',0) for r in results):raise SystemExit(1)

if __name__=='__main__':main()
