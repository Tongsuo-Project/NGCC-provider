#!/usr/bin/env python3
"""Replay a native sig API diagnostic, without modifying author code."""
import argparse
import hashlib
import json
import os
import platform
import re
import subprocess
import time
from pathlib import Path
from ngcc_test_policy import openssl_prefix, library_dirs, host_environment

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('id');ap.add_argument('label')
    ap.add_argument('--count',type=int,default=80)
    ap.add_argument('--first',type=int,default=0)
    ap.add_argument('--timeout',type=int,default=600)
    args=ap.parse_args()
    row=next(r for r in json.loads((ROOT/'ngcc/metadata/selected.json').read_text())
             if r['id']==args.id and r['label']==args.label and r['type']=='sig')
    symbol='ngcc_ref_'+re.sub(r'[^A-Za-z0-9_]', '_', 'sig-'+row['id'].split('-')[1]+'_'+row['label'])
    obj=ROOT/'build/ngcc'/row['id']/row['label']/'instance.o'
    prefix=openssl_prefix()
    gmp=Path(os.environ.get('GMP_ROOT','/opt/homebrew/opt/gmp' if platform.system()=='Darwin' else '/usr'))
    stem='native-replay--'+args.id+'--'+args.label+'--'+str(args.first)
    exe=ROOT/'build'/stem
    subprocess.run([os.environ.get('CC','cc'),'-O2','-I'+str(ROOT/'src'),'-I'+str(prefix/'include'),
        '-DNGCC_GETTER='+symbol,str(ROOT/'cc_test/ngcc_native_sig.c'),str(obj),
        str(ROOT/'build/ngcc_compat.o'),*['-L'+str(p) for p in library_dirs(prefix)],
        '-lcrypto','-L'+str(gmp/'lib'),'-lgmp','-o',str(exe)],check=True)
    env=host_environment(prefix)
    log=ROOT/'test-results'/(stem+'.log');start=time.monotonic();code=None
    with log.open('w') as f:
        try:
            code=subprocess.run([str(exe),str(args.count),str(args.first)],env=env,stdout=f,
                                stderr=subprocess.STDOUT,timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:f.write('\nTIMEOUT after '+str(args.timeout)+' seconds\n')
    result={'id':args.id,'label':args.label,'first':args.first,'count':args.count,
        'status':'timeout' if code is None else 'passed' if code==0 else 'failed',
        'exit_code':code,'seconds':round(time.monotonic()-start,3),'log':str(log.relative_to(ROOT)),
        'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest()}
    log.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    raise SystemExit(124 if code is None else code)

if __name__=='__main__':main()
