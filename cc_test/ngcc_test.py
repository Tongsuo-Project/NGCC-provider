#!/usr/bin/env python3
"""Compile and run EVP checks using the configured OpenSSL installation."""
import argparse
import concurrent.futures
import hashlib
import json
import subprocess
import time
from pathlib import Path
from ngcc_test_policy import (configured_names, slow_parameters, test_hosts,
                              host_environment, library_dirs)

ROOT=Path(__file__).resolve().parents[1]


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--id',action='append')
    ap.add_argument('--jobs',type=int,default=4)
    ap.add_argument('--timeout',type=int,default=1800)
    ap.add_argument('--repeat',type=int,default=1)
    ap.add_argument('--output',default='evp.json')
    ap.add_argument('--full',action='store_true',help='Explicitly include parameters previously taking more than one second')
    args=ap.parse_args()
    slow={} if args.full else slow_parameters()
    rows=json.loads((ROOT/'ngcc/metadata/registered.json').read_text())
    enabled=configured_names()
    rows=[r for r in rows if r['provider_name'] in enabled and (not args.id or r['id'] in args.id)]
    hosts=test_hosts()
    available_hosts=set(hosts)
    out=ROOT/'test-results'
    module_hash=hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest()
    source_hash=hashlib.sha256((ROOT/'ngcc/metadata/source-integrity.json').read_bytes()).hexdigest()
    tasks=[]
    for host,prefix in hosts.items():
        binary=ROOT/'build'/('ngcc_evp_'+host)
        cmd=['cc','-O2','-Wall','-Wextra','-I'+str(prefix/'include'),str(ROOT/'cc_test/ngcc_evp.c'),
             *['-L'+str(x) for x in library_dirs(prefix)],'-lcrypto','-o',str(binary)]
        subprocess.run(cmd,check=True)
        for r in rows:
            for repeat in range(args.repeat):tasks.append((host,prefix,binary,r,repeat))

    def test(task):
        host,prefix,binary,r,repeat=task
        if r['provider_name'] in slow:
            return {'host':host,'id':r['id'],'label':r['label'],'provider_name':r['provider_name'],
                    'repeat':repeat,'module_sha256':module_hash,'source_manifest_sha256':source_hash,
                    'status':'skipped','reason':'previous-case-over-1s',
                    'skip_evidence':slow[r['provider_name']]}
        env=host_environment(prefix)
        suffix=('.repeat'+str(repeat)) if args.repeat>1 else ''
        log=out/(Path(args.output).stem+'--'+r['id']+'--'+r['label']+'.'+host+suffix+'.test.log')
        started=time.monotonic()
        status='failed'
        with log.open('w') as f:
            try:
                p=subprocess.run([str(binary),str(ROOT),r['type'],r['provider_name']],env=env,
                                 stdout=f,stderr=subprocess.STDOUT,timeout=args.timeout)
                status='passed' if p.returncode==0 else 'crashed' if p.returncode<0 else 'failed'
                code=p.returncode
            except subprocess.TimeoutExpired:
                status='timeout';code=None
        result={'host':host,'id':r['id'],'label':r['label'],'provider_name':r['provider_name'],
                'repeat':repeat,'module_sha256':module_hash,'source_manifest_sha256':source_hash,
                'status':status,'exit_code':code,'seconds':round(time.monotonic()-started,3),'log':str(log.relative_to(ROOT))}
        print(host,r['id'],r['label'],status,flush=True)
        return result

    checkpoint=out/(Path(args.output).stem+'.partial.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures=[pool.submit(test,t) for t in tasks]
        results=[]
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())
            temp=checkpoint.with_suffix('.tmp')
            temp.write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n')
            temp.replace(checkpoint)
    if hashlib.sha256((ROOT/'ngcc.so').read_bytes()).hexdigest()!=module_hash:
        raise SystemExit('Module changed while tests were running; results are invalid')
    path=out/args.output
    old=json.loads(path.read_text()) if path.exists() else []
    keys={(r['host'],r['id'],r['label'],r['repeat']) for r in results}
    registered={(r['id'],r['label']) for r in json.loads((ROOT/'ngcc/metadata/registered.json').read_text())
                if r['provider_name'] in enabled}
    results.extend(r for r in old if r.get('module_sha256')==module_hash and
                   r['host'] in available_hosts and
                   (r['id'],r['label']) in registered and
                   (r['host'],r['id'],r['label'],r.get('repeat',0)) not in keys)
    results.sort(key=lambda r:(r['host'],r['id'],r['label']))
    path.write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n')
    print(sum(r['status']=='passed' for r in results),'passed /',len(results),'tests')
    print(sum(r['status']=='skipped' for r in results),'skipped by runtime policy')
    if any(r['status'] not in ('passed','skipped') for r in results):raise SystemExit(1)


if __name__=='__main__':main()
