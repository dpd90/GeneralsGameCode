M = {}
F = {}

M['MissileLauncherBuildingUpdate'] = dict(
 sum="Superweapon silo door logic (Nuke silo, SCUD Storm): opens doors when the power is ready, fires, closes.",
 body="""Tied to <b>SpecialPowerTemplate</b>. As the power becomes ready it opens its doors over <b>DoorOpenTime</b> (DOOR_1_OPENING, then DOOR_1_OPEN states with <b>DoorOpeningFX</b>/<b>DoorOpenFX</b>/<b>DoorOpenIdleAudio</b>), stays open until the player fires, waits <b>DoorWaitOpenTime</b>, then closes over <b>DoorCloseTime</b> with their respective FX.""",
 ex="""Behavior = MissileLauncherBuildingUpdate ModuleTag_Silo
  SpecialPowerTemplate = SuperweaponNeutronMissile
  DoorOpenTime      = 6000
  DoorWaitOpenTime  = 5000
  DoorCloseTime     = 6000
  DoorOpeningFX     = FX_NukeSiloOpening
  DoorOpenFX        = FX_NukeSiloOpen
  DoorClosingFX     = FX_NukeSiloClosing
  DoorOpenIdleAudio = NukeSiloOpenLoop
End""")
F.update({
'MissileLauncherBuildingUpdateModuleData.SpecialPowerTemplate': ("Superweapon special power controlling the doors.", "SuperweaponNeutronMissile"),
'MissileLauncherBuildingUpdateModuleData.DoorOpenTime': ("Time (ms) the door opening animation takes.", "6000"),
'MissileLauncherBuildingUpdateModuleData.DoorWaitOpenTime': ("Time (ms) the door stays open after firing.", "5000"),
'MissileLauncherBuildingUpdateModuleData.DoorCloseTime': ("Time (ms) the door closing animation takes.", "6000"),
'MissileLauncherBuildingUpdateModuleData.DoorOpeningFX': ("FXList when the doors start opening.", "FX_NukeSiloOpening"),
'MissileLauncherBuildingUpdateModuleData.DoorOpenFX': ("FXList when the doors are fully open.", "FX_NukeSiloOpen"),
'MissileLauncherBuildingUpdateModuleData.DoorWaitingToCloseFX': ("FXList while waiting to close.", ""),
'MissileLauncherBuildingUpdateModuleData.DoorClosingFX': ("FXList when the doors start closing.", "FX_NukeSiloClosing"),
'MissileLauncherBuildingUpdateModuleData.DoorClosedFX': ("FXList when the doors are closed.", ""),
'MissileLauncherBuildingUpdateModuleData.DoorOpenIdleAudio': ("Looping audio while the doors are open.", "NukeSiloOpenLoop"),
})

M['SupplyCenterProductionExitUpdate'] = dict(
 sum="Production exit of Supply Centers: new supply trucks/workers are put straight to work gathering.",
 body="""Like DefaultProductionExitUpdate (<b>UnitCreatePoint</b>, <b>NaturalRallyPoint</b>) but every produced gatherer is immediately ordered to go gather supplies. <b>GrantTemporaryStealth</b> gives the new unit stealth for that long (GLA).""",
 ex="""Behavior = SupplyCenterProductionExitUpdate ModuleTag_Exit
  UnitCreatePoint   = X:-30 Y:0 Z:0
  NaturalRallyPoint = X:-60 Y:0 Z:0
End""")
F.update({
'SupplyCenterProductionExitUpdateModuleData.UnitCreatePoint': ("Relative spawn position of produced gatherers.", "X:-30 Y:0 Z:0"),
'SupplyCenterProductionExitUpdateModuleData.NaturalRallyPoint': ("Relative point new units move to first.", "X:-60 Y:0 Z:0"),
'SupplyCenterProductionExitUpdateModuleData.GrantTemporaryStealth': ("Time (ms) of temporary stealth given to new units.", "0"),
})

M['SupplyCenterDockUpdate'] = dict(
 sum="Supply Center dock: gatherers unload boxes here and they become money.",
 body="""Dock module for supply centers. A docking gatherer's boxes are converted to money for the owner (value per box from GameData / upgrades). <b>GrantTemporaryStealth</b> gives the truck temporary stealth when it leaves (used by GLA with the Camo upgrade).""",
 ex="""Behavior = SupplyCenterDockUpdate ModuleTag_Dock
  NumberApproachPositions = -1
  AllowsPassthrough       = No
End""")
F.update({'SupplyCenterDockUpdateModuleData.GrantTemporaryStealth': ("Time (ms) of stealth given to a gatherer leaving the dock.", "0")})

M['SupplyWarehouseDockUpdate'] = dict(
 sum="Supply warehouse / supply pile dock: holds a number of boxes that gatherers take away.",
 body="""Holds <b>StartingBoxes</b> supply boxes. Each docking gatherer takes boxes until full or the pile is empty. The model shows depletion via condition states. With <b>DeleteWhenEmpty</b> the object disappears when emptied (small supply piles).""",
 ex="""Behavior = SupplyWarehouseDockUpdate ModuleTag_Dock
  NumberApproachPositions = -1
  AllowsPassthrough       = No
  StartingBoxes           = 400
  DeleteWhenEmpty         = No
End""")
F.update({
'SupplyWarehouseDockUpdateModuleData.StartingBoxes': ("Number of supply boxes initially available.", "400"),
'SupplyWarehouseDockUpdateModuleData.DeleteWhenEmpty': ("If Yes, the object is removed once all boxes are taken.", "No"),
})

M['DozerAIUpdate'] = dict(
 sum="Dozer AI: builds structures, repairs buildings/bridges, clears mines, and auto-repairs when bored.",
 body="""AIUpdate for construction units (KindOf DOZER). Handles building placement orders, repairing structures at <b>RepairHealthPercentPerSecond</b>, repairing bridges, disarming mines and fortifying civilian buildings. When idle for <b>BoredTime</b> it looks within <b>BoredRange</b> for damaged friendly structures and repairs them on its own. Build speed comes from the structure's BuildTime.""",
 ex="""Behavior = DozerAIUpdate ModuleTag_AI
  RepairHealthPercentPerSecond = 2%
  BoredTime   = 5000
  BoredRange  = 150
  AutoAcquireEnemiesWhenIdle = Yes
End""")
F.update({
'DozerAIUpdateModuleData.RepairHealthPercentPerSecond': ("Percent of a structure's max health repaired per second.", "2%"),
'DozerAIUpdateModuleData.BoredTime': ("Idle time (ms) after which the dozer looks for something to repair.", "5000"),
'DozerAIUpdateModuleData.BoredRange': ("Search radius for auto-repair when bored.", "150"),
})

M['HealAIUpdateV2'] = dict(
 sum="Auto-heal AI for any unit: when idle, finds a damaged ally (KindOf-filtered) and heals it with continuous weapon fire until full.",
 body="""Mod-original, a slimmed-down copy of DozerAIUpdate's 'bored' auto-scan keeping only the heal/repair part (no building/fortifying/mines). Does not require KindOf DOZER. After being idle for <b>BoredTime</b> it searches <b>BoredRange</b> for a damaged ally matching <b>KindOf</b> (and not <b>ForbiddenKindOf</b>) and attacks it with its healing weapon (a weapon whose DamageType is a healing type). The module stops the 'attack' itself as soon as the target is back to full health. Heal per shot comes from the weapon's damage; turret aiming is handled by the normal Turret sub-block.""",
 ex="""Behavior = HealAIUpdateV2 ModuleTag_AI
  AutoAcquireEnemiesWhenIdle = Yes
  BoredTime       = 2000
  BoredRange      = 200
  KindOf          = VEHICLE
  ForbiddenKindOf = AIRCRAFT
  Turret
    TurretTurnRate        = 120
    ControlledWeaponSlots = SECONDARY
  End
End""")
F.update({
'HealAIUpdateV2ModuleData.BoredTime': ("Continuous idle time (ms) before looking for something to heal.", "2000"),
'HealAIUpdateV2ModuleData.BoredRange': ("Search radius for damaged allies.", "200"),
'HealAIUpdateV2ModuleData.KindOf': ("Only allies matching one of these KindOfs are eligible heal targets.", "VEHICLE"),
'HealAIUpdateV2ModuleData.ForbiddenKindOf': ("Allies matching any of these KindOfs are never healed.", "AIRCRAFT"),
})

M['POWTruckAIUpdate'] = dict(
 sum="AI of the (cut) POW truck: collects surrendered enemies and brings them to a prison.",
 body="""In automatic mode, after <b>BoredTime</b> the truck seeks surrendered enemy infantry, loads them and returns to a prison (within <b>AtPrisonDistance</b> counts as arrived). Unused in the shipped game but functional.""",
 ex="""Behavior = POWTruckAIUpdate ModuleTag_AI
  BoredTime        = 5000
  AtPrisonDistance = 60
End""")
F.update({
'POWTruckAIUpdateModuleData.BoredTime': ("Idle time (ms) after which the truck looks for prisoners on its own.", "5000"),
'POWTruckAIUpdateModuleData.AtPrisonDistance': ("Distance at which the truck counts as being at the prison.", "60"),
})

M['RailedTransportAIUpdate'] = dict(
 sum="AI for transports that travel only along waypoint rails between stations.",
 body="""Moves the transport along map waypoint paths whose names start with <b>PathPrefixName</b> from station to station. Players order it to go to the next station; passengers load/unload at RailedTransportDockUpdate stations.""",
 ex="""Behavior = RailedTransportAIUpdate ModuleTag_AI
  PathPrefixName = TrainPath
End""")
F.update({'RailedTransportAIUpdateModuleData.PathPrefixName': ("Prefix of the waypoint path names the transport follows.", "TrainPath")})

M['ProductionUpdate'] = dict(
 sum="Allows a building (or unit) to produce units and research upgrades from a queue.",
 body="""Required on anything with a build queue. Holds up to <b>MaxQueueEntries</b> items, spends money over time, and hands finished units to the object's ProductionExit module. Door animations (<b>NumDoorAnimations</b>, <b>DoorOpeningTime</b>, <b>DoorWaitOpenTime</b>, <b>DoorCloseTime</b>) play when units leave; <b>ConstructionCompleteDuration</b> holds the CONSTRUCTION_COMPLETE state. <b>QuantityModifier</b> makes one purchase produce several units (e.g. <code>QuantityModifier = ChinaInfantryRedguard 2</code>). <b>DisabledTypesToProcess</b> lists disabled states during which production continues.""",
 ex="""Behavior = ProductionUpdate ModuleTag_Production
  MaxQueueEntries   = 9
  NumDoorAnimations = 1
  DoorOpeningTime   = 1500
  DoorWaitOpenTime  = 3000
  DoorCloseTime     = 1500
  ConstructionCompleteDuration = 1500
  QuantityModifier  = ChinaInfantryRedguard 2
End""")
F.update({
'ProductionUpdateModuleData.MaxQueueEntries': ("Maximum number of items in the production queue.", "9"),
'ProductionUpdateModuleData.NumDoorAnimations': ("Number of door animations (DOOR_1..DOOR_4 states) used when units exit.", "1"),
'ProductionUpdateModuleData.DoorOpeningTime': ("Time (ms) for a door to open.", "1500"),
'ProductionUpdateModuleData.DoorWaitOpenTime': ("Time (ms) the door stays open.", "3000"),
'ProductionUpdateModuleData.DoorCloseTime': ("Time (ms) for a door to close.", "1500"),
'ProductionUpdateModuleData.ConstructionCompleteDuration': ("Time (ms) the CONSTRUCTION_COMPLETE model state is held after producing something.", "1500"),
'ProductionUpdateModuleData.QuantityModifier': ("<ObjectTemplate> <count>: buying that unit produces count copies. Repeatable.", "ChinaInfantryRedguard 2"),
'ProductionUpdateModuleData.DisabledTypesToProcess': ("Disabled states during which production still progresses (default DISABLED_HELD).", "DISABLED_HELD"),
})

M['ProneUpdate'] = dict(
 sum="Makes infantry go prone (duck) when shot at, for a time proportional to the damage received.",
 body="""When the unit takes damage it enters the PRONE model condition for a number of frames equal to damage × <b>DamageToFramesRatio</b>, during which it can't move or fire.""",
 ex="""Behavior = ProneUpdate ModuleTag_Prone
  DamageToFramesRatio = 2.0
End""")
F.update({'ProneUpdateModuleData.DamageToFramesRatio': ("Frames spent prone per point of damage taken.", "2.0")})

M['StickyBombUpdate'] = dict(
 sum="Keeps a sticky bomb (timed/remote charge) attached to its target and handles its geometry-sized detonation.",
 body="""Used on bombs that stick to units (Colonel Burton's charges, TNT, Bomb Truck payload). Each frame the bomb is kept at the target's <b>AttachToTargetBone</b> (or <b>OffsetZ</b> above it). When it detonates, <b>GeometryBasedDamageWeapon</b> is fired with a radius based on the target's geometry, plus <b>GeometryBasedDamageFX</b>. Usually combined with LifetimeUpdate / FireWeaponWhenDeadBehavior.""",
 ex="""Behavior = StickyBombUpdate ModuleTag_Sticky
  AttachToTargetBone        = None
  OffsetZ                   = 10
  GeometryBasedDamageWeapon = TimedChargeBonusWeapon
  GeometryBasedDamageFX     = FX_TimedChargeExplosion
End""")
F.update({
'StickyBombUpdateModuleData.AttachToTargetBone': ("Bone on the target to attach to.", ""),
'StickyBombUpdateModuleData.OffsetZ': ("Height offset above the target if no bone.", "10"),
'StickyBombUpdateModuleData.GeometryBasedDamageWeapon': ("Weapon fired on detonation with radius scaled to the target's geometry.", "TimedChargeBonusWeapon"),
'StickyBombUpdateModuleData.GeometryBasedDamageFX': ("FXList played on detonation.", "FX_TimedChargeExplosion"),
})

M['FireOCLAfterWeaponCooldownUpdate'] = dict(
 sum="When the unit stops firing a weapon after a burst, creates an OCL whose lifetime scales with how long it fired (e.g. smoke after firing).",
 body="""Tracks firing of the weapon in <b>WeaponSlot</b>. Once it stops firing after at least <b>MinShotsToCreateOCL</b> shots, <b>OCL</b> is created, with lifetime <b>OCLLifetimePerSecond</b> per second of firing, capped at <b>OCLLifetimeMaxCap</b>. Upgrade-gated through TriggeredBy (e.g. China Gattling barrel smoke / a 'overheat' effect).""",
 ex="""Behavior = FireOCLAfterWeaponCooldownUpdate ModuleTag_Smoke
  TriggeredBy          = Upgrade_SomeUpgrade
  WeaponSlot           = PRIMARY
  OCL                  = OCL_BarrelSmoke
  MinShotsToCreateOCL  = 5
  OCLLifetimePerSecond = 1000
  OCLLifetimeMaxCap    = 8000
End""")
F.update({
'FireOCLAfterWeaponCooldownUpdateModuleData.WeaponSlot': ("Weapon slot being tracked.", "PRIMARY"),
'FireOCLAfterWeaponCooldownUpdateModuleData.OCL': ("OCL created when firing stops.", "OCL_BarrelSmoke"),
'FireOCLAfterWeaponCooldownUpdateModuleData.MinShotsToCreateOCL': ("Minimum shots in the burst needed to create the OCL.", "5"),
'FireOCLAfterWeaponCooldownUpdateModuleData.OCLLifetimePerSecond': ("Lifetime (ms) of the created objects per second of continuous firing.", "1000"),
'FireOCLAfterWeaponCooldownUpdateModuleData.OCLLifetimeMaxCap': ("Maximum lifetime (ms) of the created objects.", "8000"),
})

M['HijackerUpdate'] = dict(
 sum="Keeps the (hidden) Hijacker with the vehicle he stole; when it dies he pops out (by parachute if airborne).",
 body="""After a hijack the Hijacker is hidden and kept at the vehicle's position (<b>AttachToTargetBone</b>). When the vehicle is destroyed he reappears and becomes a normal unit again; if the vehicle was in the air he's put into <b>ParachuteName</b>.""",
 ex="""Behavior = HijackerUpdate ModuleTag_Hijacker
  AttachToTargetBone = None
  ParachuteName      = AmericaParachute
End""")
F.update({
'HijackerUpdateModuleData.AttachToTargetBone': ("Bone on the vehicle where the hijacker is kept.", ""),
'HijackerUpdateModuleData.ParachuteName': ("Parachute object used if the vehicle dies in the air.", "AmericaParachute"),
})

M['StructureToppleUpdate'] = dict(
 sum="Structure death that makes tall buildings topple over in a direction, crushing what they fall on.",
 body="""On death, after a random delay (<b>Min/MaxToppleDelay</b>) the structure starts to fall over (angle from the killing blow), with bursts of FX every <b>Min/MaxToppleBurstDelay</b>. <b>StructuralIntegrity</b>/<b>StructuralDecay</b> control how fast it accelerates. Everything it lands on is hit with <b>CrushingWeaponName</b> and <b>CrushingFX</b>. FX hooks: <b>ToppleDelayFX</b>, <b>ToppleStartFX</b>, <b>TopplingFX</b>, <b>ToppleDoneFX</b>; <b>AngleFX</b> plays FX at specific fall angles; <b>OCL</b> is phase-keyed (e.g. debris). <b>DamageFXTypes</b> limit which damage types allow the FX.""",
 ex="""Behavior = StructureToppleUpdate ModuleTag_Topple
  MinToppleDelay      = 500
  MaxToppleDelay      = 1000
  MinToppleBurstDelay = 200
  MaxToppleBurstDelay = 500
  StructuralIntegrity = 0.1
  StructuralDecay     = 0.9
  CrushingWeaponName  = TowerCrushWeapon
  ToppleStartFX       = FX_TowerToppleStart
  ToppleDoneFX        = FX_TowerToppleDone
  CrushingFX          = FX_TowerCrush
End""")
F.update({
'StructureToppleUpdateModuleData.MinToppleDelay': ("Minimum time (ms) after death before toppling starts.", "500"),
'StructureToppleUpdateModuleData.MaxToppleDelay': ("Maximum time (ms) before toppling starts.", "1000"),
'StructureToppleUpdateModuleData.MinToppleBurstDelay': ("Minimum time (ms) between FX bursts while toppling.", "200"),
'StructureToppleUpdateModuleData.MaxToppleBurstDelay': ("Maximum time (ms) between bursts.", "500"),
'StructureToppleUpdateModuleData.StructuralIntegrity': ("Initial resistance to falling (lower = falls faster).", "0.1"),
'StructureToppleUpdateModuleData.StructuralDecay': ("Per-frame decay of integrity (how quickly the fall accelerates).", "0.9"),
'StructureToppleUpdateModuleData.DamageFXTypes': ("Damage types whose deaths play the FX.", "ALL"),
'StructureToppleUpdateModuleData.TopplingFX': ("FXList while toppling.", "FX_TowerToppling"),
'StructureToppleUpdateModuleData.ToppleDelayFX': ("FXList during the pre-topple delay.", ""),
'StructureToppleUpdateModuleData.ToppleStartFX': ("FXList when toppling starts.", "FX_TowerToppleStart"),
'StructureToppleUpdateModuleData.ToppleDoneFX': ("FXList when it has hit the ground.", "FX_TowerToppleDone"),
'StructureToppleUpdateModuleData.CrushingFX': ("FXList where it crushes things.", "FX_TowerCrush"),
'StructureToppleUpdateModuleData.CrushingWeaponName': ("Weapon applied to everything the structure lands on.", "TowerCrushWeapon"),
'StructureToppleUpdateModuleData.OCL': ("Phase-keyed OCL (<phase> <OCL>). Repeatable.", "FINAL OCL_TowerRubble"),
'StructureToppleUpdateModuleData.AngleFX': ("FXList played when the fall reaches a given angle: <angle> <FXList>. Repeatable.", "45 FX_TowerMidFall"),
})

M['StructureCollapseUpdate'] = dict(
 sum="Structure death that makes buildings collapse straight down into the ground with shudders and debris bursts.",
 body="""On death, after a random <b>Min/MaxCollapseDelay</b> the building sinks into the ground (damped by <b>CollapseDamping</b>) while shuddering up to <b>MaxShudder</b>. Bursts of phase-keyed <b>FXList</b>/<b>OCL</b> (INITIAL, DELAY, BURST, FINAL phases) happen every <b>Min/MaxBurstDelay</b>, with a big burst every <b>BigBurstFrequency</b> bursts.""",
 ex="""Behavior = StructureCollapseUpdate ModuleTag_Collapse
  MinCollapseDelay  = 1000
  MaxCollapseDelay  = 1500
  CollapseDamping   = 0.5
  MaxShudder        = 1.0
  MinBurstDelay     = 250
  MaxBurstDelay     = 800
  BigBurstFrequency = 4
  FXList = INITIAL FX_StructureMediumDeath
  FXList = BURST   FX_BuildingCollapseBurst
  OCL    = FINAL   OCL_LargeStructureDebris
End""")
F.update({
'StructureCollapseUpdateModuleData.MinCollapseDelay': ("Minimum time (ms) after death before collapsing starts.", "1000"),
'StructureCollapseUpdateModuleData.MaxCollapseDelay': ("Maximum time (ms) before collapsing starts.", "1500"),
'StructureCollapseUpdateModuleData.MinBurstDelay': ("Minimum time (ms) between collapse bursts.", "250"),
'StructureCollapseUpdateModuleData.MaxBurstDelay': ("Maximum time (ms) between bursts.", "800"),
'StructureCollapseUpdateModuleData.CollapseDamping': ("Damping of the sinking speed.", "0.5"),
'StructureCollapseUpdateModuleData.MaxShudder': ("Maximum shake displacement while collapsing.", "1.0"),
'StructureCollapseUpdateModuleData.BigBurstFrequency': ("Every Nth burst is a big burst.", "4"),
'StructureCollapseUpdateModuleData.OCL': ("Phase-keyed OCL: <INITIAL|DELAY|BURST|FINAL> <OCL>. Repeatable.", "FINAL OCL_LargeStructureDebris"),
'StructureCollapseUpdateModuleData.FXList': ("Phase-keyed FXList: <INITIAL|DELAY|BURST|FINAL> <FXList>. Repeatable.", "INITIAL FX_StructureMediumDeath"),
})

M['BoneFXUpdate'] = dict(
 sum="Randomly plays FX, OCLs and particle systems at model bones, depending on the body damage state (e.g. fires and sparks on a damaged building).",
 body="""For each body damage state (Pristine, Damaged, ReallyDamaged, Rubble) up to 8 FXLists, 8 OCLs and 8 particle systems can be defined. Each entry specifies a bone, whether it's played once or repeatedly, and a random delay range: <code>DamagedFXList1 = bone:Fire01 OnlyOnce:No 1000 3000 FXList:FX_BuildingFire</code>. When the object is in that state, each entry fires at its bone at random intervals. <b>DamageFXTypes</b>/<b>DamageOCLTypes</b>/<b>DamageParticleTypes</b> restrict which damage types cause the effects.""",
 ex="""Behavior = BoneFXUpdate ModuleTag_BoneFX
  DamageFXTypes            = ALL
  DamagedFXList1           = bone:Fire01 OnlyOnce:No 1000 3000 FXList:FX_BuildingSmallFire
  ReallyDamagedParticleSystem1 = bone:Fire02 OnlyOnce:Yes 0 0 PSys:BuildingFireLarge
  RubbleOCL1               = bone:Center OnlyOnce:Yes 0 0 OCL:OCL_RubbleSmoke
End""")
F.update({
'BoneFXUpdateModuleData.DamageFXTypes': ("Damage types whose damage causes FXList entries to play.", "ALL"),
'BoneFXUpdateModuleData.DamageOCLTypes': ("Damage types whose damage causes OCL entries to play.", "ALL"),
'BoneFXUpdateModuleData.DamageParticleTypes': ("Damage types whose damage causes particle entries to play.", "ALL"),
})

M['RadarUpdate'] = dict(
 sum="Radar dish/tower extension animation; the radar becomes available after RadarExtendTime.",
 body="""When the radar is granted (by RadarUpgrade or construction), the object plays its radar extending animation for <b>RadarExtendTime</b> before the radar actually turns on for the player.""",
 ex="""Behavior = RadarUpdate ModuleTag_Radar
  RadarExtendTime = 4000
End""")
F.update({'RadarUpdateModuleData.RadarExtendTime': ("Time (ms) for the radar to extend before it works.", "4000")})

M['AnimationSteeringUpdate'] = dict(
 sum="Plays turning animations (TURN_LEFT/TURN_RIGHT model conditions) while the unit steers.",
 body="""Sets TURN_LEFT/TURN_RIGHT (and centering transitions) model conditions as the unit's heading changes, so vehicles like bikes or hovercraft can lean into turns. <b>MinTransitionTime</b> is the minimum time between state changes to avoid flicker.""",
 ex="""Behavior = AnimationSteeringUpdate ModuleTag_Steering
  MinTransitionTime = 300
End""")
F.update({'AnimationSteeringUpdateModuleData.MinTransitionTime': ("Minimum time (ms) between steering animation state changes.", "300")})

M['TransportAIUpdate'] = dict(
 sum="AI for transports: validates evacuate orders and moves to a legal spot before unloading.",
 body="""Standard AIUpdate for transports (APCs, helicopters). When told to evacuate it checks that unloading is legal here (e.g. not over water/cliff) and may move to a better place first. Uses only the AIUpdate fields.""",
 ex="""Behavior = TransportAIUpdate ModuleTag_AI
  AutoAcquireEnemiesWhenIdle = Yes
End""")

M['WanderAIUpdate'] = dict(
 sum="AI that gives the unit random move commands so it wanders around (civilians, animals).",
 body="""When idle, the unit periodically picks a random nearby destination and walks there. Used on ambient civilians/animals. Uses only the AIUpdate fields.""",
 ex="""Behavior = WanderAIUpdate ModuleTag_AI
End""")

M['WaveGuideUpdate'] = dict(
 sum="Dam-break flood wave: moves water surface, damages and topples objects, splashes on bridges.",
 body="""Used on map 'waveguide' objects activated when a dam dies (DamDie). After <b>WaveDelay</b> it sweeps across the map: it raises water along a wave front of <b>YSize</b> sampled every <b>LinearWaveSpacing</b> (curvature <b>WaveBendMagnitude</b>), pushes water at <b>WaterVelocity</b> leaving it at <b>PreferredHeight</b>, deals <b>DamageAmount</b> within <b>DamageRadius</b> of sample points and topples things with <b>ToppleForce</b>. Plays splash/looping sounds and a particle when hitting bridges.""",
 ex="""Behavior = WaveGuideUpdate ModuleTag_Wave
  WaveDelay          = 1000
  YSize              = 400
  LinearWaveSpacing  = 20
  WaveBendMagnitude  = 50
  WaterVelocity      = 30
  PreferredHeight    = 15
  ShorelineEffectDistance = 30
  DamageRadius       = 30
  DamageAmount       = 1000
  ToppleForce        = 10
  LoopingSound       = DamWaveLoop
End""")
F.update({
'WaveGuideUpdateModuleData.WaveDelay': ("Delay (ms) from being enabled to the wave starting.", "1000"),
'WaveGuideUpdateModuleData.YSize': ("Width of the wave object in Y.", "400"),
'WaveGuideUpdateModuleData.LinearWaveSpacing': ("Spacing of sample points along the wave front.", "20"),
'WaveGuideUpdateModuleData.WaveBendMagnitude': ("Wave curvature; larger = straighter.", "50"),
'WaveGuideUpdateModuleData.WaterVelocity': ("Force/speed applied to the water.", "30"),
'WaveGuideUpdateModuleData.PreferredHeight': ("Water height after the wave passes.", "15"),
'WaveGuideUpdateModuleData.ShorelineEffectDistance': ("Distance behind the wave where it 'hits' the shore.", "30"),
'WaveGuideUpdateModuleData.DamageRadius': ("Damage radius around each sample point.", "30"),
'WaveGuideUpdateModuleData.DamageAmount': ("Damage dealt to objects hit by the wave.", "1000"),
'WaveGuideUpdateModuleData.ToppleForce': ("Force used to topple trees/objects.", "10"),
'WaveGuideUpdateModuleData.RandomSplashSound': ("Occasional splash sound during the wave.", "WaveSplash"),
'WaveGuideUpdateModuleData.RandomSplashSoundFrequency': ("Chance threshold 1..100 for the random splash.", "80"),
'WaveGuideUpdateModuleData.BridgeParticle': ("Particle system when the wave hits a bridge.", "WaveBridgeSplash"),
'WaveGuideUpdateModuleData.BridgeParticleAngleFudge': ("Angle offset for the bridge particle system.", "0"),
'WaveGuideUpdateModuleData.LoopingSound': ("Looping sound once the wave is triggered.", "DamWaveLoop"),
})

M['WorkerAIUpdate'] = dict(
 sum="GLA Worker AI: a unit that is both a Dozer (build/repair) and a Supply Truck (gather).",
 body="""Combines DozerAIUpdate and SupplyTruckAIUpdate: the Worker builds and repairs (<b>RepairHealthPercentPerSecond</b>, <b>BoredTime</b>, <b>BoredRange</b>) and gathers supplies (<b>MaxBoxes</b>, <b>SupplyCenterActionDelay</b>, <b>SupplyWarehouseActionDelay</b>, <b>SupplyWarehouseScanDistance</b>, <b>UpgradedSupplyBoost</b>). Note the engine requires editing both Dozer and Worker data if extending them.""",
 ex="""Behavior = WorkerAIUpdate ModuleTag_AI
  RepairHealthPercentPerSecond = 2%
  BoredTime                  = 5000
  BoredRange                 = 150
  MaxBoxes                   = 1
  SupplyCenterActionDelay    = 1000
  SupplyWarehouseActionDelay = 1000
  SupplyWarehouseScanDistance= 300
  UpgradedSupplyBoost        = 0
  SuppliesDepletedVoice      = WorkerVoiceSuppliesDepleted
End""")
F.update({
'WorkerAIUpdateModuleData.MaxBoxes': ("Boxes carried per trip.", "1"),
'WorkerAIUpdateModuleData.RepairHealthPercentPerSecond': ("Percent of structure max health repaired per second.", "2%"),
'WorkerAIUpdateModuleData.BoredTime': ("Idle time (ms) before auto-repairing nearby structures.", "5000"),
'WorkerAIUpdateModuleData.BoredRange': ("Auto-repair search radius.", "150"),
'WorkerAIUpdateModuleData.SupplyCenterActionDelay': ("Time (ms) to unload at a supply center.", "1000"),
'WorkerAIUpdateModuleData.SupplyWarehouseActionDelay': ("Time (ms) per box at a warehouse.", "1000"),
'WorkerAIUpdateModuleData.SupplyWarehouseScanDistance': ("Search radius for another warehouse.", "300"),
'WorkerAIUpdateModuleData.SuppliesDepletedVoice': ("Voice when taking the last box.", "WorkerVoiceSuppliesDepleted"),
'WorkerAIUpdateModuleData.UpgradedSupplyBoost': ("Extra cash per load with the supply upgrade.", "0"),
})

M['PowerPlantUpdate'] = dict(
 sum="Power plant control-rod extension animation when upgraded (Advanced Control Rods).",
 body="""When the power plant is upgraded (PowerPlantUpgrade), the rods extend over <b>RodsExtendTime</b> (POWER_PLANT_UPGRADING then POWER_PLANT_UPGRADED model conditions) before the extra power is applied.""",
 ex="""Behavior = PowerPlantUpdate ModuleTag_PowerPlant
  RodsExtendTime = 2000
End""")
F.update({'PowerPlantUpdateModuleData.RodsExtendTime': ("Time (ms) for the rods to extend.", "2000")})

M['CheckpointUpdate'] = dict(
 sum="Checkpoint gate: opens when allies are near and no enemies are near.",
 body="""Every <b>ScanDelayTime</b> checks for nearby allies and enemies; opens the gate (DOOR_1_OPENING state and removes geometry) when an ally is near and no enemy is, closes otherwise.""",
 ex="""Behavior = CheckpointUpdate ModuleTag_Checkpoint
  ScanDelayTime = 1000
End""")
F.update({'CheckpointUpdateModuleData.ScanDelayTime': ("Interval (ms) between scans.", "1000")})

M['CostModifierUpgrade'] = dict(
 sum="Upgrade that changes the cost of all objects of certain KindOfs for the player (e.g. cheaper buildings).",
 body="""When triggered, every object of the player matching <b>EffectKindOf</b> costs <b>Percentage</b> more (negative = cheaper). Used by general's powers like 'Emergency Repair'-style discounts or GLA 'Cash Bounty'-like economics. Removed when the upgrade is removed.""",
 ex="""Behavior = CostModifierUpgrade ModuleTag_Cost
  TriggeredBy  = Upgrade_CheaperBuildings
  EffectKindOf = STRUCTURE
  Percentage   = -25%
End""")
F.update({
'CostModifierUpgradeModuleData.EffectKindOf': ("KindOfs whose cost is modified.", "STRUCTURE"),
'CostModifierUpgradeModuleData.Percentage': ("Cost change in percent (negative = cheaper).", "-25%"),
})

M['ActiveShroudUpgrade'] = dict(
 sum="Upgrade that sets the object's active shroud range (makes it cast shroud over enemies, e.g. GLA Radar Jammer).",
 body="""When triggered, the object's ShroudRange (the area in which it re-shrouds enemy vision) is set to <b>NewShroudRange</b>.""",
 ex="""Behavior = ActiveShroudUpgrade ModuleTag_Shroud
  TriggeredBy    = Upgrade_ShroudGenerator
  NewShroudRange = 200
End""")
F.update({'ActiveShroudUpgradeModuleData.NewShroudRange': ("New shroud-casting range.", "200")})

M['ArmorUpgrade'] = dict(
 sum="Upgrade that switches the object to its upgraded armor set (PLAYER_UPGRADE armor set flag).",
 body="""When triggered, sets the PLAYER_UPGRADE ArmorSet condition so the ArmorSet with <code>Conditions = PLAYER_UPGRADE</code> is used (e.g. Composite Armor, Chemical Suits — which also paints the chem-suit decal). No fields besides the upgrade triggers.""",
 ex="""Behavior = ArmorUpgrade ModuleTag_Armor
  TriggeredBy = Upgrade_AmericaCompositeArmor
End""")

M['CommandSetUpgrade'] = dict(
 sum="Upgrade that replaces the object's command set (new buttons after an upgrade).",
 body="""When triggered, the object uses <b>CommandSet</b> instead of its normal command set. If the player also has <b>TriggerAlt</b>, <b>CommandSetAlt</b> is used instead (lets two upgrades combine).""",
 ex="""Behavior = CommandSetUpgrade ModuleTag_Commands
  TriggeredBy   = Upgrade_UnlockAbility
  CommandSet    = MyUnitCommandSet_Upgraded
  CommandSetAlt = MyUnitCommandSet_UpgradedBoth
  TriggerAlt    = Upgrade_SecondAbility
End""")
F.update({
'CommandSetUpgradeModuleData.CommandSet': ("Command set to use once upgraded.", "MyUnitCommandSet_Upgraded"),
'CommandSetUpgradeModuleData.CommandSetAlt': ("Alternate command set used if TriggerAlt is also present.", "MyUnitCommandSet_UpgradedBoth"),
'CommandSetUpgradeModuleData.TriggerAlt': ("Upgrade that selects CommandSetAlt instead.", "Upgrade_SecondAbility"),
})

M['GrantScienceUpgrade'] = dict(
 sum="Upgrade that grants a science (general's power/rank unlock) to the player when triggered.",
 body="""When the upgrade triggers, the player receives <b>GrantScience</b> (from Science.ini) for free, unlocking whatever depends on it.""",
 ex="""Behavior = GrantScienceUpgrade ModuleTag_Science
  TriggeredBy  = Upgrade_ResearchSomething
  GrantScience = SCIENCE_SomePower
End""")
F.update({'GrantScienceUpgradeModuleData.GrantScience': ("Science granted to the player.", "SCIENCE_SomePower")})

M['PassengersFireUpgrade'] = dict(
 sum="Upgrade that allows passengers to fire out of this container (sets PassengersAllowedToFire).",
 body="""When triggered, the object's contain module gets PassengersAllowedToFire = Yes (e.g. a transport that gains firing ports after an upgrade).""",
 ex="""Behavior = PassengersFireUpgrade ModuleTag_FirePorts
  TriggeredBy = Upgrade_FiringPorts
End""")

M['StatusBitsUpgrade'] = dict(
 sum="Upgrade that sets and/or clears object status bits.",
 body="""When triggered, sets <b>StatusToSet</b> and clears <b>StatusToClear</b> on the object — a generic way to toggle behaviours that check status bits (e.g. CAN_STEALTH, stealth detection, REPULSOR…).""",
 ex="""Behavior = StatusBitsUpgrade ModuleTag_Status
  TriggeredBy   = Upgrade_SomeUpgrade
  StatusToSet   = CAN_STEALTH
  StatusToClear = NONE
End""")
F.update({
'StatusBitsUpgradeModuleData.StatusToSet': ("Object status bits set when upgraded.", "CAN_STEALTH"),
'StatusBitsUpgradeModuleData.StatusToClear': ("Object status bits cleared when upgraded.", ""),
})

M['SubObjectsUpgrade'] = dict(
 sum="Upgrade that shows and/or hides model subobjects (visual upgrade parts like armor plates).",
 body="""When triggered, subobjects listed in <b>ShowSubObjects</b> become visible and those in <b>HideSubObjects</b> are hidden, on every model condition state.""",
 ex="""Behavior = SubObjectsUpgrade ModuleTag_Visual
  TriggeredBy    = Upgrade_AmericaCompositeArmor
  ShowSubObjects = ArmorPlates01 ArmorPlates02
  HideSubObjects = OldHull
End""")
F.update({
'SubObjectsUpgradeModuleData.ShowSubObjects': ("Subobjects to show.", "ArmorPlates01 ArmorPlates02"),
'SubObjectsUpgradeModuleData.HideSubObjects': ("Subobjects to hide.", "OldHull"),
})

M['StealthUpgrade'] = dict(
 sum="Upgrade that gives the object the ability to stealth (sets CAN_STEALTH).",
 body="""When triggered, sets OBJECT_STATUS_CAN_STEALTH so the object's StealthUpdate starts working (e.g. GLA Camouflage).""",
 ex="""Behavior = StealthUpgrade ModuleTag_Camo
  TriggeredBy = Upgrade_GLACamouflage
End""")

M['RadarUpgrade'] = dict(
 sum="Upgrade that gives the player radar (optionally radar that can't be disabled).",
 body="""When triggered, the player gets a radar (a radar count is added while the object lives). <b>DisableProof</b> makes it a 'super radar' that keeps working even when radar is disabled (e.g. by low power).""",
 ex="""Behavior = RadarUpgrade ModuleTag_Radar
  TriggeredBy  = Upgrade_GLARadar
  DisableProof = No
End""")
F.update({'RadarUpgradeModuleData.DisableProof': ("If Yes, this radar ignores radar-disabling conditions.", "No")})

M['PowerPlantUpgrade'] = dict(
 sum="Upgrade that increases a power plant's output (uses the template's EnergyBonus).",
 body="""When triggered, the object's energy production is increased by its EnergyBonus (from the object template). Works with PowerPlantUpdate for the rods animation.""",
 ex="""Behavior = PowerPlantUpgrade ModuleTag_Rods
  TriggeredBy = Upgrade_AmericaAdvancedControlRods
End""")

M['LocomotorSetUpgrade'] = dict(
 sum="Upgrade that switches the unit to its upgraded locomotor set (SET_NORMAL_UPGRADED).",
 body="""When triggered, the unit uses its <code>Locomotor = SET_NORMAL_UPGRADED ...</code> locomotors instead of SET_NORMAL (e.g. faster after an engine upgrade).""",
 ex="""Behavior = LocomotorSetUpgrade ModuleTag_Engines
  TriggeredBy = Upgrade_ChinaNuclearTanks
End""")

M['ObjectCreationUpgrade'] = dict(
 sum="Upgrade that creates an ObjectCreationList when triggered (e.g. spawn a drone, add-on, or effect).",
 body="""When the upgrade completes, <b>UpgradeObject</b> (OCL) is created at the object — used for drones, Overlord add-ons, etc. Often paired with UpgradeDie on the created object so it can be rebought when destroyed.""",
 ex="""Behavior = ObjectCreationUpgrade ModuleTag_Drone
  TriggeredBy   = Upgrade_AmericaScoutDrone
  ConflictsWith = Upgrade_AmericaBattleDrone
  UpgradeObject = OCL_AmericanScoutDrone
End""")
F.update({'ObjectCreationUpgradeModuleData.UpgradeObject': ("OCL created when the upgrade triggers.", "OCL_AmericanScoutDrone")})

M['ReplaceObjectUpgrade'] = dict(
 sum="Upgrade that replaces this object with a different object template in the same place.",
 body="""When triggered, creates <b>ReplaceObject</b> at the exact position/orientation (keeping team and selection) and deletes the original. Used for structures that transform into another building after an upgrade.""",
 ex="""Behavior = ReplaceObjectUpgrade ModuleTag_Replace
  TriggeredBy   = Upgrade_Fortify
  ReplaceObject = MyFortifiedBunker
End""")
F.update({'ReplaceObjectUpgradeModuleData.ReplaceObject': ("Object template that replaces this one.", "MyFortifiedBunker")})

M['ModelConditionUpgrade'] = dict(
 sum="Upgrade that sets a model condition flag (to show upgraded visuals via condition states).",
 body="""When triggered, sets <b>ConditionFlag</b> on the object permanently so a ConditionState with that flag is used (e.g. UPGRADE_1 or USER_1).""",
 ex="""Behavior = ModelConditionUpgrade ModuleTag_Look
  TriggeredBy   = Upgrade_SomeUpgrade
  ConditionFlag = USER_1
End""")
F.update({'ModelConditionUpgradeModuleData.ConditionFlag': ("Model condition flag set when upgraded.", "USER_1")})

M['UnpauseSpecialPowerUpgrade'] = dict(
 sum="Upgrade that starts (unpauses) the recharge timer of a special power that StartsPaused.",
 body="""Pairs with a SpecialPowerModule that has <b>StartsPaused = Yes</b>: when this upgrade triggers, the timer of <b>SpecialPowerTemplate</b> starts ticking, so the power only becomes available after the upgrade (logic-side equivalent of NEED_UPGRADE on buttons).""",
 ex="""Behavior = UnpauseSpecialPowerUpgrade ModuleTag_Unpause
  TriggeredBy          = Upgrade_UnlockPower
  SpecialPowerTemplate = SpecialPower_MyPower
End""")
F.update({'UnpauseSpecialPowerUpgradeModuleData.SpecialPowerTemplate': ("Special power whose timer is unpaused.", "SpecialPower_MyPower")})

M['WeaponBonusUpgrade'] = dict(
 sum="Upgrade that grants the PLAYER_UPGRADE weapon bonus condition (e.g. +damage/+range from research).",
 body="""When triggered, sets the WEAPONBONUSCONDITION_PLAYER_UPGRADE on the object; the actual effect is defined by WeaponBonus = PLAYER_UPGRADE ... lines in GameData.ini (or per-weapon bonus sets).""",
 ex="""Behavior = WeaponBonusUpgrade ModuleTag_Bonus
  TriggeredBy = Upgrade_ChinaUraniumShells
End""")

M['WeaponSetUpgrade'] = dict(
 sum="Upgrade that switches the object to its upgraded weapon set (PLAYER_UPGRADE weapon set flag).",
 body="""When triggered, sets the PLAYER_UPGRADE weapon set flag so the WeaponSet with <code>Conditions = PLAYER_UPGRADE</code> is chosen (e.g. TOW missile on a Humvee).""",
 ex="""Behavior = WeaponSetUpgrade ModuleTag_Weapons
  TriggeredBy = Upgrade_AmericaTOWMissile
End""")
