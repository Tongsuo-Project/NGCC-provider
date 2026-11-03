#!/usr/bin/env python3
"""Transcribe the authors' own build recipes into the ngcc build catalog.

The provider must not invent compiler options: for every instance the
optimisation, tuning, instruction-set and language flags are taken from the
recipe shipped with the submission (Makefile / GNUmakefile / CMakeLists.txt /
build*.sh).  Only flags the provider genuinely needs to produce a shared object
are added on top (see REF_CFLAGS in make/ref.mk).

Usage:
    python3 cc_test/ngcc_author_flags.py --report
    python3 cc_test/ngcc_author_flags.py --write
    python3 cc_test/ngcc_author_flags.py --write --schemes sign-03,sign-04
"""

import argparse
import collections
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAND = os.path.join(ROOT, 'ngcc', 'candidates')
SCHEMES_DIR = os.path.join(ROOT, 'make', 'schemes')
IMPL_JSON = os.path.join(ROOT, 'make', 'implementations.json')

OPT_TOKENS = ('-O0', '-O1', '-O2', '-O3', '-Os', '-Oz', '-Ofast')

# Flags the provider owns; never copied from the author recipe.
PROVIDER_OWNED = ('-flto', '-ffat-lto-objects', '-fno-lto',
                  '-fPIC', '-fpic', '-fPIE', '-fpie',
                  '-fcommon', '-fno-common',
                  '-fno-strict-aliasing', '-fstrict-aliasing',
                  '-fwrapv', '-fno-wrapv',
                  '-fvisibility=hidden', '-pipe',
                  '-g', '-g0', '-g1', '-g2', '-g3')

ISA_RE = re.compile(r'^-m(?:avx|aes|bmi|popcnt|pclmul|sse|ssse|adx|lzcnt|sha|gfni|'
                    r'vaes|vpclmul|sm3|sm4|movbe|fma|xop|rdrnd|rdseed|mmx|abm|'
                    r'f16c|fma4|cx16|prfchw|clflushopt)')

TUNE_TOKENS = ('-funroll-loops', '-fno-unroll-loops',
               '-fomit-frame-pointer', '-fno-omit-frame-pointer',
               '-ftree-vectorize', '-fno-tree-vectorize',
               '-fno-tree-slp-vectorize', '-fno-tree-loop-vectorize',
               '-ffast-math', '-fno-math-errno', '-falign-functions',
               '-fno-semantic-interposition', '-fipa-pta')

TOKEN_RE = re.compile(r'(?<![\w-])-[A-Za-z][\w\-]*(?:=[^\s\\)"\'`;|&]*)?')

FLAG_LINE_RE = re.compile(
    r'(?:^|\s)(?:CFLAGS|OPTFLAGS|CCFLAGS|C_FLAGS|CXXFLAGS|USER_CFLAGS|'
    r'EXTRA_CFLAGS|ADDITIONAL_CFLAGS|COMPILE_FLAGS|CMAKE_C_FLAGS(?:_[A-Z]+)?)'
    r'\s*[:+?]?=', re.I)

NEGATIVE_RE = re.compile(r'\b(debug|test|kat|sanitiz|coverage|profile)\b', re.I)
# 作者的 CMakeLists/Makefile 常同时覆盖 x86 与 ARM 分支，构建 linux-x86_64
# 时只保留 x86 可用的 arch 取值，避免把 -march=armv8-a+crypto 带进来。
ARM_ARCH_RE = re.compile(r'(arm|aarch64|cortex|neoverse|tsv\d|sve|thunderx|'
                         r'kunpeng|ft2000|phytium|apple-m)', re.I)
FALLBACK_OPT = '-O2'


def classify(tok):
    if tok in OPT_TOKENS:
        return 'opt'
    if tok.startswith(('-march=', '-mtune=', '-mcpu=')):
        return 'arch'
    if ISA_RE.match(tok):
        return 'isa'
    if tok in TUNE_TOKENS:
        return 'tune'
    if tok.startswith('-std='):
        return 'std'
    return None


def tokens_of(text):
    """Ordered, de-duplicated author flags of interest in *text*."""
    seen, out = set(), []
    for tok in TOKEN_RE.findall(text):
        if '$' in tok or '`' in tok or '(' in tok:
            continue                      # unexpanded make/cmake variable
        if tok in PROVIDER_OWNED:
            continue                      # provider decides these itself
        if classify(tok) is None:
            continue
        if tok not in seen:
            seen.add(tok)
            out.append(tok)
    return out


def recipe_files(directory, depth=2):
    """Recipe files in *directory*, else in its ancestors up to *depth*."""
    found = []
    cur, lvl = os.path.abspath(directory), 0
    while lvl <= depth:
        if os.path.isdir(cur):
            for name in ('Makefile', 'makefile', 'GNUmakefile'):
                p = os.path.join(cur, name)
                if os.path.isfile(p):
                    found.append(p)
            for p in sorted(glob.glob(os.path.join(cur, '*.mk'))):
                found.append(p)
            for name in ('CMakeLists.txt', 'build.sh', 'build_all.sh', 'compile.sh'):
                p = os.path.join(cur, name)
                if os.path.isfile(p):
                    found.append(p)
        if found:
            break
        cur = os.path.dirname(cur)
        lvl += 1
    return found


def recipe_flags(paths):
    """Flags from the highest-priority flag group that carries an -O level."""
    groups = []
    for p in paths:
        try:
            lines = open(p, encoding='utf-8', errors='replace').read().splitlines()
        except OSError:
            continue
        for line in lines:
            s = line.strip()
            if not s or s.startswith(('#', '//', ';')):
                continue
            flags = tokens_of(s)
            if not flags:
                continue
            prio = 3 if FLAG_LINE_RE.search(s) else 1
            if NEGATIVE_RE.search(s):
                prio -= 2
            groups.append((prio, flags))
    if not groups:
        return [], None
    best = max(g[0] for g in groups)
    chosen, seen = [], set()
    for prio, flags in groups:
        if prio != best:
            continue
        for f in flags:
            if f not in seen:
                seen.add(f)
                chosen.append(f)
    return sanitize(chosen), best


def arch_ok(tok):
    """True when an -march/-mtune/-mcpu value is usable for linux-x86_64."""
    if '+' in tok:            # ARM feature suffix syntax, e.g. armv8-a+crypto
        return False
    return ARM_ARCH_RE.search(tok) is None


def sanitize(flags):
    """Resolve conflicting flags coming from multi-target author recipes.

    - keep a single -march/-mtune/-mcpu and drop non-x86 values
    - keep a single -O level
    - keep a single C language standard; drop -std=c++* because ref.mk forces
      -std=c++2b for C++ translation units anyway
    """
    out, seen = [], set()
    for f in flags:
        cat = classify(f)
        if cat == 'arch':
            if not arch_ok(f):
                continue
            key = f.split('=')[0]
            if key in seen:
                continue
            seen.add(key)
        elif cat == 'opt':
            if 'opt' in seen:
                continue
            seen.add('opt')
        elif cat == 'std':
            if 'c++' in f:
                continue
            if 'std' in seen:
                continue
            seen.add('std')
            # -std=c23/gnu23 are the same language version as c2x/gnu2x; the
            # older spelling is accepted by every GCC that supports it.
            f = f.replace('=c23', '=c2x').replace('=gnu23', '=gnu2x')
        out.append(f)
    return out


def merge_flags(author_flags, original):
    """Author flags first, then the catalog's own non-build flags.

    Entries such as -Imake/compat/sign-07, -include immintrin.h or
    -mno-avx512f are provider compatibility workarounds, not author build
    options; they must survive a catalog refresh.
    """
    out = list(author_flags)
    for tok in (original or '').split():
        cat = classify(tok)
        # instruction-set requirements are verified by the provider (intrinsics
        # will not compile without them); compatibility flags have no author
        # counterpart either.  Optimisation/tuning/arch/std are replaced by the
        # author recipe.
        if cat in (None, 'isa') and tok not in out:
            out.append(tok)
    return out


def ensure_opt(flags):
    return list(flags) if any(f in OPT_TOKENS for f in flags) else list(flags) + [FALLBACK_OPT]


def parse_scheme_files():
    """[{evp, scheme, srcdir, mk, cflags_idx}] in file order."""
    entries = []
    for mk in sorted(glob.glob(os.path.join(SCHEMES_DIR, '*.mk'))):
        lines = open(mk, encoding='utf-8', errors='replace').read().splitlines()
        srcdir = None
        cflags_idx = None
        for idx, line in enumerate(lines):
            m = re.match(r'\s*SRCDIR\s*:=\s*(.+?)\s*$', line)
            if m:
                srcdir = m.group(1)
            if re.match(r'\s*INSTANCE_CFLAGS\s*:=', line):
                cflags_idx = idx
            # first argument is the candidate directory (sign-03), which differs
            # from the EVP name prefix (sig-03)
            m = re.search(r'ngcc_instance,\s*([^,]+),\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*([^,]+),', line)
            if m and srcdir is not None:
                orig = ''
                if cflags_idx is not None:
                    orig = re.sub(r'^\s*INSTANCE_CFLAGS\s*:=\s*', '', lines[cflags_idx])
                entries.append(dict(evp=m.group(2).strip(), scheme=m.group(1).strip(),
                                    srcdir=srcdir, mk=mk, cflags_idx=cflags_idx,
                                    cflags_orig=orig.strip()))
                srcdir, cflags_idx = None, None
    return entries


def load_impl():
    with open(IMPL_JSON, encoding='utf-8') as fh:
        return json.load(fh)


def build_plan(only_schemes=None):
    scheme_entries = parse_scheme_files()
    impl = load_impl()

    opt_cflags = {}
    for evp, entry in impl.items():
        e = entry.get('linux-x86_64')
        if e and not e.get('unavailable'):
            opt_cflags[evp] = e.get('INSTANCE_CFLAGS', '')

    plan = []
    for ent in scheme_entries:
        if only_schemes and ent['scheme'] not in only_schemes:
            continue
        d = os.path.join(CAND, ent['scheme'], ent['srcdir'].rstrip('/'))
        paths = recipe_files(d)
        flags, _prio = recipe_flags(paths)
        if flags:
            source = 'author'
        else:
            # no author recipe: mirror the opt flags so ref/opt stay comparable
            flags = [f for f in opt_cflags.get(ent['evp'], '').split() if classify(f)]
            source = 'mirror-opt' if flags else 'fallback'
            if not flags:
                flags = [FALLBACK_OPT]
        flags = merge_flags(flags, ent.get('cflags_orig', ''))
        plan.append(dict(kind='ref', evp=ent['evp'], scheme=ent['scheme'], dir=d,
                         flags=ensure_opt(flags), source=source,
                         recipe=paths[:1], cflags_idx=ent['cflags_idx'], mk=ent['mk']))

    evp_to_scheme = {e['evp']: e['scheme'] for e in scheme_entries}
    for evp, entry in impl.items():
        e = entry.get('linux-x86_64')
        if not e or e.get('unavailable'):
            continue
        scheme = evp_to_scheme.get(evp)
        if only_schemes and scheme not in only_schemes:
            continue
        d = os.path.join(CAND, scheme, e['SRCDIR'].rstrip('/')) if scheme else ''
        paths = recipe_files(d)
        flags, _prio = recipe_flags(paths)
        if flags:
            source = 'author'
        else:
            # keep the ISA requirement (intrinsics need it), neutral -O level
            keep = [f for f in e.get('INSTANCE_CFLAGS', '').split()
                    if classify(f) in ('isa', 'arch')]
            flags = keep
            source = 'isa-only'
        flags = merge_flags(flags, e.get('INSTANCE_CFLAGS', ''))
        plan.append(dict(kind='opt', evp=evp, scheme=scheme, dir=d,
                         flags=ensure_opt(flags), source=source,
                         recipe=paths[:1], cflags_idx=None, mk=None))
    return plan


def report(plan):
    for kind in ('ref', 'opt'):
        rows = [p for p in plan if p['kind'] == kind]
        print('=' * 78)
        print('%s 实例: %d' % (kind.upper(), len(rows)))
        print('=' * 78)
        for k, v in sorted(collections.Counter(p['source'] for p in rows).items()):
            print('  %-12s %d' % (k, v))
        cat = collections.Counter()
        for p in rows:
            for c in {classify(f) for f in p['flags']}:
                cat[c] += 1
        print('  含该类别实例数:', dict(sorted(cat.items())))
        o = collections.Counter(f for p in rows for f in p['flags'] if classify(f) == 'opt')
        print('  -O 分布      :', dict(o.most_common()))
        a = collections.Counter(f for p in rows for f in p['flags'] if classify(f) == 'arch')
        print('  arch 分布    :', dict(a.most_common(8)))
        t = collections.Counter(f for p in rows for f in p['flags'] if classify(f) == 'tune')
        print('  tune 分布    :', dict(t.most_common(8)))
        print()
    print('--- 抽样 ---')
    for name in ('sig-03-cedrusc-160f', 'sig-04-cedrusalpha-160f',
                 'sig-11-flextree-160f', 'kem-02-amoeba128'):
        for p in plan:
            if p['evp'] == name:
                print('  %-4s %-26s %-11s %s' % (p['kind'], p['evp'], p['source'],
                                                 ' '.join(p['flags'])))


def write_scheme_files(plan):
    by_mk = collections.defaultdict(list)
    for p in plan:
        if p['kind'] == 'ref' and p['cflags_idx'] is not None:
            by_mk[p['mk']].append(p)
    changed = 0
    for mk, rows in by_mk.items():
        lines = open(mk, encoding='utf-8', errors='replace').read().splitlines()
        for p in rows:
            new = 'INSTANCE_CFLAGS := %s' % ' '.join(p['flags'])
            if lines[p['cflags_idx']].rstrip() != new.rstrip():
                lines[p['cflags_idx']] = new
                changed += 1
        with open(mk, 'w', encoding='utf-8') as fh:
            fh.write('\n'.join(lines) + '\n')
    return changed


def write_impl_json(plan):
    impl = load_impl()
    changed = 0
    for p in plan:
        if p['kind'] != 'opt':
            continue
        e = impl.get(p['evp'], {}).get('linux-x86_64')
        if not e or e.get('unavailable'):
            continue
        new = ' '.join(p['flags'])
        if e.get('INSTANCE_CFLAGS', '') != new:
            e['INSTANCE_CFLAGS'] = new
            changed += 1
    with open(IMPL_JSON, 'w', encoding='utf-8') as fh:
        json.dump(impl, fh, indent=2, ensure_ascii=False)
        fh.write('\n')
    return changed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--report', action='store_true')
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--schemes', default='')
    args = ap.parse_args()
    only = {s.strip() for s in args.schemes.split(',') if s.strip()} or None
    plan = build_plan(only)
    if args.report or not args.write:
        report(plan)
    if args.write:
        n1 = write_scheme_files(plan)
        n2 = write_impl_json(plan)
        print('已更新: make/schemes/*.mk %d 行, implementations.json %d 条' % (n1, n2))
    return 0


if __name__ == '__main__':
    sys.exit(main())