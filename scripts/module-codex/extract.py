import os, re, json, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
# preference order: GeneralsMD first, then Core
files = {}
order = ['GeneralsMD', 'Core']
allfiles = []
for pre in order:
    for dp, dn, fn in os.walk(os.path.join(ROOT, pre)):
        for f in fn:
            if f.endswith(('.h', '.cpp', '.inl')):
                p = os.path.join(dp, f)
                allfiles.append(p)
texts = {}
for p in allfiles:
    try:
        texts[p] = open(p, encoding='latin-1').read()
    except Exception as e:
        pass

def rel(p): return os.path.relpath(p, ROOT).replace('\\', '/')

def strip_comments_keep(s):
    return s

# module list
mf = open(os.path.join(ROOT, 'GeneralsMD/Code/GameEngine/Source/Common/Thing/ModuleFactory.cpp'), encoding='latin-1').read()
wmf = open(os.path.join(ROOT, 'Core/GameEngineDevice/Source/W3DDevice/Common/Thing/W3DModuleFactory.cpp'), encoding='latin-1').read()
def addmods(t):
    out = []
    for m in re.finditer(r'^\s*addModule\(\s*(\w+)\s*\)', t, re.M):
        if m.group(1) not in out: out.append(m.group(1))
    return out
logic_mods = addmods(mf)
draw_mods = addmods(wmf)

# find class definitions: class X : public A, public B {
classdef_re = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*(?:final\s*)?:\s*([^{;]+)\{', re.M)
classes = {}  # name -> (file, bases, start, bodytext)
def find_body(t, start):
    # start at '{'
    i = t.index('{', start)
    depth = 0
    j = i
    while j < len(t):
        c = t[j]
        if c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return t[i:j+1], i
        j += 1
    return t[i:], i
for p in allfiles:
    t = texts.get(p)
    if t is None or not p.endswith('.h'): continue
    for m in classdef_re.finditer(t):
        name = m.group(1)
        if name in classes: continue
        bases = [b.strip().replace('public', '').replace('virtual', '').replace('protected','').replace('private','').strip() for b in m.group(2).split(',')]
        body, bstart = find_body(t, m.end()-1)
        classes[name] = dict(file=p, bases=bases, body=body, pos=m.start())
# also classes with no bases
for p in allfiles:
    t = texts.get(p)
    if t is None or not p.endswith('.h'): continue
    for m in re.finditer(r'^\s*(?:class|struct)\s+(\w+)\s*(?://[^\n]*)?\s*\{', t, re.M):
        name = m.group(1)
        if name in classes: continue
        body, bstart = find_body(t, m.end()-1)
        classes[name] = dict(file=p, bases=[], body=body, pos=m.start())

def module_data_class(cls, seen=None):
    if seen is None: seen = set()
    if cls in seen or cls not in classes: return None
    seen.add(cls)
    body = classes[cls]['body']
    m = re.search(r'MAKE_STANDARD_MODULE_(?:DATA_)?MACRO(?:_WITH_MODULE_DATA)?(?:_ABC)?\(\s*' + cls + r'\s*,\s*(\w+)\s*\)', body)
    if m: return m.group(1)
    for b in classes[cls]['bases']:
        r = module_data_class(b, seen)
        if r: return r
    return None

def find_func_body(name_regex):
    """find definition body of function; returns (file, body)"""
    key = re.findall(r'\w{4,}', name_regex.replace('\\b',''))
    for p in allfiles:
        t = texts[p]
        if key and key[0] not in t: continue
        for m in re.finditer(name_regex, t):
            # ensure it's a definition: next non-space after ')' is '{'
            k = m.end()
            # find matching ')' after
            rest = t[k:k+400]
            mm = re.match(r'[^;{]*\{', rest)
            if mm:
                body, s = find_body(t, k)
                return p, body
    return None, None

def get_buildfieldparse(dc):
    # inline in class body?
    if dc in classes:
        body = classes[dc]['body']
        m = re.search(r'static\s+void\s+buildFieldParse\s*\(\s*MultiIniFieldParse\s*&\s*\w+\s*\)\s*\{', body)
        if m:
            b, s = find_body(body, m.end()-1)
            return classes[dc]['file'], b
        if re.search(r'buildFieldParse', body):
            p, b = find_func_body(r'\b' + dc + r'::buildFieldParse\s*\(')
            if b: return p, b
    return None, None

entry_re = re.compile(r'\{\s*"([^"]+)"\s*,\s*([\w:]+)\s*,\s*([^,{}]*?)\s*,\s*(offsetof\s*\(\s*(\w+)\s*,\s*([\w\.\[\]]+)\s*\)|[^}]*?)\s*\}', re.S)

def parse_table_text(txt):
    out = []
    # remove line comments but record them
    for m in entry_re.finditer(txt):
        name, parser, ud, off = m.group(1), m.group(2), m.group(3), m.group(4)
        member = m.group(6)
        owner = m.group(5)
        # trailing comment on same line
        line_end = txt.find('\n', m.end())
        trail = txt[m.end():line_end if line_end>=0 else None]
        cm = re.search(r'//+\s*<?\s*(.*)', trail)
        out.append(dict(name=name, parser=parser, userData=ud.strip(), member=member, owner=owner, inlineComment=cm.group(1).strip() if cm else ''))
    return out

def find_table(tname, near_text, allow_global=True):
    m = re.search(r'FieldParse\s+' + re.escape(tname) + r'\s*\[\s*\]\s*=\s*\{', near_text)
    if m:
        b, s = find_body(near_text, m.end()-1)
        return b
    if not allow_global: return None
    for p in allfiles:
        t = texts[p]
        if tname not in t: continue
        m = re.search(r'FieldParse\s+(?:\w+::)?' + re.escape(tname) + r'\s*\[\s*\]\s*=\s*\{', t)
        if m:
            b, s = find_body(t, m.end()-1)
            return b
    return None

mux_cache = {}
def mux_fields(muxcls):
    if muxcls in mux_cache: return mux_cache[muxcls]
    p, body = find_func_body(r'\b' + muxcls + r'::getFieldParse\s*\(\s*\)')
    if body is None and muxcls in classes:
        cb = classes[muxcls]['body']
        mm = re.search(r'getFieldParse\s*\(\s*\)\s*\{', cb)
        if mm:
            body, _ = find_body(cb, mm.end()-1); p = classes[muxcls]['file']
    fields = []
    if body:
        m = re.search(r'FieldParse\s+(\w+)\s*\[\s*\]\s*=\s*\{', body)
        if m:
            tb, s = find_body(body, m.end()-1)
            fields = parse_table_text(tb)
    mux_cache[muxcls] = (fields, rel(p) if p else None)
    return mux_cache[muxcls]

def data_parent(dc):
    if dc in classes and classes[dc]['bases']:
        return classes[dc]['bases'][0]
    return None

def collect_fields(dc, depth=0, seen=None):
    """returns list of groups: [{source: className, fields:[...]}] ordered base-first"""
    if seen is None: seen=set()
    if not dc or dc in seen or depth > 12: return []
    seen.add(dc)
    p, body = get_buildfieldparse(dc)
    groups = []
    if body is None:
        par = data_parent(dc)
        if par: return collect_fields(par, depth+1, seen)
        return []
    # parent calls
    for m in re.finditer(r'(\w+)::buildFieldParse\s*\(\s*\w+\s*\)', body):
        if m.group(1) != dc:
            groups += collect_fields(m.group(1), depth+1, seen)
    own = []
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    body = '\n'.join(l for l in body.split('\n') if not l.strip().startswith('//'))
    for m in re.finditer(r'\.add\s*\(\s*([\w:]+)\s*(\(\s*\))?\s*(,\s*offsetof\s*\(\s*\w+\s*,\s*(\w+)\s*\))?\s*\)', body):
        tname = m.group(1)
        if '::getFieldParse' in tname or m.group(2):
            muxcls = tname.split('::')[0]
            f, mp = mux_fields(muxcls)
            if f:
                groups.append(dict(source=muxcls, via=m.group(4), fields=f, file=mp))
            continue
        tb = find_table(tname, body, allow_global=False)
        if tb is None and p: tb = find_table(tname, texts[p], allow_global=(tname != 'dataFieldParse'))
        if tb:
            own += parse_table_text(tb)
    if own:
        groups.append(dict(source=dc, fields=own, file=rel(p)))
    return groups

def member_info(dc, member):
    """find comment + type for member declaration in data class (or its parents)"""
    seen=set()
    c = dc
    while c and c in classes and c not in seen:
        seen.add(c)
        body = classes[c]['body']
        base_member = member.split('.')[0].split('[')[0]
        lines = body.split('\n')
        for i, ln in enumerate(lines):
            if re.search(r'\b' + re.escape(base_member) + r'\b\s*(\[[^\]]*\])?\s*(=[^;]*)?;', ln) and '(' not in ln.split(base_member)[0][-3:]:
                code = ln.split('//')[0]
                typ = code.split(base_member)[0].strip()
                cm = ''
                if '//' in ln:
                    cm = re.sub(r'^/+\s*<?\s*', '', ln[ln.index('//'):]).strip()
                # preceding comment lines
                pre = []
                j = i-1
                while j >= 0 and lines[j].strip().startswith('//') and len(pre) < 6:
                    pre.insert(0, re.sub(r'^\s*/+\s*<?\s*', '', lines[j]).strip())
                    j -= 1
                return dict(type=typ, comment=cm, precomment=' '.join(x for x in pre if x and not set(x) <= set('-=/*')))
        c = data_parent(c)
    return {}

def defaults_of(dc):
    d = {}
    # inline ctor in class body
    bodies = []
    if dc in classes:
        body = classes[dc]['body']
        m = re.search(r'\b' + dc + r'\s*\(\s*\)\s*(:[^{]*)?\{', body)
        if m:
            b, s = find_body(body, m.end()-1)
            bodies.append((m.group(1) or '') + b)
    p, b = find_func_body(r'\b' + dc + r'::' + dc + r'\s*\(\s*\)')
    if b:
        # include initializer list
        t = texts[p]
        idx = t.find(b)
        m = re.search(r'\b' + dc + r'::' + dc + r'\s*\(\s*\)\s*(:[^{]*)?\{', t)
        bodies.append(((m.group(1) or '') if m else '') + b)
    for bb in bodies:
        for m in re.finditer(r'\b(m_\w+(?:\.\w+)*(?:\[[^\]]*\])?)\s*=\s*([^;=]+);', bb):
            d.setdefault(m.group(1), m.group(2).strip())
        for m in re.finditer(r'\b(m_\w+)\s*\(([^()]*(?:\([^()]*\))?[^()]*)\)', bb.split('{')[0] if bb.startswith(':') else ''):
            d.setdefault(m.group(1), m.group(2).strip())
    return d

def header_doc(p):
    t = texts[p]
    # strip license
    t2 = re.sub(r'/\*.*?\*/', '', t, count=1, flags=re.S)
    desc = ''
    m = re.search(r'//\s*Desc(?:ription)?:\s*(.*)', t2)
    if m: desc = m.group(1).strip()
    author = ''
    m = re.search(r'//\s*Author:\s*(.*)', t2)
    if m: author = m.group(1).strip()
    # collect comment blocks (//) of length >= 3 lines that are not separators
    blocks = []
    cur = []
    for ln in t2.split('\n'):
        s = ln.strip()
        if s.startswith('//') or s.startswith('*') or s.startswith('/*'):
            c = re.sub(r'^(/\*+|\*+/?|/+)\s*<?\s*', '', s).strip()
            if c and not set(c) <= set('-=/*_ '):
                cur.append(c)
        else:
            if len(cur) >= 3: blocks.append(' '.join(cur))
            cur = []
    return desc, author, blocks

def ancestors(cls):
    out = []
    c = cls; seen=set()
    stack=[cls]
    while stack:
        c = stack.pop(0)
        if c in seen: continue
        seen.add(c); out.append(c)
        if c in classes:
            stack += classes[c]['bases']
    return out

def find_cpp(cls):
    for p in allfiles:
        if p.endswith('.cpp') and cls in texts[p] and re.search(r'\b' + cls + r'::' + cls + r'\s*\(', texts[p]):
            return p
    return None

result = []
for kind, mods in (('logic', logic_mods), ('draw', draw_mods)):
    for mname in mods:
        if mname not in classes:
            result.append(dict(name=mname, error='class not found')); continue
        c = classes[mname]
        anc = ancestors(mname)
        if 'BodyModule' in anc: iniKey = 'Body'
        elif 'DrawModule' in anc: iniKey = 'Draw'
        elif 'ClientUpdateModule' in anc: iniKey = 'ClientUpdate'
        else: iniKey = 'Behavior'
        dc = module_data_class(mname)
        groups = collect_fields(dc) if dc else []
        defs = {}
        chain = []
        x = dc; s=set()
        while x and x not in s:
            s.add(x); chain.append(x)
            defs.update({k:v for k,v in defaults_of(x).items() if k not in defs})
            x = data_parent(x)
        for g in groups:
            for f in g['fields']:
                if f['member']:
                    mi = member_info(g['source'] if g['source'] in classes else dc, f['member']) or member_info(dc, f['member'])
                    f.update({('m'+k): v for k, v in mi.items()})
                    f['default'] = defs.get(f['member'])
        hp = c['file']
        cpp = find_cpp(mname)
        desc, author, blocks = header_doc(hp)
        cdesc, cauthor, cblocks = header_doc(cpp) if cpp else ('', '', [])
        # interfaces
        result.append(dict(name=mname, kind=kind, iniKey=iniKey, header=rel(hp), cpp=rel(cpp) if cpp else None,
            bases=c['bases'], ancestors=anc, dataClass=dc, dataChain=chain, groups=groups,
            desc=desc, author=author, headerBlocks=blocks[:12], cppDesc=cdesc, cppBlocks=cblocks[:6]))

json.dump(result, open(os.path.join(HERE, 'modules_raw.json'), 'w'), indent=1)
print(len(result), 'modules')
print('errors', [r['name'] for r in result if 'error' in r])
print('nofields', [r['name'] for r in result if 'error' not in r and not any(g['fields'] for g in r['groups'])])

# ---- nested sub-block groups ----
def table_in_func(func_regex, tbl='FieldParse'):
    p, body = find_func_body(func_regex)
    if not body: return [], None
    m = re.search(r'FieldParse\s+(\w+)\s*\[\s*\]\s*=\s*\{', body)
    if not m: return [], None
    tb, s = find_body(body, m.end()-1)
    return parse_table_text(tb), rel(p)
cond_fields, cond_file = table_in_func(r'\bW3DModelDrawModuleData::parseConditionState\s*\(')
turret_fields, turret_file = table_in_func(r'\bTurretAIData::buildFieldParse\s*\(')
for f in turret_fields:
    if f['member']:
        mi = member_info('TurretAIData', f['member']); f.update({('m'+k): v for k, v in mi.items()})
        f['default'] = defaults_of('TurretAIData').get(f['member'])
for f in cond_fields:
    if f['member']:
        mi = member_info('ModelConditionInfo', f['member'].split('.')[0].split('[')[0]); f.update({('m'+k): v for k, v in mi.items()})
for r in result:
    srcs = [g['source'] for g in r.get('groups', [])]
    if 'W3DModelDrawModuleData' in srcs:
        r['groups'].append(dict(source='ModelConditionInfo', nested='ConditionState / DefaultConditionState / TransitionState sub-block', fields=cond_fields, file=cond_file))
    if 'AIUpdateModuleData' in srcs:
        r['groups'].append(dict(source='TurretAIData', nested='Turret / AltTurret sub-block', fields=turret_fields, file=turret_file))
json.dump(result, open(os.path.join(HERE, 'modules_raw.json'), 'w'), indent=1)
print('nested', len(cond_fields), len(turret_fields))

rd_fields, rd_file = table_in_func(r'\bRadiusDecalTemplate::parseRadiusDecalTemplate\s*\(')
for r in result:
    names = [f['name'] for g in r.get('groups', []) for f in g['fields'] if 'parseRadiusDecalTemplate' in f['parser']]
    if names:
        r['groups'].append(dict(source='RadiusDecalTemplate', nested=' / '.join(names) + ' sub-block', fields=rd_fields, file=rd_file))
json.dump(result, open(os.path.join(HERE, 'modules_raw.json'), 'w'), indent=1)
print('rd', len(rd_fields))
