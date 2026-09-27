import json, glob, re, html, os
HERE = os.path.dirname(os.path.abspath(__file__))

d = json.load(open(os.path.join(HERE, 'modules_raw.json')))
M = {}; F = {}
for f in sorted(glob.glob(os.path.join(HERE, 'desc', 'd*.py'))):
    ns = {}; exec(open(f).read(), ns); M.update(ns['M']); F.update(ns['F'])

TYPE = {
 'parseBool': 'Bool (Yes/No)', 'parseInt': 'Integer', 'parseUnsignedInt': 'Integer (≥0)', 'parseReal': 'Real',
 'parsePositiveNonZeroReal': 'Real (>0)', 'parsePercentToReal': 'Percent (e.g. 25%)',
 'parseDurationUnsignedInt': 'Duration (ms)', 'parseDurationReal': 'Duration (ms)',
 'parseVelocityReal': 'Velocity (units/s)', 'parseAccelerationReal': 'Acceleration (units/s²)',
 'parseAngleReal': 'Angle (degrees)', 'parseAngularVelocityReal': 'Angular velocity (deg/s)',
 'parseFrictionPerSec': 'Friction (%/s)', 'parseHeightToSpeed': 'Height', 'parseCoord3D': 'Coord (X: Y: Z:)',
 'parseRGBColor': 'Colour (R: G: B:)', 'parseColorInt': 'Colour (R: G: B: A:)',
 'parseAsciiString': 'String', 'parseAsciiStringLC': 'String', 'parseAsciiStringVector': 'String list',
 'parseAsciiStringVectorAppend': 'String list (appends)', 'parseAudioEventRTS': 'Audio event',
 'parseFXList': 'FXList', 'parseFX': 'FXList (phase-keyed / repeatable)', 'parseOCL': 'OCL (phase-keyed / repeatable)',
 'parseObjectCreationList': 'ObjectCreationList', 'parseWeaponTemplate': 'Weapon', 'parseWeapon': 'Weapon (phase-keyed / repeatable)',
 'parseParticleSystemTemplate': 'ParticleSystem', 'parseSpecialPowerTemplate': 'SpecialPower', 'parseScience': 'Science',
 'KindOfMaskType::parseFromINI': 'KindOf list', 'ObjectStatusMaskType::parseFromINI': 'ObjectStatus list',
 'ModelConditionFlags::parseFromINI': 'ModelCondition list', 'ModelConditionFlags::parseSingleBitFromINI': 'ModelCondition flag',
 'DisabledMaskType::parseFromINI': 'DisabledType list', 'parseDisabledMaskType': 'DisabledType list (ALL/NONE/+X/-X)',
 'parseDamageTypeFlags': 'DamageType list (ALL/NONE/+X/-X)', 'DamageTypeFlags::parseSingleBitFromINI': 'DamageType',
 'parseDeathTypeFlags': 'DeathType list (ALL/NONE/+X/-X)', 'parseVeterancyLevelFlags': 'Veterancy list',
 'parseBitString32': 'Flag list', 'parseIndexList': 'Enum', 'parseLookupList': 'Enum (weapon slot)',
 'parseStaticGameLODLevel': 'LOD level', 'Eva::parseEvaMessageFromIni': 'EVA event',
 'RadiusDecalTemplate::parseRadiusDecalTemplate': 'Decal sub-block', 'W3DModelDrawModuleData::parseConditionState': 'Condition-state sub-block',
 'AIUpdateModuleData::parseTurret': 'Turret sub-block', 'parseRealRange': 'Real range (min max)',
 'parseBoneNameList': 'Bone name list', 'parseBoneNameKey': 'Bone name', 'parseLowercaseNameKey': 'Name',
 'parseWeaponBoneName': 'WeaponSlot + bone base', 'parseShowHideSubObject': 'Subobject list', 'parseAnimation': 'Animation',
 'parseParticleSysBone': 'Bone + ParticleSystem', 'parseTWS': 'Weapon slot list', 'parseStateInfo': 'State (6 values)',
 'parseRider': 'Rider row (repeatable)', 'parseRiderInfo': 'Rider row', 'parseUpgradePair': 'Upgrade + boost',
 'parseInitialPayload': 'Template + count', 'parseInitialRoster': 'Template + count', 'parseAppendQuantityModifier': 'Template + count',
 'parseRunwayStrip': 'Bone pair', 'parseOCLUpgradePair': 'Science + OCL', 'parseCashHackUpgradePair': 'Science + amount',
 'parseBountyUpgradePair': 'Science + percent', 'parseFactionObjectCreationList': 'Faction + OCL', 'parseAngleFX': 'Angle + FXList',
 'CreateCrateDieModuleData::parseCrateData': 'CrateData name', 'RadiusDecalTemplate::parseOpacityMin': 'Percent',
 'RadiusDecalTemplate::parseOpacityMax': 'Percent', 'TurretAIData::parseTurretSweep': 'WeaponSlot + angle',
 'TurretAIData::parseTurretSweepSpeed': 'WeaponSlot + multiplier',
}
def typelabel(p):
    p = p.replace('INI::', '')
    if p in TYPE: return TYPE[p]
    if p.endswith('parseFXList'): return 'Bone FXList entry'
    if p.endswith('parseObjectCreationList'): return 'Bone OCL entry'
    if p.endswith('parseParticleSystem'): return 'Bone ParticleSystem entry'
    return p

def prettydef(v, parser):
    if v is None: return ''
    v = v.strip()
    m = {'TRUE': 'Yes', 'true': 'Yes', 'FALSE': 'No', 'false': 'No', 'nullptr': '—', 'NULL': '—', 'None': ''}
    if v in m: return m[v]
    v = re.sub(r'^(-?\d+\.?\d*)f$', r'\1', v)
    v = re.sub(r'(\d)\.0$', r'\1', v)
    if 'Duration' in parser and re.fullmatch(r'\d+', v) and v != '0':
        return v + ' frames'
    if 'LOGICFRAMES_PER_SECOND' in v:
        try:
            n = eval(v.replace('LOGICFRAMES_PER_SECOND', '30'))
            return f"{int(n*1000/30)} ms" if 'Duration' in parser else str(n)
        except Exception: pass
    if 'Percent' in parser:
        try: return f"{float(v)*100:g}%"
        except Exception: pass
    for pre in ('MODELCONDITION_', 'LOCOMOTORSET_', 'DAMAGE_TYPE_FLAGS_', 'LEVEL_', 'SHADOW_', 'CREATE_GUNSHIP_', 'WEAPONBONUSCONDITION_', 'HORDEACTION_', 'STATIC_GAME_LOD_'):
        if v.startswith(pre): return v[len(pre):] if pre not in ('SHADOW_', 'CREATE_GUNSHIP_') else v
    if v == 'PRIMARY_WEAPON': return 'PRIMARY'
    if v in ('KINDOFMASK_NONE', 'DISABLEDMASK_NONE'): return '(none)'
    if v == 'DISABLEDMASK_ALL': return 'ALL'
    if v == 'SCIENCE_INVALID': return '(none)'
    if v == 'AsciiString::TheEmptyString' or v == '""': return '(empty)'
    return v

# ---- pattern descriptions ----
STATE = {'Pristine': 'PRISTINE', 'Damaged': 'DAMAGED', 'ReallyDamaged': 'REALLYDAMAGED', 'Rubble': 'RUBBLE'}
BLAST = {
 'Enabled': ("Enable blast wave {n}.", "Yes"), 'Delay': ("Time (ms) after detonation when blast wave {n} happens.", "{d}"),
 'ScorchDelay': ("Time (ms) after detonation when blast wave {n} places its scorch mark.", "{d}"),
 'InnerRadius': ("Blast wave {n}: radius within which MaxDamage is dealt.", "{r1}"),
 'OuterRadius': ("Blast wave {n}: outer radius; damage falls off to MinDamage here.", "{r2}"),
 'MaxDamage': ("Blast wave {n}: damage at the inner radius.", "1000"), 'MinDamage': ("Blast wave {n}: damage at the outer radius.", "200"),
 'ToppleSpeed': ("Blast wave {n}: speed at which trees/topple-able objects are knocked over.", "0.5"),
 'PushForce': ("Blast wave {n}: force pushing units away from the centre.", "10"),
}
def pattern_desc(src, name):
    m = re.fullmatch(r'Blast(\d)(\w+)', name)
    if m and src.startswith('NeutronMissile') and m.group(2) in BLAST:
        n = int(m.group(1)); t, e = BLAST[m.group(2)]
        return t.format(n=n), e.format(d=(n-1)*300, r1=n*50, r2=n*100)
    m = re.fullmatch(r'(Pristine|Damaged|ReallyDamaged|Rubble)(FXList|OCL|ParticleSystem)(\d+)', name)
    if m:
        st, kind, n = m.groups()
        what = {'FXList': 'FXList', 'OCL': 'ObjectCreationList', 'ParticleSystem': 'particle system'}[kind]
        tag = {'FXList': 'FXList:FX_Name', 'OCL': 'OCL:OCL_Name', 'ParticleSystem': 'PSys:PSysName'}[kind]
        if src.startswith('BoneFX'):
            return (f"Effect slot {n} for the {STATE[st]} state: a {what} fired at a bone at random intervals while the object is in this state. Syntax: bone:<Bone> OnlyOnce:<Yes/No> <minDelay ms> <maxDelay ms> {tag.split(':')[0]}:<name>.",
                    f"bone:Fire0{n if int(n)<10 else 1} OnlyOnce:No 1000 3000 {tag}")
        return (f"Effect slot {n} for the transition into {STATE[st]}: a {what} played once at a location or bone. Syntax: Loc: X:0 Y:0 Z:0 {tag.split(':')[0]}:<name>, or Bone:<Bone> RandomBone:<Yes/No> {tag.split(':')[0]}:<name>.",
                f"Bone:Fire0{n if int(n)<10 else 1} RandomBone:No {tag}")
    m = re.fullmatch(r'ParticleSystem(\d+)', name)
    if m and src.startswith('Firestorm'):
        return (f"Particle system #{m.group(1)} created when the firestorm starts; scaled with the growing geometry.", "FirestormFlames")
    return None

PAIRS = {
 'MissileAIUpdateV2': ['MissileAIUpdate'], 'OverlordContainV2': ['OverlordContain'], 'RiderChangeContainV2': ['RiderChangeContain'],
 'WeaponBonusUpdateV2': ['WeaponBonusUpdate'], 'PointDefenseUpdateV2': ['PointDefenseLaserUpdate'],
 'ContainedTransitionDamageFXV2': ['TransitionDamageFX'], 'FireOCLBehaviorV2': ['GrantStealthBehavior'],
 'DecalUpdateV2': ['RadiusDecalUpdate'], 'PersistentDecalUpdateV2': ['RadiusDecalUpdate', 'ArmorUpgrade'],
 'HealAIUpdateV2': ['DozerAIUpdate'], 'DamageOverTimeUpdateV2': ['PoisonedBehavior', 'FlammableUpdate'],
 'BreakApartDeathBehaviorV2': ['SlowDeathBehavior'], 'SwitchStateWhenDamagedBehaviorV2': ['FireWeaponWhenDamagedBehavior'],
 'SwitchStateV2': ['SpecialAbilityUpdate'], 'ShieldGeneratorUpdateV2': ['SwitchStateV2'],
 'ShieldGeneratorActivateV2': ['SpecialAbility', 'SwitchStateV2Activate'], 'SwitchStateV2Activate': ['SpecialAbility'],
 'ShieldedBody': ['ActiveBody'], 'W3DPersistentAnimModelDraw': ['W3DModelDraw'], 'W3DWheeledTankDraw': ['W3DTankDraw', 'W3DTruckDraw'],
 'W3DOverlordWheeledTankDraw': ['W3DOverlordTankDraw'], 'W3DBreakApartPieceDraw': ['W3DDebrisDraw'],
 'ProjectileClipFeedbackUpdateV2': [],
}
MOD_EXTRA = {'ShieldedBody', 'W3DPersistentAnimModelDraw', 'W3DWheeledTankDraw', 'W3DBreakApartPieceDraw', 'W3DOverlordWheeledTankDraw'}
PATCHED = {'W3DModelDraw', 'W3DTankDraw', 'W3DTankTruckDraw'}
REV = {}
for k, vs in PAIRS.items():
    for v in vs: REV.setdefault(v, []).append(k)

def category(r):
    a = r['ancestors']; b = set(a)
    if r['iniKey'] == 'Body': return 'Body'
    if r['iniKey'] == 'Draw': return 'Draw'
    if r['iniKey'] == 'ClientUpdate': return 'Client update'
    if 'OpenContain' in b or 'ContainModuleInterface' in b: return 'Contain'
    if 'UpgradeModule' in b: return 'Upgrade'
    if 'CreateModule' in b: return 'Create'
    if 'CollideModule' in b or 'CrateCollide' in b: return 'Collide / crate'
    if 'SpecialPowerModule' in b: return 'Special power'
    if 'AIUpdateInterface' in b: return 'AI update'
    if 'DieModule' in b or 'DieModuleInterface' in b and 'OpenContain' not in b: return 'Die / death'
    if 'DamageModule' in b: return 'Damage'
    return 'Update / behavior'

# counterpart lookup for field desc fallback
name_index = {}
for k in F:
    s, n = k.split('.', 1); name_index.setdefault(n, []).append(k)

def field_desc(src, fname, f, modname):
    k = src + '.' + fname
    if k in F: return F[k], False
    p = pattern_desc(src, fname)
    if p: return p, False
    if 'V2' in src:
        k2 = src.replace('V2', '') + '.' + fname
        if k2 in F: return F[k2], False
        for cp in PAIRS.get(modname, []):
            for kk in name_index.get(fname, []):
                if kk.startswith(cp): return F[kk], False
    if fname in name_index: return F[name_index[fname][0]], True
    c = ' '.join(x for x in [f.get('inlineComment', ''), f.get('mprecomment', ''), f.get('mcomment', '')] if x)
    return (c or 'No description available.', ''), True

MISSING = []
groups = {}
mods = []
for r in d:
    info = M.get(r['name'])
    if info is None:
        MISSING.append(r['name'])
        lines = [f"{r['iniKey']} = {r['name']} ModuleTag_01"] + [f"  {f['name']} = ..." for g in r['groups'] if not g.get('nested') for f in g['fields'][:12]] + ["End"]
        info = dict(sum='(No description written yet - add one in scripts/module-codex/desc/.)', body='This module was found in the source but has no written description yet. Its field list below is extracted from the code.', ex='\n'.join(lines))
    origin = 'mod' if (r['name'].endswith('V2') or r['name'] in MOD_EXTRA) else ('patched' if r['name'] in PATCHED else 'vanilla')
    grefs = []
    own = r['dataClass']
    for g in r['groups']:
        gid = g['source'] + ('#' + r['name'] if g['source'].endswith('V2ModuleData') else '')
        gid = g['source']
        if gid not in groups:
            fl = []
            for f in g['fields']:
                (desc, ex), inferred = field_desc(g['source'], f['name'], f, r['name'])
                fl.append(dict(n=f['name'], t=typelabel(f['parser']), d=prettydef(f.get('default'), f['parser']), x=desc, e=ex, i=bool(inferred)))
            groups[gid] = dict(src=g['source'], nested=g.get('nested', ''), file=g.get('file'), fields=fl)
        role = 'nested' if g.get('nested') else ('own' if g['source'] == own else ('mixin' if g['source'] in ('UpgradeMuxData', 'DieMuxData') else 'inherited'))
        grefs.append([gid, role])
    # order: own first, then mixin, inherited, nested
    orderk = {'own': 0, 'mixin': 1, 'inherited': 2, 'nested': 3}
    grefs.sort(key=lambda x: orderk[x[1]])
    notes = []
    if origin != 'vanilla':
        for b in r['headerBlocks'] + r['cppBlocks']:
            if 'GeneralsMod' in b or origin == 'mod':
                if '@debug' in b: continue
                if b not in notes and len(b) > 60: notes.append(b)
    cps = PAIRS.get(r['name'], []) + REV.get(r['name'], [])
    mods.append(dict(name=r['name'], key=r['iniKey'], cat=category(r), origin=origin, sum=info['sum'], body=info['body'], ex=info['ex'],
        header=r['header'], cpp=r['cpp'], bases=[b for b in r['bases'] if '//' not in b], data=r['dataClass'] or '(none)', groups=grefs,
        cp=cps, notes=notes[:10]))

mods.sort(key=lambda m: m['name'].lower())
data = dict(mods=mods, groups=groups)
js = json.dumps(data, separators=(',', ':'), ensure_ascii=False).replace('</', '<\\/')
tpl = open(os.path.join(HERE, 'template.html')).read()
out = tpl.replace('/*__DATA__*/', 'const DATA=' + js + ';')
open(os.path.join(HERE, '..', '..', 'generals-module-reference.html'), 'w').write(out)
print('modules without a description:', MISSING)
print(len(mods), 'modules', len(groups), 'groups', len(out)//1024, 'KB')
inf = sum(1 for g in groups.values() for f in g['fields'] if f['i'])
print('inferred', inf, [ (g['src'],f['n']) for g in groups.values() for f in g['fields'] if f['i']][:40])
