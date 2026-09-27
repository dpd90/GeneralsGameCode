M = {}
F = {}

# ---------------- shared mix-in groups ----------------
F.update({
'UpgradeMuxData.TriggeredBy': ("One or more Upgrade names that activate this module. The module stays dormant until the object (or its player, for player upgrades) has the upgrade. With several names, any one of them is enough unless RequiresAllTriggers = Yes.", "Upgrade_AmericaAdvancedTraining"),
'UpgradeMuxData.ConflictsWith': ("Upgrade names that block this module. If any of these upgrades is present, this module will not activate even when TriggeredBy is satisfied. Used to make mutually-exclusive upgrade branches.", "Upgrade_SomeOtherBranch"),
'UpgradeMuxData.RemovesUpgrades': ("Upgrade names that are removed from the object at the moment this module activates. Handy for 'swap' upgrades where buying one should revoke another.", "Upgrade_OldVersion"),
'UpgradeMuxData.FXListUpgrade': ("FXList played on the object at the moment this module is activated by its upgrade.", "FX_UpgradeComplete"),
'UpgradeMuxData.RequiresAllTriggers': ("When Yes, every upgrade listed in TriggeredBy must be present before activation (AND logic). When No (default), any single one suffices (OR logic).", "No"),

'DieMuxData.DeathTypes': ("Which death types this die module reacts to. Uses the DeathType list syntax: ALL, NONE, +TYPE, -TYPE (e.g. 'ALL -CRUSHED -SPLATTED' or 'NONE +EXPLODED'). Default is ALL. Use it to split different death effects between several die modules on the same object.", "ALL"),
'DieMuxData.VeterancyLevels': ("Which veterancy levels of the dying object this module applies to (ALL, NONE, +REGULAR, +VETERAN, +ELITE, +HEROIC, -X). Default is ALL. Lets you give heroic units a different death effect.", "ALL"),
'DieMuxData.ExemptStatus': ("Object status bits that exempt the object from this die module: if ANY listed status is set at the moment of death, the module is skipped (e.g. UNDER_CONSTRUCTION so half-built structures don't trigger it).", "UNDER_CONSTRUCTION"),
'DieMuxData.RequiredStatus': ("Object status bits that are required: if ANY listed status is NOT set at the moment of death, the module is skipped.", ""),

'OpenContainModuleData.ContainMax': ("Maximum number of objects that can be inside. -1 (default) means 'no limit enforced by this field' (subclasses such as TransportContain use Slots instead).", "10"),
'OpenContainModuleData.EnterSound': ("Audio event played when an object enters the container.", "GarrisonEnter"),
'OpenContainModuleData.ExitSound': ("Audio event played when an object exits the container.", "GarrisonExit"),
'OpenContainModuleData.DamagePercentToUnits': ("When the container dies, this percentage of each passenger's max health is dealt to it as damage (100% kills everyone inside). 0 means passengers are ejected unharmed.", "100%"),
'OpenContainModuleData.BurnedDeathToUnits': ("If Yes (default), passengers killed by DamagePercentToUnits die with a BURNED death type; set No to use a normal death instead.", "Yes"),
'OpenContainModuleData.AllowInsideKindOf': ("KindOf filter: an object must have at least ONE of these KindOf bits to be allowed to enter. Empty = anything.", "INFANTRY"),
'OpenContainModuleData.ForbidInsideKindOf': ("KindOf filter: objects having ANY of these KindOf bits can never enter.", "AIRCRAFT"),
'OpenContainModuleData.PassengersAllowedToFire': ("If Yes, passengers can shoot out of the container from its FIREPOINT bones (like a Battle Bus or a garrisoned building).", "No"),
'OpenContainModuleData.PassengersInTurret': ("If Yes, the passenger fire-point bones are located on the turret rather than on the chassis, so they rotate with the turret.", "No"),
'OpenContainModuleData.NumberOfExitPaths': ("Number of ExitStart/ExitEnd bone pairs to alternate through when unloading (ExitStart01/ExitEnd01, ExitStart02/ExitEnd02...).", "1"),
'OpenContainModuleData.DoorOpenTime': ("Duration (ms) the DOOR_1_OPENING model condition is held while units exit, i.e. how long the unload door animation takes.", "1000"),
'OpenContainModuleData.WeaponBonusPassedToPassengers': ("If Yes, the container's current weapon bonus conditions (e.g. from Battle Plans, horde, veterancy bonuses) are also applied to passengers firing from inside.", "No"),
'OpenContainModuleData.AllowAlliesInside': ("Allow units of allied players to enter.", "Yes"),
'OpenContainModuleData.AllowEnemiesInside': ("Allow units of enemy players to enter (used by e.g. neutral civilian buildings that anyone can garrison).", "No"),
'OpenContainModuleData.AllowNeutralInside': ("Allow neutral units to enter.", "Yes"),

'TransportContainModuleData.Slots': ("Transport capacity in slots. Each passenger takes as many slots as its TransportSlotCount on its object definition.", "8"),
'TransportContainModuleData.ScatterNearbyOnExit': ("If Yes (default), unloaded passengers move a short distance away from the transport instead of standing on the exit point.", "Yes"),
'TransportContainModuleData.OrientLikeContainerOnExit': ("If Yes, exiting passengers are rotated to face the same direction as the transport.", "No"),
'TransportContainModuleData.KeepContainerVelocityOnExit': ("If Yes, passengers inherit the transport's current velocity when leaving (used for units dropped from moving vehicles/aircraft).", "No"),
'TransportContainModuleData.GoAggressiveOnExit': ("If Yes, passengers switch to aggressive AI mood when unloaded so they immediately engage enemies.", "No"),
'TransportContainModuleData.ResetMoodCheckTimeOnExit': ("If Yes (default), the passenger's mood/auto-acquire timer is reset on exit so it scans for targets right away.", "Yes"),
'TransportContainModuleData.DestroyRidersWhoAreNotFreeToExit': ("If Yes, passengers that cannot legally exit when the transport is destroyed (e.g. over water or in the air) are destroyed instead of being placed.", "No"),
'TransportContainModuleData.ExitBone': ("Name of the bone passengers are placed at when exiting (instead of the object's centre / ExitStart path).", "EXITBONE"),
'TransportContainModuleData.ExitPitchRate': ("Angular velocity (deg/sec) applied to the pitch of exiting passengers; used for paradrop-like ejection visuals.", "0"),
'TransportContainModuleData.InitialPayload': ("Objects created inside the transport when it is built. Syntax: <ObjectTemplate> <Count>. Can be repeated.", "AmericaInfantryRanger 2"),
'TransportContainModuleData.HealthRegen%PerSec': ("Percentage of max health (written as a plain number, e.g. 10 = 10%) healed per second for every passenger while inside.", "10"),
'TransportContainModuleData.ExitDelay': ("Duration (ms) between each passenger leaving when the transport unloads everyone.", "250"),
'TransportContainModuleData.ArmedRidersUpgradeMyWeaponSet': ("If Yes, the transport gets the PLAYER_UPGRADE weapon set flag while it carries at least one armed passenger (used by e.g. the Humvee/Battle Bus style 'passengers make me stronger').", "No"),
'TransportContainModuleData.DelayExitInAir': ("If Yes, passengers ordered to exit while the transport is airborne wait until it lands.", "No"),
})

# ---------------- modules ----------------
M['AutoHealBehavior'] = dict(
 sum="Periodically heals the object itself, or everything in a radius around it.",
 body="""AutoHealBehavior is a timed healing pulse. Every <b>HealingDelay</b> it restores <b>HealingAmount</b> health points. With <b>Radius</b> = 0 it heals only its own object (classic self-repair on vehicles and structures); with a non-zero Radius it becomes an area heal that affects every friendly object in range that passes the <b>KindOf</b>/<b>ForbiddenKindOf</b> filter (e.g. an ambulance or a repair drone aura).
<br><br>Because it is an upgrade module (it inherits UpgradeMux), it can be gated behind an upgrade with <b>TriggeredBy</b>, or made always-on with <b>StartsActive = Yes</b>. <b>StartHealingDelay</b> makes it wait until the object has not been damaged for a while (out-of-combat regeneration). <b>SingleBurst</b> heals only once when triggered instead of looping. <b>AffectsWholePlayer</b> heals every object the owner has, regardless of distance.""",
 ex="""Behavior = AutoHealBehavior ModuleTag_SelfRepair
  StartsActive      = No
  TriggeredBy       = Upgrade_AmericaAdvancedRepair
  HealingAmount     = 4
  HealingDelay      = 1000        ; one pulse per second
  StartHealingDelay = 5000        ; only after 5s without taking damage
End

; Area heal aura
Behavior = AutoHealBehavior ModuleTag_HealAura
  StartsActive   = Yes
  HealingAmount  = 10
  HealingDelay   = 1000
  Radius         = 150
  KindOf         = INFANTRY
  SkipSelfForHealing = Yes
  UnitHealPulseParticleSystemName = HealingPulse
End""")
F.update({
'AutoHealBehaviorModuleData.StartsActive': ("If Yes the healing is active from creation without needing any upgrade. If No, the module waits for TriggeredBy.", "Yes"),
'AutoHealBehaviorModuleData.SingleBurst': ("If Yes, heal only once (one pulse) when the module activates, instead of every HealingDelay.", "No"),
'AutoHealBehaviorModuleData.HealingAmount': ("Health points restored per pulse, to each affected object.", "5"),
'AutoHealBehaviorModuleData.HealingDelay': ("Time (ms) between healing pulses.", "1000"),
'AutoHealBehaviorModuleData.Radius': ("If 0, only the owner heals. If non-zero, every qualifying object within this radius is healed each pulse (area effect).", "0"),
'AutoHealBehaviorModuleData.KindOf': ("Only objects with at least one of these KindOf bits are healed by the area effect. Empty = everything.", "INFANTRY VEHICLE"),
'AutoHealBehaviorModuleData.ForbiddenKindOf': ("Objects with any of these KindOf bits are never healed by the area effect.", "STRUCTURE"),
'AutoHealBehaviorModuleData.RadiusParticleSystemName': ("Particle system attached to the healer for the whole time the area effect runs.", "HealAuraGlow"),
'AutoHealBehaviorModuleData.UnitHealPulseParticleSystemName': ("Particle system spawned on each object that receives a heal pulse.", "HealPulseSparkle"),
'AutoHealBehaviorModuleData.StartHealingDelay': ("Time (ms) that must pass since the object was last damaged before healing starts/resumes. 0 = heal even while under fire.", "3000"),
'AutoHealBehaviorModuleData.AffectsWholePlayer': ("If Yes, ignore Radius and heal every object owned by the same player (subject to the KindOf filters).", "No"),
'AutoHealBehaviorModuleData.SkipSelfForHealing': ("If Yes, the healer never heals itself, only others.", "No"),
})

M['GrantStealthBehavior'] = dict(
 sum="Expanding-radius sweep that grants permanent stealth to allied units, then deletes itself.",
 body="""Put this on a short-lived helper object (typically created by an OCL from a special power). Every frame the scan radius grows from <b>StartRadius</b> by <b>RadiusGrowRate</b> until it reaches <b>FinalRadius</b>. Every allied object inside the radius that matches <b>KindOf</b> and whose StealthUpdate has <i>GrantedBySpecialPower = Yes</i> receives permanent stealth. When the final radius is reached the helper object destroys itself. This is the mechanism behind the GLA 'Camo Netting / Sneak Attack' style powers.""",
 ex="""Behavior = GrantStealthBehavior ModuleTag_Grant
  StartRadius     = 0
  FinalRadius     = 200
  RadiusGrowRate  = 10
  KindOf          = INFANTRY VEHICLE
  RadiusParticleSystemName = StealthRingEffect
End""")
F.update({
'GrantStealthBehaviorModuleData.StartRadius': ("Scan radius on the first frame.", "0"),
'GrantStealthBehaviorModuleData.FinalRadius': ("Radius at which the sweep ends and the object deletes itself.", "200"),
'GrantStealthBehaviorModuleData.RadiusGrowRate': ("How much the radius grows each logic frame (30 frames = 1 second).", "10"),
'GrantStealthBehaviorModuleData.KindOf': ("Only allied objects with one of these KindOf bits are granted stealth. Empty = all.", "INFANTRY VEHICLE"),
'GrantStealthBehaviorModuleData.RadiusParticleSystemName': ("Particle system shown for the duration of the sweep (usually an expanding ring).", "StealthRing"),
})

M['FireOCLBehaviorV2'] = dict(
 sum="Expanding-radius sweep (like GrantStealthBehavior) that fires an ObjectCreationList once on every matching object found.",
 body="""A mod-original module built on the exact scan pattern of GrantStealthBehavior: the radius grows from <b>StartRadius</b> by <b>RadiusGrowRate</b> each frame up to <b>FinalRadius</b>, after which the carrying object deletes itself. Instead of granting stealth, every object inside the radius that passes the <b>KindOf</b>/<b>ForbiddenKindOf</b> filters has the <b>OCL</b> created at/on it — exactly once per object for the whole sweep (objects already hit are remembered, so the cumulative re-scan each frame does not fire twice).
<br><br>Typical use: a special power spawns an invisible, inert trigger object (KindOf NO_COLLIDE IMMOBILE UNATTACKABLE INERT) that carries this behavior, so that a buff/debuff/effect object is attached to every unit in an expanding circle. It is intentionally not rate-limited per target, because the triggering special power already has its own cooldown.""",
 ex="""Object FireOCLSweepTrigger
  KindOf = NO_COLLIDE IMMOBILE UNATTACKABLE INERT
  Behavior = FireOCLBehaviorV2 ModuleTag_Sweep
    StartRadius      = 0
    FinalRadius      = 250
    RadiusGrowRate   = 15
    KindOf           = INFANTRY VEHICLE
    ForbiddenKindOf  = AIRCRAFT
    OCL              = OCL_AttachBuffMarker
    RadiusParticleSystemName = ShockwaveRing
  End
End""")
F.update({
'FireOCLBehaviorV2ModuleData.StartRadius': ("Scan radius on the first frame.", "0"),
'FireOCLBehaviorV2ModuleData.FinalRadius': ("Radius at which the sweep finishes and the trigger object deletes itself.", "250"),
'FireOCLBehaviorV2ModuleData.RadiusGrowRate': ("Radius growth per logic frame.", "15"),
'FireOCLBehaviorV2ModuleData.KindOf': ("Only objects with at least one of these KindOf bits are affected. Empty = everything.", "INFANTRY VEHICLE"),
'FireOCLBehaviorV2ModuleData.ForbiddenKindOf': ("Objects with any of these KindOf bits are never affected.", "AIRCRAFT"),
'FireOCLBehaviorV2ModuleData.RadiusParticleSystemName': ("Particle system shown for the whole sweep.", "ShockwaveRing"),
'FireOCLBehaviorV2ModuleData.OCL': ("ObjectCreationList created once for each matching object found inside the radius (the found object is the OCL's primary object).", "OCL_AttachBuffMarker"),
})

M['NeutronBlastBehavior'] = dict(
 sum="On death, kills all infantry in a radius — even inside buildings and vehicles.",
 body="""When the object carrying this module dies (typically a neutron shell/missile projectile), every INFANTRY object within <b>BlastRadius</b> is killed outright, including infantry that is garrisoned in a building or riding inside a transport. The containers themselves are left intact and empty. <b>AffectAirborne</b> controls whether passengers of aircraft are killed, <b>AffectAllies</b> whether friendly infantry is also affected. Used by China's Neutron shells.""",
 ex="""Behavior = NeutronBlastBehavior ModuleTag_NeutronBlast
  BlastRadius     = 75
  AffectAirborne  = No
  AffectAllies    = Yes
End""")
F.update({
'NeutronBlastBehaviorModuleData.BlastRadius': ("Radius around the dying object in which infantry is killed.", "75"),
'NeutronBlastBehaviorModuleData.AffectAirborne': ("If Yes, infantry inside airborne containers (helicopters, planes) is also killed.", "No"),
'NeutronBlastBehaviorModuleData.AffectAllies': ("If Yes, allied/own infantry is killed too; if No only enemies.", "Yes"),
})

M['BridgeBehavior'] = dict(
 sum="Core logic of a destructible/repairable bridge: damage states, scaffold repair, death FX.",
 body="""Placed on the bridge object itself (map bridges created from Bridge definitions). It tracks the bridge's body damage state and propagates it to the bridge towers, handles the bridge dying (objects on it fall and are killed, bridge FX/OCLs are played), and when a Dozer repairs a destroyed bridge it drives the scaffold objects that rise and slide into place (<b>LateralScaffoldSpeed</b>, <b>VerticalScaffoldSpeed</b>). <b>BridgeDieFX</b>/<b>BridgeDieOCL</b> can be repeated and are fired along the bridge span with a delay and bone name.""",
 ex="""Behavior = BridgeBehavior ModuleTag_Bridge
  LateralScaffoldSpeed  = 30
  VerticalScaffoldSpeed = 20
  BridgeDieFX  = FX:FX_BridgeDie Delay:0 Bone:Center
  BridgeDieOCL = OCL:OCL_BridgeDebris Delay:200 Bone:Center
End""")
F.update({
'BridgeBehaviorModuleData.LateralScaffoldSpeed': ("Speed (units/sec) at which repair scaffold pieces slide horizontally into position.", "30"),
'BridgeBehaviorModuleData.VerticalScaffoldSpeed': ("Speed (units/sec) at which repair scaffold pieces rise from the ground.", "20"),
'BridgeBehaviorModuleData.BridgeDieFX': ("FXList played when the bridge is destroyed. Repeatable. Syntax: FX:<FXList> Delay:<ms> Bone:<bone name>.", "FX:FX_BridgeDie Delay:0 Bone:Center"),
'BridgeBehaviorModuleData.BridgeDieOCL': ("ObjectCreationList created when the bridge is destroyed. Repeatable. Syntax: OCL:<OCL> Delay:<ms> Bone:<bone name>.", "OCL:OCL_BridgeDebris Delay:200 Bone:Center"),
})
M['BridgeScaffoldBehavior'] = dict(
 sum="Moves the temporary scaffold pieces during bridge repair. No INI fields.",
 body="""Used internally on the scaffold objects that BridgeBehavior creates while a destroyed bridge is being rebuilt by a Dozer. It moves the scaffold from its rise position to its build position and back when done. It has no configurable fields — only list it on the scaffold object template.""",
 ex="""Behavior = BridgeScaffoldBehavior ModuleTag_Scaffold
End""")
M['BridgeTowerBehavior'] = dict(
 sum="Links a bridge tower (the targetable end piece) to its bridge. No INI fields.",
 body="""Bridge towers are the targetable objects at the corners of a bridge. This module forwards damage and healing between the tower and the bridge object so that attacking a tower damages the bridge and repairing it repairs the bridge. It has no fields.""",
 ex="""Behavior = BridgeTowerBehavior ModuleTag_Tower
End""")

M['CountermeasuresBehavior'] = dict(
 sum="Aircraft flare countermeasures: launches flare volleys and diverts incoming missiles.",
 body="""When a missile is fired at the aircraft, this module launches volleys of flare objects (<b>FlareTemplateName</b>) from bones named <b>FlareBoneBaseName</b>01, 02… Each incoming missile has an <b>EvasionRate</b> chance of being diverted to a flare after <b>MissileDecoyDelay</b>. The aircraft has <b>NumberOfVolleys</b> volleys of <b>VolleySize</b> flares, separated by <b>DelayBetweenVolleys</b>; when empty it reloads over <b>ReloadTime</b> — or, if <b>MustReloadAtAirfield</b> = Yes, only when it lands at an airfield. Gated by an upgrade through TriggeredBy (the USA 'Countermeasures' upgrade).""",
 ex="""Behavior = CountermeasuresBehavior ModuleTag_Flares
  TriggeredBy          = Upgrade_AmericaCountermeasures
  FlareTemplateName    = CountermeasureFlare
  FlareBoneBaseName    = Flare
  VolleySize           = 2
  VolleyArcAngle       = 60
  VolleyVelocityFactor = 2.0
  DelayBetweenVolleys  = 1000
  NumberOfVolleys      = 3
  ReloadTime           = 0
  EvasionRate          = 30%
  MustReloadAtAirfield = Yes
  MissileDecoyDelay    = 200
  ReactionLaunchLatency= 100
End""")
F.update({
'CountermeasuresBehaviorModuleData.FlareTemplateName': ("Object template spawned as a flare. Diverted missiles home onto these.", "CountermeasureFlare"),
'CountermeasuresBehaviorModuleData.FlareBoneBaseName': ("Bone name prefix used for flare launch points; bones Name01, Name02... are used in turn.", "Flare"),
'CountermeasuresBehaviorModuleData.VolleySize': ("Number of flares launched per volley.", "2"),
'CountermeasuresBehaviorModuleData.VolleyArcAngle': ("Angle (degrees) of the arc over which the flares of a volley are spread.", "60"),
'CountermeasuresBehaviorModuleData.VolleyVelocityFactor': ("Multiplier on the launch speed of flares.", "2.0"),
'CountermeasuresBehaviorModuleData.DelayBetweenVolleys': ("Time (ms) between consecutive volleys.", "1000"),
'CountermeasuresBehaviorModuleData.NumberOfVolleys': ("Number of volleys available before a reload is required.", "3"),
'CountermeasuresBehaviorModuleData.ReloadTime': ("Time (ms) to refill all volleys (ignored when MustReloadAtAirfield = Yes).", "0"),
'CountermeasuresBehaviorModuleData.EvasionRate': ("Probability (percent) that each incoming missile is diverted onto a flare.", "30%"),
'CountermeasuresBehaviorModuleData.MustReloadAtAirfield': ("If Yes, spent flares are only restored when the aircraft is docked at an airfield/parking place.", "Yes"),
'CountermeasuresBehaviorModuleData.MissileDecoyDelay': ("Time (ms) after a flare is launched before a diverted missile actually switches target to it.", "200"),
'CountermeasuresBehaviorModuleData.ReactionLaunchLatency': ("Time (ms) between the aircraft detecting an incoming missile and launching the first volley.", "100"),
})

M['DumbProjectileBehavior'] = dict(
 sum="Ballistic (unguided) projectile movement along a Bezier arc from launcher to target.",
 body="""Used on shell/rocket objects created by a weapon's ProjectileObject. The projectile flies a precomputed Bezier curve from launch point to target position, whose shape is controlled by two control points: the first is <b>FirstHeight</b> above the highest terrain in the way and <b>FirstPercentIndent</b> along the line, the second uses <b>SecondHeight</b>/<b>SecondPercentIndent</b>. When the curve ends (or it hits something) the projectile detonates and deals its weapon's damage. <b>FlightPathAdjustDistPerSecond</b> lets it slightly track a moving target. The GarrisonHitKill fields implement 'shell kills N garrisoned infantry when hitting a building' (e.g. China's Nuke Cannon / Inferno vs. garrisons).""",
 ex="""Behavior = DumbProjectileBehavior ModuleTag_Projectile
  MaxLifespan        = 10000
  TumbleRandomly     = No
  DetonateCallsKill  = Yes
  OrientToFlightPath = Yes
  FirstHeight        = 25
  SecondHeight       = 25
  FirstPercentIndent = 30%
  SecondPercentIndent= 70%
  FlightPathAdjustDistPerSecond = 50
End""")
F.update({
'DumbProjectileBehaviorModuleData.MaxLifespan': ("Maximum lifetime (ms) of the projectile before it is removed even if it never arrives.", "10000"),
'DumbProjectileBehaviorModuleData.TumbleRandomly': ("If Yes, the projectile spins randomly in flight (e.g. thrown debris or grenades).", "No"),
'DumbProjectileBehaviorModuleData.DetonateCallsKill': ("If Yes, detonation kills the projectile object (so its die modules/FX run); if No it is silently destroyed.", "Yes"),
'DumbProjectileBehaviorModuleData.OrientToFlightPath': ("If Yes, the model is rotated to face along its direction of travel.", "Yes"),
'DumbProjectileBehaviorModuleData.FirstHeight': ("Height of the first Bezier control point above the highest intervening terrain.", "25"),
'DumbProjectileBehaviorModuleData.SecondHeight': ("Height of the second Bezier control point.", "25"),
'DumbProjectileBehaviorModuleData.FirstPercentIndent': ("Position of the first control point, as a percent of the distance from launcher to target.", "30%"),
'DumbProjectileBehaviorModuleData.SecondPercentIndent': ("Position of the second control point, as a percent of the distance.", "70%"),
'DumbProjectileBehaviorModuleData.GarrisonHitKillRequiredKindOf': ("KindOf filter a building must match for the garrison-kill effect to apply on impact.", "STRUCTURE"),
'DumbProjectileBehaviorModuleData.GarrisonHitKillForbiddenKindOf': ("KindOf bits that exclude a building from the garrison-kill effect.", ""),
'DumbProjectileBehaviorModuleData.GarrisonHitKillCount': ("Number of garrisoned occupants killed when the projectile hits a qualifying building. 0 disables the effect.", "2"),
'DumbProjectileBehaviorModuleData.GarrisonHitKillFX': ("FXList played at the building when garrisoned occupants are killed this way.", "FX_GarrisonKill"),
'DumbProjectileBehaviorModuleData.FlightPathAdjustDistPerSecond': ("How far (units/sec) the projectile may bend its path to follow a moving target. 0 = pure ballistic.", "0"),
})

M['PhysicsBehavior'] = dict(
 sum="Rigid-body physics for the object: mass, friction, bouncing, collisions, falling damage.",
 body="""Almost every mobile object has a PhysicsBehavior. It integrates forces (locomotor thrust, gravity, explosions, collisions) into velocity and position, applies friction per axis, and produces the little pitch/roll 'shock' reactions when the unit is hit or fires (<b>ShockResistance</b>, <b>ShockMax*</b>). It also handles falling damage (<b>MinFallHeightForDamage</b>, <b>FallHeightDamageFactor</b>), bouncing debris (<b>AllowBouncing</b>), being pushed by other objects (<b>AllowCollideForce</b>) and what weapon is fired when a flung vehicle crashes into something.""",
 ex="""Behavior = PhysicsBehavior ModuleTag_Physics
  Mass                  = 50.0
  ForwardFriction       = 33%
  LateralFriction       = 33%
  ZFriction             = 50%
  AerodynamicFriction   = 2%
  CenterOfMassOffset    = 0
  AllowBouncing         = No
  KillWhenRestingOnGround = No
  MinFallHeightForDamage= 40
  FallHeightDamageFactor= 1.0
End""")
F.update({
'PhysicsBehaviorModuleData.Mass': ("Mass of the object. Heavier objects accelerate less from the same force (explosions, collisions).", "50.0"),
'PhysicsBehaviorModuleData.ShockResistance': ("Resistance to the visual pitch/roll 'shock' when hit or firing. Higher = less wobble.", "0.0"),
'PhysicsBehaviorModuleData.ShockMaxYaw': ("Maximum yaw shock rotation.", "0.05"),
'PhysicsBehaviorModuleData.ShockMaxPitch': ("Maximum pitch shock rotation.", "0.025"),
'PhysicsBehaviorModuleData.ShockMaxRoll': ("Maximum roll shock rotation.", "0.025"),
'PhysicsBehaviorModuleData.ForwardFriction': ("Friction along the forward axis, as a percent of velocity lost per second.", "33%"),
'PhysicsBehaviorModuleData.LateralFriction': ("Friction sideways (drift), percent per second.", "33%"),
'PhysicsBehaviorModuleData.ZFriction': ("Vertical friction, percent per second.", "50%"),
'PhysicsBehaviorModuleData.AerodynamicFriction': ("Air resistance applied while airborne, percent per second.", "2%"),
'PhysicsBehaviorModuleData.CenterOfMassOffset': ("Distance of the centre of mass from the geometric centre; controls how the object pitches when falling/flung.", "0"),
'PhysicsBehaviorModuleData.AllowBouncing': ("If Yes, the object bounces off the ground on impact (debris, grenades).", "No"),
'PhysicsBehaviorModuleData.AllowCollideForce': ("If Yes (default), collisions with other objects push this object.", "Yes"),
'PhysicsBehaviorModuleData.KillWhenRestingOnGround': ("If Yes, the object is killed as soon as it is on the ground and not moving (used for debris that should disappear once it lands).", "No"),
'PhysicsBehaviorModuleData.MinFallHeightForDamage': ("Fall height (converted internally to an impact speed) above which landing causes damage.", "40"),
'PhysicsBehaviorModuleData.FallHeightDamageFactor': ("Multiplier for falling damage.", "1.0"),
'PhysicsBehaviorModuleData.PitchRollYawFactor': ("Scale of the tumbling rotation applied when the object is flung by an explosion.", "2.0"),
'PhysicsBehaviorModuleData.VehicleCrashesIntoBuildingWeaponTemplate': ("Weapon fired when a flung/falling vehicle crashes into a building.", "VehicleCrashesIntoBuildingWeapon"),
'PhysicsBehaviorModuleData.VehicleCrashesIntoNonBuildingWeaponTemplate': ("Weapon fired when a flung/falling vehicle crashes into a non-building object.", "VehicleCrashesIntoNonBuildingWeapon"),
})

M['InstantDeathBehavior'] = dict(
 sum="Die module that plays FX, creates OCLs and fires weapons immediately at the moment of death.",
 body="""The simplest 'on death' effect module. When the object dies with a death type that matches <b>DeathTypes</b> (and passes the veterancy/status filters), it immediately plays every listed <b>FX</b>, creates every <b>OCL</b> and fires every <b>Weapon</b> at the object's position. Each of FX/OCL/Weapon may be listed several times; if more than one entry is listed of the same kind, one is picked at random. Use several InstantDeathBehavior modules with different DeathTypes to give an object different deaths (e.g. burned vs. exploded vs. crushed).""",
 ex="""Behavior = InstantDeathBehavior ModuleTag_DeathExplode
  DeathTypes = ALL -CRUSHED -SPLATTED
  FX         = FX_GenericTankDeathEffect
  OCL        = OCL_GenericTankDeathEffect
  Weapon     = SmallTankDeathWeapon
End""")
F.update({
'InstantDeathBehaviorModuleData.FX': ("FXList to play on death. May be repeated; if several are listed one is chosen at random.", "FX_GenericTankDeathEffect"),
'InstantDeathBehaviorModuleData.OCL': ("ObjectCreationList to create on death. May be repeated; one is chosen at random.", "OCL_GenericTankDeathEffect"),
'InstantDeathBehaviorModuleData.Weapon': ("Weapon fired at the object's position on death. May be repeated; one is chosen at random.", "SmallTankDeathWeapon"),
})

M['BreakApartDeathBehaviorV2'] = dict(
 sum="Die module that breaks the dying unit apart into its real subobjects as flying debris pieces, with normal / overkill / extreme-overkill tiers.",
 body="""A mod-original die module. On death it picks a tier based on how violent the killing blow was, then recursively walks the bone hierarchy of a dedicated reference model (<b>BreakApartModel</b>) starting from every root bone listed for that tier (<b>BreakApartSubObject</b>, <b>OverkillBreakApartSubObject</b> or <b>ExtremeOverkillBreakApartSubObject</b>). For each bone in that subtree that has geometry, children before parents, it spawns one piece from <b>BreakApartPieceOCL</b> and tells that piece's <i>W3DBreakApartPieceDraw</i> which single subobject of the reference model to display — so the piece looks exactly like that part of the unit and starts in exactly the same place. Everything that was not broken off is spawned once as a remainder piece via <b>RemainPieceOCL</b>. <b>RemainSubObject</b> protects a bone (and its whole subtree) from being broken off.
<br><br><b>Tier selection:</b> a tier triggers only if BOTH the killing hit was at least <i>…PercentageFromMaxHealth</i> of the unit's max health AND the wasted damage (damage minus health left before the hit) was at least <i>…DamageGreaterOrEqThan</i>. Extreme overkill is checked first, then overkill, otherwise the normal tier is used. Defaults disable both overkill tiers.
<br><br>Pieces are pushed outward from the unit's centre in the direction of the part (<b>Force</b>, <b>OverkillForce</b>, <b>ExtremeOverkillForce</b>) with up to <b>ForceSpreadAngle</b> random scatter. The dying object is removed after <b>DestructionDelay</b> (+ random variance), which can be overridden per tier. <b>FX</b>/<b>OCL</b>/<b>Weapon</b> are independent decorative effects fired per phase exactly like SlowDeathBehavior (INITIAL / MIDPOINT / FINAL). Because every die module runs on death, partition <b>DeathTypes</b> between this and any SlowDeathBehavior on the same object.""",
 ex="""Behavior = BreakApartDeathBehaviorV2 ModuleTag_BreakApart
  DeathTypes          = ALL -CRUSHED -SPLATTED
  BreakApartModel     = AVCrusader_BA          ; reference model with the full hierarchy
  BreakApartPieceOCL  = OCL_BreakApartPiece    ; object uses Draw = W3DBreakApartPieceDraw
  RemainPieceOCL      = OCL_BreakApartHull
  BreakApartPieceFX   = FX_SmallDebrisPuff

  BreakApartSubObject               = TURRET01
  OverkillBreakApartSubObject       = TURRET01 TREADSL TREADSR
  ExtremeOverkillBreakApartSubObject= TURRET01 TREADSL TREADSR FENDER
  RemainSubObject                   = ANTENNA01

  OverkillPercentageFromMaxHealth        = 60%
  OverkillDamageGreaterOrEqThan          = 100
  ExtremeOverkillPercentageFromMaxHealth = 150%
  ExtremeOverkillDamageGreaterOrEqThan   = 400

  Force               = 8
  OverkillForce       = 14
  ExtremeOverkillForce= 22
  ForceSpreadAngle    = 45

  DestructionDelay    = 100
  FX = INITIAL FX_GenericTankDeathEffect
End""")
F.update({
'BreakApartDeathBehaviorV2ModuleData.DestructionDelay': ("Time (ms) after death before the dying object itself is removed (the debris pieces live on independently).", "100"),
'BreakApartDeathBehaviorV2ModuleData.DestructionDelayVariance': ("Random extra time (ms, 0..value) added to DestructionDelay.", "0"),
'BreakApartDeathBehaviorV2ModuleData.OverkillDestructionDelay': ("DestructionDelay used when the overkill tier fires. If not set, inherits DestructionDelay.", "50"),
'BreakApartDeathBehaviorV2ModuleData.OverkillDestructionDelayVariance': ("Variance used for the overkill tier. If not set, inherits DestructionDelayVariance.", "0"),
'BreakApartDeathBehaviorV2ModuleData.ExtremeOverkillDestructionDelay': ("DestructionDelay used for the extreme-overkill tier. If not set, inherits DestructionDelay.", "0"),
'BreakApartDeathBehaviorV2ModuleData.ExtremeOverkillDestructionDelayVariance': ("Variance used for the extreme-overkill tier. If not set, inherits DestructionDelayVariance.", "0"),
'BreakApartDeathBehaviorV2ModuleData.FX': ("Phase-keyed FXList: <phase> <FXList>, phase = INITIAL, MIDPOINT or FINAL. Repeatable. Ambient effect independent of the debris pieces.", "INITIAL FX_GenericTankDeathEffect"),
'BreakApartDeathBehaviorV2ModuleData.OCL': ("Phase-keyed ObjectCreationList: <phase> <OCL>. Repeatable.", "FINAL OCL_TankScorch"),
'BreakApartDeathBehaviorV2ModuleData.Weapon': ("Phase-keyed weapon fired at the object's position: <phase> <Weapon>. Repeatable.", "INITIAL SmallTankDeathWeapon"),
'BreakApartDeathBehaviorV2ModuleData.BreakApartModel': ("Name of the W3D reference model whose bone hierarchy is walked and whose subobjects are shown on the pieces. Usually the same mesh as the unit, exported with all parts as separate subobjects.", "AVCrusader_BA"),
'BreakApartDeathBehaviorV2ModuleData.BreakApartSubObject': ("Normal-tier root bone/subobject names to break apart. Each named bone pulls in its whole subtree (children spawn first, then the bone). Space-separated, may be repeated to append.", "TURRET01"),
'BreakApartDeathBehaviorV2ModuleData.RemainSubObject': ("Bones to PROTECT from breaking off, whatever tier is chosen. Protects the named bone and its whole subtree; they stay on the RemainPieceOCL remainder piece.", "ANTENNA01"),
'BreakApartDeathBehaviorV2ModuleData.OverkillPercentageFromMaxHealth': ("Overkill tier condition 1: the killing hit's damage must be at least this percent of the unit's max health. Default disables the tier.", "60%"),
'BreakApartDeathBehaviorV2ModuleData.OverkillDamageGreaterOrEqThan': ("Overkill tier condition 2: wasted damage (hit damage minus health left before the hit) must be at least this much. Default disables the tier.", "100"),
'BreakApartDeathBehaviorV2ModuleData.OverkillBreakApartSubObject': ("Root bones broken apart when the overkill tier triggers (replaces the normal list for that death).", "TURRET01 TREADSL TREADSR"),
'BreakApartDeathBehaviorV2ModuleData.ExtremeOverkillPercentageFromMaxHealth': ("Extreme-overkill condition 1 (percent of max health). Checked before the overkill tier.", "150%"),
'BreakApartDeathBehaviorV2ModuleData.ExtremeOverkillDamageGreaterOrEqThan': ("Extreme-overkill condition 2 (wasted damage floor).", "400"),
'BreakApartDeathBehaviorV2ModuleData.ExtremeOverkillBreakApartSubObject': ("Root bones broken apart when the extreme-overkill tier triggers.", "TURRET01 TREADSL TREADSR FENDER"),
'BreakApartDeathBehaviorV2ModuleData.Force': ("Outward launch force applied to each piece in the normal tier.", "8"),
'BreakApartDeathBehaviorV2ModuleData.OverkillForce': ("Launch force for pieces in the overkill tier.", "14"),
'BreakApartDeathBehaviorV2ModuleData.ExtremeOverkillForce': ("Launch force for pieces in the extreme-overkill tier.", "22"),
'BreakApartDeathBehaviorV2ModuleData.ForceSpreadAngle': ("Half-angle (degrees) of random scatter around each piece's outward-from-centre direction. Default 60. The remainder piece always gets a fully random direction.", "45"),
'BreakApartDeathBehaviorV2ModuleData.BreakApartPieceOCL': ("The ONE generic OCL used to spawn every broken-off piece. Its object must use Draw = W3DBreakApartPieceDraw (and usually PhysicsBehavior + a lifetime).", "OCL_BreakApartPiece"),
'BreakApartDeathBehaviorV2ModuleData.RemainPieceOCL': ("OCL spawned at most once per death for everything that was NOT broken off (the hull that stays behind). Also uses W3DBreakApartPieceDraw.", "OCL_BreakApartHull"),
'BreakApartDeathBehaviorV2ModuleData.BreakApartPieceFX': ("FXList played at each broken-off piece's position when it spawns.", "FX_SmallDebrisPuff"),
'BreakApartDeathBehaviorV2ModuleData.RemainPieceFX': ("FXList played at the remainder piece when it spawns.", "FX_HullSmoke"),
})

M['SlowDeathBehavior'] = dict(
 sum="Multi-phase death: the corpse lingers, sinks into the ground, plays phased FX/OCL/weapons, then is destroyed.",
 body="""The standard 'corpse' module. When it handles a death, the object gets the DYING status and stays in the world: after <b>SinkDelay</b> (+variance) it starts sinking at <b>SinkRate</b>; it is finally destroyed after <b>DestructionDelay</b> (+variance) or when it sinks below <b>DestructionAltitude</b>. Effects are keyed to three phases: INITIAL (at death), MIDPOINT (half-way to destruction) and FINAL (at destruction) — write e.g. <code>FX = INITIAL FX_Name</code>. Optional fling (<b>FlingForce</b>, <b>FlingPitch</b>) throws the corpse (used for infantry blown into the air).
<br><br><b>Choosing between several SlowDeathBehaviors:</b> if an object has several SlowDeathBehavior modules that all accept the death type, exactly ONE is chosen by weighted lottery using <b>ProbabilityModifier</b>, plus <b>ModifierBonusPerOverkillPercent</b> which increases the weight for each percent of overkill damage — this is how 'blown-up' deaths become more likely with bigger hits.""",
 ex="""Behavior = SlowDeathBehavior ModuleTag_Death
  DeathTypes        = ALL -CRUSHED -SPLATTED
  ProbabilityModifier = 10
  SinkDelay         = 3000
  SinkDelayVariance = 1000
  SinkRate          = 0.40
  DestructionDelay  = 8000
  FX  = INITIAL FX_GenericTankDeathEffect
  OCL = INITIAL OCL_TankScorch
  OCL = FINAL   OCL_TankDeathDebris
End""")
F.update({
'SlowDeathBehaviorModuleData.SinkRate': ("Speed (units/sec) at which the corpse sinks into the ground once sinking starts.", "0.40"),
'SlowDeathBehaviorModuleData.ProbabilityModifier': ("Lottery weight when several SlowDeathBehaviors could handle the same death. Must be >= 1.", "10"),
'SlowDeathBehaviorModuleData.ModifierBonusPerOverkillPercent': ("Extra lottery weight per percent of overkill damage (damage beyond what was needed to kill).", "20%"),
'SlowDeathBehaviorModuleData.SinkDelay': ("Time (ms) after death before the corpse starts sinking.", "3000"),
'SlowDeathBehaviorModuleData.SinkDelayVariance': ("Random extra time (ms) added to SinkDelay.", "1000"),
'SlowDeathBehaviorModuleData.DestructionDelay': ("Time (ms) after death at which the object is destroyed (FINAL phase).", "8000"),
'SlowDeathBehaviorModuleData.DestructionDelayVariance': ("Random extra time (ms) added to DestructionDelay.", "0"),
'SlowDeathBehaviorModuleData.DestructionAltitude': ("If the object sinks below this height relative to the terrain, it is destroyed immediately.", "-10"),
'SlowDeathBehaviorModuleData.FX': ("Phase-keyed FXList: <INITIAL|MIDPOINT|FINAL> <FXList>. Repeatable; one random entry per phase.", "INITIAL FX_GenericTankDeathEffect"),
'SlowDeathBehaviorModuleData.OCL': ("Phase-keyed ObjectCreationList: <phase> <OCL>. Repeatable.", "FINAL OCL_TankDeathDebris"),
'SlowDeathBehaviorModuleData.Weapon': ("Phase-keyed weapon fired at the corpse: <phase> <Weapon>. Repeatable.", "INITIAL SmallTankDeathWeapon"),
'SlowDeathBehaviorModuleData.FlingForce': ("Force used to fling the corpse into the air at death. 0 = no fling.", "8"),
'SlowDeathBehaviorModuleData.FlingForceVariance': ("Random extra fling force.", "3"),
'SlowDeathBehaviorModuleData.FlingPitch': ("Pitch angle (degrees above horizontal) of the fling direction.", "60"),
'SlowDeathBehaviorModuleData.FlingPitchVariance': ("Random extra pitch (degrees).", "10"),
})

M['HelicopterSlowDeathBehavior'] = dict(
 sum="SlowDeathBehavior for helicopters: spirals down, loses its rotor blade, crashes and blows up.",
 body="""Extends SlowDeathBehavior with a scripted helicopter crash. On death the helicopter keeps flying in a downward spiral (<b>SpiralOrbitTurnRate</b>, <b>SpiralOrbitForwardSpeed</b>) while spinning around itself (<b>MinSelfSpin</b>/<b>MaxSelfSpin</b>) and losing lift (<b>FallHowFast</b>). At a random time between Min/MaxBladeFlyOffDelay the main rotor (<b>BladeBoneName</b>) is hidden and a separate blade object (<b>BladeObjectName</b>) is thrown off with <b>FXBlade</b>/<b>OCLBlade</b>. When it hits the ground <b>FXHitGround</b>/<b>OCLHitGround</b> play, and after <b>DelayFromGroundToFinalDeath</b> the final explosion (<b>FXFinalBlowUp</b>/<b>OCLFinalBlowUp</b>) and the <b>FinalRubbleObject</b> are created. All normal SlowDeathBehavior fields still apply.""",
 ex="""Behavior = HelicopterSlowDeathBehavior ModuleTag_HeliDeath
  DeathTypes               = ALL
  SpiralOrbitTurnRate      = 140
  SpiralOrbitForwardSpeed  = 350
  SpiralOrbitForwardSpeedDamping = 0.8
  MinSelfSpin              = 100
  MaxSelfSpin              = 300
  SelfSpinUpdateDelay      = 500
  SelfSpinUpdateAmount     = 20
  FallHowFast              = 15%
  MinBladeFlyOffDelay      = 1000
  MaxBladeFlyOffDelay      = 2500
  BladeObjectName          = ComancheBlade
  BladeBoneName            = Propeller01
  FXBlade                  = FX_HelicopterBladeFlyOff
  FXHitGround              = FX_HelicopterHitGround
  DelayFromGroundToFinalDeath = 1500
  FXFinalBlowUp            = FX_HelicopterFinalBlowUp
  FinalRubbleObject        = ComancheHulk
  SoundDeathLoop           = HelicopterDeathLoop
End""")
F.update({
'HelicopterSlowDeathBehaviorModuleData.SpiralOrbitTurnRate': ("Angular speed (deg/sec) of the big circles the helicopter flies while falling.", "140"),
'HelicopterSlowDeathBehaviorModuleData.SpiralOrbitForwardSpeed': ("Forward speed (units/sec) along the spiral.", "350"),
'HelicopterSlowDeathBehaviorModuleData.SpiralOrbitForwardSpeedDamping': ("Each frame the forward speed is multiplied by this value (1 = no slowdown).", "0.8"),
'HelicopterSlowDeathBehaviorModuleData.MinSelfSpin': ("Minimum spin rate (deg/sec) around the helicopter's own centre.", "100"),
'HelicopterSlowDeathBehaviorModuleData.MaxSelfSpin': ("Maximum spin rate (deg/sec).", "300"),
'HelicopterSlowDeathBehaviorModuleData.SelfSpinUpdateDelay': ("Interval (ms) at which the spin rate is changed.", "500"),
'HelicopterSlowDeathBehaviorModuleData.SelfSpinUpdateAmount': ("Amount (degrees) the spin rate changes each update, clamped between min and max.", "20"),
'HelicopterSlowDeathBehaviorModuleData.FallHowFast': ("Fraction of gravity used to reduce the locomotor lift, i.e. how fast it drops.", "15%"),
'HelicopterSlowDeathBehaviorModuleData.MinBladeFlyOffDelay': ("Earliest time (ms) after death at which the rotor blade flies off.", "1000"),
'HelicopterSlowDeathBehaviorModuleData.MaxBladeFlyOffDelay': ("Latest time (ms) after death at which the rotor blade flies off.", "2500"),
'HelicopterSlowDeathBehaviorModuleData.AttachParticle': ("Particle system (e.g. smoke trail) attached to the falling helicopter.", "SmokeTrail"),
'HelicopterSlowDeathBehaviorModuleData.AttachParticleBone': ("Bone to attach AttachParticle to.", "Engine01"),
'HelicopterSlowDeathBehaviorModuleData.AttachParticleLoc': ("Offset (X:Y:Z) to attach the particle system when the bone does not exist.", "X:0 Y:0 Z:5"),
'HelicopterSlowDeathBehaviorModuleData.BladeObjectName': ("Object template thrown off as the detached rotor blade.", "ComancheBlade"),
'HelicopterSlowDeathBehaviorModuleData.BladeBoneName': ("Bone/subobject of the rotor that is hidden when the blade flies off.", "Propeller01"),
'HelicopterSlowDeathBehaviorModuleData.OCLEjectPilot': ("OCL used to eject a pilot at death (veteran pilots).", "OCL_EjectPilotViaParachute"),
'HelicopterSlowDeathBehaviorModuleData.FXBlade': ("FXList played when the blade flies off.", "FX_HelicopterBladeFlyOff"),
'HelicopterSlowDeathBehaviorModuleData.OCLBlade': ("OCL created when the blade flies off.", "OCL_BladeDebris"),
'HelicopterSlowDeathBehaviorModuleData.FXHitGround': ("FXList played when the wreck hits the ground.", "FX_HelicopterHitGround"),
'HelicopterSlowDeathBehaviorModuleData.OCLHitGround': ("OCL created when the wreck hits the ground.", "OCL_HeliCrashDebris"),
'HelicopterSlowDeathBehaviorModuleData.FXFinalBlowUp': ("FXList for the final explosion.", "FX_HelicopterFinalBlowUp"),
'HelicopterSlowDeathBehaviorModuleData.OCLFinalBlowUp': ("OCL for the final explosion.", "OCL_HeliFinalDebris"),
'HelicopterSlowDeathBehaviorModuleData.DelayFromGroundToFinalDeath': ("Time (ms) between hitting the ground and the final explosion.", "1500"),
'HelicopterSlowDeathBehaviorModuleData.FinalRubbleObject': ("Object template created as the burning hulk after the final explosion.", "ComancheHulk"),
'HelicopterSlowDeathBehaviorModuleData.SoundDeathLoop': ("Looping sound played during the whole crash sequence.", "HelicopterDeathLoop"),
'HelicopterSlowDeathBehaviorModuleData.MaxBraking': ("Maximum braking acceleration the locomotor may use during the spiral.", "99999"),
})

M['NeutronMissileSlowDeathBehavior'] = dict(
 sum="Death sequence of the Nuclear Missile superweapon warhead: up to 9 staged blast rings with scorch marks, damage and push.",
 body="""SlowDeathBehavior variant used on the nuclear missile object. When the missile 'dies' (arrives), it plays <b>FXList</b> and then runs up to nine independent blast waves, Blast1…Blast9. Each wave is enabled with BlastNEnabled and after BlastNDelay deals damage that falls off from BlastNMaxDamage at BlastNInnerRadius to BlastNMinDamage at BlastNOuterRadius, topples trees/structures (BlastNToppleSpeed) and pushes units (BlastNPushForce). BlastNScorchDelay controls when the scorch mark (<b>ScorchMarkSize</b>) is placed. Combining several waves with growing radii produces the expanding nuke effect.""",
 ex="""Behavior = NeutronMissileSlowDeathBehavior ModuleTag_NukeDeath
  DestructionDelay = 5000
  ScorchMarkSize   = 400
  FXList           = FX_NukeExplosion
  Blast1Enabled     = Yes
  Blast1Delay       = 0
  Blast1ScorchDelay = 0
  Blast1InnerRadius = 50
  Blast1OuterRadius = 100
  Blast1MaxDamage   = 2000
  Blast1MinDamage   = 500
  Blast1ToppleSpeed = 0.5
  Blast1PushForce   = 10
  Blast2Enabled     = Yes
  Blast2Delay       = 300
  Blast2InnerRadius = 100
  Blast2OuterRadius = 200
  Blast2MaxDamage   = 800
  Blast2MinDamage   = 100
End""")
F.update({
'NeutronMissileSlowDeathBehaviorModuleData.ScorchMarkSize': ("Diameter of the scorch decal left on the ground.", "400"),
'NeutronMissileSlowDeathBehaviorModuleData.FXList': ("FXList that creates all the visuals of the detonation.", "FX_NukeExplosion"),
})

M['CaveContain'] = dict(
 sum="GLA tunnel-style container that stores passengers in a shared cave network identified by CaveIndex.",
 body="""A version of OpenContain that does not keep passengers itself: it stores them in one of the global 'cave' networks managed by the CaveSystem. All caves with the same <b>CaveIndex</b> share a single passenger list regardless of owner, so units entering one cave can exit from any other cave of the same index. Used for map-placed neutral cave/tunnel networks (as opposed to TunnelContain which is per-player). The index can also be changed by map scripts.""",
 ex="""Behavior = CaveContain ModuleTag_Cave
  ContainMax      = 10
  CaveIndex       = 1
  AllowInsideKindOf = INFANTRY VEHICLE
  EnterSound      = GarrisonEnter
  ExitSound       = GarrisonExit
End""")
F.update({'CaveContainModuleData.CaveIndex': ("ID of the cave network this cave belongs to. All caves sharing the index share their passengers.", "1")})

M['OpenContain'] = dict(
 sum="Base container: lets objects enter and exit this object. Parent of every Contain module.",
 body="""OpenContain is the generic container that every other *Contain module extends (Transport, Garrison, Tunnel, Heal, Overlord, Helix, Parachute, Prison…). It handles who may enter (<b>AllowInsideKindOf</b>, <b>ForbidInsideKindOf</b>, <b>AllowAllies/Enemies/NeutralInside</b>), how many (<b>ContainMax</b>), entering/exiting sounds, unloading via ExitStart/ExitEnd bone paths and door animation time, passengers firing out of fire-points, and what happens to passengers when the container dies (<b>DamagePercentToUnits</b>). It also carries DieMux fields because a container needs to react to its own death. On its own it is rarely used; pick the specialised subclass.""",
 ex="""Behavior = OpenContain ModuleTag_Contain
  ContainMax          = 5
  AllowInsideKindOf   = INFANTRY
  PassengersAllowedToFire = Yes
  DamagePercentToUnits= 100%
End""")

M['OverchargeBehavior'] = dict(
 sum="Lets a power plant overcharge (extra power) while slowly draining its own health.",
 body="""Adds the Overcharge toggle used by the USA Cold Fusion Reactor / China Nuclear Reactor. While overcharged the object produces its extra (upgraded) power and loses <b>HealthPercentToDrainPerSecond</b> of its max health each second. Overcharge cannot be turned on, and turns itself off, when health is below <b>NotAllowedWhenHealthBelowPercent</b>. The on/off toggle comes from a command button using the OVERCHARGE command.""",
 ex="""Behavior = OverchargeBehavior ModuleTag_Overcharge
  HealthPercentToDrainPerSecond   = 3%
  NotAllowedWhenHealthBelowPercent = 50%
End""")
F.update({
'OverchargeBehaviorModuleData.HealthPercentToDrainPerSecond': ("Percent of max health lost per second while overcharged.", "3%"),
'OverchargeBehaviorModuleData.NotAllowedWhenHealthBelowPercent': ("Overcharge is disabled and switched off when health falls below this percent.", "50%"),
})

M['HealContain'] = dict(
 sum="Container that heals its passengers to full over a fixed time, then releases them.",
 body="""Objects that enter are healed so that they reach full health after <b>TimeForFullHeal</b>, after which they are automatically ejected. Used on the Hospital-style structures and some tech buildings. All OpenContain fields apply.""",
 ex="""Behavior = HealContain ModuleTag_Heal
  ContainMax      = 5
  AllowInsideKindOf = INFANTRY
  TimeForFullHeal = 5000
End""")
F.update({'HealContainModuleData.TimeForFullHeal': ("Time (ms) for a passenger to go from any health to full health; it is ejected when healed.", "5000")})

M['GarrisonContain'] = dict(
 sum="Building garrison: infantry enter and fire from windows (garrison points), move between sides of the building.",
 body="""The container for garrisonable structures. Passengers are distributed over GARRISON bones and automatically shuffle to the side of the building closest to their target. It supports up to 10 occupants. <b>MobileGarrison</b> is for vehicles that act like a garrison (e.g. a bunker vehicle). <b>HealObjects</b> + <b>TimeForFullHeal</b> heals the occupants. <b>InitialRoster</b> spawns occupants at start. <b>ImmuneToClearBuildingAttacks</b> protects the occupants from 'clear garrison' weapons (flame, toxin, Ranger flashbang), and <b>IsEnclosingContainer</b> = No lets occupants be hit by outside attacks.""",
 ex="""Behavior = GarrisonContain ModuleTag_Garrison
  ContainMax             = 10
  EnterSound             = GarrisonEnter
  ExitSound              = GarrisonExit
  ImmuneToClearBuildingAttacks = No
  AllowInsideKindOf      = INFANTRY
  PassengersAllowedToFire= Yes
  DamagePercentToUnits   = 100%
End""")
F.update({
'GarrisonContainModuleData.MobileGarrison': ("If Yes, the container can move (garrison on a vehicle) and fire points follow it.", "No"),
'GarrisonContainModuleData.HealObjects': ("If Yes, occupants are healed while inside.", "No"),
'GarrisonContainModuleData.TimeForFullHeal': ("Time (ms) to fully heal an occupant when HealObjects = Yes.", "5000"),
'GarrisonContainModuleData.InitialRoster': ("Occupants created inside at start: <ObjectTemplate> <Count>.", "ChinaInfantryRedguard 3"),
'GarrisonContainModuleData.ImmuneToClearBuildingAttacks': ("If Yes, weapons that clear garrisons (DamageType flame/poison/etc. with the 'clears garrison' behaviour) do not kill occupants.", "No"),
'GarrisonContainModuleData.IsEnclosingContainer': ("If Yes (default), occupants are fully enclosed and cannot be hit directly. No = occupants remain targetable (like a trench).", "Yes"),
})

M['InternetHackContain'] = dict(
 sum="Transport container (Internet Center) that orders every hacker inside to hack the internet for money.",
 body="""A TransportContain for China's Internet Center: every passenger that enters is automatically given the 'hack internet' AI command so it generates cash while inside (see HackInternetAIUpdate). Uses all TransportContain fields (Slots, AllowInsideKindOf…).""",
 ex="""Behavior = InternetHackContain ModuleTag_Hack
  Slots             = 8
  AllowInsideKindOf = INFANTRY
  ScatterNearbyOnExit = Yes
End""")

M['TransportContain'] = dict(
 sum="Vehicle/aircraft transport: carries units in slots, unloads them, optional healing and armed-passenger upgrades.",
 body="""The standard transport container used on Humvees, APCs, Chinooks, Battle Buses, etc. Capacity is measured in <b>Slots</b> (each passenger's TransportSlotCount). Controls where and how passengers exit (<b>ExitBone</b>, <b>ScatterNearbyOnExit</b>, <b>ExitDelay</b>, <b>DelayExitInAir</b>), whether they are pre-loaded (<b>InitialPayload</b>), healed (<b>HealthRegen%PerSec</b>) and whether armed passengers switch the transport to its upgraded weapon set (<b>ArmedRidersUpgradeMyWeaponSet</b>). All OpenContain fields also apply (PassengersAllowedToFire for firing ports, DamagePercentToUnits on death...).""",
 ex="""Behavior = TransportContain ModuleTag_Transport
  Slots                 = 5
  AllowInsideKindOf     = INFANTRY
  ForbidInsideKindOf    = HERO
  PassengersAllowedToFire = Yes
  ExitDelay             = 250
  NumberOfExitPaths     = 1
  DoorOpenTime          = 1000
  ScatterNearbyOnExit   = Yes
  HealthRegen%PerSec    = 0
  DamagePercentToUnits  = 100%
  EnterSound            = HumveeEnter
End""")
