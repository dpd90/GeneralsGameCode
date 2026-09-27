M = {}
F = {}

F.update({
'SupplyTruckAIUpdateModuleData.MaxBoxes': ("Number of supply boxes the truck can carry per trip.", "5"),
'SupplyTruckAIUpdateModuleData.SupplyCenterActionDelay': ("Time (ms) spent at the supply center dropping off boxes.", "1500"),
'SupplyTruckAIUpdateModuleData.SupplyWarehouseActionDelay': ("Time (ms) per box taken at the warehouse.", "600"),
'SupplyTruckAIUpdateModuleData.SupplyWarehouseScanDistance': ("How far the truck looks for another warehouse when the current one runs dry.", "300"),
'SupplyTruckAIUpdateModuleData.SuppliesDepletedVoice': ("Voice played when the truck takes the last box from a warehouse.", "SupplyTruckVoiceSuppliesDepleted"),
})

M['SupplyTruckAIUpdate'] = dict(
 sum="AI for supply gatherers: automatically shuttles between supply warehouses and supply centers.",
 body="""The harvesting AI. The truck docks with a supply warehouse/dock, takes up to <b>MaxBoxes</b> boxes (<b>SupplyWarehouseActionDelay</b> per box), drives to the nearest supply center and unloads (<b>SupplyCenterActionDelay</b>), converting boxes into money. When a warehouse is empty it searches <b>SupplyWarehouseScanDistance</b> for another. Also used as the base for ChinookAIUpdate and WorkerAIUpdate's gathering half.""",
 ex="""Behavior = SupplyTruckAIUpdate ModuleTag_AI
  MaxBoxes                   = 5
  SupplyCenterActionDelay    = 1500
  SupplyWarehouseActionDelay = 600
  SupplyWarehouseScanDistance= 300
  SuppliesDepletedVoice      = SupplyTruckVoiceSuppliesDepleted
End""")

M['ChinookAIUpdate'] = dict(
 sum="Chinook AI: supply-gathering helicopter that can also rappel/ combat-drop infantry via ropes.",
 body="""Extends SupplyTruckAIUpdate for the USA Chinook: it gathers supplies like a truck (MaxBoxes etc.), and additionally supports the combat drop: hovering at least <b>MinDropHeight</b> over a spot and lowering <b>NumRopes</b> ropes (rope object <b>RopeName</b>, width/colour/wobble configurable) down which passengers rappel at <b>RappelSpeed</b>. Each rope starts after a random delay between PerRopeDelayMin/Max. <b>RotorWashParticleSystem</b> kicks up dust while hovering low. <b>UpgradedSupplyBoost</b> adds extra cash per trip after the Supply Lines style upgrade.""",
 ex="""Behavior = ChinookAIUpdate ModuleTag_AI
  MaxBoxes                   = 8
  SupplyCenterActionDelay    = 1500
  SupplyWarehouseActionDelay = 400
  SupplyWarehouseScanDistance= 700
  NumRopes         = 4
  RopeName         = GenericRope
  RappelSpeed      = 30
  RopeDropSpeed    = 300
  RopeWidth        = 0.5
  RopeColor        = R:0 G:0 B:0
  PerRopeDelayMin  = 900
  PerRopeDelayMax  = 1500
  MinDropHeight    = 30
  WaitForRopesToDrop = Yes
  RotorWashParticleSystem = HelicopterRotorWash
  UpgradedSupplyBoost = 30
End""")
F.update({
'ChinookAIUpdateModuleData.RappelSpeed': ("Speed at which infantry slides down the ropes.", "30"),
'ChinookAIUpdateModuleData.RopeDropSpeed': ("Speed at which the ropes are lowered.", "300"),
'ChinookAIUpdateModuleData.RopeName': ("Object template used for each rope.", "GenericRope"),
'ChinookAIUpdateModuleData.RopeFinalHeight': ("Height at the bottom where the rope ends.", "0"),
'ChinookAIUpdateModuleData.RopeWidth': ("Rope thickness.", "0.5"),
'ChinookAIUpdateModuleData.RopeWobbleLen': ("Length of rope wobble waves.", "10"),
'ChinookAIUpdateModuleData.RopeWobbleAmplitude': ("Amplitude of the rope wobble.", "1.0"),
'ChinookAIUpdateModuleData.RopeWobbleRate': ("Speed (deg/sec) of the wobble.", "10"),
'ChinookAIUpdateModuleData.RopeColor': ("Rope colour (R:G:B).", "R:0 G:0 B:0"),
'ChinookAIUpdateModuleData.NumRopes': ("Number of ropes used for rappelling.", "4"),
'ChinookAIUpdateModuleData.PerRopeDelayMin': ("Minimum delay (ms) before each rope's next rappeller.", "900"),
'ChinookAIUpdateModuleData.PerRopeDelayMax': ("Maximum delay (ms) before each rope's next rappeller.", "1500"),
'ChinookAIUpdateModuleData.MinDropHeight': ("Minimum hover height for a combat drop.", "30"),
'ChinookAIUpdateModuleData.WaitForRopesToDrop': ("If Yes, rappelling begins only after the ropes are fully lowered.", "Yes"),
'ChinookAIUpdateModuleData.RotorWashParticleSystem': ("Particle system of dust kicked up while hovering low.", "HelicopterRotorWash"),
'ChinookAIUpdateModuleData.UpgradedSupplyBoost': ("Extra cash per delivered load when the owner has the supply upgrade.", "30"),
})

M['JetAIUpdate'] = dict(
 sum="AI for airplanes: runway takeoff/landing, returning to the airfield for ammo, lock-on warning, attack locomotor.",
 body="""AIUpdate for jets that live on an airfield (ParkingPlaceBehavior). Handles taxiing (<b>MinHeight</b>, <b>ParkingOffset</b>), runway takeoff (<b>NeedsRunway</b>, <b>TakeoffPause</b>, <b>TakeoffDistForMaxLift</b>), returning to base when out of ammo or idle (<b>ReturnToBaseIdleTime</b>), and taking damage while out of ammo in the air (<b>OutOfAmmoDamagePerSecond</b>). Can switch to a special locomotor while attacking (<b>AttackLocomotorType</b>) and returning (<b>ReturnForAmmoLocomotorType</b>), and give incoming attacks a miss window after its attack run (<b>AttackersMissPersistTime</b>). The Lockon* fields draw the animated lock-on cursor enemies see when targeting it (Aurora style).""",
 ex="""Behavior = JetAIUpdate ModuleTag_AI
  OutOfAmmoDamagePerSecond = 10%
  NeedsRunway              = Yes
  KeepsParkingSpaceWhenAirborne = Yes
  TakeoffDistForMaxLift    = 0%
  TakeoffPause             = 500
  MinHeight                = 5
  ParkingOffset            = -5
  SneakyOffsetWhenAttacking= 0
  AttackLocomotorType      = SET_SUPERSONIC
  AttackLocomotorPersistTime = 500
  AttackersMissPersistTime = 0
  ReturnToBaseIdleTime     = 10000
  AutoAcquireEnemiesWhenIdle = Yes
End""")
F.update({
'JetAIUpdateModuleData.OutOfAmmoDamagePerSecond': ("Percent of max health lost per second while airborne with no ammo (forces return).", "10%"),
'JetAIUpdateModuleData.NeedsRunway': ("If Yes, needs a runway to take off/land (jets); No for VTOL.", "Yes"),
'JetAIUpdateModuleData.KeepsParkingSpaceWhenAirborne': ("If Yes, keeps its parking reservation while flying.", "Yes"),
'JetAIUpdateModuleData.TakeoffDistForMaxLift': ("Point along the runway (percent) at which max lift is reached. Higher = lifts off sooner.", "0%"),
'JetAIUpdateModuleData.TakeoffPause': ("Pause (ms) at the start of the runway before takeoff.", "500"),
'JetAIUpdateModuleData.MinHeight': ("Height the model is lifted while taxiing.", "5"),
'JetAIUpdateModuleData.ParkingOffset': ("Adjustment to the parking location.", "-5"),
'JetAIUpdateModuleData.SneakyOffsetWhenAttacking': ("Offset applied to its targetable position while attacking (makes it harder to hit).", "0"),
'JetAIUpdateModuleData.AttackLocomotorType': ("Locomotor set used while attacking (e.g. SET_SUPERSONIC).", "SET_SUPERSONIC"),
'JetAIUpdateModuleData.AttackLocomotorPersistTime': ("How long (ms) the attack locomotor persists after the attack ends.", "500"),
'JetAIUpdateModuleData.AttackersMissPersistTime': ("How long (ms) after its attack enemy shots automatically miss it.", "0"),
'JetAIUpdateModuleData.ReturnForAmmoLocomotorType': ("Locomotor set used while returning to rearm.", "SET_NORMAL"),
'JetAIUpdateModuleData.LockonTime': ("Time (ms) for enemies to lock onto this aircraft.", "0"),
'JetAIUpdateModuleData.LockonCursor': ("Template of the lock-on cursor effect.", ""),
'JetAIUpdateModuleData.LockonInitialDist': ("Starting distance of the lock-on cursor.", "100"),
'JetAIUpdateModuleData.LockonFreq': ("Frequency of the lock-on cursor animation.", "0.5"),
'JetAIUpdateModuleData.LockonAngleSpin': ("Degrees the lock-on cursor spins while closing in.", "720"),
'JetAIUpdateModuleData.LockonBlinky': ("If Yes, the lock-on cursor blinks.", "No"),
'JetAIUpdateModuleData.ReturnToBaseIdleTime': ("If idle in the air this long (ms), return to the airfield.", "10000"),
})

M['AIUpdateInterface'] = dict(
 sum="Standard unit AI: movement, pathfinding, attacking, auto-acquiring targets, turrets. Base of all *AIUpdate modules.",
 body="""The generic AI brain for mobile units. It executes player/AI commands (move, attack, guard, attack-move, enter, dock…) through a state machine, uses the object's Locomotor for pathfinding, handles idle auto-targeting (<b>AutoAcquireEnemiesWhenIdle</b>, re-checked every <b>MoodAttackCheckRate</b>) and drives up to two turrets defined in <b>Turret</b>/<b>AltTurret</b> sub-blocks. Every specialised AI (Dozer, Jet, Missile, SupplyTruck, Chinook, DeployStyle, Transport…) builds on this. Tanks and infantry with no special behaviour just use AIUpdateInterface.""",
 ex="""Behavior = AIUpdateInterface ModuleTag_AI
  AutoAcquireEnemiesWhenIdle = Yes
  MoodAttackCheckRate        = 250
  Turret
    TurretTurnRate        = 180
    ControlledWeaponSlots = PRIMARY
    NaturalTurretAngle    = 0
    RecenterTime          = 5000
  End
End""")

M['DeliverPayloadAIUpdate'] = dict(
 sum="AI for aircraft that fly to a target, drop their cargo (paratroopers, bombs, crates) and leave.",
 body="""Used by transport/bomber planes spawned by special-power OCLs (DeliverPayload nugget). The plane approaches the target, opens its doors (<b>DoorDelay</b>), and within <b>DeliveryDistance</b> releases its contents (<b>DropDelay</b>, <b>DropOffset</b>, <b>DropVariance</b>), optionally putting each dropped item into a <b>PutInContainer</b> (e.g. a parachute). If it overshoots it turns around and retries up to <b>MaxAttempts</b> times. A <b>DeliveryDecal</b> marks the drop zone. Most values are normally overridden by the OCL's DeliverPayload nugget; these INI values are used for script-only reinforcements.""",
 ex="""Behavior = DeliverPayloadAIUpdate ModuleTag_AI
  DoorDelay        = 1000
  MaxAttempts      = 3
  DropOffset       = X:0 Y:0 Z:-10
  DropVariance     = X:20 Y:20 Z:0
  DropDelay        = 300
  PutInContainer   = AmericaParachute
  DeliveryDistance = 50
End""")
F.update({
'DeliverPayloadAIUpdateModuleData.DoorDelay': ("Time (ms) for the cargo doors to open before dropping.", "1000"),
'DeliverPayloadAIUpdateModuleData.PutInContainer': ("Object template each dropped item is put into (e.g. a parachute).", "AmericaParachute"),
'DeliverPayloadAIUpdateModuleData.DeliveryDistance': ("How far from the target the drop may begin, and how far past it the plane turns around.", "50"),
'DeliverPayloadAIUpdateModuleData.MaxAttempts': ("How many approach attempts before giving up.", "3"),
'DeliverPayloadAIUpdateModuleData.DropDelay': ("Delay (ms) between dropping each item.", "300"),
'DeliverPayloadAIUpdateModuleData.DropOffset': ("Offset (X:Y:Z) relative to the plane where items are released.", "X:0 Y:0 Z:-10"),
'DeliverPayloadAIUpdateModuleData.DropVariance': ("Random variance (X:Y:Z) of each item's drop position.", "X:20 Y:20 Z:0"),
'DeliverPayloadAIUpdateModuleData.DeliveryDecal': ("Sub-block: decal shown at the drop zone (RadiusDecal fields).", "(sub-block)"),
'DeliverPayloadAIUpdateModuleData.DeliveryDecalRadius': ("Radius of the drop-zone decal.", "100"),
})

M['HackInternetAIUpdate'] = dict(
 sum="AI for the China Hacker: unpacks a laptop and generates money periodically ('hack the internet').",
 body="""When ordered to hack, the unit unpacks (<b>UnpackTime</b>, randomised by <b>PackUnpackVariationFactor</b>) and then every <b>CashUpdateDelay</b> (or <b>CashUpdateDelayFast</b> when inside an Internet Center) gives the owner cash depending on its veterancy (<b>Regular/Veteran/Elite/HeroicCashAmount</b>), gaining <b>XpPerCashUpdate</b> experience each time. Moving requires packing up (<b>PackTime</b>).""",
 ex="""Behavior = HackInternetAIUpdate ModuleTag_AI
  UnpackTime          = 7000
  PackTime            = 2000
  PackUnpackVariationFactor = 0.5
  CashUpdateDelay     = 2000
  CashUpdateDelayFast = 1800
  RegularCashAmount   = 5
  VeteranCashAmount   = 6
  EliteCashAmount     = 8
  HeroicCashAmount    = 10
  XpPerCashUpdate     = 1
End""")
F.update({
'HackInternetAIUpdateModuleData.UnpackTime': ("Time (ms) to unpack before hacking starts.", "7000"),
'HackInternetAIUpdateModuleData.PackTime': ("Time (ms) to pack up before moving.", "2000"),
'HackInternetAIUpdateModuleData.PackUnpackVariationFactor': ("Random variation factor applied to pack/unpack times (0.5 = +/-50%).", "0.5"),
'HackInternetAIUpdateModuleData.CashUpdateDelay': ("Interval (ms) between cash payments while hacking in the field.", "2000"),
'HackInternetAIUpdateModuleData.CashUpdateDelayFast': ("Interval (ms) between payments while inside an Internet Center.", "1800"),
'HackInternetAIUpdateModuleData.RegularCashAmount': ("Cash per payment at REGULAR rank.", "5"),
'HackInternetAIUpdateModuleData.VeteranCashAmount': ("Cash per payment at VETERAN rank.", "6"),
'HackInternetAIUpdateModuleData.EliteCashAmount': ("Cash per payment at ELITE rank.", "8"),
'HackInternetAIUpdateModuleData.HeroicCashAmount': ("Cash per payment at HEROIC rank.", "10"),
'HackInternetAIUpdateModuleData.XpPerCashUpdate': ("Experience gained per payment.", "1"),
})

M['DynamicGeometryInfoUpdate'] = dict(
 sum="Changes the object's collision geometry (height/radii) over time — for growing or shrinking effects.",
 body="""After <b>InitialDelay</b>, the geometry interpolates from Initial(Height/MajorRadius/MinorRadius) to Final* over <b>TransitionTime</b>, optionally reversing back (<b>ReverseAtTransitionTime</b>). Used on hazard objects (e.g. fire walls, gas clouds) whose collision/affected area grows.""",
 ex="""Behavior = DynamicGeometryInfoUpdate ModuleTag_Geometry
  InitialDelay       = 0
  InitialHeight      = 10
  InitialMajorRadius = 10
  FinalHeight        = 40
  FinalMajorRadius   = 100
  TransitionTime     = 3000
  ReverseAtTransitionTime = No
End""")
F.update({
'DynamicGeometryInfoUpdateModuleData.InitialDelay': ("Delay (ms) before the change begins.", "0"),
'DynamicGeometryInfoUpdateModuleData.InitialHeight': ("Geometry height at the start.", "10"),
'DynamicGeometryInfoUpdateModuleData.InitialMajorRadius': ("Major radius at the start.", "10"),
'DynamicGeometryInfoUpdateModuleData.InitialMinorRadius': ("Minor radius at the start (boxes).", "10"),
'DynamicGeometryInfoUpdateModuleData.FinalHeight': ("Geometry height at the end.", "40"),
'DynamicGeometryInfoUpdateModuleData.FinalMajorRadius': ("Major radius at the end.", "100"),
'DynamicGeometryInfoUpdateModuleData.FinalMinorRadius': ("Minor radius at the end.", "100"),
'DynamicGeometryInfoUpdateModuleData.TransitionTime': ("Duration (ms) of the transition.", "3000"),
'DynamicGeometryInfoUpdateModuleData.ReverseAtTransitionTime': ("If Yes, the geometry reverses back toward the initial size once the transition completes.", "No"),
})

M['FirestormDynamicGeometryInfoUpdate'] = dict(
 sum="Firestorm (China Inferno / napalm firestorm) — growing geometry that deals periodic fire damage and drives up to 16 particle systems.",
 body="""Extends DynamicGeometryInfoUpdate: while the geometry grows it creates up to 16 particle systems (ParticleSystem1…16) plus <b>FXList</b> at height offset <b>ParticleOffsetZ</b>, scaling them with the geometry, leaves a scorch mark of <b>ScorchSize</b>, and every <b>DelayBetweenDamageFrames</b> deals <b>DamageAmount</b> (flame) to everything inside that is lower than <b>MaxHeightForDamage</b>.""",
 ex="""Behavior = FirestormDynamicGeometryInfoUpdate ModuleTag_Firestorm
  InitialHeight      = 10
  InitialMajorRadius = 10
  FinalHeight        = 80
  FinalMajorRadius   = 120
  TransitionTime     = 5000
  ParticleSystem1    = FirestormBase
  ParticleSystem2    = FirestormFlames
  FXList             = FX_FirestormSound
  ParticleOffsetZ    = 0
  ScorchSize         = 200
  DelayBetweenDamageFrames = 500
  DamageAmount       = 30
  MaxHeightForDamage = 20
End""")
F.update({
'FirestormDynamicGeometryInfoUpdateModuleData.DelayBetweenDamageFrames': ("Interval (ms) between damage pulses.", "500"),
'FirestormDynamicGeometryInfoUpdateModuleData.DamageAmount': ("Damage per pulse to everything inside.", "30"),
'FirestormDynamicGeometryInfoUpdateModuleData.MaxHeightForDamage': ("Objects higher than this above the firestorm take no damage.", "20"),
'FirestormDynamicGeometryInfoUpdateModuleData.FXList': ("FXList played when the firestorm starts.", "FX_FirestormSound"),
'FirestormDynamicGeometryInfoUpdateModuleData.ParticleOffsetZ': ("Height offset for the particle systems.", "0"),
'FirestormDynamicGeometryInfoUpdateModuleData.ScorchSize': ("Size of the scorch mark left behind.", "200"),
})

M['LaserUpdate'] = dict(
 sum="Client update that drives a laser beam object (Laser Crusader, Patriot link, Particle beam): endpoints, muzzle and target particles.",
 body="""A ClientUpdate (listed as <code>ClientUpdate = LaserUpdate</code>) on laser beam objects created by weapons. It keeps the beam stretched between the firer's bone and the target each frame and attaches <b>MuzzleParticleSystem</b> at the source and <b>TargetParticleSystem</b> at the impact. If the target disappears, <b>PunchThroughScalar</b> extends the beam length beyond where the target was. Rendering is done by W3DLaserDraw.""",
 ex="""ClientUpdate = LaserUpdate ModuleTag_Laser
  MuzzleParticleSystem = LaserMuzzleFlare
  TargetParticleSystem = LaserTargetBurn
  PunchThroughScalar   = 1.5
End""")
F.update({
'LaserUpdateModuleData.MuzzleParticleSystem': ("Particle system at the firing end while the laser is active.", "LaserMuzzleFlare"),
'LaserUpdateModuleData.TargetParticleSystem': ("Particle system at the target end while the laser is active.", "LaserTargetBurn"),
'LaserUpdateModuleData.PunchThroughScalar': ("If non-zero, beam length multiplier used when the original target is gone.", "1.5"),
})

M['PointDefenseLaserUpdate'] = dict(
 sum="Vanilla point-defense laser: independently scans for and shoots down incoming missiles/projectiles.",
 body="""Every <b>ScanRate</b> it looks within <b>ScanRange</b> for enemies of <b>PrimaryTargetTypes</b> (preferred) or <b>SecondaryTargetTypes</b> (e.g. BALLISTIC_MISSILE, SMALL_MISSILE, PROJECTILE) and fires <b>WeaponTemplate</b> at the closest one, independent of the unit's normal weapons. <b>PredictTargetVelocityFactor</b> was meant to lead fast targets, but in the vanilla code the predicted position is discarded (see PointDefenseUpdateV2). Used by USA Avenger/Paladin point-defense lasers.""",
 ex="""Behavior = PointDefenseLaserUpdate ModuleTag_PDL
  WeaponTemplate        = PaladinPointDefenseLaser
  PrimaryTargetTypes    = SMALL_MISSILE BALLISTIC_MISSILE
  SecondaryTargetTypes  = PROJECTILE
  ScanRate              = 0
  ScanRange             = 150
  PredictTargetVelocityFactor = 2.0
End""")
F.update({
'PointDefenseLaserUpdateModuleData.WeaponTemplate': ("Weapon fired at intercepted targets.", "PaladinPointDefenseLaser"),
'PointDefenseLaserUpdateModuleData.PrimaryTargetTypes': ("KindOfs the module prefers to shoot.", "SMALL_MISSILE BALLISTIC_MISSILE"),
'PointDefenseLaserUpdateModuleData.SecondaryTargetTypes': ("KindOfs shot only when no primary targets exist.", "PROJECTILE"),
'PointDefenseLaserUpdateModuleData.ScanRate': ("Interval (ms) between scans.", "0"),
'PointDefenseLaserUpdateModuleData.ScanRange': ("Search radius.", "150"),
'PointDefenseLaserUpdateModuleData.PredictTargetVelocityFactor': ("Multiplier on target velocity for leading (effectively unused in vanilla).", "2.0"),
})

M['PointDefenseUpdateV2'] = dict(
 sum="Reworked point-defense targeting: working velocity prediction, cached multi-target tracking, proper Anti* mask filtering, minimum intercept range, idle sleep.",
 body="""Mod-original rework of PointDefenseLaserUpdate that coexists with it. Improvements: <b>PredictTargetVelocityFactor</b> is actually applied (distance is measured to the predicted point); squared-distance math and enemy filtering inside the partition query; up to <b>MaxTrackedTargets</b> candidates cached per scan so losing a target doesn't force an immediate rescan; the target's classification is checked against the weapon's Anti* mask exactly like the engine's WeaponSet does (so a weapon with AntiProjectile but not AntiGround can intercept ballistic shells); <b>MinimumInterceptRange</b> leaves projectiles alone once they are closer than that radius (use this instead of the weapon's MinimumAttackRange, which would jam the module); failed shots no longer start a reload. Optional <b>IdleScanRate</b> puts the module to sleep when a scan finds nothing (0 = never sleep, vanilla vigilance).""",
 ex="""Behavior = PointDefenseUpdateV2 ModuleTag_PD
  WeaponTemplate        = ShieldInterceptorLaser
  PrimaryTargetTypes    = SMALL_MISSILE BALLISTIC_MISSILE
  SecondaryTargetTypes  = PROJECTILE
  ScanRate              = 100
  IdleScanRate          = 500
  ScanRange             = 180
  PredictTargetVelocityFactor = 1.0
  MaxTrackedTargets     = 4
  MinimumInterceptRange = 40
End""")
F.update({
'PointDefenseUpdateV2ModuleData.WeaponTemplate': ("Weapon fired at intercepted targets. Its Anti* flags (AntiProjectile, AntiSmallMissile, AntiBallisticMissile, AntiGround...) decide what can be shot.", "ShieldInterceptorLaser"),
'PointDefenseUpdateV2ModuleData.PrimaryTargetTypes': ("KindOfs preferred as targets.", "SMALL_MISSILE BALLISTIC_MISSILE"),
'PointDefenseUpdateV2ModuleData.SecondaryTargetTypes': ("KindOfs used when no primary target exists.", "PROJECTILE"),
'PointDefenseUpdateV2ModuleData.ScanRate': ("Interval (ms) between full re-scans while something is nearby.", "100"),
'PointDefenseUpdateV2ModuleData.IdleScanRate': ("Sleep time (ms) after a scan that found nothing at all. 0 = never idle-sleep.", "500"),
'PointDefenseUpdateV2ModuleData.ScanRange': ("Search radius.", "180"),
'PointDefenseUpdateV2ModuleData.PredictTargetVelocityFactor': ("Target velocity multiplier used to predict the intercept point (now actually used).", "1.0"),
'PointDefenseUpdateV2ModuleData.MaxTrackedTargets': ("Number of candidates cached per scan (clamped to an engine maximum).", "4"),
'PointDefenseUpdateV2ModuleData.MinimumInterceptRange': ("Never intercept a target closer than this (0 = disabled). Lets shots through once the shooter is inside the shield.", "40"),
})

M['ProjectileClipFeedbackUpdateV2'] = dict(
 sum="Keeps launch-bone projectile visibility (and optionally weapon model conditions) in sync for units riding inside a container.",
 body="""Mod-original, opt-in fix module. Normally the per-frame weapon status helper pushes the current ammo count to the Drawable so projectiles on launch bones hide/show (e.g. rockets on a pod). But units held inside a container (Transport/Overlord/Helix/Garrison…) are DISABLED_HELD, which skips that helper, so a reload that finishes outside an attack never updates the model. This module keeps running while held and re-pushes the state every <b>CheckInterval</b>. With <b>OnlyWhenContained</b> it does nothing unless inside a container; <b>UpdateWeaponConditions</b> also refreshes FIRING/RELOADING/BETWEEN/PREATTACK flags.""",
 ex="""Behavior = ProjectileClipFeedbackUpdateV2 ModuleTag_ClipFeedback
  CheckInterval          = 250
  OnlyWhenContained      = Yes
  UpdateWeaponConditions = No
End""")
F.update({
'ProjectileClipFeedbackUpdateV2ModuleData.CheckInterval': ("Time (ms) between checks (min 1 frame).", "250"),
'ProjectileClipFeedbackUpdateV2ModuleData.OnlyWhenContained': ("If Yes (default), do nothing unless the object is inside a container.", "Yes"),
'ProjectileClipFeedbackUpdateV2ModuleData.UpdateWeaponConditions': ("If Yes, also refresh weapon model-condition flags (FIRING/RELOADING/...); No = ammo feedback only.", "No"),
})

M['CleanupHazardUpdate'] = dict(
 sum="Lets a unit automatically target and clean up hazards (toxin puddles, radiation) near it.",
 body="""Every <b>ScanRate</b> an idle unit scans <b>ScanRange</b> for CLEANUP_HAZARD objects and attacks them with the weapon in <b>WeaponSlot</b> (e.g. the Ambulance/Hazmat cleaning spray).""",
 ex="""Behavior = CleanupHazardUpdate ModuleTag_Cleanup
  WeaponSlot = SECONDARY
  ScanRate   = 1000
  ScanRange  = 100
End""")
F.update({
'CleanupHazardUpdateModuleData.WeaponSlot': ("Weapon slot used to clean hazards.", "SECONDARY"),
'CleanupHazardUpdateModuleData.ScanRate': ("Interval (ms) between scans.", "1000"),
'CleanupHazardUpdateModuleData.ScanRange': ("Search radius.", "100"),
})

M['CommandButtonHuntUpdate'] = dict(
 sum="Implements 'hunt' command buttons: the unit repeatedly seeks targets for a special ability (e.g. auto-hijack, auto-snipe).",
 body="""When the player uses a command button in 'hunt' mode, the unit keeps searching within <b>ScanRange</b> every <b>ScanRate</b> for a valid target for that button's ability and executes it, until given another order. Used for abilities like the Hijacker or Jarmen Kell's 'kill pilot' hunt.""",
 ex="""Behavior = CommandButtonHuntUpdate ModuleTag_Hunt
  ScanRate  = 1000
  ScanRange = 9999
End""")
F.update({
'CommandButtonHuntUpdateModuleData.ScanRate': ("Interval (ms) between target searches.", "1000"),
'CommandButtonHuntUpdateModuleData.ScanRange': ("Search radius.", "9999"),
})

M['PilotFindVehicleUpdate'] = dict(
 sum="Lets ejected pilots automatically find and board an empty/damaged friendly vehicle.",
 body="""An idle pilot scans <b>ScanRange</b> every <b>ScanRate</b> for a friendly vehicle it can enter (to give it veterancy) and whose health is at least <b>MinHealth</b>, then goes to it.""",
 ex="""Behavior = PilotFindVehicleUpdate ModuleTag_FindVehicle
  ScanRate  = 1000
  ScanRange = 300
  MinHealth = 0.5
End""")
F.update({
'PilotFindVehicleUpdateModuleData.ScanRate': ("Interval (ms) between scans.", "1000"),
'PilotFindVehicleUpdateModuleData.ScanRange': ("Search radius.", "300"),
'PilotFindVehicleUpdateModuleData.MinHealth': ("Minimum health fraction of a vehicle to be considered.", "0.5"),
})

M['DemoTrapUpdate'] = dict(
 sum="GLA Demo Trap logic: proximity or manual detonation modes.",
 body="""The trap has two modes toggled by weapon-slot buttons: proximity mode (<b>ProximityModeWeaponSlot</b>) where it scans <b>TriggerDetonationRange</b> every <b>ScanRate</b> and explodes when an enemy (not of <b>IgnoreTargetTypes</b>) enters, and manual mode (<b>ManualModeWeaponSlot</b>) where it only explodes on command (<b>DetonationWeaponSlot</b>). <b>DefaultProximityMode</b> picks the start mode. <b>AutoDetonationWithFriendsInvolved</b> allows detonating even if friends are in range; <b>DetonateWhenKilled</b> makes it explode when destroyed. <b>DetonationWeapon</b> is the explosion.""",
 ex="""Behavior = DemoTrapUpdate ModuleTag_Trap
  DefaultProximityMode     = Yes
  DetonationWeaponSlot     = PRIMARY
  ProximityModeWeaponSlot  = SECONDARY
  ManualModeWeaponSlot     = TERTIARY
  TriggerDetonationRange   = 30
  IgnoreTargetTypes        = PROJECTILE
  ScanRate                 = 500
  AutoDetonationWithFriendsInvolved = No
  DetonationWeapon         = DemoTrapDetonationWeapon
  DetonateWhenKilled       = Yes
End""")
F.update({
'DemoTrapUpdateModuleData.DefaultProximityMode': ("If Yes, the trap starts in proximity mode.", "Yes"),
'DemoTrapUpdateModuleData.DetonationWeaponSlot': ("Weapon slot whose button detonates the trap manually.", "PRIMARY"),
'DemoTrapUpdateModuleData.ProximityModeWeaponSlot': ("Weapon slot whose button switches to proximity mode.", "SECONDARY"),
'DemoTrapUpdateModuleData.ManualModeWeaponSlot': ("Weapon slot whose button switches to manual mode.", "TERTIARY"),
'DemoTrapUpdateModuleData.TriggerDetonationRange': ("Radius that triggers detonation in proximity mode.", "30"),
'DemoTrapUpdateModuleData.IgnoreTargetTypes': ("KindOfs that never trigger the trap.", "PROJECTILE"),
'DemoTrapUpdateModuleData.ScanRate': ("Interval (ms) between proximity scans.", "500"),
'DemoTrapUpdateModuleData.AutoDetonationWithFriendsInvolved': ("If Yes, proximity detonation happens even if friendly units are in range.", "No"),
'DemoTrapUpdateModuleData.DetonationWeapon': ("Weapon fired as the explosion.", "DemoTrapDetonationWeapon"),
'DemoTrapUpdateModuleData.DetonateWhenKilled': ("If Yes, the trap explodes when destroyed.", "Yes"),
})

M['ParticleUplinkCannonUpdate'] = dict(
 sum="USA Particle Cannon superweapon: charge-up states, beam firing, player-steerable swath of destruction.",
 body="""Drives the Particle Uplink Cannon building for <b>SpecialPowerTemplate</b>. It runs through visual states: charging (<b>BeginChargeTime</b>, outer node flares on bones <b>OuterEffectBoneName</b>01..N), raising the antenna (<b>RaiseAntennaTime</b>), ready (<b>ReadyDelayTime</b>), then firing: connector lasers between nodes, a base flare and the main orbital beam (<b>ParticleBeamLaserName</b>) that grows in width (<b>WidthGrowTime</b>), travels down (<b>BeamTravelTime</b>) and burns for <b>TotalFiringTime</b>. On the ground it deals <b>DamagePerSecond</b> in <b>TotalDamagePulses</b> pulses (with <b>DamageType</b>/<b>DeathType</b>), leaves scorch marks and remnant objects, reveals <b>RevealRange</b>, and wobbles in a 'swath of death' (<b>SwathOfDeathDistance/Amplitude</b>). In Zero Hour the player can steer the beam (<b>ManualDrivingSpeed</b>, double-click for <b>ManualFastDrivingSpeed</b>). Many *SoundLoop fields add audio.""",
 ex="""Behavior = ParticleUplinkCannonUpdate ModuleTag_PUC
  SpecialPowerTemplate = SuperweaponParticleUplinkCannon
  BeginChargeTime      = 5000
  RaiseAntennaTime     = 5000
  ReadyDelayTime       = 2000
  WidthGrowTime        = 2000
  BeamTravelTime       = 2500
  TotalFiringTime      = 10000
  RevealRange          = 200
  OuterEffectBoneName  = FX
  OuterEffectNumBones  = 5
  ConnectorBoneName    = FXConnector
  FireBoneName         = FireBone
  ParticleBeamLaserName= ParticleUplinkCannon_OrbitalLaser
  DamagePerSecond      = 400
  TotalDamagePulses    = 20
  DamageType           = PARTICLE_BEAM
  DeathType            = LASERED
  DamageRadiusScalar   = 1.0
  SwathOfDeathDistance = 100
  SwathOfDeathAmplitude= 50
  ManualDrivingSpeed   = 20
  ManualFastDrivingSpeed = 40
End""")
F.update({
'ParticleUplinkCannonUpdateModuleData.SpecialPowerTemplate': ("The superweapon special power this update belongs to.", "SuperweaponParticleUplinkCannon"),
'ParticleUplinkCannonUpdateModuleData.BeginChargeTime': ("Time (ms) of the initial charge-up phase.", "5000"),
'ParticleUplinkCannonUpdateModuleData.RaiseAntennaTime': ("Time (ms) to raise the antenna.", "5000"),
'ParticleUplinkCannonUpdateModuleData.ReadyDelayTime': ("Delay (ms) in the ready state before firing.", "2000"),
'ParticleUplinkCannonUpdateModuleData.WidthGrowTime': ("Time (ms) for the beam to reach full width.", "2000"),
'ParticleUplinkCannonUpdateModuleData.BeamTravelTime': ("Time (ms) for the beam to travel from sky to ground.", "2500"),
'ParticleUplinkCannonUpdateModuleData.TotalFiringTime': ("Total time (ms) the beam fires.", "10000"),
'ParticleUplinkCannonUpdateModuleData.RevealRange': ("Shroud reveal radius around the beam impact.", "200"),
'ParticleUplinkCannonUpdateModuleData.OuterEffectBoneName': ("Base name of the outer node bones (Name01..N).", "FX"),
'ParticleUplinkCannonUpdateModuleData.OuterEffectNumBones': ("Number of outer node bones.", "5"),
'ParticleUplinkCannonUpdateModuleData.OuterNodesLightFlareParticleSystem': ("Particle system on outer nodes during light charge.", "PUCOuterNodeLightFlare"),
'ParticleUplinkCannonUpdateModuleData.OuterNodesMediumFlareParticleSystem': ("Particle system on outer nodes during medium charge.", "PUCOuterNodeMediumFlare"),
'ParticleUplinkCannonUpdateModuleData.OuterNodesIntenseFlareParticleSystem': ("Particle system on outer nodes at full charge.", "PUCOuterNodeIntenseFlare"),
'ParticleUplinkCannonUpdateModuleData.ConnectorBoneName': ("Bone where connector lasers meet.", "FXConnector"),
'ParticleUplinkCannonUpdateModuleData.ConnectorMediumLaserName': ("Laser object for medium-intensity connectors.", "PUCConnectorMediumLaser"),
'ParticleUplinkCannonUpdateModuleData.ConnectorIntenseLaserName': ("Laser object for intense connectors.", "PUCConnectorIntenseLaser"),
'ParticleUplinkCannonUpdateModuleData.ConnectorMediumFlare': ("Flare particle system at the connector (medium).", "PUCConnectorMediumFlare"),
'ParticleUplinkCannonUpdateModuleData.ConnectorIntenseFlare': ("Flare particle system at the connector (intense).", "PUCConnectorIntenseFlare"),
'ParticleUplinkCannonUpdateModuleData.FireBoneName': ("Bone where the main beam is emitted.", "FireBone"),
'ParticleUplinkCannonUpdateModuleData.LaserBaseLightFlareParticleSystemName': ("Particle system at the beam base (light).", "PUCBaseLightFlare"),
'ParticleUplinkCannonUpdateModuleData.LaserBaseMediumFlareParticleSystemName': ("Particle system at the beam base (medium).", "PUCBaseMediumFlare"),
'ParticleUplinkCannonUpdateModuleData.LaserBaseIntenseFlareParticleSystemName': ("Particle system at the beam base (intense).", "PUCBaseIntenseFlare"),
'ParticleUplinkCannonUpdateModuleData.ParticleBeamLaserName': ("Laser object for the main orbital beam.", "ParticleUplinkCannon_OrbitalLaser"),
'ParticleUplinkCannonUpdateModuleData.SwathOfDeathDistance': ("Length of the automatic 'swath' the beam sweeps along.", "100"),
'ParticleUplinkCannonUpdateModuleData.SwathOfDeathAmplitude': ("Side-to-side amplitude of the swath.", "50"),
'ParticleUplinkCannonUpdateModuleData.TotalScorchMarks': ("Number of scorch marks left along the path.", "20"),
'ParticleUplinkCannonUpdateModuleData.ScorchMarkScalar': ("Size multiplier for scorch marks.", "1.0"),
'ParticleUplinkCannonUpdateModuleData.BeamLaunchFX': ("FXList played repeatedly at the launch point while firing.", "FX_PUCLaunch"),
'ParticleUplinkCannonUpdateModuleData.DelayBetweenLaunchFX': ("Interval (ms) between BeamLaunchFX plays.", "1000"),
'ParticleUplinkCannonUpdateModuleData.GroundHitFX': ("FXList at the ground impact point.", "FX_PUCGroundHit"),
'ParticleUplinkCannonUpdateModuleData.DamagePerSecond': ("Damage per second dealt at the impact point.", "400"),
'ParticleUplinkCannonUpdateModuleData.TotalDamagePulses': ("Number of damage pulses spread over the firing time.", "20"),
'ParticleUplinkCannonUpdateModuleData.DamageType': ("DamageType of the beam damage.", "PARTICLE_BEAM"),
'ParticleUplinkCannonUpdateModuleData.DeathType': ("DeathType of units killed by the beam.", "LASERED"),
'ParticleUplinkCannonUpdateModuleData.DamageRadiusScalar': ("Multiplier on the damage radius.", "1.0"),
'ParticleUplinkCannonUpdateModuleData.PoweringUpSoundLoop': ("Looping sound while powering up.", "PUCPoweringUp"),
'ParticleUplinkCannonUpdateModuleData.UnpackToIdleSoundLoop': ("Looping sound while ready/idle.", "PUCIdle"),
'ParticleUplinkCannonUpdateModuleData.FiringToPackSoundLoop': ("Looping sound while firing/packing.", "PUCFiring"),
'ParticleUplinkCannonUpdateModuleData.GroundAnnihilationSoundLoop': ("Looping sound at the ground impact.", "PUCGroundAnnihilation"),
'ParticleUplinkCannonUpdateModuleData.DamagePulseRemnantObjectName': ("Object created at each damage pulse (e.g. burning remnant).", "ParticleUplinkCannonRemnant"),
'ParticleUplinkCannonUpdateModuleData.ManualDrivingSpeed': ("Speed at which the player can steer the beam.", "20"),
'ParticleUplinkCannonUpdateModuleData.ManualFastDrivingSpeed': ("Steering speed after a double-click.", "40"),
'ParticleUplinkCannonUpdateModuleData.DoubleClickToFastDriveDelay': ("Max time (ms) between clicks to count as a double-click for fast steering.", "500"),
})

M['SpectreGunshipUpdate'] = dict(
 sum="USA Spectre Gunship special power aircraft: orbits a target area firing a howitzer and a strafing gattling.",
 body="""Controls the gunship object: it inserts into orbit (<b>OrbitInsertionSlope</b>) around the target at <b>GunshipOrbitRadius</b> for <b>OrbitTime</b>. While orbiting its howitzer (<b>HowitzerWeaponTemplate</b>, every <b>HowitzerFiringRate</b>, with <b>RandomOffsetForHowitzer</b> and <b>HowitzerFollowLag</b>) and a separate gattling object (<b>GattlingTemplateName</b>, strafing with <b>StrafingIncrement</b> and <b>GattlingStrafeFXParticleSystem</b>) attack targets inside <b>AttackAreaRadius</b>. The player moves the targeting reticle; <b>AttackAreaDecal</b> and <b>TargetingReticleDecal</b> show the area and reticle.""",
 ex="""Behavior = SpectreGunshipUpdate ModuleTag_Spectre
  SpecialPowerTemplate  = SuperweaponSpectreGunship
  GattlingTemplateName  = AmericaSpectreGunshipGattling
  HowitzerWeaponTemplate= SpectreHowitzerGun
  HowitzerFiringRate    = 1000
  HowitzerFollowLag     = 500
  OrbitTime             = 20000
  AttackAreaRadius      = 200
  StrafingIncrement     = 20
  OrbitInsertionSlope   = 0.7
  RandomOffsetForHowitzer = 20
  TargetingReticleRadius= 25
  GunshipOrbitRadius    = 250
  AttackAreaDecal
    Texture = SCCSpectreAttackArea
    Style   = SHADOW_ALPHA_DECAL
    OnlyVisibleToOwningPlayer = Yes
  End
  TargetingReticleDecal
    Texture = SCCSpectreReticle
    Style   = SHADOW_ALPHA_DECAL
  End
End""")
F.update({
'SpectreGunshipUpdateModuleData.SpecialPowerTemplate': ("Special power this gunship belongs to.", "SuperweaponSpectreGunship"),
'SpectreGunshipUpdateModuleData.GattlingTemplateName': ("Object template of the gattling gun object that strafes.", "AmericaSpectreGunshipGattling"),
'SpectreGunshipUpdateModuleData.HowitzerFiringRate': ("Interval (ms) between howitzer shots.", "1000"),
'SpectreGunshipUpdateModuleData.OrbitTime': ("How long (ms) the gunship orbits before leaving.", "20000"),
'SpectreGunshipUpdateModuleData.HowitzerFollowLag': ("Lag (ms) with which the howitzer aim follows the reticle.", "500"),
'SpectreGunshipUpdateModuleData.AttackAreaRadius': ("Radius of the attack area around the orbit centre.", "200"),
'SpectreGunshipUpdateModuleData.StrafingIncrement': ("Distance the gattling strafing point advances per step.", "20"),
'SpectreGunshipUpdateModuleData.OrbitInsertionSlope': ("Slope of the approach path into orbit.", "0.7"),
'SpectreGunshipUpdateModuleData.RandomOffsetForHowitzer': ("Random scatter of howitzer shots.", "20"),
'SpectreGunshipUpdateModuleData.TargetingReticleRadius': ("Radius of the targeting reticle decal.", "25"),
'SpectreGunshipUpdateModuleData.GunshipOrbitRadius': ("Radius of the gunship's orbit.", "250"),
'SpectreGunshipUpdateModuleData.HowitzerWeaponTemplate': ("Weapon used by the howitzer.", "SpectreHowitzerGun"),
'SpectreGunshipUpdateModuleData.GattlingStrafeFXParticleSystem': ("Particle system along the strafing path.", "SpectreGattlingStrafe"),
'SpectreGunshipUpdateModuleData.AttackAreaDecal': ("Sub-block: decal showing the attack area.", "(sub-block)"),
'SpectreGunshipUpdateModuleData.TargetingReticleDecal': ("Sub-block: decal showing the targeting reticle.", "(sub-block)"),
})

M['SpectreGunshipDeploymentUpdate'] = dict(
 sum="Command-center side of the Spectre Gunship power: spawns the gunship at the map edge and sends it to the target.",
 body="""Placed on the building that owns the Spectre power. When the power is used (and <b>RequiredScience</b> is owned) it creates <b>GunshipTemplateName</b> at the location chosen by <b>CreateLocation</b> (e.g. CREATE_AT_EDGE_FARTHEST_FROM_TARGET, CREATE_AT_EDGE_NEAR_SOURCE, CREATE_AT_LOCATION) and hands it the target with <b>AttackAreaRadius</b>.""",
 ex="""Behavior = SpectreGunshipDeploymentUpdate ModuleTag_SpectreDeploy
  SpecialPowerTemplate = SuperweaponSpectreGunship
  RequiredScience      = SCIENCE_SpectreGunship1
  GunshipTemplateName  = AmericaSpectreGunship
  AttackAreaRadius     = 200
  CreateLocation       = CREATE_AT_EDGE_FARTHEST_FROM_TARGET
End""")
F.update({
'SpectreGunshipDeploymentUpdateModuleData.GunshipTemplateName': ("Object template of the gunship to create.", "AmericaSpectreGunship"),
'SpectreGunshipDeploymentUpdateModuleData.RequiredScience': ("Science (general's power rank) required to use the power.", "SCIENCE_SpectreGunship1"),
'SpectreGunshipDeploymentUpdateModuleData.SpecialPowerTemplate': ("Special power that triggers the deployment.", "SuperweaponSpectreGunship"),
'SpectreGunshipDeploymentUpdateModuleData.AttackAreaRadius': ("Attack area radius passed to the gunship.", "200"),
'SpectreGunshipDeploymentUpdateModuleData.CreateLocation': ("Where the gunship is created: CREATE_AT_EDGE_NEAR_SOURCE, CREATE_AT_EDGE_FARTHEST_FROM_SOURCE, CREATE_AT_EDGE_NEAR_TARGET, CREATE_AT_EDGE_FARTHEST_FROM_TARGET, CREATE_AT_LOCATION.", "CREATE_AT_EDGE_FARTHEST_FROM_TARGET"),
})

# SpecialPowerModuleData shared
F.update({
'SpecialPowerModuleData.SpecialPowerTemplate': ("The SpecialPower (from SpecialPower.ini) this module implements; its reload time, sciences and command button come from there.", "SuperweaponScudStorm"),
'SpecialPowerModuleData.UpdateModuleStartsAttack': ("If Yes, a companion update module (e.g. SpecialAbilityUpdate, ParticleUplinkCannonUpdate) decides when the power actually fires; the countdown restarts from there.", "No"),
'SpecialPowerModuleData.StartsPaused': ("If Yes, the recharge timer starts paused (unpaused later by an upgrade/script, e.g. UnpauseSpecialPowerUpgrade).", "No"),
'SpecialPowerModuleData.StartsReady': ("Yes: the power is ready as soon as the object is created. No: it starts recharging.", "No"),
'SpecialPowerModuleData.InitiateSound': ("Sound played when the power is activated.", "ScudStormInitiate"),
'SpecialPowerModuleData.ScriptedSpecialPowerOnly': ("If Yes, only map scripts can trigger this power (no button).", "No"),
})

M['BaikonurLaunchPower'] = dict(
 sum="Campaign special power that launches the Baikonur rocket and creates a detonation object at the target.",
 body="""A SpecialPowerModule used by the Baikonur Cosmodrome map object (USA campaign). When triggered it plays the launch and creates <b>DetonationObject</b> at the target location. Includes the standard SpecialPowerModule fields.""",
 ex="""Behavior = BaikonurLaunchPower ModuleTag_Launch
  SpecialPowerTemplate = SuperweaponBaikonurLaunch
  DetonationObject     = BaikonurRocketDetonation
End""")
F.update({'BaikonurLaunchPowerModuleData.DetonationObject': ("Object created at the target when the launch detonates.", "BaikonurRocketDetonation")})

M['BattlePlanUpdate'] = dict(
 sum="USA Strategy Center battle plans: Bombardment, Hold the Line, Search and Destroy — global buffs with building animations.",
 body="""Drives the Strategy Center. Selecting a plan plays pack/unpack animations (<b>*PlanAnimationTime</b>, <b>TransitionIdleTime</b>) and sounds, then applies the plan's bonus to every eligible unit of the player (filtered by <b>ValidMemberKindOf</b>/<b>InvalidMemberKindOf</b>): Bombardment gives a weapon bonus; Hold the Line multiplies armor damage by <b>HoldTheLinePlanArmorDamageScalar</b>; Search and Destroy multiplies sight by <b>SearchAndDestroyPlanSightRangeScalar</b>. The building itself gets its own bonuses (StrategyCenter* fields: extra vision/stealth detection, more max health). Changing plans paralyzes units for <b>BattlePlanChangeParalyzeTime</b>. <b>VisionObjectName</b> can reveal shroud.""",
 ex="""Behavior = BattlePlanUpdate ModuleTag_BattlePlan
  SpecialPowerTemplate               = SpecialAbilityChangeBattlePlans
  BombardmentPlanAnimationTime       = 2000
  HoldTheLinePlanAnimationTime       = 2000
  SearchAndDestroyPlanAnimationTime  = 2000
  TransitionIdleTime                 = 3000
  ValidMemberKindOf                  = INFANTRY VEHICLE
  InvalidMemberKindOf                = AIRCRAFT
  BattlePlanChangeParalyzeTime       = 5000
  HoldTheLinePlanArmorDamageScalar   = 0.9
  SearchAndDestroyPlanSightRangeScalar = 1.2
  StrategyCenterSearchAndDestroySightRangeScalar = 2.0
  StrategyCenterSearchAndDestroyDetectsStealth   = Yes
  StrategyCenterHoldTheLineMaxHealthScalar       = 2.0
  StrategyCenterHoldTheLineMaxHealthChangeType   = PRESERVE_RATIO
End""")
for plan, name in (('Bombardment', 'Bombardment'), ('HoldTheLine', 'Hold the Line'), ('SearchAndDestroy', 'Search and Destroy')):
    F.update({
    f'BattlePlanUpdateModuleData.{plan}PlanAnimationTime': (f"Time (ms) of the {name} deploy animation.", "2000"),
    f'BattlePlanUpdateModuleData.{plan}PlanUnpackSoundName': (f"Sound when the {name} plan unpacks.", f"{plan}Unpack"),
    f'BattlePlanUpdateModuleData.{plan}PlanPackSoundName': (f"Sound when the {name} plan packs.", f"{plan}Pack"),
    f'BattlePlanUpdateModuleData.{plan}MessageLabel': (f"UI message label (string key) shown when {name} is activated.", f"GUI:{plan}Activated"),
    f'BattlePlanUpdateModuleData.{plan}AnnouncementName': (f"Announcement/EVA sound for {name}.", f"{plan}Announcement"),
    })
F.update({
'BattlePlanUpdateModuleData.SpecialPowerTemplate': ("Special power used by the plan-change buttons.", "SpecialAbilityChangeBattlePlans"),
'BattlePlanUpdateModuleData.TransitionIdleTime': ("Idle time (ms) between packing one plan and unpacking the next.", "3000"),
'BattlePlanUpdateModuleData.SearchAndDestroyPlanIdleLoopSoundName': ("Looping sound while Search and Destroy is active.", "SearchAndDestroyIdleLoop"),
'BattlePlanUpdateModuleData.ValidMemberKindOf': ("Units must have one of these KindOfs to receive plan bonuses.", "INFANTRY VEHICLE"),
'BattlePlanUpdateModuleData.InvalidMemberKindOf': ("Units with any of these KindOfs never receive plan bonuses.", "AIRCRAFT"),
'BattlePlanUpdateModuleData.BattlePlanChangeParalyzeTime': ("Time (ms) units are paralyzed when the plan changes.", "5000"),
'BattlePlanUpdateModuleData.HoldTheLinePlanArmorDamageScalar': ("Damage multiplier taken by units under Hold the Line (<1 = tougher).", "0.9"),
'BattlePlanUpdateModuleData.SearchAndDestroyPlanSightRangeScalar': ("Vision multiplier for units under Search and Destroy.", "1.2"),
'BattlePlanUpdateModuleData.StrategyCenterSearchAndDestroySightRangeScalar': ("Vision multiplier for the Strategy Center itself under Search and Destroy.", "2.0"),
'BattlePlanUpdateModuleData.StrategyCenterSearchAndDestroyDetectsStealth': ("If Yes, the Strategy Center detects stealth under Search and Destroy.", "Yes"),
'BattlePlanUpdateModuleData.StrategyCenterHoldTheLineMaxHealthScalar': ("Max health multiplier for the Strategy Center under Hold the Line.", "2.0"),
'BattlePlanUpdateModuleData.StrategyCenterHoldTheLineMaxHealthChangeType': ("How current health adjusts when max health changes: PRESERVE_RATIO, ADD_CURRENT_HEALTH_TOO, SAME_CURRENTHEALTH.", "PRESERVE_RATIO"),
'BattlePlanUpdateModuleData.VisionObjectName': ("Object created to reveal the Strategy Center to all players.", ""),
})

M['ProjectileStreamUpdate'] = dict(
 sum="Tracks all projectiles fired by a weapon so they can be drawn as a continuous stream (e.g. Toxin Tractor spray).",
 body="""Placed on a stream helper object used by weapons with a projectile stream (flame/toxin spray). It keeps a list of the in-flight projectiles so W3DProjectileStreamDraw can render a connected ribbon through them. No INI fields.""",
 ex="""Behavior = ProjectileStreamUpdate ModuleTag_Stream
End""")

M['QueueProductionExitUpdate'] = dict(
 sum="Production exit that releases produced units one at a time, waiting for each to clear the exit (e.g. airfields, war factories).",
 body="""Receives units from ProductionUpdate and places them at <b>UnitCreatePoint</b> (relative to the building), sending them to <b>NaturalRallyPoint</b> (or the player's rally point). It won't release the next unit until <b>ExitDelay</b> has passed and the previous one moved away. <b>AllowAirborneCreation</b> lets units be created in the air; <b>InitialBurst</b> releases that many immediately.""",
 ex="""Behavior = QueueProductionExitUpdate ModuleTag_Exit
  UnitCreatePoint   = X:0 Y:0 Z:0
  NaturalRallyPoint = X:50 Y:0 Z:0
  ExitDelay         = 300
  AllowAirborneCreation = No
End""")
F.update({
'QueueProductionExitUpdateModuleData.UnitCreatePoint': ("Position (X:Y:Z, relative to the building) where new units appear.", "X:0 Y:0 Z:0"),
'QueueProductionExitUpdateModuleData.NaturalRallyPoint': ("Relative point new units move to when no rally point is set.", "X:50 Y:0 Z:0"),
'QueueProductionExitUpdateModuleData.ExitDelay': ("Minimum time (ms) between releasing units.", "300"),
'QueueProductionExitUpdateModuleData.AllowAirborneCreation': ("If Yes, units can be created above ground.", "No"),
'QueueProductionExitUpdateModuleData.InitialBurst': ("Number of units released immediately without delay.", "0"),
})

F.update({
'DockUpdateModuleData.NumberApproachPositions': ("How many units can queue at the dock's approach positions (DockWaiting bones). -1 = dynamic approach vector.", "5"),
'DockUpdateModuleData.AllowsPassthrough': ("If Yes, docking units drive through the dock (in one side, out the other) instead of backing out.", "Yes"),
})

M['RepairDockUpdate'] = dict(
 sum="Repair bay dock: vehicles that dock are repaired to full over TimeForFullHeal.",
 body="""Dock module for repair structures (USA/China War Factory repair bays, GLA Arms Dealer…). Vehicles approach via DockWaiting/DockStart/DockEnd bones and are healed to full over <b>TimeForFullHeal</b>.""",
 ex="""Behavior = RepairDockUpdate ModuleTag_Repair
  NumberApproachPositions = 5
  AllowsPassthrough       = No
  TimeForFullHeal         = 5000
End""")
F.update({'RepairDockUpdateModuleData.TimeForFullHeal': ("Time (ms) to fully repair a docked vehicle.", "5000")})

M['PrisonDockUpdate'] = dict(
 sum="Dock update for prisons: POW trucks dock here to unload prisoners.",
 body="""Used together with PrisonBehavior (cut POW feature): POW trucks approach and unload captured infantry into the prison. Only the DockUpdate fields.""",
 ex="""Behavior = PrisonDockUpdate ModuleTag_Dock
  NumberApproachPositions = 3
End""")

M['RailedTransportDockUpdate'] = dict(
 sum="Station dock for rail transports: pulls the transport inside and pushes it back out.",
 body="""Dock module for railed transport stations. When the transport arrives within <b>ToleranceDistance</b> it is pulled into the station over <b>PullInsideDuration</b> so passengers can load/unload, then pushed out over <b>PushOutsideDuration</b>.""",
 ex="""Behavior = RailedTransportDockUpdate ModuleTag_Dock
  PullInsideDuration  = 2000
  PushOutsideDuration = 2000
  ToleranceDistance   = 50
End""")
F.update({
'RailedTransportDockUpdateModuleData.PullInsideDuration': ("Time (ms) to pull the transport inside.", "2000"),
'RailedTransportDockUpdateModuleData.PushOutsideDuration': ("Time (ms) to push the transport back out.", "2000"),
'RailedTransportDockUpdateModuleData.ToleranceDistance': ("Max distance at which the transport can cheat-dock.", "50"),
})

M['DefaultProductionExitUpdate'] = dict(
 sum="Simplest production exit: spawns produced units at a point and sends them to a rally point.",
 body="""Produced units appear at <b>UnitCreatePoint</b> (relative to the building) and move to <b>NaturalRallyPoint</b> or the player's rally point. <b>UseSpawnRallyPoint</b> makes them use a spawn-point bone rally instead. Used by barracks and most factories.""",
 ex="""Behavior = DefaultProductionExitUpdate ModuleTag_Exit
  UnitCreatePoint   = X:20 Y:0 Z:0
  NaturalRallyPoint = X:60 Y:0 Z:0
End""")
F.update({
'DefaultProductionExitUpdateModuleData.UnitCreatePoint': ("Position (X:Y:Z relative to the building) where produced units spawn.", "X:20 Y:0 Z:0"),
'DefaultProductionExitUpdateModuleData.NaturalRallyPoint': ("Relative point units walk to when no rally point is set.", "X:60 Y:0 Z:0"),
'DefaultProductionExitUpdateModuleData.UseSpawnRallyPoint': ("If Yes, use the spawn rally point instead.", "No"),
})

M['SpawnPointProductionExitUpdate'] = dict(
 sum="Production exit that places produced units on named bones (e.g. Stinger Site soldiers at their fighting positions).",
 body="""Each produced/spawned unit is placed at the next free bone named <b>SpawnPointBoneName</b>01, 02… instead of a single exit point. Commonly combined with SpawnBehavior.""",
 ex="""Behavior = SpawnPointProductionExitUpdate ModuleTag_Exit
  SpawnPointBoneName = SpawnPoint
End""")
F.update({'SpawnPointProductionExitUpdateModuleData.SpawnPointBoneName': ("Base bone name of the spawn points (Name01, Name02...).", "SpawnPoint")})

M['SpyVisionUpdate'] = dict(
 sum="Lets the owner see everything enemies see (shares enemy vision) for a period — GLA radar/Spy Vision.",
 body="""While active, the owning player sees what enemy players' objects of kind <b>SpyOnKindof</b> can see. Normally activated by SpyVisionSpecialPower; with <b>SelfPowered</b> it cycles on its own: on for <b>SelfPoweredDuration</b> every <b>SelfPoweredInterval</b>. <b>NeedsUpgrade</b> makes it require the TriggeredBy upgrade first.""",
 ex="""Behavior = SpyVisionUpdate ModuleTag_SpyVision
  NeedsUpgrade        = Yes
  TriggeredBy         = Upgrade_GLARadar
  SelfPowered         = Yes
  SelfPoweredDuration = 10000
  SelfPoweredInterval = 30000
  SpyOnKindof         = STRUCTURE
End""")
F.update({
'SpyVisionUpdateModuleData.NeedsUpgrade': ("If Yes, requires the TriggeredBy upgrade before it can work.", "Yes"),
'SpyVisionUpdateModuleData.SelfPowered': ("If Yes, activates itself periodically without a special power.", "Yes"),
'SpyVisionUpdateModuleData.SelfPoweredDuration': ("How long (ms) each self-powered activation lasts.", "10000"),
'SpyVisionUpdateModuleData.SelfPoweredInterval': ("Time (ms) between self-powered activations.", "30000"),
'SpyVisionUpdateModuleData.SpyOnKindof': ("Only enemy objects of these KindOfs share their vision. Empty = all.", "STRUCTURE"),
})

M['SlavedUpdate'] = dict(
 sum="Keeps a slave (drone, Stinger soldier) near its master: guard, attack, scout and repair behaviours.",
 body="""For objects spawned by SpawnBehavior or object upgrades. The slave stays within <b>GuardMaxRange</b> of an idle master (wandering <b>GuardWanderRange</b>), goes to the master's target when it attacks (<b>AttackRange</b>/<b>AttackWanderRange</b>), scouts ahead when it moves (<b>ScoutRange</b>). Being within <b>DistToTargetToGrantRangeBonus</b> of the target grants the master a range bonus (Scout Drone spotting). Battle drones repair the master when its health is below <b>RepairWhenBelowHealth%</b> at <b>RepairRatePerSecond</b>, hovering between RepairMin/MaxAltitude and playing a welding particle system at <b>RepairWeldingFXBone</b>.""",
 ex="""Behavior = SlavedUpdate ModuleTag_Slaved
  GuardMaxRange        = 30
  GuardWanderRange     = 20
  AttackRange          = 100
  AttackWanderRange    = 20
  ScoutRange           = 100
  ScoutWanderRange     = 10
  RepairRange          = 20
  RepairMinAltitude    = 20
  RepairMaxAltitude    = 25
  RepairRatePerSecond  = 5
  RepairWhenBelowHealth% = 90
  RepairMinReadyTime   = 1000
  RepairMaxReadyTime   = 2000
  RepairMinWeldTime    = 500
  RepairMaxWeldTime    = 1500
  RepairWeldingSys     = BattleDroneWelding
  RepairWeldingFXBone  = WeldPoint
  StayOnSameLayerAsMaster = No
End""")
F.update({
'SlavedUpdateModuleData.GuardMaxRange': ("Max distance from an idle master before the slave comes back.", "30"),
'SlavedUpdateModuleData.GuardWanderRange': ("Wander distance while guarding the master.", "20"),
'SlavedUpdateModuleData.AttackRange': ("How far the slave may go toward the master's target.", "100"),
'SlavedUpdateModuleData.AttackWanderRange': ("Wander distance around the target.", "20"),
'SlavedUpdateModuleData.ScoutRange': ("How far ahead the slave scouts while the master moves.", "100"),
'SlavedUpdateModuleData.ScoutWanderRange': ("Wander distance at the scout point.", "10"),
'SlavedUpdateModuleData.RepairRange': ("Distance from the master at which repairs are done.", "20"),
'SlavedUpdateModuleData.RepairMinAltitude': ("Minimum hover height while repairing.", "20"),
'SlavedUpdateModuleData.RepairMaxAltitude': ("Maximum hover height while repairing.", "25"),
'SlavedUpdateModuleData.DistToTargetToGrantRangeBonus': ("Being this close to the master's target grants the master a range bonus.", "0"),
'SlavedUpdateModuleData.RepairRatePerSecond': ("Health repaired per second.", "5"),
'SlavedUpdateModuleData.RepairWhenBelowHealth%': ("Start repairing when the master's health is below this percent (plain number).", "90"),
'SlavedUpdateModuleData.RepairMinReadyTime': ("Min time (ms) before each repair burst.", "1000"),
'SlavedUpdateModuleData.RepairMaxReadyTime': ("Max time (ms) before each repair burst.", "2000"),
'SlavedUpdateModuleData.RepairMinWeldTime': ("Min duration (ms) of a weld burst.", "500"),
'SlavedUpdateModuleData.RepairMaxWeldTime': ("Max duration (ms) of a weld burst.", "1500"),
'SlavedUpdateModuleData.RepairWeldingSys': ("Particle system used while welding.", "BattleDroneWelding"),
'SlavedUpdateModuleData.RepairWeldingFXBone': ("Bone the welding particles come from.", "WeldPoint"),
'SlavedUpdateModuleData.StayOnSameLayerAsMaster': ("If Yes, the slave stays on the same layer (ground/bridge) as the master.", "No"),
})

M['MobMemberSlavedUpdate'] = dict(
 sum="Keeps Angry Mob members together around their mob nexus, with catch-up and 'squirrelly' wandering.",
 body="""For the individual members of a mob. If a member is further than <b>MustCatchUpRadius</b> from the nexus it runs to catch up; within <b>NoNeedToCatchUpRadius</b> it relaxes. If stuck outside for <b>CatchUpCrisisBailTime</b> frames it teleports back. <b>Squirrelliness</b> adds random jittery movement.""",
 ex="""Behavior = MobMemberSlavedUpdate ModuleTag_MobMember
  MustCatchUpRadius     = 50
  NoNeedToCatchUpRadius = 20
  Squirrelliness        = 0.3
  CatchUpCrisisBailTime = 999999
End""")
F.update({
'MobMemberSlavedUpdateModuleData.MustCatchUpRadius': ("Beyond this distance from the nexus the member hurries back.", "50"),
'MobMemberSlavedUpdateModuleData.CatchUpCrisisBailTime': ("Consecutive frames outside the catch-up radius before teleporting to the nexus.", "999999"),
'MobMemberSlavedUpdateModuleData.NoNeedToCatchUpRadius': ("Within this distance no catching up is needed.", "20"),
'MobMemberSlavedUpdateModuleData.Squirrelliness': ("Amount of random jittery movement (0..1).", "0.3"),
})

M['OCLUpdate'] = dict(
 sum="Creates an ObjectCreationList repeatedly on a random timer (e.g. supply drops, periodic spawns).",
 body="""Every random delay between <b>MinDelay</b> and <b>MaxDelay</b> it creates <b>OCL</b> on itself, or at the map edge with <b>CreateAtEdge</b> (e.g. the USA Supply Drop Zone plane). <b>FactionOCL</b> picks a different OCL based on the owning player's faction (<code>Faction:America OCL:OCL_X</code>); with <b>FactionTriggered</b> the update only runs once a faction is set. The building shows a progress bar for the timer.""",
 ex="""Behavior = OCLUpdate ModuleTag_SupplyDrop
  OCL          = OCL_AmericaSupplyDropZoneCargoPlane
  MinDelay     = 120000
  MaxDelay     = 120000
  CreateAtEdge = Yes
End""")
F.update({
'OCLUpdateModuleData.OCL': ("ObjectCreationList created each time the timer fires.", "OCL_AmericaSupplyDropZoneCargoPlane"),
'OCLUpdateModuleData.FactionOCL': ("Faction-specific OCL: Faction:<Side> OCL:<OCL>. Repeatable; overrides OCL for that faction.", "Faction:America OCL:OCL_AmericaDrop"),
'OCLUpdateModuleData.MinDelay': ("Minimum time (ms) between creations.", "120000"),
'OCLUpdateModuleData.MaxDelay': ("Maximum time (ms) between creations.", "120000"),
'OCLUpdateModuleData.CreateAtEdge': ("If Yes, the OCL is created at the map edge instead of on the object.", "Yes"),
'OCLUpdateModuleData.FactionTriggered': ("If Yes, nothing happens until the object has a faction owner.", "No"),
})

M['SpecialAbilityUpdate'] = dict(
 sum="Does the work of unit special abilities (Black Lotus capture/hack, Colonel Burton timed charges, Jarmen Kell snipe, Hijack...).",
 body="""Paired with a SpecialAbility (button) module sharing the same <b>SpecialPowerTemplate</b>. When triggered the unit moves within <b>StartAbilityRange</b> of the target (aborting beyond <b>AbilityAbortRange</b>), optionally needing line of sight/facing, unpacks (<b>UnpackTime</b>), prepares for <b>PreparationTime</b> (e.g. capture/hack time) then triggers the effect, and packs up (<b>PackTime</b>). Depending on the power type it can place <b>SpecialObject</b>s (charges, up to <b>MaxSpecialObjects</b>, optionally persistent), disable the target for <b>EffectDuration</b> (with <b>DisableFXParticleSystem</b>), capture buildings (<b>DoCaptureFX</b>), steal cash (<b>EffectValue</b>) etc. Can lose stealth on trigger, award XP/skill points, and flee afterward (<b>FleeRangeAfterCompletion</b>).""",
 ex="""Behavior = SpecialAbilityUpdate ModuleTag_Ability
  SpecialPowerTemplate = SpecialAbilityBlackLotusCaptureBuilding
  StartAbilityRange    = 15
  UnpackTime           = 500
  PreparationTime      = 20000
  PackTime             = 500
  DoCaptureFX          = Yes
  LoseStealthOnTrigger = Yes
  UnpackSound          = BlackLotusHackStart
  PrepSoundLoop        = BlackLotusHackLoop
  TriggerSound         = BlackLotusHackComplete
  AwardXPForTriggering = 25
End""")
F.update({
'SpecialAbilityUpdateModuleData.SpecialPowerTemplate': ("Special power (must match the companion SpecialAbility module).", "SpecialAbilityBlackLotusCaptureBuilding"),
'SpecialAbilityUpdateModuleData.StartAbilityRange': ("Distance to the target at which the ability starts.", "15"),
'SpecialAbilityUpdateModuleData.AbilityAbortRange': ("If the target gets further than this, the ability aborts.", "999999"),
'SpecialAbilityUpdateModuleData.PreparationTime': ("Time (ms) spent preparing (e.g. hacking, planting) before the effect triggers.", "20000"),
'SpecialAbilityUpdateModuleData.PersistentPrepTime': ("For persistent abilities: interval (ms) between repeated triggers.", "0"),
'SpecialAbilityUpdateModuleData.PackTime': ("Time (ms) to pack up after the ability.", "500"),
'SpecialAbilityUpdateModuleData.UnpackTime': ("Time (ms) to unpack before preparing.", "500"),
'SpecialAbilityUpdateModuleData.PreTriggerUnstealthTime': ("Time (ms) before triggering at which the unit loses stealth.", "0"),
'SpecialAbilityUpdateModuleData.SkipPackingWithNoTarget': ("If Yes, don't play the pack animation if there was no target.", "No"),
'SpecialAbilityUpdateModuleData.PackUnpackVariationFactor': ("Random variation of pack/unpack times.", "0"),
'SpecialAbilityUpdateModuleData.SpecialObject': ("Object template placed by the ability (e.g. timed charge).", "TimedChargeObject"),
'SpecialAbilityUpdateModuleData.SpecialObjectAttachToBone': ("Bone on the target to attach the special object to.", ""),
'SpecialAbilityUpdateModuleData.MaxSpecialObjects': ("Maximum special objects alive at once.", "1"),
'SpecialAbilityUpdateModuleData.SpecialObjectsPersistent': ("If Yes, special objects remain after the ability ends.", "No"),
'SpecialAbilityUpdateModuleData.EffectDuration': ("Duration (ms) of the effect on the target (e.g. disable time).", "0"),
'SpecialAbilityUpdateModuleData.EffectValue': ("Generic numeric effect value (e.g. cash stolen).", "1"),
'SpecialAbilityUpdateModuleData.UniqueSpecialObjectTargets': ("If Yes, only one special object per target.", "No"),
'SpecialAbilityUpdateModuleData.SpecialObjectsPersistWhenOwnerDies': ("If Yes, special objects survive the owner's death.", "No"),
'SpecialAbilityUpdateModuleData.AlwaysValidateSpecialObjects': ("Re-validate special objects every frame.", "No"),
'SpecialAbilityUpdateModuleData.FlipOwnerAfterPacking': ("If Yes, the unit changes owner after packing (used for defection-style abilities).", "No"),
'SpecialAbilityUpdateModuleData.FlipOwnerAfterUnpacking': ("If Yes, the unit changes owner after unpacking.", "No"),
'SpecialAbilityUpdateModuleData.FleeRangeAfterCompletion': ("Distance the unit runs away after completing the ability.", "0"),
'SpecialAbilityUpdateModuleData.DisableFXParticleSystem': ("Particle system on the target while disabled.", "DisabledEffect"),
'SpecialAbilityUpdateModuleData.DoCaptureFX': ("If Yes, flash house colours on the target while capturing.", "Yes"),
'SpecialAbilityUpdateModuleData.PackSound': ("Sound when packing.", ""),
'SpecialAbilityUpdateModuleData.UnpackSound': ("Sound when unpacking.", "BlackLotusHackStart"),
'SpecialAbilityUpdateModuleData.PrepSoundLoop': ("Looping sound during preparation.", "BlackLotusHackLoop"),
'SpecialAbilityUpdateModuleData.TriggerSound': ("Sound when the ability triggers.", "BlackLotusHackComplete"),
'SpecialAbilityUpdateModuleData.LoseStealthOnTrigger': ("If Yes, the unit is revealed when the ability triggers.", "Yes"),
'SpecialAbilityUpdateModuleData.AwardXPForTriggering': ("Experience awarded when the ability triggers.", "25"),
'SpecialAbilityUpdateModuleData.SkillPointsForTriggering': ("General's skill points awarded (-1 = none).", "-1"),
'SpecialAbilityUpdateModuleData.ApproachRequiresLOS': ("If Yes, the unit needs line of sight to the target to start.", "Yes"),
'SpecialAbilityUpdateModuleData.NeedToFaceTarget': ("If Yes, the unit turns to face the target first.", "Yes"),
'SpecialAbilityUpdateModuleData.PersistenceRequiresRecharge': ("If Yes, persistent abilities need a recharge between triggers.", "No"),
})

M['SwitchStateV2'] = dict(
 sum="Toggles a unit in place between two full configurations (model condition, weapon set, armor set, status, command set, locomotor), optionally timed.",
 body="""Mod-original. Holds two configurations, <b>DefaultState</b> and <b>AlteredState</b>; each is <code>&lt;ModelCondition&gt; &lt;WeaponSetFlag&gt; &lt;ArmorSetFlag&gt; &lt;ObjectStatus&gt; &lt;CommandSet&gt; &lt;LocomotorSet&gt;</code> — every value must be given (repeat the same value in both states for anything that should not change). Pressing the button toggles between them, with no container or second unit involved. <b>Lifetime</b> &gt; 0 makes AlteredState auto-revert after that time. On switching to Altered it fires <b>WeaponIn</b>/<b>OCLIn</b>/<b>FXListIn</b> at itself; switching back fires the *Out ones.
<br><br><b>Simple mode:</b> if <b>ConditionStateType</b> is set, Default/AlteredState are ignored entirely and toggling only sets/clears that one model condition flag (e.g. USER_1).
<br><br><b>Pairing:</b> the button side is the companion module <i>SwitchStateV2Activate</i>, which must use the same <b>SpecialPowerTemplate</b>. A <i>SwitchStateWhenDamagedBehaviorV2</i> with the same template can trigger it automatically from damage (always drives toward Altered; with Lifetime it keeps re-arming while damage continues).""",
 ex="""Behavior = SwitchStateV2Activate ModuleTag_SwitchButton
  SpecialPowerTemplate = SpecialPower_SiegeMode
End
Behavior = SwitchStateV2 ModuleTag_Switch
  SpecialPowerTemplate = SpecialPower_SiegeMode
  ;               ModelCond   WeaponSet   ArmorSet   Status        CommandSet             Locomotor
  DefaultState  = NONE        NONE        NONE       NONE          MyTank_NormalCmdSet    SET_NORMAL
  AlteredState  = DEPLOYED    PLAYER_UPGRADE CRATE_UPGRADE_ONE DEPLOYED MyTank_SiegeCmdSet SET_SLUGGISH
  Lifetime      = 0          ; permanent until toggled back
  FXListIn      = FX_SiegeDeploy
  FXListOut     = FX_SiegeUndeploy
End""")
F.update({
'SwitchStateV2ModuleData.SpecialPowerTemplate': ("Special power shared with the companion SwitchStateV2Activate (and optionally SwitchStateWhenDamagedBehaviorV2).", "SpecialPower_SiegeMode"),
'SwitchStateV2ModuleData.DefaultState': ("Normal configuration: <ModelCondition> <WeaponSetFlag> <ArmorSetFlag> <ObjectStatus> <CommandSet> <LocomotorSet>. All six values required.", "NONE NONE NONE NONE MyTank_NormalCmdSet SET_NORMAL"),
'SwitchStateV2ModuleData.AlteredState': ("Switched configuration, same six-value format as DefaultState.", "DEPLOYED PLAYER_UPGRADE CRATE_UPGRADE_ONE DEPLOYED MyTank_SiegeCmdSet SET_SLUGGISH"),
'SwitchStateV2ModuleData.ConditionStateType': ("Optional simple mode: if set, Default/AlteredState are ignored and toggling only sets/clears this model condition flag.", "USER_1"),
'SwitchStateV2ModuleData.Lifetime': ("0 = permanent until toggled back; otherwise time (ms) AlteredState lasts before auto-reverting.", "0"),
'SwitchStateV2ModuleData.WeaponIn': ("Weapon fired once at self when switching to AlteredState.", "SiegeDeployShockwave"),
'SwitchStateV2ModuleData.WeaponOut': ("Weapon fired once at self when switching back to DefaultState.", ""),
'SwitchStateV2ModuleData.OCLIn': ("OCL created at self when switching to AlteredState.", ""),
'SwitchStateV2ModuleData.OCLOut': ("OCL created at self when switching back.", ""),
'SwitchStateV2ModuleData.FXListIn': ("FXList played when switching to AlteredState.", "FX_SiegeDeploy"),
'SwitchStateV2ModuleData.FXListOut': ("FXList played when switching back.", "FX_SiegeUndeploy"),
})

M['ShieldGeneratorUpdateV2'] = dict(
 sum="Temporary energy shield: grants an absorption pool that refunds incoming damage (optionally only certain damage types) for a duration.",
 body="""Mod-original. Activated by the companion <i>ShieldGeneratorActivateV2</i> button (same <b>SpecialPowerTemplate</b>). On activation it creates a private absorption pool of <b>ShieldAmount</b> + <b>ShieldAmount%</b> × current max health, sets the optional <b>ConditionStateType</b> model condition (shield glow) and fires <b>WeaponIn</b>/<b>OCLIn</b>/<b>FXListIn</b>. Every hit whose type matches <b>DamageTypes</b> is immediately refunded from the pool (bypassing armor), so only the overflow is real damage; non-matching types pass straight through. The unit's real max health is never changed, so health bar, damage states and movement penalties only reflect real damage.
<br><br>The shield ends after <b>Lifetime</b> (0 = never on its own) or, with <b>RevertEarlyWhenDepleted</b> (default Yes), when the pool is used up — firing <b>WeaponOut</b>/<b>OCLOut</b>/<b>FXListOut</b> and clearing the condition. <b>RunOutLogicOnlyWhenDepleted</b> restricts the Out effects to the 'shield broke' case. Pressing the button again while shielded recasts (fresh pool), it never stacks. Pair with Body = ShieldedBody for extra damage-state safety.""",
 ex="""Behavior = ShieldGeneratorActivateV2 ModuleTag_ShieldButton
  SpecialPowerTemplate = SpecialPower_Shield
End
Behavior = ShieldGeneratorUpdateV2 ModuleTag_Shield
  SpecialPowerTemplate    = SpecialPower_Shield
  Lifetime                = 10000
  ShieldAmount            = 500
  ShieldAmount%           = 25%
  DamageTypes             = NONE +ARMOR_PIERCING +EXPLOSION
  ConditionStateType      = USER_1
  RevertEarlyWhenDepleted = Yes
  RunOutLogicOnlyWhenDepleted = Yes
  FXListIn                = FX_ShieldUp
  FXListOut               = FX_ShieldShatter
End""")
F.update({
'ShieldGeneratorUpdateV2ModuleData.SpecialPowerTemplate': ("Special power shared with the companion ShieldGeneratorActivateV2 button.", "SpecialPower_Shield"),
'ShieldGeneratorUpdateV2ModuleData.Lifetime': ("Time (ms) the shield lasts. 0 = never expires on its own (can still end on depletion).", "10000"),
'ShieldGeneratorUpdateV2ModuleData.WeaponIn': ("Weapon fired once at self on activation.", ""),
'ShieldGeneratorUpdateV2ModuleData.WeaponOut': ("Weapon fired once at self on reversion.", "ShieldShatterPushWeapon"),
'ShieldGeneratorUpdateV2ModuleData.OCLIn': ("OCL created at self on activation.", ""),
'ShieldGeneratorUpdateV2ModuleData.OCLOut': ("OCL created at self on reversion.", ""),
'ShieldGeneratorUpdateV2ModuleData.FXListIn': ("FXList played on activation.", "FX_ShieldUp"),
'ShieldGeneratorUpdateV2ModuleData.FXListOut': ("FXList played on reversion.", "FX_ShieldShatter"),
'ShieldGeneratorUpdateV2ModuleData.ShieldAmount': ("Flat size of the absorption pool.", "500"),
'ShieldGeneratorUpdateV2ModuleData.ShieldAmount%': ("Percent of max health at activation added to the pool (additive with ShieldAmount).", "25%"),
'ShieldGeneratorUpdateV2ModuleData.DamageTypes': ("Damage types the pool absorbs (ALL / NONE / +TYPE / -TYPE). Others pass through to real health.", "NONE +ARMOR_PIERCING +EXPLOSION"),
'ShieldGeneratorUpdateV2ModuleData.ConditionStateType': ("Model condition flag set while shielded (e.g. USER_1 for a glow).", "USER_1"),
'ShieldGeneratorUpdateV2ModuleData.RevertEarlyWhenDepleted': ("Yes (default): the shield ends as soon as the pool is used up. No: stays 'active' (cosmetically) until Lifetime.", "Yes"),
'ShieldGeneratorUpdateV2ModuleData.RunOutLogicOnlyWhenDepleted': ("Yes: Out weapon/OCL/FX only fire when the pool was depleted, not on timeout or recast. Ignored if RevertEarlyWhenDepleted = No.", "Yes"),
})

M['DamageOverTimeUpdateV2'] = dict(
 sum="Generic damage-over-time effect triggered by dedicated OVERTIME1..8 damage channels; tick damage respects armor and can be multi-typed.",
 body="""Mod-original, modelled on PoisonedBehavior. Put it on the TARGET (often on DefaultThingTemplate via InheritableModule so every unit can suffer it). When hit by a damage type in <b>TriggerDamageTypes</b> (default NONE +OVERTIME1), it starts dealing <b>Amount</b> + <b>Amount%</b> of the triggering hit's actual damage every <b>Rate</b> for <b>Lifetime</b>, once per type listed in <b>DealDamageTypes</b> (each type gets the full amount and goes through the target's armor for that type). Each tick can also fire <b>Weapon</b>/<b>OCL</b>/<b>FXList</b> at the target. <b>FireInitially</b> adds an immediate tick on every qualifying hit.
<br><br>Re-hits refresh (not stack): the amount is recaptured from the newest hit and Lifetime restarts, while the tick cadence is preserved. Healing cancels the effect. Up to 8 instances with different OVERTIME channels (and ModuleTags) can coexist on one object; <b>RequiredKindOf</b>/<b>ForbiddenKindOf</b> restrict which targets are affected (e.g. 3 KindOf variants × 8 channels). Untriggered instances cost no per-frame update. Give the attacker's weapon <code>DamageType = OVERTIME1</code> (or whichever channel).""",
 ex="""; On DefaultThingTemplate (so every object can burn):
InheritableModule
  Behavior = DamageOverTimeUpdateV2 ModuleTag_DoT_Burn_Vehicle
    TriggerDamageTypes = NONE +OVERTIME1
    DealDamageTypes    = NONE +FLAME
    RequiredKindOf     = VEHICLE
    Lifetime           = 8000
    Rate               = 1000
    Amount             = 5
    Amount%            = 10%
    FireInitially      = No
    FXList             = FX_BurningTick
  End
End

; Attacker's weapon:
Weapon NapalmStrikeWeapon
  DamageType = OVERTIME1
  ...
End""")
F.update({
'DamageOverTimeUpdateV2ModuleData.Lifetime': ("Total duration (ms) of the effect after being triggered/refreshed.", "8000"),
'DamageOverTimeUpdateV2ModuleData.Rate': ("Interval (ms) between ticks.", "1000"),
'DamageOverTimeUpdateV2ModuleData.Amount': ("Flat damage per tick (per DealDamageTypes entry).", "5"),
'DamageOverTimeUpdateV2ModuleData.Amount%': ("Plus this percent of the triggering hit's actual dealt damage, captured at trigger/refresh time.", "10%"),
'DamageOverTimeUpdateV2ModuleData.FireInitially': ("If Yes, every qualifying hit also deals one tick immediately instead of waiting Rate for the first one.", "No"),
'DamageOverTimeUpdateV2ModuleData.Weapon': ("Weapon fired at the target's own position on every tick.", ""),
'DamageOverTimeUpdateV2ModuleData.OCL': ("OCL created at the target on every tick.", ""),
'DamageOverTimeUpdateV2ModuleData.FXList': ("FXList played at the target on every tick.", "FX_BurningTick"),
'DamageOverTimeUpdateV2ModuleData.TriggerDamageTypes': ("Incoming damage types that start/refresh the effect. Default NONE +OVERTIME1. Use one channel per instance to keep instances independent.", "NONE +OVERTIME1"),
'DamageOverTimeUpdateV2ModuleData.DealDamageTypes': ("Damage type(s) the tick damage is dealt as; each listed type deals the FULL amount separately and is mitigated by that type's armor. Default NONE +OVERTIME1.", "NONE +FLAME"),
'DamageOverTimeUpdateV2ModuleData.RequiredKindOf': ("If set, the target must match at least one of these KindOfs to be affected.", "VEHICLE"),
'DamageOverTimeUpdateV2ModuleData.ForbiddenKindOf': ("If the target matches any of these KindOfs, this instance never triggers on it.", "INFANTRY"),
})
