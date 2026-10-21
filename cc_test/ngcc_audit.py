#!/usr/bin/env python3
"""Check policy, source selection, module exports and exact-module test coverage."""
import argparse
import collections
import hashlib
import json
import platform
import re
import shlex
import subprocess
import uuid
from pathlib import Path
from ngcc_test_policy import (configured_names, slow_parameters, test_hosts,
                              host_binary, host_environment)

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--require-tests',action='store_true')
    ap.add_argument('--allow-skips',action='store_true')
    args=ap.parse_args()
    meta=ROOT/'ngcc/metadata'
    policy=json.loads((meta/'integration-policy.json').read_text())
    rows=json.loads((meta/'registered.json').read_text())
    selected=json.loads((meta/'selected.json').read_text())
    expected=set(policy['included'])
    assert len(expected)==len(rows)==len({r['provider_name'] for r in rows})
    for row in rows:
        assert re.fullmatch(row['type']+r'-[a-z0-9]+(?:-[a-z0-9]+)*',row['provider_name'])
        assert row['names'].split(':')[0]==row['provider_name']
        assert len(row['names'].split(':'))==len(set(row['names'].split(':')))
        assert row['symbol'].startswith('ngcc_ref_'+row['type']+'_')
        assert not any(name.startswith(('kem-kem-','sig-sign-')) for name in row['names'].split(':'))
    assert expected=={r['provider_name'] for r in rows}
    assert expected|{r['provider_name'] for r in policy['adaptation_exclusions']}=={r['provider_name'] for r in selected}
    assert not expected & {r['provider_name'] for r in policy['adaptation_exclusions']}
    available=rows
    configuration=json.loads((ROOT/'configdata.json').read_text())
    enabled=configured_names()
    assert enabled <= expected and enabled
    assert len(enabled)==len(configuration['enabled'])
    rows=[r for r in rows if r['provider_name'] in enabled]
    attested=json.loads((meta/'source-integrity.json').read_text())['files']
    checksums={}
    for line in (meta/'sources.sha256').read_text().splitlines():
        checksum, path = line.split('  ',1)
        checksums[path] = checksum
    assert checksums=={name:item['sha256'] for name,item in attested.items()}
    make_instances=subprocess.check_output(['make','-s','print-instances'],cwd=ROOT,text=True).splitlines()
    assert len(make_instances)==len(rows)
    assert set(make_instances)=={r['id']+'/'+r['label'] for r in rows}
    compiled_sources=set()
    for r in rows:
        assert r['security_bits'] in (128,160,192,256) and r['id']!='sign-29'
        assert 'SHAKE' not in r['label'].upper()
        assert not re.search(r'-m(?:avx|sse)|-march=native',r['CFLAGS']+' '+r['CXXFLAGS'])
        folder=ROOT/'ngcc/upstream'/r['id']
        for name in shlex.split(r['SRCS']):
            p=(folder/'src'/r['label']/name).resolve()
            rel=str(p.relative_to(ROOT/'ngcc/upstream'))
            assert rel in attested,rel+' is not attested author source'
            assert not re.search(r'(?i)(optimized_implementation|additional_implementation|/avx\d*/)',rel),rel
            compiled_sources.add(rel)
    make_sources=subprocess.check_output(['make','-s','print-ref-sources'],cwd=ROOT,text=True).splitlines()
    assert {str((ROOT/p).resolve().relative_to(ROOT/'ngcc/upstream')) for p in make_sources}==compiled_sources
    groups=json.loads((meta/'tls-groups.json').read_text())['groups']
    sigs=json.loads((meta/'sig-oids.json').read_text())['sigs']
    total_sigs=len(sigs)
    groups=[g for g in groups if g['provider_name'] in enabled]
    sigs=[s for s in sigs if s['provider_name'] in enabled]
    assert {g['provider_name'] for g in groups}=={r['provider_name'] for r in rows if r['type']=='kem'}
    assert {g['provider_name'] for g in sigs}=={r['provider_name'] for r in rows if r['type']=='sig'}
    assert len({g['group_id'] for g in groups})==len(groups)
    assert len({g['oid'] for g in sigs})==len(sigs)
    assert len({g['uuid'] for g in sigs})==len(sigs)
    oid_macros=dict(re.findall(r'^#define\s+(\w+)\s+"([0-9.]+)"$',
                              (ROOT/'src/ngcc_oid.h').read_text(),re.M))
    assert len(oid_macros)==total_sigs
    registered_sigs={r['provider_name']:r for r in rows if r['type']=='sig'}
    for sig in sigs:
        identifier=uuid.UUID(sig['uuid'])
        assert identifier.version==4 and identifier.variant==uuid.RFC_4122
        assert str(identifier)==sig['uuid']
        assert sig['oid']=='2.25.'+str(identifier.int)
        assert oid_macros[sig['oid_macro']]==sig['oid']
        names=registered_sigs[sig['provider_name']]['names'].split(':')
        assert sig['oid'] in names
        assert not any(name.startswith('1.2.156.10197.99999.') for name in names)
    assert len({g['code_point'] for g in sigs})==len(sigs)
    assert all(g['group_name']==g['provider_name'] for g in groups)
    assert all(g['tls_name']==g['provider_name'] for g in sigs)
    assert all(len(g['tls_name'])<40 for g in sigs)
    assert all(g['enabled']==(g['public_key_bytes']<=65529 and g['ciphertext_bytes']<=65521) for g in groups)
    assert all(g['tls_enabled']==(g['sig_capacity']<=65535) for g in sigs)
    assert not (ROOT/'src/ngcc_ngcc_provider.c').exists()
    assert not (ROOT/'src/ngcc_algorithms.inc').exists()
    assert not list((ROOT/'build/ngcc').glob('*/*/bridge.c'))
    source_files=[p for p in (ROOT/'src').iterdir() if p.suffix in ('.c','.h')]
    assert all(p.name.startswith('ngcc_') for p in (ROOT/'src').iterdir() if p.is_file())
    assert not list((ROOT/'src').glob('*signature*'))
    assert not list((ROOT/'make/schemes').glob('sign-*'))
    assert not any(re.search(r'\bngcc_len\b',p.read_text()) for p in source_files)
    for p in source_files:
        columns={len(line) for line in p.read_text().splitlines() if line.endswith('\\')}
        assert len(columns)<=1, (p, 'unaligned macro continuations')
    module=ROOT/'ngcc.so';digest=hashlib.sha256(module.read_bytes()).hexdigest()
    assert not (ROOT/'nsm.so').exists()
    if platform.system()=='Darwin':
        assert (ROOT/'ngcc.dylib').is_symlink()
        assert (ROOT/'ngcc.dylib').resolve()==module.resolve()
        exports=subprocess.check_output(['nm','-gU',str(module)],text=True)
        symbols=[x.split()[-1] for x in exports.splitlines() if x.split()]
        assert symbols==['_OSSL_provider_init'],symbols
        dependency_output=subprocess.check_output(['otool','-L',str(module)],text=True)
        dependencies=[x.strip().split(' (')[0] for x in dependency_output.splitlines()[1:]]
        assert dependencies==['/usr/lib/libSystem.B.dylib'],dependencies
    else:
        exports=subprocess.check_output(['nm','-D','--defined-only',str(module)],text=True)
        assert [x.split()[-1] for x in exports.splitlines() if x.split()]==['OSSL_provider_init']
        dependency_output=subprocess.check_output(['readelf','-d',str(module)],text=True)
        dependencies=re.findall(r'NEEDED.*\[(.*?)\]',dependency_output)
        assert all(re.match(r'lib(crypto|c|m|gcc_s|pthread)\.',x) for x in dependencies),dependencies
    versions={}
    for host,prefix in test_hosts().items():
        binary=host_binary(prefix)
        env=host_environment(prefix)
        versions[host]=subprocess.check_output([str(binary),'version'],env=env,text=True).strip()
        for kind in ['kem','sig']:
            listing=subprocess.check_output([str(binary),'list','-'+('signature' if kind=='sig' else kind)+'-algorithms',
                '-provider-path',str(ROOT),'-provider','default','-provider','ngcc','-propquery','provider=ngcc'],env=env,text=True)
            assert 'kem-kem-' not in listing and 'sig-sign-' not in listing
            assert '@ nsm' not in listing
            if any(r['type']==kind for r in rows):
                assert '@ ngcc' in listing
            listed=set(re.findall(r'[A-Za-z0-9][A-Za-z0-9._-]*',listing))
            for r in rows:
                if r['type']==kind:assert r['provider_name'] in listed
            assert not any(r['provider_name'] in listed for r in policy['adaptation_exclusions'])
            assert not any(r['provider_name'] in listed for r in available if r['provider_name'] not in enabled)
            (ROOT/'test-results'/('list-'+host+'-'+kind+'.txt')).write_text(listing)
    result={'module_sha256':digest,'platform':platform.platform(),'exports':['OSSL_provider_init'],
        'target':configuration['target'],'configured_algorithms':configuration['enabled'],
        'dependencies':dependencies,'host_versions':versions,'instances':len(rows),
        'families':len({r['id'] for r in rows}),'by_type':dict(collections.Counter(r['type'] for r in rows)),
        'compiled_author_sources':len(compiled_sources),'tests_required':args.require_tests,
        'tls_groups':sum(g['enabled'] for g in groups),
        'sig_oids':len(sigs),'sig_oid_format':'2.25.<UUIDv4 unsigned integer>',
        'tls_sigs':sum(g['tls_enabled'] for g in sigs)}
    assert (ROOT/'build/module.sha256').read_text().split()[0]==digest
    provider_sources=subprocess.check_output(['make','-s','print-provider-sources'],cwd=ROOT,text=True).splitlines()
    wanted_sources={'ngcc_prov.c','ngcc_keymgmt.c','ngcc_capabilities.c','ngcc_context.c','ngcc_compat.c'}
    if groups:
        wanted_sources.add('ngcc_kem.c')
    if sigs:
        wanted_sources.update(('ngcc_sig.c','ngcc_encoder.c','ngcc_decoder.c'))
    assert wanted_sources==set(provider_sources)
    result['provider_sources']=provider_sources
    build={'module_sha256':digest,'platform':platform.platform(),'instances':len(rows),
           'build_system':'GNU make (no Python in compilation or linking)',
           'provider_sources':provider_sources,'tls_groups':result['tls_groups'],
           'tls_sigs':result['tls_sigs']}
    (ROOT/'test-results/module-build.json').write_text(json.dumps(build,indent=2)+'\n')
    if args.require_tests:
        slow=slow_parameters() if args.allow_skips else {}
        def allowed_skip(t):
            return (t.get('status')=='skipped' and t.get('reason')=='previous-case-over-1s'
                    and t['provider_name'] in slow and t['module_sha256']==digest
                    and t.get('skip_evidence',{}).get('seconds',0)>1)
        tests=json.loads((ROOT/'test-results/evp.json').read_text())
        wanted={(h,r['provider_name']) for h in versions for r in rows}
        passed={(t['host'],t['provider_name']) for t in tests if t['module_sha256']==digest and t['status']=='passed'}
        skipped={(t['host'],t['provider_name']) for t in tests if allowed_skip(t)}
        assert passed|skipped==wanted,('Missing test results',wanted-passed-skipped)
        assert len(tests)==len(wanted) and all(t['status']=='passed' or allowed_skip(t) for t in tests)
        threads=json.loads((ROOT/'test-results/threads.json').read_text())
        samples={'kem':'kem-bw-kem-128','sig':'sig-01-aigis-sig1'}
        tested_kinds=[kind for kind,name in samples.items() if name in enabled and name not in slow]
        assert {t['host'] for t in threads if t['module_sha256']==digest}==set(versions)
        if tested_kinds:
            assert all(t['exit_code']==0 and t['lifecycle_exit_code']==0
                       and t.get('kinds')==tested_kinds for t in threads)
        else:
            assert all(t['status']=='skipped' and t['reason']=='test-parameters-not-enabled-or-slow' for t in threads)
        result['passed_evp_tests']=len(passed);result['skipped_evp_tests']=len(skipped)
        result['passed_concurrency_hosts']=len(threads) if tested_kinds else 0
        result['passed_lifecycle_hosts']=len(threads) if tested_kinds else 0
        result['lifecycle_kinds']=tested_kinds
        tls=json.loads((ROOT/'test-results/tls.json').read_text())
        wanted_tls=set()
        for host in versions:
            for g in groups:
                for mode in (['direct','hrr'] if g['enabled'] else ['excluded']):
                    wanted_tls.add((host,g['provider_name'],mode))
            if any(g['enabled'] for g in groups):
                wanted_tls.add((host,next(g['provider_name'] for g in groups if g['enabled']),'tls12'))
            for g in sigs:
                wanted_tls.add((host,g['provider_name'],'certificate-tls' if g['tls_enabled'] else 'certificate-only'))
        passed_tls={(t['host'],t['provider_name'],t['mode']) for t in tls
                    if t['module_sha256']==digest and t['status']=='passed'}
        skipped_tls={(t['host'],t['provider_name'],t['mode']) for t in tls if allowed_skip(t)}
        assert passed_tls|skipped_tls==wanted_tls, ('Missing TLS/certificate results',wanted_tls-passed_tls-skipped_tls)
        assert len(tls)==len(wanted_tls)
        caps=json.loads((ROOT/'test-results/tls-capabilities.json').read_text())
        assert {(r['host'],r['kind'],r['count']) for r in caps if r['module_sha256']==digest}=={
            (host,kind,count) for host in versions for kind,count in
            [('kem',result['tls_groups']),('sig',result['tls_sigs'])]}
        completed_tls=[t for t in tls if t['status']=='passed']
        result['passed_tls_certificate_cases']=len(completed_tls)
        result['skipped_tls_certificate_cases']=len(skipped_tls)
        result['passed_certificates']=sum(t['kind']=='sig' for t in completed_tls)
        result['passed_tls_handshakes']=sum(1+bool(t['kem_group']) if t['mode']=='certificate-tls' else
                                          1 if t['mode'] in ('direct','hrr') else 0 for t in completed_tls)
        result['passed_tls_negative_cases']=sum(t['mode'] in ('excluded','tls12','certificate-only') for t in completed_tls)
        result['full_suite_passed']=not skipped and not skipped_tls
        interop=json.loads((ROOT/'test-results/interop.json').read_text())
        assert {(t['server'],t['client']) for t in interop if t['module_sha256']==digest}=={
            (host,host) for host in versions}
        if len(tested_kinds)==2:
            assert all(t['status']=='passed' for t in interop)
        else:
            assert all(t['status']=='skipped' and t['reason']=='test-parameters-not-enabled-or-slow' for t in interop)
        result['passed_cli_handshakes']=sum(t['status']=='passed' for t in interop)


    (ROOT/'test-results/audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print('PASS audit:',len(rows),'instances;',len(compiled_sources),'compiled author sources; one module')

if __name__=='__main__':main()
