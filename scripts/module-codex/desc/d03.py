M = {}
F = {}

F.update({
'RadiusDecalTemplate.Texture': ("Texture (TGA/DDS) projected onto the terrain.", "SCCHaloTarget"),
'RadiusDecalTemplate.Style': ("Blend style of the decal: SHADOW_ALPHA_DECAL (normal alpha blend) or SHADOW_ADDITIVE_DECAL (glowing, additive).", "SHADOW_ADDITIVE_DECAL"),
'RadiusDecalTemplate.OpacityMin': ("Minimum opacity (percent) of the throb effect.", "25%"),
'RadiusDecalTemplate.OpacityMax': ("Maximum opacity (percent) of the throb effect.", "100%"),
'RadiusDecalTemplate.OpacityThrobTime': ("Duration (ms) of one full opacity throb cycle between min and max.", "500"),
'RadiusDecalTemplate.Color': ("Tint colour of the decal (R:G:B:A or 0 to use the player colour).", "R:255 G:0 B:0 A:255"),
'RadiusDecalTemplate.OnlyVisibleToOwningPlayer': ("If Yes (default), only the owning player (and allies) see the decal; No makes it visible to everyone.", "Yes"),
})

M['AssistedTargetingUpdate'] = dict(
 sum="Lets other units (e.g. a Patriot with a Laser-assisted target) order this unit to fire at targets beyond its own acquisition, drawing a laser between them.",
 body="""Used by the USA Patriot Missile System's 'assisted targeting': when a friendly unit with this module is told by an 'assisting' ally to attack something, it fires <b>AssistingClipSize</b> shots of its <b>AssistingWeaponSlot</b> weapon at that target even if it would not normally acquire it, and draws a laser object from the assisting unit (<b>LaserFromAssisted</b>) and to the target (<b>LaserToTarget</b>) to visualize the link.""",
 ex="""Behavior = AssistedTargetingUpdate ModuleTag_Assist
  AssistingClipSize   = 4
  AssistingWeaponSlot = SECONDARY
  LaserFromAssisted   = PatriotBinaryDataStream
  LaserToTarget       = PatriotBinaryDataStream
End""")
F.update({
'AssistedTargetingUpdateModuleData.AssistingClipSize': ("Number of shots fired per assist request.", "4"),
'AssistedTargetingUpdateModuleData.AssistingWeaponSlot': ("Weapon slot used for assisted shots (PRIMARY/SECONDARY/TERTIARY).", "SECONDARY"),
'AssistedTargetingUpdateModuleData.LaserFromAssisted': ("Object template of the laser drawn from the assisting unit to this unit.", "PatriotBinaryDataStream"),
'AssistedTargetingUpdateModuleData.LaserToTarget': ("Object template of the laser drawn from this unit to the target.", "PatriotBinaryDataStream"),
})

M['AutoFindHealingUpdate'] = dict(
 sum="Wounded idle infantry automatically walk to the nearest healing unit/structure.",
 body="""Every <b>ScanRate</b> an idle unit checks its health. If it is below <b>AlwaysHeal</b> (fraction of max health) it looks within <b>ScanRange</b> for something that can heal it (ambulance, barracks, hospital…) and goes there. Between <b>AlwaysHeal</b> and <b>NeverHeal</b> it only does so when idle; above <b>NeverHeal</b> it never bothers.""",
 ex="""Behavior = AutoFindHealingUpdate ModuleTag_FindHeal
  ScanRate   = 1000
  ScanRange  = 300
  NeverHeal  = 0.85
  AlwaysHeal = 0.25
End""")
F.update({
'AutoFindHealingUpdateModuleData.ScanRate': ("Interval (ms) between health checks/scans.", "1000"),
'AutoFindHealingUpdateModuleData.ScanRange': ("Radius searched for a healer.", "300"),
'AutoFindHealingUpdateModuleData.NeverHeal': ("Health fraction (0..1) above which the unit never seeks healing.", "0.85"),
'AutoFindHealingUpdateModuleData.AlwaysHeal': ("Health fraction (0..1) below which the unit always seeks healing.", "0.25"),
})

M['BaseRegenerateUpdate'] = dict(
 sum="Base structures regenerate health automatically after not being damaged for a while (rates from GameData.ini).",
 body="""Put on structures. After the structure has not been damaged for GameData's <i>BaseRegenDelay</i>, it heals <i>BaseRegenHealthPercentPerSecond</i> of its max health per second. The values are global (GameData.ini), so the module has no fields of its own. Structures under construction, sold or disabled do not regenerate.""",
 ex="""Behavior = BaseRegenerateUpdate ModuleTag_BaseRegen
End""")

M['StealthDetectorUpdate'] = dict(
 sum="Detects (reveals) stealthed enemy units within a range.",
 body="""Every <b>DetectionRate</b> the object scans <b>DetectionRange</b> (0 = use its vision range) and marks every stealthed enemy in range as DETECTED so everyone can see and attack it. Plays ping sounds and IR particle effects on detected units. <b>ExtraRequiredKindOf</b>/<b>ExtraForbiddenKindOf</b> filter what can be detected. By default it doesn't work while the detector is garrisoned or inside a transport (see CanDetectWhile*). <b>InitiallyDisabled</b> lets an upgrade (via StatusBitsUpgrade or code) enable it later.""",
 ex="""Behavior = StealthDetectorUpdate ModuleTag_Detector
  DetectionRate           = 500
  DetectionRange          = 250
  InitiallyDisabled       = No
  PingSound               = StealthDetectorPing
  IRParticleSysName       = IRCameraEffect
  IRParticleSysBone       = Detector01
  CanDetectWhileGarrisoned= Yes
  CanDetectWhileContained = No
End""")
F.update({
'StealthDetectorUpdateModuleData.DetectionRate': ("Interval (ms) between detection scans.", "500"),
'StealthDetectorUpdateModuleData.DetectionRange': ("Detection radius. 0 = use the object's vision range.", "250"),
'StealthDetectorUpdateModuleData.InitiallyDisabled': ("If Yes, detection is off until enabled by an upgrade/code.", "No"),
'StealthDetectorUpdateModuleData.PingSound': ("Sound played on each scan that detects something.", "StealthDetectorPing"),
'StealthDetectorUpdateModuleData.LoudPingSound': ("Louder ping used for more significant detections.", "StealthDetectorPingLoud"),
'StealthDetectorUpdateModuleData.IRBeaconParticleSysName': ("IR beacon particle system (local-player visual).", ""),
'StealthDetectorUpdateModuleData.IRParticleSysName': ("IR particle system shown on the detector while scanning.", "IRCameraEffect"),
'StealthDetectorUpdateModuleData.IRBrightParticleSysName': ("Bright IR particle variant.", ""),
'StealthDetectorUpdateModuleData.IRGridParticleSysName': ("IR grid particle variant.", ""),
'StealthDetectorUpdateModuleData.IRParticleSysBone': ("Bone the IR particle systems are attached to.", "Detector01"),
'StealthDetectorUpdateModuleData.ExtraRequiredKindOf': ("Stealthed units must have one of these KindOfs to be detected.", ""),
'StealthDetectorUpdateModuleData.ExtraForbiddenKindOf': ("Stealthed units with any of these KindOfs can't be detected by this detector.", ""),
'StealthDetectorUpdateModuleData.CanDetectWhileGarrisoned': ("If Yes, still detects while garrisoned in a building.", "Yes"),
'StealthDetectorUpdateModuleData.CanDetectWhileContained': ("If Yes, still detects while inside a transport.", "No"),
})

M['StealthUpdate'] = dict(
 sum="Makes the object stealthed (invisible to enemies) under configurable conditions, including disguise (Bomb Truck).",
 body="""After <b>StealthDelay</b> without breaking stealth the object becomes stealthed: enemies can't see or target it unless a detector reveals it. <b>StealthForbiddenConditions</b> lists what breaks stealth (ATTACKING, MOVING, USING_ABILITY, FIRING_PRIMARY, TAKING_DAMAGE, RIDERS_ATTACKING, NO_BLACK_MARKET…), <b>MoveThresholdSpeed</b> lets it move slowly while staying hidden. Friendly players see it at an opacity pulsing between <b>FriendlyOpacityMin/Max</b>. <b>InnateStealth</b> = No means stealth comes only from an upgrade/special power (<b>GrantedBySpecialPower</b>). <b>DisguisesAsTeam</b> enables the GLA Bomb Truck style disguise (with transition FX). <b>RequiredStatus</b>/<b>ForbiddenStatus</b> gate stealth on object status bits, e.g. only while DEPLOYED.""",
 ex="""Behavior = StealthUpdate ModuleTag_Stealth
  StealthDelay               = 2500
  StealthForbiddenConditions = ATTACKING USING_ABILITY
  MoveThresholdSpeed         = 3
  InnateStealth              = Yes
  FriendlyOpacityMin         = 50%
  FriendlyOpacityMax         = 100%
  PulseFrequency             = 500
  OrderIdleEnemiesToAttackMeUponReveal = No
  EnemyDetectionEvaEvent     = EnemyBlackLotusDetected
  OwnDetectionEvaEvent       = OwnBlackLotusDetected
End""")
F.update({
'StealthUpdateModuleData.StealthDelay': ("Time (ms) the object must go without breaking stealth before it becomes stealthed.", "2500"),
'StealthUpdateModuleData.MoveThresholdSpeed': ("Max speed at which the unit may move without breaking stealth (only relevant if MOVING is a forbidden condition).", "3"),
'StealthUpdateModuleData.StealthForbiddenConditions': ("Conditions that break/prevent stealth: ATTACKING, MOVING, USING_ABILITY, FIRING_PRIMARY, FIRING_SECONDARY, FIRING_TERTIARY, NO_BLACK_MARKET, TAKING_DAMAGE, RIDERS_ATTACKING.", "ATTACKING USING_ABILITY"),
'StealthUpdateModuleData.HintDetectableConditions': ("Object statuses that make the unit shimmer/hint to enemies even while stealthed.", "IS_FIRING_WEAPON"),
'StealthUpdateModuleData.RequiredStatus': ("Stealth only works while ALL these object statuses are set.", "DEPLOYED"),
'StealthUpdateModuleData.ForbiddenStatus': ("Stealth is disabled while any of these statuses is set.", ""),
'StealthUpdateModuleData.FriendlyOpacityMin': ("Minimum opacity friendly players see the stealthed unit at.", "50%"),
'StealthUpdateModuleData.FriendlyOpacityMax': ("Maximum opacity friendly players see.", "100%"),
'StealthUpdateModuleData.PulseFrequency': ("Duration (ms) of one friendly-opacity pulse cycle.", "500"),
'StealthUpdateModuleData.DisguisesAsTeam': ("If Yes, the unit can disguise as an enemy unit it targets (Bomb Truck).", "No"),
'StealthUpdateModuleData.RevealDistanceFromTarget': ("For disguised units: distance to its target at which the disguise is dropped.", "0"),
'StealthUpdateModuleData.OrderIdleEnemiesToAttackMeUponReveal': ("If Yes, idle enemies nearby immediately attack the unit when it gets revealed.", "No"),
'StealthUpdateModuleData.DisguiseFX': ("FXList played when disguising.", "FX_BombTruckDisguise"),
'StealthUpdateModuleData.DisguiseRevealFX': ("FXList played when the disguise is revealed.", "FX_BombTruckDisguiseReveal"),
'StealthUpdateModuleData.DisguiseTransitionTime': ("Time (ms) of the fade into disguise.", "500"),
'StealthUpdateModuleData.DisguiseRevealTransitionTime': ("Time (ms) of the fade out of disguise.", "500"),
'StealthUpdateModuleData.InnateStealth': ("If Yes (default), the unit is stealthy on its own. No = only when granted by an upgrade/special power.", "Yes"),
'StealthUpdateModuleData.UseRiderStealth': ("If Yes, the container uses its rider's stealth settings (e.g. an Overlord add-on).", "No"),
'StealthUpdateModuleData.EnemyDetectionEvaEvent': ("EVA event when an enemy stealth unit of this type is detected.", "EnemyBlackLotusDetected"),
'StealthUpdateModuleData.OwnDetectionEvaEvent': ("EVA event when your own unit of this type gets detected.", "OwnBlackLotusDetected"),
'StealthUpdateModuleData.BlackMarketCheckDelay': ("Interval (ms) to check whether a Black Market exists (for NO_BLACK_MARKET).", "0"),
'StealthUpdateModuleData.GrantedBySpecialPower': ("If Yes, stealth can be granted permanently by GrantStealthBehavior / special power.", "No"),
})

M['DeletionUpdate'] = dict(
 sum="Silently deletes the object after a random lifetime (no death, no die modules).",
 body="""Like LifetimeUpdate, but when the time (random between <b>MinLifetime</b> and <b>MaxLifetime</b>) is up the object is removed from the world directly without being killed — no death FX, no die modules, no score. Use it for purely cosmetic helper objects.""",
 ex="""Behavior = DeletionUpdate ModuleTag_Delete
  MinLifetime = 3000
  MaxLifetime = 5000
End""")
F.update({
'DeletionUpdateModuleData.MinLifetime': ("Minimum lifetime (ms).", "3000"),
'DeletionUpdateModuleData.MaxLifetime': ("Maximum lifetime (ms).", "5000"),
})

M['SmartBombTargetHomingUpdate'] = dict(
 sum="Nudges a falling bomb toward its target position each frame so it lands more accurately.",
 body="""For bombs dropped from aircraft: each frame the bomb's horizontal position is blended toward the target by <b>CourseCorrectionScalar</b> (0.99 = keep 99% of its own position, move 1% toward target), compensating for the aircraft's movement so 'smart bombs' hit their mark.""",
 ex="""Behavior = SmartBombTargetHomingUpdate ModuleTag_SmartBomb
  CourseCorrectionScalar = 0.99
End""")
F.update({'SmartBombTargetHomingUpdateModuleData.CourseCorrectionScalar': ("Fraction of the bomb's own position kept each frame; the remainder is pulled toward the target. Lower = stronger homing.", "0.99")})

M['DynamicShroudClearingRangeUpdate'] = dict(
 sum="Animates the object's vision (shroud clearing) range: grows, holds, then shrinks — e.g. spy satellite / radar scan reveals.",
 body="""Over time changes how much shroud the object clears. After <b>GrowDelay</b> the range grows over <b>GrowTime</b>, holds, then after <b>ShrinkDelay</b> shrinks over <b>ShrinkTime</b> down to <b>FinalVision</b>. Vision is updated every <b>ChangeInterval</b> (or <b>GrowInterval</b> while growing). An optional <b>GridDecalTemplate</b> draws a 'radar grid' decal matching the current radius. Used by the Spy Satellite / Radar Van Scan reveal objects.""",
 ex="""Behavior = DynamicShroudClearingRangeUpdate ModuleTag_Reveal
  ChangeInterval = 200
  GrowInterval   = 30
  GrowDelay      = 0
  GrowTime       = 1000
  ShrinkDelay    = 20000
  ShrinkTime     = 5000
  FinalVision    = 0
  GridDecalTemplate
    Texture    = RadarScanGrid
    Style      = SHADOW_ALPHA_DECAL
    OpacityMin = 25%
    OpacityMax = 50%
    Color      = R:0 G:255 B:0
    OnlyVisibleToOwningPlayer = Yes
  End
End""")
F.update({
'DynamicShroudClearingRangeUpdateModuleData.ChangeInterval': ("How often (ms) the object's vision range is updated.", "200"),
'DynamicShroudClearingRangeUpdateModuleData.GrowInterval': ("Update interval (ms) used while growing (usually smaller for a smooth grow).", "30"),
'DynamicShroudClearingRangeUpdateModuleData.ShrinkDelay': ("Time (ms) from creation until shrinking starts.", "20000"),
'DynamicShroudClearingRangeUpdateModuleData.ShrinkTime': ("Duration (ms) of the shrink.", "5000"),
'DynamicShroudClearingRangeUpdateModuleData.GrowDelay': ("Time (ms) from creation until growing starts.", "0"),
'DynamicShroudClearingRangeUpdateModuleData.GrowTime': ("Duration (ms) to grow from 0 to the object's ShroudClearingRange.", "1000"),
'DynamicShroudClearingRangeUpdateModuleData.FinalVision': ("Vision range at the end of the shrink.", "0"),
'DynamicShroudClearingRangeUpdateModuleData.GridDecalTemplate': ("Sub-block describing a ground decal that follows the current reveal radius (see RadiusDecal sub-block fields).", "(sub-block)"),
})

M['DeployStyleAIUpdate'] = dict(
 sum="AI for units that must deploy to attack and pack up to move (Nuke Cannon, Tomahawk, Inferno style).",
 body="""A normal AIUpdate plus a deploy state machine. When ordered to attack, the unit stops and plays its unpack animation for <b>UnpackTime</b> (DEPLOYED model condition), fires, and packs up for <b>PackTime</b> before it can move again. Turrets can be restricted to work only while deployed (<b>TurretsFunctionOnlyWhenDeployed</b>) and must recentre before packing (<b>TurretsMustCenterBeforePacking</b>, <b>ResetTurretBeforePacking</b>). <b>ManualDeployAnimations</b> lets the animations be driven by the model's condition states rather than timed.""",
 ex="""Behavior = DeployStyleAIUpdate ModuleTag_AI
  AutoAcquireEnemiesWhenIdle = Yes
  UnpackTime = 3000
  PackTime   = 3000
  TurretsFunctionOnlyWhenDeployed = Yes
  TurretsMustCenterBeforePacking  = Yes
  Turret
    TurretTurnRate       = 60
    ControlledWeaponSlots= PRIMARY
  End
End""")
F.update({
'DeployStyleAIUpdateModuleData.UnpackTime': ("Time (ms) to deploy before being able to fire.", "3000"),
'DeployStyleAIUpdateModuleData.PackTime': ("Time (ms) to pack up before being able to move.", "3000"),
'DeployStyleAIUpdateModuleData.ResetTurretBeforePacking': ("If Yes, the turret returns to its natural angle before packing.", "No"),
'DeployStyleAIUpdateModuleData.TurretsFunctionOnlyWhenDeployed': ("If Yes, turrets can't rotate/aim unless deployed.", "Yes"),
'DeployStyleAIUpdateModuleData.TurretsMustCenterBeforePacking': ("If Yes, packing waits until turrets are centred.", "Yes"),
'DeployStyleAIUpdateModuleData.ManualDeployAnimations': ("If Yes, deploy/undeploy timing follows the model's animation states instead of Pack/UnpackTime.", "No"),
})

M['AssaultTransportAIUpdate'] = dict(
 sum="AI for the Troop Crawler: auto-deploys its passengers to attack, and recalls wounded ones to heal inside.",
 body="""When the transport is ordered to attack, it unloads its passengers and orders them to attack the target; when the attack is over they return inside. Passengers whose health drops below <b>MembersGetHealedAtLifeRatio</b> return to the transport to heal. During attack-move the troops keep fighting until the area within <b>ClearRangeRequiredToContinueAttackMove</b> is clear.""",
 ex="""Behavior = AssaultTransportAIUpdate ModuleTag_AI
  AutoAcquireEnemiesWhenIdle = Yes
  MembersGetHealedAtLifeRatio = 0.5
  ClearRangeRequiredToContinueAttackMove = 50
End""")
F.update({
'AssaultTransportAIUpdateModuleData.MembersGetHealedAtLifeRatio': ("Health fraction (0..1) below which a deployed member returns to heal.", "0.5"),
'AssaultTransportAIUpdateModuleData.ClearRangeRequiredToContinueAttackMove': ("During attack-move, radius that must be clear of enemies before the group moves on.", "50"),
})

M['HordeUpdate'] = dict(
 sum="Grants the HORDE weapon bonus (and flag subobjects) when enough similar units are grouped together.",
 body="""Every <b>UpdateRate</b> the unit counts nearby units matching <b>KindOf</b> (and <b>ExactMatch</b>/<b>AlliesOnly</b>) within <b>Radius</b>. If at least <b>Count</b> are found it becomes a true horde member; units within <b>RubOffRadius</b> of a true member also get horde status. Horde status sets the HORDE weapon bonus condition (or the action chosen by <b>Action</b>), shows <b>FlagSubObjectNames</b>, and with <b>AllowedNationalism</b> the Nationalism upgrade adds its bonus on top. Used by China infantry and tanks.""",
 ex="""Behavior = HordeUpdate ModuleTag_Horde
  UpdateRate   = 1000
  KindOf       = INFANTRY
  AlliesOnly   = Yes
  ExactMatch   = No
  Count        = 5
  Radius       = 60
  RubOffRadius = 20
  AllowedNationalism = Yes
  FlagSubObjectNames = Flag01
End""")
F.update({
'HordeUpdateModuleData.UpdateRate': ("Interval (ms) between horde checks.", "1000"),
'HordeUpdateModuleData.KindOf': ("KindOfs of units that count toward the horde.", "INFANTRY"),
'HordeUpdateModuleData.Count': ("Minimum number of units (including itself) required.", "5"),
'HordeUpdateModuleData.Radius': ("Search radius for horde members.", "60"),
'HordeUpdateModuleData.RubOffRadius': ("If this close to a true horde member, horde status rubs off on the unit.", "20"),
'HordeUpdateModuleData.AlliesOnly': ("If Yes, only allied units count.", "Yes"),
'HordeUpdateModuleData.ExactMatch': ("If Yes, only units of the exact same type count.", "No"),
'HordeUpdateModuleData.Action': ("What horde status does. Default behaviour sets the HORDE weapon bonus; the enum also offers a fixed implementation added by TheSuperHackers.", "HORDE"),
'HordeUpdateModuleData.FlagSubObjectNames': ("Subobject names shown while in horde (the little flags on Red Guards).", "Flag01"),
'HordeUpdateModuleData.AllowedNationalism': ("If Yes, the Nationalism upgrade adds its bonus on top of horde.", "Yes"),
})

M['ToppleUpdate'] = dict(
 sum="Makes an object (tree, lamp post, sign) topple over when a vehicle hits it.",
 body="""When crushed/collided with by a vehicle (or hit by blast push), the object falls over in the direction of impact, accelerating from <b>InitialVelocityPercent</b>/<b>InitialAccelPercent</b> and bouncing (<b>BounceVelocityPercent</b>, <b>BounceFX</b>). It may leave a <b>StumpName</b> object, and can be killed when it starts or finishes toppling. <b>ToppleLeftOrRightOnly</b> constrains the fall direction (fences).""",
 ex="""Behavior = ToppleUpdate ModuleTag_Topple
  ToppleFX   = FX_TreeFall
  BounceFX   = FX_TreeBounce
  StumpName  = TreeStumpSmall
  KillWhenFinishedToppling = Yes
  InitialVelocityPercent   = 20%
  InitialAccelPercent      = 1%
  BounceVelocityPercent    = 30%
End""")
F.update({
'ToppleUpdateModuleData.ToppleFX': ("FXList when toppling starts.", "FX_TreeFall"),
'ToppleUpdateModuleData.BounceFX': ("FXList each time it bounces on the ground.", "FX_TreeBounce"),
'ToppleUpdateModuleData.StumpName': ("Object template left behind as the stump.", "TreeStumpSmall"),
'ToppleUpdateModuleData.KillWhenStartToppling': ("Kill the object as soon as it starts falling.", "No"),
'ToppleUpdateModuleData.KillWhenFinishedToppling': ("Kill the object once it's flat on the ground (default Yes).", "Yes"),
'ToppleUpdateModuleData.KillStumpWhenToppled': ("Also kill the stump when toppled.", "No"),
'ToppleUpdateModuleData.ToppleLeftOrRightOnly': ("Only topple to the object's left or right side.", "No"),
'ToppleUpdateModuleData.ReorientToppledRubble': ("Re-orient the rubble after falling.", "No"),
'ToppleUpdateModuleData.InitialVelocityPercent': ("Initial angular velocity of the fall, as percent.", "20%"),
'ToppleUpdateModuleData.InitialAccelPercent': ("Angular acceleration of the fall, as percent.", "1%"),
'ToppleUpdateModuleData.BounceVelocityPercent': ("Percent of velocity kept after each bounce.", "30%"),
})

M['EnemyNearUpdate'] = dict(
 sum="Sets the ENEMYNEAR model condition while an enemy is within vision range.",
 body="""Every <b>ScanDelayTime</b> it checks whether an enemy is within the object's vision range and sets/clears the ENEMYNEAR model condition, so the model can play an alert animation (e.g. civilians panicking, gates closing).""",
 ex="""Behavior = EnemyNearUpdate ModuleTag_EnemyNear
  ScanDelayTime = 1000
End""")
F.update({'EnemyNearUpdateModuleData.ScanDelayTime': ("Interval (ms) between scans.", "1000")})

M['LifetimeUpdate'] = dict(
 sum="Kills the object after a random lifetime between MinLifetime and MaxLifetime.",
 body="""A countdown chosen randomly between <b>MinLifetime</b> and <b>MaxLifetime</b> at creation. When it expires the object is KILLED (so die modules, death FX and SlowDeath run) — unlike DeletionUpdate which silently removes it. Used for temporary objects: debris, projectiles, hazards, summoned effects.""",
 ex="""Behavior = LifetimeUpdate ModuleTag_Lifetime
  MinLifetime = 10000
  MaxLifetime = 12000
End""")
F.update({
'LifetimeUpdateModuleData.MinLifetime': ("Minimum lifetime (ms).", "10000"),
'LifetimeUpdateModuleData.MaxLifetime': ("Maximum lifetime (ms).", "12000"),
})

M['RadiusDecalUpdate'] = dict(
 sum="Shows a radius decal (targeting circle) on the ground at the object for the owner, used by delivery/superweapon targets.",
 body="""Displays <b>DeliveryDecal</b> with radius <b>DeliveryDecalRadius</b> at the object's position (typically created by an OCL to mark where a special power will land). The decal lasts until the object dies or is killed; OnlyVisibleToOwningPlayer in the sub-block controls who sees it.""",
 ex="""Behavior = RadiusDecalUpdate ModuleTag_Decal
  DeliveryDecalRadius = 100
  DeliveryDecal
    Texture          = SCCHaloTarget
    Style            = SHADOW_ALPHA_DECAL
    OpacityMin       = 25%
    OpacityMax       = 50%
    OpacityThrobTime = 500
    Color            = R:255 G:0 B:0 A:255
    OnlyVisibleToOwningPlayer = Yes
  End
End""")
F.update({
'RadiusDecalUpdateModuleData.DeliveryDecal': ("Sub-block defining the decal (Texture, Style, OpacityMin/Max, OpacityThrobTime, Color, OnlyVisibleToOwningPlayer).", "(sub-block)"),
'RadiusDecalUpdateModuleData.DeliveryDecalRadius': ("Radius of the decal.", "100"),
})

M['DecalUpdateV2'] = dict(
 sum="Self-contained timed ground decal: paints a RadiusDecal at the object, fades/resizes it over Duration, then destroys its object.",
 body="""Mod-original. Paints the <b>DecalTemplate</b> decal at this object's position, eases opacity in over <b>FadeInTime</b> and out over <b>FadeOutTime</b>, eases radius from <b>RadiusStart</b> to <b>RadiusEnd</b> over <b>ResizeInTime</b> (and back over <b>ResizeOutTime</b>), and after <b>Duration</b> destroys its own object, firing <b>OnRemovalOCL</b>/<b>OnRemovalFX</b>. One authoritative timer — no separate LifetimeUpdate needed. Typically put on a short-lived invisible hazard/superweapon-effect object alongside a FireWeaponUpdate that does the actual damage.
<br><br>Visibility to enemies is controlled by the sub-block's <b>OnlyVisibleToOwningPlayer</b> (set No to show to everyone). Concurrency is capped globally by GameData.ini <i>MaxDecalCount</i>: when full, new instances simply paint nothing (the object and its other modules are unaffected); <b>CountToMaxDecalCount = No</b> exempts an instance. Performance: the update sleeps through settled hold windows (not when an OpacityMin/Max throb is configured), and these decals are culled when off-screen or in fog of war.""",
 ex="""Behavior = DecalUpdateV2 ModuleTag_Scorch
  RadiusStart   = 20
  RadiusEnd     = 120
  Duration      = 15000
  FadeInTime    = 300
  FadeOutTime   = 2000
  ResizeInTime  = 500
  ResizeOutTime = 0
  CountToMaxDecalCount = Yes
  OnRemovalFX   = FX_ToxinPuddleEnd
  DecalTemplate
    Texture = EXToxinPuddle
    Style   = SHADOW_ALPHA_DECAL
    Color   = R:255 G:255 B:255 A:255
    OnlyVisibleToOwningPlayer = No
  End
End""")
F.update({
'DecalUpdateV2ModuleData.DecalTemplate': ("Sub-block defining the decal texture/style/colour/opacity throb/visibility (RadiusDecal sub-block fields). Set OnlyVisibleToOwningPlayer = No to show it to all players.", "(sub-block)"),
'DecalUpdateV2ModuleData.RadiusStart': ("Decal radius at spawn.", "20"),
'DecalUpdateV2ModuleData.RadiusEnd': ("Decal radius after ResizeInTime (equal to RadiusStart = no resize).", "120"),
'DecalUpdateV2ModuleData.Duration': ("Total lifetime (ms); the object destroys itself when it elapses.", "15000"),
'DecalUpdateV2ModuleData.FadeInTime': ("Time (ms) to ease opacity 0 -> 1 at the start (0 = instant).", "300"),
'DecalUpdateV2ModuleData.FadeOutTime': ("Time (ms) to ease opacity 1 -> 0 at the end (0 = vanish instantly).", "2000"),
'DecalUpdateV2ModuleData.ResizeInTime': ("Time (ms) to ease radius RadiusStart -> RadiusEnd at the start.", "500"),
'DecalUpdateV2ModuleData.ResizeOutTime': ("Time (ms) to ease radius RadiusEnd -> RadiusStart at the end (0 = no shrink-back).", "0"),
'DecalUpdateV2ModuleData.CountToMaxDecalCount': ("If No, this instance is exempt from GameData.ini MaxDecalCount and always paints.", "Yes"),
'DecalUpdateV2ModuleData.OnRemovalOCL': ("OCL fired when the object is removed at the end of Duration.", "OCL_ToxinCloudEnd"),
'DecalUpdateV2ModuleData.OnRemovalFX': ("FXList played when the object is removed at the end of Duration.", "FX_ToxinPuddleEnd"),
})

M['PersistentDecalUpdateV2'] = dict(
 sum="Paints one permanent ground decal under the unit (any texture, set in INI), optionally upgrade-triggered — like the Chemical Suits decal.",
 body="""Mod-original. Uses the same object-bound terrain decal slot as the vanilla Chemical Suits / fake-structure decal, but the texture (<b>Texture</b>), size (<b>SizeX</b>/<b>SizeY</b>) and blend <b>Style</b> come from INI, so any modder can add new decal looks without engine changes. The decal follows the unit, is skipped automatically when the unit is off-screen or shrouded, and is released when the unit is destroyed.
<br><br>Activation: with <b>StartsActive</b> = Yes (default) it paints on creation; to make it upgrade-gated set <b>StartsActive = No</b> and <b>TriggeredBy</b>. Note that a TriggeredBy block without StartsActive = No still paints immediately. <b>OnlyVisibleToOwningPlayer</b> shows it only to the owner (re-evaluated when the unit is captured/hijacked). Only one persistent decal per unit (shares the slot with vanilla terrain decals). Survives save/load for free via upgrade replay.""",
 ex="""Behavior = PersistentDecalUpdateV2 ModuleTag_AuraRing
  Texture      = MyAuraRing.tga
  SizeX        = 60
  SizeY        = 60
  Style        = SHADOW_ADDITIVE_DECAL
  StartsActive = No
  TriggeredBy  = Upgrade_MyAuraUpgrade
  OnlyVisibleToOwningPlayer = Yes
End""")
F.update({
'PersistentDecalUpdateV2ModuleData.Texture': ("Decal texture file name (no MaxDecalCount cap).", "MyAuraRing.tga"),
'PersistentDecalUpdateV2ModuleData.SizeX': ("World-space decal width.", "60"),
'PersistentDecalUpdateV2ModuleData.SizeY': ("World-space decal height.", "60"),
'PersistentDecalUpdateV2ModuleData.StartsActive': ("Yes (default): paint immediately on creation. No: wait for TriggeredBy.", "No"),
'PersistentDecalUpdateV2ModuleData.Style': ("SHADOW_ALPHA_DECAL (default) or SHADOW_ADDITIVE_DECAL.", "SHADOW_ADDITIVE_DECAL"),
'PersistentDecalUpdateV2ModuleData.OnlyVisibleToOwningPlayer': ("If Yes, only the owning player's client paints it (re-checked on capture). Default No.", "Yes"),
})

M['EMPUpdate'] = dict(
 sum="Electromagnetic pulse field object: grows, spins and fades, disabling vehicles/structures in its radius.",
 body="""Put on the EMP blast effect object. On creation it disables every eligible object within <b>EffectRadius</b> for <b>DisabledDuration</b> (DISABLED_EMP; aircraft in flight crash), spawning sparks (<b>DisableFXParticleSystem</b>, density <b>SparksPerCubicFoot</b>). Visually the object scales from <b>StartScale</b> to a random value between <b>TargetScaleMin/Max</b>, spins up to <b>SpinRateMax</b>, lerps colour from <b>StartColor</b> to <b>EndColor</b>, and starts fading at <b>StartFadeTime</b>, dying at <b>Lifetime</b>. Filters: <b>DoesNotAffect</b> (SELF, ALLIES, ENEMIES, NEUTRALS…), <b>DoesNotAffectMyOwnBuildings</b>, <b>VictimRequired/ForbiddenKindOf</b>.""",
 ex="""Behavior = EMPUpdate ModuleTag_EMP
  Lifetime         = 3000
  StartFadeTime    = 2000
  StartScale       = 0.1
  TargetScaleMin   = 1.8
  TargetScaleMax   = 2.2
  StartColor       = R:128 G:128 B:255
  EndColor         = R:0 G:0 B:64
  DisabledDuration = 15000
  EffectRadius     = 200
  DisableFXParticleSystem = EMPSparks
  SparksPerCubicFoot = 0.001
  DoesNotAffect    = ALLIES
End""")
F.update({
'EMPUpdateModuleData.Lifetime': ("Lifetime (ms) of the EMP effect object.", "3000"),
'EMPUpdateModuleData.StartFadeTime': ("Time (ms) at which the visual starts fading.", "2000"),
'EMPUpdateModuleData.StartScale': ("Initial visual scale.", "0.1"),
'EMPUpdateModuleData.DisabledDuration': ("How long (ms) victims stay disabled.", "15000"),
'EMPUpdateModuleData.SpinRateMax': ("Maximum spin rate of the visual.", "0.1"),
'EMPUpdateModuleData.TargetScaleMax': ("Maximum final scale (random between min and max).", "2.2"),
'EMPUpdateModuleData.TargetScaleMin': ("Minimum final scale.", "1.8"),
'EMPUpdateModuleData.StartColor': ("Colour at the start (R:G:B).", "R:128 G:128 B:255"),
'EMPUpdateModuleData.EndColor': ("Colour at the end.", "R:0 G:0 B:64"),
'EMPUpdateModuleData.DisableFXParticleSystem': ("Particle system (sparks) attached to disabled victims.", "EMPSparks"),
'EMPUpdateModuleData.SparksPerCubicFoot': ("Spark particle density relative to the victim's volume.", "0.001"),
'EMPUpdateModuleData.EffectRadius': ("Radius in which objects are disabled.", "200"),
'EMPUpdateModuleData.DoesNotAffect': ("Relationships/flags excluded: SELF, ALLIES, ENEMIES, NEUTRALS, SUICIDE, NOT_SIMILAR, NOT_AIRBORNE.", "ALLIES"),
'EMPUpdateModuleData.DoesNotAffectMyOwnBuildings': ("If Yes, the owner's own structures are never disabled.", "Yes"),
'EMPUpdateModuleData.VictimRequiredKindOf': ("Victims must have one of these KindOfs.", ""),
'EMPUpdateModuleData.VictimForbiddenKindOf': ("Victims with any of these KindOfs are unaffected.", "INFANTRY"),
})

M['LeafletDropBehavior'] = dict(
 sum="Propaganda leaflet drop: after a delay, disables enemy units in a radius (the 'Leaflet Drop' power).",
 body="""Put on the leaflet-bomb object. After <b>Delay</b> (or on death), it plays <b>LeafletFXParticleSystem</b> and disables every enemy unit within <b>AffectRadius</b> for <b>DisabledDuration</b> (units stop fighting, like being subdued).""",
 ex="""Behavior = LeafletDropBehavior ModuleTag_Leaflet
  Delay            = 3000
  DisabledDuration = 10000
  AffectRadius     = 200
  LeafletFXParticleSystem = LeafletDropExplosion
End""")
F.update({
'LeafletDropBehaviorModuleData.Delay': ("Time (ms) before the leaflets take effect.", "3000"),
'LeafletDropBehaviorModuleData.DisabledDuration': ("How long (ms) affected units are disabled.", "10000"),
'LeafletDropBehaviorModuleData.AffectRadius': ("Radius of effect.", "200"),
'LeafletDropBehaviorModuleData.LeafletFXParticleSystem': ("Particle system of falling leaflets.", "LeafletDropExplosion"),
})

M['AutoDepositUpdate'] = dict(
 sum="Periodically gives money to the owner (oil derricks, Supply Drop Zone, Hackers' bonus buildings).",
 body="""Every <b>DepositTiming</b> the owner receives <b>DepositAmount</b> money (and a floating '+$' text). When a player captures the building for the first time they get <b>InitialCaptureBonus</b>. <b>UpgradedBoost</b> adds extra money per deposit for owners who have a given upgrade (syntax <code>UpgradeType:Upgrade_X Boost:20</code>, repeatable). <b>ActualMoney</b> = No shows the text without giving cash.""",
 ex="""Behavior = AutoDepositUpdate ModuleTag_Money
  DepositTiming      = 3000
  DepositAmount      = 20
  InitialCaptureBonus= 1000
  ActualMoney        = Yes
  UpgradedBoost      = UpgradeType:Upgrade_AmericaSupplyLines Boost:5
End""")
F.update({
'AutoDepositUpdateModuleData.DepositTiming': ("Interval (ms) between payments.", "3000"),
'AutoDepositUpdateModuleData.DepositAmount': ("Money per payment.", "20"),
'AutoDepositUpdateModuleData.InitialCaptureBonus': ("One-time money when first captured.", "1000"),
'AutoDepositUpdateModuleData.ActualMoney': ("If No, only the floating text is shown and no real money is given.", "Yes"),
'AutoDepositUpdateModuleData.UpgradedBoost': ("Extra money per payment when the owner has an upgrade: UpgradeType:<Upgrade> Boost:<amount>. Repeatable.", "UpgradeType:Upgrade_AmericaSupplyLines Boost:5"),
})

M['WeaponBonusUpdate'] = dict(
 sum="Periodically grants a weapon bonus condition to itself/nearby units (aura buff like the GLA Frenzy).",
 body="""Every <b>BonusDelay</b> it pulses: every allied object within <b>BonusRange</b> that passes <b>RequiredAffectKindOf</b>/<b>ForbiddenAffectKindOf</b> receives the <b>BonusConditionType</b> weapon-bonus condition (e.g. FRENZY_ONE, ENTHUSIASTIC) for <b>BonusDuration</b>. Affected units get the hardcoded frenzy tint. The actual numeric effect comes from WeaponBonus entries in GameData.ini/Weapon bonus sets.""",
 ex="""Behavior = WeaponBonusUpdate ModuleTag_Frenzy
  RequiredAffectKindOf = INFANTRY VEHICLE
  ForbiddenAffectKindOf= AIRCRAFT
  BonusDuration        = 5000
  BonusDelay           = 1000
  BonusRange           = 150
  BonusConditionType   = FRENZY_ONE
End""")
F.update({
'WeaponBonusUpdateModuleData.RequiredAffectKindOf': ("Targets must have one of these KindOfs.", "INFANTRY VEHICLE"),
'WeaponBonusUpdateModuleData.ForbiddenAffectKindOf': ("Targets with any of these KindOfs are skipped.", "AIRCRAFT"),
'WeaponBonusUpdateModuleData.BonusDuration': ("How long (ms) each pulse's bonus lasts on a target.", "5000"),
'WeaponBonusUpdateModuleData.BonusDelay': ("Interval (ms) between pulses.", "1000"),
'WeaponBonusUpdateModuleData.BonusRange': ("Radius of the aura (0 = only itself).", "150"),
'WeaponBonusUpdateModuleData.BonusConditionType': ("Weapon bonus condition granted (e.g. FRENZY_ONE, FRENZY_TWO, FRENZY_THREE, ENTHUSIASTIC, DRONE_SPOTTING...).", "FRENZY_ONE"),
})

M['WeaponBonusUpdateV2'] = dict(
 sum="WeaponBonusUpdate with a custom INI 'Tint' colour applied to every affected unit (infantry and vehicles alike).",
 body="""Mod-original copy of WeaponBonusUpdate. Behaves identically (periodic pulse granting <b>BonusConditionType</b> to units within <b>BonusRange</b> for <b>BonusDuration</b>), but instead of the hardcoded FRENZY_COLOR / FRENZY_COLOR_INFANTRY tint it uses the <b>Tint</b> colour from INI for all kinds of units. Kept as a separate module so the vanilla one stays untouched.""",
 ex="""Behavior = WeaponBonusUpdateV2 ModuleTag_Aura
  RequiredAffectKindOf = INFANTRY VEHICLE
  BonusDuration        = 5000
  BonusDelay           = 1000
  BonusRange           = 150
  BonusConditionType   = FRENZY_TWO
  Tint                 = R:0 G:180 B:255
End""")
F.update({'WeaponBonusUpdateV2ModuleData.Tint': ("Tint colour (R:G:B) applied to affected units while they have the bonus — same colour for all KindOfs.", "R:0 G:180 B:255")})

M['MissileAIUpdate'] = dict(
 sum="Guided missile flight AI: ignition, fuel, homing on target, diving, lock-on distance, jamming scatter.",
 body="""AI for guided missiles created as a weapon's ProjectileObject. After <b>IgnitionDelay</b> the missile ignites (<b>IgnitionFX</b>) and accelerates from <b>InitialVelocity</b> using its locomotor; it flies straight for <b>DistanceToTravelBeforeTurning</b> and then homes on the target object (<b>TryToFollowTarget</b>) or position. Within <b>DistanceToTargetBeforeDiving</b> it ignores its preferred height and dives; within <b>DistanceToTargetForLock</b> a hit is guaranteed. <b>FuelLifetime</b> limits thrust (0 = infinite); <b>DetonateOnNoFuel</b> explodes when out. When jammed (ECM) it scatters by <b>DistanceScatterWhenJammed</b>. If the tracked target dies, the vanilla missile simply disappears.""",
 ex="""Behavior = MissileAIUpdate ModuleTag_MissileAI
  TryToFollowTarget          = Yes
  FuelLifetime               = 3000
  IgnitionDelay              = 0
  InitialVelocity            = 100
  DistanceToTravelBeforeTurning = 10
  DistanceToTargetBeforeDiving  = 50
  DistanceToTargetForLock    = 20
  IgnitionFX                 = FX_MissileIgnite
  UseWeaponSpeed             = Yes
  DetonateOnNoFuel           = Yes
  DetonateCallsKill          = Yes
End""")
F.update({
'MissileAIUpdateModuleData.TryToFollowTarget': ("If Yes, home on the target object; if No fly to the target position.", "Yes"),
'MissileAIUpdateModuleData.FuelLifetime': ("Time (ms) of powered flight. 0 = unlimited.", "3000"),
'MissileAIUpdateModuleData.IgnitionDelay': ("Time (ms) from launch to ignition (the missile drops/coasts before).", "0"),
'MissileAIUpdateModuleData.InitialVelocity': ("Speed at launch.", "100"),
'MissileAIUpdateModuleData.DistanceToTravelBeforeTurning': ("Distance flown straight before starting to steer.", "10"),
'MissileAIUpdateModuleData.DistanceToTargetBeforeDiving': ("Within this distance of the target the missile ignores its cruise height and dives.", "50"),
'MissileAIUpdateModuleData.DistanceToTargetForLock': ("Within this distance a hit is guaranteed.", "20"),
'MissileAIUpdateModuleData.IgnitionFX': ("FXList when the missile ignites.", "FX_MissileIgnite"),
'MissileAIUpdateModuleData.UseWeaponSpeed': ("If Yes, max speed is limited to the firing weapon's WeaponSpeed.", "Yes"),
'MissileAIUpdateModuleData.DetonateOnNoFuel': ("If Yes, explode when fuel runs out instead of falling.", "Yes"),
'MissileAIUpdateModuleData.DistanceScatterWhenJammed': ("How far off target the missile scatters when jammed.", "75"),
'MissileAIUpdateModuleData.GarrisonHitKillRequiredKindOf': ("KindOf a building must have for the garrison-kill effect.", "STRUCTURE"),
'MissileAIUpdateModuleData.GarrisonHitKillForbiddenKindOf': ("KindOfs that exclude a building from garrison-kill.", ""),
'MissileAIUpdateModuleData.GarrisonHitKillCount': ("Garrisoned occupants killed on hitting a qualifying building.", "0"),
'MissileAIUpdateModuleData.GarrisonHitKillFX': ("FXList when occupants are killed that way.", ""),
'MissileAIUpdateModuleData.DetonateCallsKill': ("If Yes, detonation kills the missile (die modules run) instead of silently destroying it.", "Yes"),
'MissileAIUpdateModuleData.KillSelfDelay': ("Delay (ms) in the kill-self state after detonation before removal.", "100"),
})

M['MissileAIUpdateV2'] = dict(
 sum="MissileAIUpdate plus: detonate at the target's last position when it dies, and optional retargeting to a new enemy.",
 body="""Mod-original copy of MissileAIUpdate that behaves identically, but changes what happens when a tracked target (TryToFollowTarget = Yes) dies. With <b>DetonateAtLastTargetPosition</b> = Yes (default) the missile flies to the target's last known position and detonates there (even mid-air) instead of vanishing. With <b>ShouldRetarget</b> = Yes, the moment the target is effectively dead it scans once within <b>RetargetRange</b> of the dead target for a new target (filtered by <b>RetargetRequiredKindOf</b>/<b>RetargetForbiddenKindOf</b>, enemy, alive, visible, not contained, and legal for the weapon's Anti* mask) and chases it; this repeats up to <b>MaxRetargets</b> times (0 = unlimited). If none is found, it follows the wreck and then detonates at its last position. If the new target has countermeasures, <b>RetargetCanBeDecoyed</b> gives it a fresh chance to decoy the missile. Decoyed missiles never retarget; position shots are unaffected.""",
 ex="""Behavior = MissileAIUpdateV2 ModuleTag_MissileAI
  TryToFollowTarget          = Yes
  FuelLifetime               = 4000
  InitialVelocity            = 80
  DistanceToTravelBeforeTurning = 10
  DistanceToTargetForLock    = 20
  UseWeaponSpeed             = Yes
  DetonateOnNoFuel           = Yes
  DetonateAtLastTargetPosition = Yes
  ShouldRetarget             = Yes
  RetargetRange              = 150
  MaxRetargets               = 2
  RetargetRequiredKindOf     = VEHICLE AIRCRAFT
  RetargetForbiddenKindOf    = INFANTRY
  RetargetCanBeDecoyed       = Yes
End""")
F.update({
'MissileAIUpdateV2ModuleData.DetonateAtLastTargetPosition': ("Yes (default): when the tracked target is destroyed, fly to its last position and detonate. No: vanilla behaviour (missile vanishes).", "Yes"),
'MissileAIUpdateV2ModuleData.ShouldRetarget': ("If Yes, when the target is effectively dead, scan once for a new target and chase it.", "Yes"),
'MissileAIUpdateV2ModuleData.RetargetRange': ("Scan radius for a new target, measured from the dead target's position.", "150"),
'MissileAIUpdateV2ModuleData.MaxRetargets': ("Maximum number of retargets per missile. 0 = unlimited.", "2"),
'MissileAIUpdateV2ModuleData.RetargetRequiredKindOf': ("If set, a new target must match at least one of these KindOfs.", "VEHICLE AIRCRAFT"),
'MissileAIUpdateV2ModuleData.RetargetForbiddenKindOf': ("A new target must match none of these KindOfs.", "INFANTRY"),
'MissileAIUpdateV2ModuleData.RetargetCanBeDecoyed': ("Yes (default): a new target with countermeasures may decoy the missile, same rules as at launch.", "Yes"),
})

M['NeutronMissileUpdate'] = dict(
 sum="Flight logic of the Nuclear Missile superweapon: vertical launch, special speed phase, then dive onto the target.",
 body="""Controls the nuke missile object after launch from the silo: launch FX, a vertical boost phase (<b>SpecialSpeedTime</b>, <b>SpecialSpeedHeight</b>, <b>SpecialAccelFactor</b>, <b>SpecialJitterDistance</b>), then turning toward the target limited by <b>MaxTurnRate</b>, approaching from directly above (<b>TargetFromDirectlyAbove</b>). It shows <b>DeliveryDecal</b> at the target. On arrival it dies, triggering NeutronMissileSlowDeathBehavior.""",
 ex="""Behavior = NeutronMissileUpdate ModuleTag_NukeFlight
  DistanceToTravelBeforeTurning = 300
  MaxTurnRate        = 180
  ForwardDamping     = 0
  RelativeSpeed      = 1.0
  TargetFromDirectlyAbove = 500
  LaunchFX           = FX_NukeLaunch
  SpecialSpeedTime   = 3000
  SpecialSpeedHeight = 500
  SpecialAccelFactor = 1.5
  SpecialJitterDistance = 0.8
  DeliveryDecalRadius = 250
  DeliveryDecal
    Texture = SCCNuclearMissile_China
    Style   = SHADOW_ALPHA_DECAL
    OnlyVisibleToOwningPlayer = No
  End
End""")
F.update({
'NeutronMissileUpdateModuleData.DistanceToTravelBeforeTurning': ("Height/distance climbed before turning toward the target.", "300"),
'NeutronMissileUpdateModuleData.MaxTurnRate': ("Maximum turn rate (deg/sec).", "180"),
'NeutronMissileUpdateModuleData.ForwardDamping': ("Damping of forward velocity.", "0"),
'NeutronMissileUpdateModuleData.RelativeSpeed': ("Speed multiplier relative to its locomotor.", "1.0"),
'NeutronMissileUpdateModuleData.TargetFromDirectlyAbove': ("First aim for a point this far above the target, then drop straight down.", "500"),
'NeutronMissileUpdateModuleData.LaunchFX': ("FXList at launch.", "FX_NukeLaunch"),
'NeutronMissileUpdateModuleData.SpecialSpeedTime': ("Duration (ms) of the special boost phase.", "3000"),
'NeutronMissileUpdateModuleData.SpecialSpeedHeight': ("Height reached during the boost phase.", "500"),
'NeutronMissileUpdateModuleData.SpecialAccelFactor': ("Acceleration multiplier during the boost phase.", "1.5"),
'NeutronMissileUpdateModuleData.SpecialJitterDistance': ("Random wobble during the boost phase.", "0.8"),
'NeutronMissileUpdateModuleData.IgnitionFX': ("FXList at ignition.", "FX_NukeIgnite"),
'NeutronMissileUpdateModuleData.DeliveryDecal': ("Sub-block: target-marker decal (RadiusDecal fields).", "(sub-block)"),
'NeutronMissileUpdateModuleData.DeliveryDecalRadius': ("Radius of the target marker decal.", "250"),
})

M['FireSpreadUpdate'] = dict(
 sum="While the object is aflame, periodically spreads fire to nearby flammable objects via an embers OCL.",
 body="""Works with FlammableUpdate. While the object has the AFLAME status, every random delay between <b>MinSpreadDelay</b> and <b>MaxSpreadDelay</b> it looks for a flammable object within <b>SpreadTryRange</b>, creates <b>OCLEmbers</b> toward it and sets it on fire. Used by trees and civilian buildings to make fires spread.""",
 ex="""Behavior = FireSpreadUpdate ModuleTag_FireSpread
  OCLEmbers      = OCL_FireEmbers
  MinSpreadDelay = 2000
  MaxSpreadDelay = 6000
  SpreadTryRange = 30
End""")
F.update({
'FireSpreadUpdateModuleData.OCLEmbers': ("OCL (embers) created when fire jumps.", "OCL_FireEmbers"),
'FireSpreadUpdateModuleData.MinSpreadDelay': ("Minimum time (ms) between spread attempts.", "2000"),
'FireSpreadUpdateModuleData.MaxSpreadDelay': ("Maximum time (ms) between spread attempts.", "6000"),
'FireSpreadUpdateModuleData.SpreadTryRange': ("Radius searched for something to ignite.", "30"),
})

M['FireWeaponUpdate'] = dict(
 sum="Fires a weapon at the object's own position as fast as the weapon allows (hazard fields, auras, radiation/toxin puddles).",
 body="""Allocates <b>Weapon</b> and fires it at the object's own feet every time it's ready (respecting the weapon's reload/clip). <b>InitialDelay</b> postpones the first shot. <b>ExclusiveWeaponDelay</b> suppresses firing if any other weapon of the object fired within that time. Classic uses: toxin/radiation fields, burning ground, pulsing damage auras.""",
 ex="""Behavior = FireWeaponUpdate ModuleTag_Pulse
  Weapon       = ToxinPoolDamageWeapon
  InitialDelay = 500
End""")
F.update({
'FireWeaponUpdateModuleData.Weapon': ("Weapon fired at the object's own position.", "ToxinPoolDamageWeapon"),
'FireWeaponUpdateModuleData.InitialDelay': ("Delay (ms) before the first shot.", "500"),
'FireWeaponUpdateModuleData.ExclusiveWeaponDelay': ("If non-zero, don't fire if any other weapon on the object fired within this time (ms).", "0"),
})

M['FlammableUpdate'] = dict(
 sum="Manages the AFLAME and BURNED statuses: catching fire from flame damage, burning damage over time, burned model.",
 body="""When the object has taken more than <b>FlameDamageLimit</b> flame damage within <b>FlameDamageExpiration</b>, it catches fire: it gets AFLAME for <b>AflameDuration</b>, takes <b>AflameDamageAmount</b> every <b>AflameDamageDelay</b>, and loops <b>BurningSoundName</b>. After <b>BurnedDelay</b> it is marked BURNED (burnt model condition), which is permanent. Trees, buildings and some units use it.""",
 ex="""Behavior = FlammableUpdate ModuleTag_Flammable
  AflameDuration     = 5000
  AflameDamageAmount = 3
  AflameDamageDelay  = 500
  BurnedDelay        = 3000
  FlameDamageLimit   = 20
  FlameDamageExpiration = 2000
  BurningSoundName   = FireLoop
End""")
F.update({
'FlammableUpdateModuleData.BurnedDelay': ("Time (ms) aflame before becoming BURNED. 0 = never.", "3000"),
'FlammableUpdateModuleData.AflameDuration': ("How long (ms) the object stays aflame.", "5000"),
'FlammableUpdateModuleData.AflameDamageDelay': ("Interval (ms) between burn damage ticks. 0 = no burn damage.", "500"),
'FlammableUpdateModuleData.AflameDamageAmount': ("Damage per tick while aflame.", "3"),
'FlammableUpdateModuleData.BurningSoundName': ("Looping sound while burning.", "FireLoop"),
'FlammableUpdateModuleData.FlameDamageLimit': ("Accumulated flame damage needed to ignite.", "20"),
'FlammableUpdateModuleData.FlameDamageExpiration': ("Window (ms) over which flame damage accumulates before resetting.", "2000"),
})

M['FloatUpdate'] = dict(
 sum="Makes the object bob and float on water surfaces.",
 body="""When <b>Enabled</b>, the object's height is kept at the water surface and it gently rocks with the waves (boats, floating debris).""",
 ex="""Behavior = FloatUpdate ModuleTag_Float
  Enabled = Yes
End""")
F.update({'FloatUpdateModuleData.Enabled': ("Turn floating on.", "Yes")})

M['TensileFormationUpdate'] = dict(
 sum="Springy linked-formation movement (avalanche/ice-sheet pieces) that cracks and slides together.",
 body="""Used for map props that move as a connected 'tensile' group (e.g. collapsing ice or avalanche chunks): once <b>Enabled</b> (usually by script), pieces pull on their neighbours like springs and slide, playing <b>CrackSound</b>.""",
 ex="""Behavior = TensileFormationUpdate ModuleTag_Tensile
  Enabled    = No
  CrackSound = IceCrack
End""")
F.update({
'TensileFormationUpdateModuleData.Enabled': ("Start active (usually No; scripts enable it).", "No"),
'TensileFormationUpdateModuleData.CrackSound': ("Sound played when the formation starts breaking.", "IceCrack"),
})

M['HeightDieUpdate'] = dict(
 sum="Kills the object when it reaches a given height above terrain (e.g. airburst shells, falling bombs).",
 body="""Each frame, if the object is at or below <b>TargetHeight</b> above the ground (optionally counting structures, <b>TargetHeightIncludesStructures</b>) — and, if <b>OnlyWhenMovingDown</b>, while descending — it is killed. Waits <b>InitialDelay</b> first. <b>SnapToGroundOnDeath</b> moves it to the ground when killed; <b>DestroyAttachedParticlesAtHeight</b> removes trailing particle systems when below that height.""",
 ex="""Behavior = HeightDieUpdate ModuleTag_HeightDie
  TargetHeight        = 5
  TargetHeightIncludesStructures = Yes
  OnlyWhenMovingDown  = Yes
  SnapToGroundOnDeath = Yes
  InitialDelay        = 500
End""")
F.update({
'HeightDieUpdateModuleData.TargetHeight': ("Height above terrain at which the object dies.", "5"),
'HeightDieUpdateModuleData.TargetHeightIncludesStructures': ("If Yes, height is measured above structures underneath as well as terrain.", "Yes"),
'HeightDieUpdateModuleData.OnlyWhenMovingDown': ("Only die while moving downward.", "Yes"),
'HeightDieUpdateModuleData.DestroyAttachedParticlesAtHeight': ("Destroy attached particle systems once below this height (-1 = never).", "-1"),
'HeightDieUpdateModuleData.SnapToGroundOnDeath': ("Move the object to ground level when it dies.", "Yes"),
'HeightDieUpdateModuleData.InitialDelay': ("Time (ms) before the height check starts.", "500"),
})
