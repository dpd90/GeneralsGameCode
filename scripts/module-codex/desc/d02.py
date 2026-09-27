M = {}
F = {}

M['RiderChangeContain'] = dict(
 sum="Combat Bike container: the vehicle changes model, weapons, status and command set depending on which rider is inside.",
 body="""A TransportContain for GLA's Combat Cycle. Up to eight rider types are defined with Rider1…Rider8. When a unit enters, the matching row is looked up by template and the bike gets that row's model condition (RIDER1…), weapon set flag (WEAPON_RIDER1…), object status (STATUS_RIDER1…), command set override and locomotor set; the rider's veterancy is transferred to the bike. When the rider is killed (KILL_PILOT, e.g. sniper) or leaves, the empty bike is 'scuttled': it gets <b>ScuttleStatus</b> (default TOPPLED) and is destroyed after <b>ScuttleDelay</b>.
<br><br>Rider row syntax: <code>RiderN = &lt;InfantryTemplate&gt; &lt;ModelCondition&gt; &lt;WeaponSetFlag&gt; &lt;ObjectStatus&gt; &lt;CommandSet&gt; &lt;LocomotorSet&gt;</code>.""",
 ex="""Behavior = RiderChangeContain ModuleTag_Riders
  Rider1 = GLAInfantryRebel       RIDER1 WEAPON_RIDER1 STATUS_RIDER1 GLAVehicleCombatBikeRebelCommandSet SET_NORMAL
  Rider2 = GLAInfantryRPGTrooper  RIDER2 WEAPON_RIDER2 STATUS_RIDER2 GLAVehicleCombatBikeRPGCommandSet   SET_NORMAL
  Rider3 = GLAInfantryTerrorist   RIDER3 WEAPON_RIDER3 STATUS_RIDER3 GLAVehicleCombatBikeTerroristCommandSet SET_NORMAL
  ScuttleDelay  = 1000
  ScuttleStatus = TOPPLED
  Slots         = 1
  AllowInsideKindOf = INFANTRY
  ExitDelay     = 100
End""")
for i in range(1, 9):
    F[f'RiderChangeContainModuleData.Rider{i}'] = (f"Rider row {i}: <InfantryTemplate> <ModelConditionFlag> <WeaponSetFlag> <ObjectStatus> <CommandSet> <LocomotorSet>. Applied while that infantry type is the rider.", f"GLAInfantryRebel RIDER{i} WEAPON_RIDER{i} STATUS_RIDER{i} CommandSet_Bike{i} SET_NORMAL")
F.update({
'RiderChangeContainModuleData.ScuttleDelay': ("Time (ms) after the rider is lost before the empty vehicle is destroyed.", "1000"),
'RiderChangeContainModuleData.ScuttleStatus': ("Model condition set on the vehicle while it is being scuttled (the 'fall over' animation state).", "TOPPLED"),
})

M['RiderChangeContainV2'] = dict(
 sum="RiderChangeContain with an unlimited, repeatable 'Rider =' list (up to 32 distinct flags) and options to keep the vehicle alive when the rider is sniped.",
 body="""Mod-original replacement for RiderChangeContain. It behaves the same way (rider swaps the vehicle's model condition / weapon set / status / command set / locomotor set, veterancy transfers to the vehicle, vehicle is scuttled when the rider leaves), but instead of fixed Rider1…Rider8 fields every rider row uses the same repeatable <b>Rider</b> field and each statement appends one more row — any number of rows. The engine was extended with RIDER1…RIDER32 model conditions, WEAPON_RIDER1…32 weapon-set flags and STATUS_RIDER1…32 statuses, and rows may share flags. Avoid STATUS_RIDER8 on rows (vanilla code uses it as a 'busy' marker).
<br><br>New behaviour on pilot kill: with <b>ScuttleOnDeath</b> = No, a KILL_PILOT death kills only the rider; the vehicle stays alive, empty and mobile under the same owner (veterancy lost) until a valid rider boards again. With <b>NoScuttleTurnNeutral</b> = Yes as well, the empty vehicle instead turns neutral and immobile, and the next valid rider of ANY player who boards it takes ownership (needs AllowNeutralInside = Yes, the default). A rider leaving voluntarily still scuttles as before.""",
 ex="""Behavior = RiderChangeContainV2 ModuleTag_Riders
  Rider = GLAInfantryRebel       RIDER1  WEAPON_RIDER1  STATUS_RIDER1  CommandSet_A SET_NORMAL
  Rider = GLAInfantryRPGTrooper  RIDER2  WEAPON_RIDER2  STATUS_RIDER2  CommandSet_B SET_NORMAL
  Rider = GLAInfantryJarmenKell  RIDER9  WEAPON_RIDER9  STATUS_RIDER9  CommandSet_C SET_NORMAL
  ScuttleDelay         = 1000
  ScuttleStatus        = TOPPLED
  ScuttleOnDeath       = No     ; sniping the rider leaves the bike alive
  NoScuttleTurnNeutral = Yes    ; ...neutral and capturable by any infantry
  Slots                = 1
  AllowInsideKindOf    = INFANTRY
End""")
F.update({
'RiderChangeContainV2ModuleData.Rider': ("One rider row, repeatable (each line appends a row, unlimited): <InfantryTemplate> <ModelCondition RIDERn> <WeaponSetFlag WEAPON_RIDERn> <Status STATUS_RIDERn> <CommandSet> <LocomotorSet>. n may go up to 32.", "GLAInfantryRebel RIDER1 WEAPON_RIDER1 STATUS_RIDER1 CommandSet_A SET_NORMAL"),
'RiderChangeContainV2ModuleData.ScuttleDelay': ("Time (ms) after the rider is lost before the empty vehicle is destroyed (when scuttling).", "1000"),
'RiderChangeContainV2ModuleData.ScuttleStatus': ("Model condition set on the vehicle while it is being scuttled.", "TOPPLED"),
'RiderChangeContainV2ModuleData.ScuttleOnDeath': ("Yes (default, vanilla behaviour): a KILL_PILOT death scuttles the vehicle. No: only the rider dies; the vehicle survives empty until re-boarded.", "No"),
'RiderChangeContainV2ModuleData.NoScuttleTurnNeutral': ("Only with ScuttleOnDeath = No. Yes: the surviving empty vehicle turns neutral and immobile, and the next valid rider (any player) captures it.", "Yes"),
})

M['RailedTransportContain'] = dict(
 sum="TransportContain for rail-bound transports (trains) that coordinates with RailedTransportAIUpdate and dock stations.",
 body="""A TransportContain used together with RailedTransportAIUpdate and RailedTransportDockUpdate on train-like transports that move along waypoint rails between stations. Passengers are only allowed to exit when the transport is docked at a station. Uses all TransportContain fields; no additional ones.""",
 ex="""Behavior = RailedTransportContain ModuleTag_Contain
  Slots             = 10
  AllowInsideKindOf = INFANTRY VEHICLE
  ExitDelay         = 300
End""")

M['MobNexusContain'] = dict(
 sum="Invisible container that represents a mob (e.g. Angry Mob) as a single selectable unit for UI/AI.",
 body="""Used on the 'nexus' object of mob units: the nexus acts as a proxy for UI selection and AI orders while the individual mob members are slaved to it (MobMemberSlavedUpdate). It has its own copy of transport-like fields (Slots, exit behaviour, InitialPayload, regeneration) rather than inheriting TransportContain.""",
 ex="""Behavior = MobNexusContain ModuleTag_MobNexus
  Slots         = 10
  ScatterNearbyOnExit = Yes
  InitialPayload = GLAInfantryAngryMobPistol01 2
End""")
F.update({
'MobNexusContainModuleData.Slots': ("Capacity of the nexus in slots.", "10"),
'MobNexusContainModuleData.ScatterNearbyOnExit': ("Exiting members move away from the nexus instead of standing on it.", "Yes"),
'MobNexusContainModuleData.OrientLikeContainerOnExit': ("Exiting members face the nexus's direction.", "No"),
'MobNexusContainModuleData.KeepContainerVelocityOnExit': ("Exiting members inherit the nexus's velocity.", "No"),
'MobNexusContainModuleData.ExitBone': ("Bone at which members are placed when exiting.", ""),
'MobNexusContainModuleData.ExitPitchRate': ("Pitch angular velocity applied to exiting members.", "0"),
'MobNexusContainModuleData.InitialPayload': ("Members created inside when the nexus is created: <Template> <Count>. Repeatable.", "GLAInfantryAngryMobPistol01 2"),
'MobNexusContainModuleData.HealthRegen%PerSec': ("Percent of max health regenerated per second by contained members.", "0"),
})

M['TunnelContain'] = dict(
 sum="GLA Tunnel Network: passengers are stored in the owning player's shared tunnel system.",
 body="""A version of OpenContain whose passengers are kept in the owning player's TunnelTracker, so every tunnel of that player shares one passenger list (capacity is set by MaxTunnelCapacity in GameData.ini). Units entering one tunnel can exit from any other. Contained units are healed to full over <b>TimeForFullHeal</b>. If the last tunnel of the network dies, everyone inside is killed.""",
 ex="""Behavior = TunnelContain ModuleTag_Tunnel
  TimeForFullHeal = 5000
  AllowInsideKindOf = INFANTRY VEHICLE
  ForbidInsideKindOf = AIRCRAFT HUGE_VEHICLE
  NumberOfExitPaths = 3
End""")
F.update({'TunnelContainModuleData.TimeForFullHeal': ("Time (ms) for units inside the tunnel network to be fully healed.", "5000")})

M['OverlordContain'] = dict(
 sum="Overlord tank container: holds one add-on (Gattling/Propaganda/Bunker) and forwards transport queries to it.",
 body="""Acts as a normal TransportContain, but once full it redirects container queries to its first passenger. This is how China's Overlord upgrades work: the add-on (e.g. the Bunker) is a separate object riding on the tank (drawn via W3DDependencyModelDraw), and because queries are forwarded, infantry that 'enter the Overlord' actually go into the Bunker rider. <b>PayloadTemplateName</b> creates the rider object(s); <b>ExperienceSinkForRider</b> makes the rider's veterancy go to the tank.""",
 ex="""Behavior = OverlordContain ModuleTag_Overlord
  Slots                  = 1
  DamagePercentToUnits   = 100%
  AllowInsideKindOf      = PORTABLE_STRUCTURE
  PayloadTemplateName    = ChinaTankOverlordGattlingCannon
  ExperienceSinkForRider = Yes
End""")
F.update({
'OverlordContainModuleData.PayloadTemplateName': ("Object template(s) created and loaded as the rider(s). Appends when repeated.", "ChinaTankOverlordGattlingCannon"),
'OverlordContainModuleData.ExperienceSinkForRider': ("If Yes, experience the rider earns is given to the carrier instead.", "Yes"),
})

M['OverlordContainV2'] = dict(
 sum="OverlordContain that supports several simultaneously-visible riders of any KindOf.",
 body="""Mod-original. Keeps OverlordContain's 'redirect transport queries to my single rider' trick for the classic one-rider case, but as soon as a second rider boards, redirection is switched off and it behaves like a normal multi-slot TransportContain whose occupants all stay visibly mounted — each drawn via its own W3DDependencyModelDraw with AttachToBoneInContainer — and all receive the ExperienceSinkForRider, stealth-grant, damage-state and on-capture treatment. Normal Slots / AllowInsideKindOf rules decide who may board, so riders can be any mix of unit types.""",
 ex="""Behavior = OverlordContainV2 ModuleTag_Overlord
  Slots                  = 3
  AllowInsideKindOf      = PORTABLE_STRUCTURE INFANTRY
  PayloadTemplateName    = MyTankTurretAddon
  PayloadTemplateName    = MyTankGunnerAddon
  ExperienceSinkForRider = Yes
  DamagePercentToUnits   = 100%
End""")
F.update({
'OverlordContainV2ModuleData.PayloadTemplateName': ("Rider object template(s) created and loaded on creation. Repeat to add more riders.", "MyTankTurretAddon"),
'OverlordContainV2ModuleData.ExperienceSinkForRider': ("If Yes, experience earned by any rider goes to the carrier.", "Yes"),
})

M['HelixContain'] = dict(
 sum="China Helix container: a transport that can also carry one portable add-on structure (Gattling, Bunker, Propaganda...).",
 body="""TransportContain variant for the Helix helicopter. It carries normal passengers in its slots and additionally hosts a portable structure rider (created from <b>PayloadTemplateName</b> or bought by upgrade) which is drawn attached to the helicopter and can fire. <b>ShouldDrawPips</b> toggles the passenger pips on the selection UI.""",
 ex="""Behavior = HelixContain ModuleTag_Helix
  Slots             = 5
  AllowInsideKindOf = INFANTRY VEHICLE PORTABLE_STRUCTURE
  ForbidInsideKindOf= AIRCRAFT HUGE_VEHICLE
  ShouldDrawPips    = Yes
End""")
F.update({
'HelixContainModuleData.PayloadTemplateName': ("Portable structure template(s) created and loaded as the add-on rider.", "HelixGattlingCannon"),
'HelixContainModuleData.ShouldDrawPips': ("If Yes (default) the passenger pips are drawn on the unit's UI.", "Yes"),
})

M['ParachuteContain'] = dict(
 sum="Parachute object that carries a unit down to the ground (paradrops, ejected pilots, supply crates).",
 body="""The parachute is a container that holds the falling unit. It free-falls until it has fallen <b>ParachuteOpenDist</b>, opens (playing <b>ParachuteOpenSound</b>) and then drifts down with sway limited by <b>PitchRateMax</b>/<b>RollRateMax</b> and damping near the ground. On landing the passenger is released and the parachute removed. If the chute never opens, the passenger takes <b>FreeFallDamagePercent</b> damage; landing in water kills it.""",
 ex="""Behavior = ParachuteContain ModuleTag_Chute
  PitchRateMax       = 60
  RollRateMax        = 60
  LowAltitudeDamping = 0.2
  ParachuteOpenDist  = 30
  FreeFallDamagePercent = 50%
  ParachuteOpenSound = ParachuteOpen
  AllowInsideKindOf  = INFANTRY VEHICLE CRATE
End""")
F.update({
'ParachuteContainModuleData.PitchRateMax': ("Maximum swing rate (deg/sec) around the pitch axis while drifting.", "60"),
'ParachuteContainModuleData.RollRateMax': ("Maximum swing rate (deg/sec) around the roll axis.", "60"),
'ParachuteContainModuleData.LowAltitudeDamping': ("Damping of the swing close to the ground so the landing is upright.", "0.2"),
'ParachuteContainModuleData.ParachuteOpenDist': ("Distance fallen before the chute opens.", "30"),
'ParachuteContainModuleData.KillWhenLandingInWaterSlop': ("Tolerance (height) used to decide whether the unit landed in water and should be killed.", "10"),
'ParachuteContainModuleData.FreeFallDamagePercent': ("Percent of max health dealt to the passenger if it lands without the chute being open.", "50%"),
'ParachuteContainModuleData.ParachuteOpenSound': ("Sound played when the chute opens.", "ParachuteOpen"),
})

M['POWTruckBehavior'] = dict(
 sum="Container for the (cut) POW truck that collects surrendered prisoners.",
 body="""Container logic for the POW Truck: it holds captured (surrendered) enemy infantry and takes them back to a prison. The feature was not used in the shipped game but the module still works with POWTruckAIUpdate. It has only the OpenContain fields.""",
 ex="""Behavior = POWTruckBehavior ModuleTag_POW
  ContainMax = 8
End""")

M['PrisonBehavior'] = dict(
 sum="Prison container that holds captured prisoners and can show them walking in a prison yard.",
 body="""Container used for the (cut) prison structure. Prisoners delivered by a POW truck are contained; with <b>ShowPrisoners</b> = Yes they are displayed walking around the yard area defined by bones starting with <b>YardBonePrefix</b>. PropagandaCenterBehavior extends it.""",
 ex="""Behavior = PrisonBehavior ModuleTag_Prison
  ContainMax     = 20
  ShowPrisoners  = Yes
  YardBonePrefix = PrisonYard
End""")
F.update({
'PrisonBehaviorModuleData.ShowPrisoners': ("If Yes, contained prisoners are visible walking inside the prison yard.", "Yes"),
'PrisonBehaviorModuleData.YardBonePrefix': ("Bone name prefix of the bones that outline the yard area.", "PrisonYard"),
})

M['PropagandaCenterBehavior'] = dict(
 sum="Prison variant that brainwashes held prisoners so they join the owner's side after a delay.",
 body="""Extends PrisonBehavior: prisoners kept inside for <b>BrainwashDuration</b> are converted to the owning player's team and released as friendly units.""",
 ex="""Behavior = PropagandaCenterBehavior ModuleTag_Brainwash
  ContainMax        = 10
  BrainwashDuration = 30000
End""")
F.update({'PropagandaCenterBehaviorModuleData.BrainwashDuration': ("Time (ms) a prisoner must spend inside before being converted.", "30000")})

M['PropagandaTowerBehavior'] = dict(
 sum="Propaganda aura: periodically heals and boosts nearby friendly units (China Speaker Tower, Overlord Propaganda).",
 body="""Every <b>DelayBetweenUpdates</b> it scans <b>Radius</b> and toggles the propaganda effect on friendly units in range: they heal <b>HealPercentEachSecond</b> of their max health per second and receive the propaganda weapon-bonus/rate-of-fire effect. When the player has <b>UpgradeRequired</b> (Subliminal Messaging), <b>UpgradedHealPercentEachSecond</b> and <b>UpgradedPulseFX</b> are used instead. Units that leave the radius lose the effect at the next scan. Also handles the effect being removed when the tower dies or is disabled.""",
 ex="""Behavior = PropagandaTowerBehavior ModuleTag_Propaganda
  Radius                       = 150
  DelayBetweenUpdates          = 2000
  HealPercentEachSecond        = 1%
  UpgradedHealPercentEachSecond= 2%
  PulseFX                      = FX_PropagandaPulse
  UpgradeRequired              = Upgrade_ChinaSubliminalMessaging
  UpgradedPulseFX              = FX_PropagandaUpgradedPulse
  AffectsSelf                  = No
End""")
F.update({
'PropagandaTowerBehaviorModuleData.Radius': ("Radius of the aura.", "150"),
'PropagandaTowerBehaviorModuleData.DelayBetweenUpdates': ("Time (ms) between scans that apply/remove the effect.", "2000"),
'PropagandaTowerBehaviorModuleData.HealPercentEachSecond': ("Percent of max health healed per second for affected units.", "1%"),
'PropagandaTowerBehaviorModuleData.UpgradedHealPercentEachSecond': ("Heal rate used once UpgradeRequired is owned.", "2%"),
'PropagandaTowerBehaviorModuleData.PulseFX': ("FXList played each scan.", "FX_PropagandaPulse"),
'PropagandaTowerBehaviorModuleData.UpgradeRequired': ("Upgrade name that switches the tower to its upgraded heal rate and FX.", "Upgrade_ChinaSubliminalMessaging"),
'PropagandaTowerBehaviorModuleData.UpgradedPulseFX': ("FXList played each scan once upgraded.", "FX_PropagandaUpgradedPulse"),
'PropagandaTowerBehaviorModuleData.AffectsSelf': ("If Yes, the tower itself also receives the effect.", "No"),
})

M['BunkerBusterBehavior'] = dict(
 sum="Bunker-buster bomb: on detonation kills or ejects the occupants of the struck garrison/tunnel/bunker.",
 body="""Put on the bomb projectile. When it dies it looks for the container it hit (garrisoned building, tunnel, bunker…). If the player has <b>UpgradeRequired</b> (or it is empty), all occupants are kicked out and damaged by <b>OccupantDamageWeaponTemplate</b> (usually lethal). It plays <b>CrashThroughBunkerFX</b> repeatedly while smashing through, <b>DetonationFX</b> on detonation, fires <b>ShockwaveWeaponTemplate</b> and shakes the camera (<b>SeismicEffectRadius/Magnitude</b>).""",
 ex="""Behavior = BunkerBusterBehavior ModuleTag_BunkerBuster
  UpgradeRequired             = Upgrade_AmericaBunkerBusters
  DetonationFX                = FX_BunkerBusterExplosion
  CrashThroughBunkerFX        = FX_BunkerBusterCrash
  CrashThroughBunkerFXFrequency = 200
  SeismicEffectRadius         = 140
  SeismicEffectMagnitude      = 6
  ShockwaveWeaponTemplate     = BunkerBusterShockwave
  OccupantDamageWeaponTemplate= BunkerBusterOccupantDamage
End""")
F.update({
'BunkerBusterBehaviorModuleData.UpgradeRequired': ("Upgrade needed for the bomb to kill garrisoned occupants. Empty = always.", "Upgrade_AmericaBunkerBusters"),
'BunkerBusterBehaviorModuleData.DetonationFX': ("FXList played on detonation.", "FX_BunkerBusterExplosion"),
'BunkerBusterBehaviorModuleData.CrashThroughBunkerFX': ("FXList played repeatedly while the bomb smashes through the structure.", "FX_BunkerBusterCrash"),
'BunkerBusterBehaviorModuleData.CrashThroughBunkerFXFrequency': ("How often (ms) the crash FX repeats.", "200"),
'BunkerBusterBehaviorModuleData.SeismicEffectRadius': ("Radius of the ground-shake effect.", "140"),
'BunkerBusterBehaviorModuleData.SeismicEffectMagnitude': ("Strength of the ground-shake effect.", "6"),
'BunkerBusterBehaviorModuleData.ShockwaveWeaponTemplate': ("Weapon fired at the impact point, mainly for a shockwave visual/push.", "BunkerBusterShockwave"),
'BunkerBusterBehaviorModuleData.OccupantDamageWeaponTemplate': ("Weapon whose damage is applied to each occupant as it is ejected.", "BunkerBusterOccupantDamage"),
})

M['FireWeaponWhenDamagedBehavior'] = dict(
 sum="Fires a weapon when the object is damaged (reaction) or continuously, chosen by current damage state.",
 body="""Two sets of weapons indexed by body damage state (Pristine / Damaged / ReallyDamaged / Rubble). A <b>ReactionWeapon*</b> is fired each time the object takes a hit that passes the <b>DamageTypes</b> filter and is at least <b>DamageAmount</b>. A <b>ContinuousWeapon*</b> is fired every time it is ready (reload permitting) regardless of hits — e.g. a burning, leaking effect that gets worse as the object is damaged. As an upgrade module it can be gated with TriggeredBy or be always on with StartsActive = Yes.""",
 ex="""Behavior = FireWeaponWhenDamagedBehavior ModuleTag_Leak
  StartsActive                  = Yes
  ReactionWeaponPristine        = SmallSparkWeapon
  ReactionWeaponDamaged         = SparkWeapon
  ContinuousWeaponReallyDamaged = ToxicLeakWeapon
  DamageTypes                   = ALL -HEALING
  DamageAmount                  = 10
End""")
F.update({
'FireWeaponWhenDamagedBehaviorModuleData.StartsActive': ("If Yes, active from creation without an upgrade.", "Yes"),
'FireWeaponWhenDamagedBehaviorModuleData.ReactionWeaponPristine': ("Weapon fired when hit while in the PRISTINE damage state.", "SmallSparkWeapon"),
'FireWeaponWhenDamagedBehaviorModuleData.ReactionWeaponDamaged': ("Weapon fired when hit while DAMAGED.", "SparkWeapon"),
'FireWeaponWhenDamagedBehaviorModuleData.ReactionWeaponReallyDamaged': ("Weapon fired when hit while REALLYDAMAGED.", "BigSparkWeapon"),
'FireWeaponWhenDamagedBehaviorModuleData.ReactionWeaponRubble': ("Weapon fired when hit while RUBBLE.", ""),
'FireWeaponWhenDamagedBehaviorModuleData.ContinuousWeaponPristine': ("Weapon fired continuously while PRISTINE.", ""),
'FireWeaponWhenDamagedBehaviorModuleData.ContinuousWeaponDamaged': ("Weapon fired continuously while DAMAGED.", ""),
'FireWeaponWhenDamagedBehaviorModuleData.ContinuousWeaponReallyDamaged': ("Weapon fired continuously while REALLYDAMAGED.", "ToxicLeakWeapon"),
'FireWeaponWhenDamagedBehaviorModuleData.ContinuousWeaponRubble': ("Weapon fired continuously while RUBBLE.", ""),
'FireWeaponWhenDamagedBehaviorModuleData.DamageTypes': ("Damage types that trigger the reaction weapons (ALL / NONE / +TYPE / -TYPE).", "ALL -HEALING"),
'FireWeaponWhenDamagedBehaviorModuleData.DamageAmount': ("Minimum damage of a single hit needed to trigger a reaction weapon.", "10"),
})

M['FireWeaponWhenDeadBehavior'] = dict(
 sum="Fires a weapon at the object's position when it dies (death explosions, upgrade-gated).",
 body="""A die module that fires <b>DeathWeapon</b> at the object's location when it dies with a matching death type. Because it is also an upgrade module, the death weapon can be enabled only after an upgrade (e.g. GLA Demo Trucks/Terrorists with the Demolitions upgrade explode for everyone) by using TriggeredBy with StartsActive = No.""",
 ex="""Behavior = FireWeaponWhenDeadBehavior ModuleTag_DeathBomb
  StartsActive = No
  TriggeredBy  = Upgrade_GLADemolitions
  DeathWeapon  = DemolitionsDeathWeapon
  DeathTypes   = ALL -CRUSHED -SPLATTED
End""")
F.update({
'FireWeaponWhenDeadBehaviorModuleData.StartsActive': ("If Yes the death weapon is active from creation; otherwise it waits for TriggeredBy.", "No"),
'FireWeaponWhenDeadBehaviorModuleData.DeathWeapon': ("Weapon fired at the object's position when it dies.", "DemolitionsDeathWeapon"),
})

M['GenerateMinefieldBehavior'] = dict(
 sum="Creates a field of mine objects around the object (on creation, on upgrade or on death).",
 body="""Spawns objects of type <b>MineName</b> around the object, either filling a circle/footprint at <b>MinesPerSquareFoot</b> density or only along the border (<b>BorderOnly</b>, <b>SmartBorder</b> hugs the footprint shape). By default it fires when its upgrade triggers (e.g. the Mines/EMP Mines upgrades for USA/China buildings); with <b>GenerateOnlyOnDeath</b> it waits for death (e.g. a mine-dropping projectile). If <b>Upgradable</b> = Yes and the <b>UpgradedTriggeredBy</b> upgrade is bought later, the existing mines are replaced by <b>UpgradedMineName</b>.""",
 ex="""Behavior = GenerateMinefieldBehavior ModuleTag_Mines
  TriggeredBy          = Upgrade_ChinaLandMines
  MineName             = ChinaStandardMine
  UpgradedMineName     = ChinaEMPMine
  UpgradedTriggeredBy  = Upgrade_ChinaEMPMines
  Upgradable           = Yes
  SmartBorder          = Yes
  SmartBorderSkipInterior = Yes
  DistanceAroundObject = 20
  GenerationFX         = FX_MinesDeploy
End""")
F.update({
'GenerateMinefieldBehaviorModuleData.MineName': ("Object template of each mine.", "ChinaStandardMine"),
'GenerateMinefieldBehaviorModuleData.UpgradedMineName': ("Mine template used after UpgradedTriggeredBy is obtained (requires Upgradable = Yes).", "ChinaEMPMine"),
'GenerateMinefieldBehaviorModuleData.UpgradedTriggeredBy': ("Upgrade that swaps existing mines to UpgradedMineName.", "Upgrade_ChinaEMPMines"),
'GenerateMinefieldBehaviorModuleData.GenerationFX': ("FXList played when the minefield is generated.", "FX_MinesDeploy"),
'GenerateMinefieldBehaviorModuleData.DistanceAroundObject': ("Distance outside the object's footprint where the mines are placed.", "20"),
'GenerateMinefieldBehaviorModuleData.MinesPerSquareFoot': ("Mine density when filling an area.", "0.01"),
'GenerateMinefieldBehaviorModuleData.GenerateOnlyOnDeath': ("If Yes, the minefield is created when the object dies instead of on upgrade.", "No"),
'GenerateMinefieldBehaviorModuleData.BorderOnly': ("If Yes (default) mines are placed only along the border ring.", "Yes"),
'GenerateMinefieldBehaviorModuleData.SmartBorder': ("If Yes, the border follows the object's actual footprint shape instead of a circle.", "Yes"),
'GenerateMinefieldBehaviorModuleData.SmartBorderSkipInterior': ("With SmartBorder, don't also fill the interior.", "Yes"),
'GenerateMinefieldBehaviorModuleData.AlwaysCircular': ("Force a circular field even for rectangular footprints.", "No"),
'GenerateMinefieldBehaviorModuleData.Upgradable': ("Allow the mines to be upgraded by UpgradedTriggeredBy.", "Yes"),
'GenerateMinefieldBehaviorModuleData.RandomJitter': ("Random position jitter applied to each mine, as a percent of spacing.", "10%"),
'GenerateMinefieldBehaviorModuleData.SkipIfThisMuchUnderStructure': ("Don't place a mine if at least this fraction of it would overlap a structure.", "33%"),
})

M['ParkingPlaceBehavior'] = dict(
 sum="Airfield parking: manages parking spaces, runways, takeoff/landing, reloading and healing of aircraft.",
 body="""Used on airfields. It defines a grid of parking spaces (<b>NumRows</b> × <b>NumCols</b>) using bones named RunwayStart/End, Parking, Prep etc. Aircraft produced here park in a free space; returning jets land, taxi to their spot and are reloaded and healed (<b>HealAmountPerSecond</b>, or <b>TimeForFullHeal</b>). <b>HasRunways</b> gives each column its own runway, <b>ParkInHangars</b> uses the production bones instead of real spots (used by helipads), and <b>ExtraHealAmount4Helicopters</b> adds bonus heal for helicopters.""",
 ex="""Behavior = ParkingPlaceBehavior ModuleTag_Parking
  NumRows        = 2
  NumCols        = 2
  HasRunways     = Yes
  ApproachHeight = 50
  HealAmountPerSecond = 10
End""")
F.update({
'ParkingPlaceBehaviorModuleData.NumRows': ("Number of parking rows.", "2"),
'ParkingPlaceBehaviorModuleData.NumCols': ("Number of parking columns (one runway per column with HasRunways).", "2"),
'ParkingPlaceBehaviorModuleData.ApproachHeight': ("Height at which aircraft approach before landing.", "50"),
'ParkingPlaceBehaviorModuleData.LandingDeckHeightOffset': ("Height offset of the landing surface (for elevated decks).", "0"),
'ParkingPlaceBehaviorModuleData.HasRunways': ("If Yes, each column has a runway in front of it that jets use to take off/land.", "Yes"),
'ParkingPlaceBehaviorModuleData.ParkInHangars': ("If Yes, aircraft park at the hangar/production spot rather than a real parking place.", "No"),
'ParkingPlaceBehaviorModuleData.HealAmountPerSecond': ("Health restored per second to parked aircraft.", "10"),
'ParkingPlaceBehaviorModuleData.ExtraHealAmount4Helicopters': ("Additional heal per second for helicopters parked here.", "0"),
'ParkingPlaceBehaviorModuleData.TimeForFullHeal': ("Alternative to HealAmountPerSecond: time (ms) to fully heal a parked aircraft.", "0"),
})

# AIUpdateModuleData shared
F.update({
'AIUpdateModuleData.Turret': ("Begins a Turret sub-block (terminated by End) describing the main turret: turn/pitch rates, which weapon slots it controls, idle scanning, etc. See the Turret sub-block fields.", "(sub-block)"),
'AIUpdateModuleData.AltTurret': ("Begins a second turret sub-block for units with two independent turrets.", "(sub-block)"),
'AIUpdateModuleData.AutoAcquireEnemiesWhenIdle': ("When idle, automatically attack enemies in range. Flags: YES, NO, STEALTHED (also attack stealthed units it can see), ATTACK_BUILDINGS, NOTWHILEATTACKING.", "Yes"),
'AIUpdateModuleData.MoodAttackCheckRate': ("How often (ms) an idle unit re-checks for enemies to auto-attack.", "2000"),
'AIUpdateModuleData.SurrenderDuration': ("How long (ms) a unit stays surrendered (unused POW feature).", "120000"),
'AIUpdateModuleData.ForbidPlayerCommands': ("If Yes, the player cannot give this unit orders; only the AI/scripts can (used for slaved drones).", "No"),
'AIUpdateModuleData.TurretsLinked': ("If Yes, Turret and AltTurret aim and fire together at the same target.", "No"),
'TurretAIData.TurretTurnRate': ("Turret rotation speed (deg/sec).", "60"),
'TurretAIData.TurretPitchRate': ("Turret pitch speed (deg/sec), needs AllowsPitch.", "60"),
'TurretAIData.NaturalTurretAngle': ("Resting yaw angle of the turret relative to the chassis.", "0"),
'TurretAIData.NaturalTurretPitch': ("Resting pitch angle.", "0"),
'TurretAIData.FirePitch': ("If non-zero, the turret is considered on target when at this fixed pitch (e.g. artillery firing at a set elevation).", "0"),
'TurretAIData.MinPhysicalPitch': ("Lowest pitch allowed (negative lets a high turret aim downward).", "0"),
'TurretAIData.GroundUnitPitch': ("Minimum pitch used when firing at ground units, giving the shot an arc.", "0"),
'TurretAIData.TurretFireAngleSweep': ("Makes the turret sweep while firing: <WeaponSlot> <angle>. Used by e.g. flame/gattling spray.", "PRIMARY 20"),
'TurretAIData.TurretSweepSpeedModifier': ("Speed multiplier of the fire sweep: <WeaponSlot> <multiplier>.", "PRIMARY 2.0"),
'TurretAIData.ControlledWeaponSlots': ("Weapon slots this turret aims (PRIMARY SECONDARY TERTIARY).", "PRIMARY"),
'TurretAIData.AllowsPitch': ("If Yes the turret can pitch up/down as well as rotate.", "No"),
'TurretAIData.InterTurretDelay': ("Delay (ms) between multiple turrets firing (battleship special case).", "0"),
'TurretAIData.MinIdleScanAngle': ("Minimum random angle the turret turns while idle-scanning.", "0"),
'TurretAIData.MaxIdleScanAngle': ("Maximum random angle the turret turns while idle-scanning.", "0"),
'TurretAIData.MinIdleScanInterval': ("Minimum time (ms) between idle scans.", "9999999"),
'TurretAIData.MaxIdleScanInterval': ("Maximum time (ms) between idle scans.", "9999999"),
'TurretAIData.RecenterTime': ("Time (ms) without targets after which the turret recentres to its natural angle.", "2000"),
'TurretAIData.InitiallyDisabled': ("If Yes, the turret starts disabled (manually enabled by code, e.g. special abilities).", "No"),
'TurretAIData.FiresWhileTurning': ("If Yes, the turret may fire while still rotating toward the target.", "No"),
})

M['FlightDeckBehavior'] = dict(
 sum="Aircraft carrier flight deck: spawns, launches (catapults), lands, parks and replaces carrier aircraft.",
 body="""An AIUpdate-based module for the aircraft carrier. It keeps a squadron of <b>PayloadTemplate</b> aircraft, parked in named deck spaces. Each runway (1 or 2) has its own list of parking-space bones (RunwayNSpaces), a takeoff strip and landing strip (start/end bone pair), taxi bones and creation bones, plus a catapult particle system. Destroyed aircraft are rebuilt after <b>ReplacementDelay</b>; launches come in waves (<b>LaunchWaveDelay</b>), with ramp animation delays (<b>LaunchRampDelay</b>, <b>LowerRampDelay</b>, <b>CatapultFireDelay</b>). Parked aircraft heal at <b>HealAmountPerSecond</b>.""",
 ex="""Behavior = FlightDeckBehavior ModuleTag_FlightDeck
  NumRunways          = 1
  NumSpacesPerRunway  = 4
  Runway1Spaces       = Space1 Space2 Space3 Space4
  Runway1Takeoff      = RunwayStart1 RunwayEnd1
  Runway1Landing      = LandingStart1 LandingEnd1
  Runway1Taxi         = Taxi1 Taxi2
  Runway1Creation     = Create1
  Runway1CatapultSystem = CatapultSteam
  ApproachHeight      = 60
  HealAmountPerSecond = 10
  PayloadTemplate     = CarrierJet
  ReplacementDelay    = 15000
  LaunchWaveDelay     = 2000
  LaunchRampDelay     = 500
  LowerRampDelay      = 500
  CatapultFireDelay   = 300
End""")
for n in ('1', '2'):
    F.update({
    f'FlightDeckBehaviorModuleData.Runway{n}Spaces': (f"Bone names of the parking spaces belonging to runway {n}, in order.", "Space1 Space2 Space3 Space4"),
    f'FlightDeckBehaviorModuleData.Runway{n}Takeoff': (f"Start and end bone of runway {n}'s takeoff strip.", f"RunwayStart{n} RunwayEnd{n}"),
    f'FlightDeckBehaviorModuleData.Runway{n}Landing': (f"Start and end bone of runway {n}'s landing strip.", f"LandingStart{n} LandingEnd{n}"),
    f'FlightDeckBehaviorModuleData.Runway{n}Taxi': (f"Taxi waypoint bones for runway {n}.", "Taxi1 Taxi2"),
    f'FlightDeckBehaviorModuleData.Runway{n}Creation': (f"Bones where new aircraft for runway {n} are created.", "Create1"),
    f'FlightDeckBehaviorModuleData.Runway{n}CatapultSystem': (f"Particle system played when runway {n}'s catapult launches a plane.", "CatapultSteam"),
    })
F.update({
'FlightDeckBehaviorModuleData.NumRunways': ("Number of runways (1 or 2).", "1"),
'FlightDeckBehaviorModuleData.NumSpacesPerRunway': ("Parking spaces per runway.", "4"),
'FlightDeckBehaviorModuleData.ApproachHeight': ("Height at which aircraft approach before landing on the deck.", "60"),
'FlightDeckBehaviorModuleData.LandingDeckHeightOffset': ("Height of the deck surface above the carrier's origin.", "20"),
'FlightDeckBehaviorModuleData.HealAmountPerSecond': ("Health restored per second to parked aircraft.", "10"),
'FlightDeckBehaviorModuleData.ParkingCleanupPeriod': ("How often (ms) the deck reorganises parking assignments.", "5000"),
'FlightDeckBehaviorModuleData.HumanFollowPeriod': ("How often (ms) carrier aircraft re-evaluate following the human player's attack orders.", "1000"),
'FlightDeckBehaviorModuleData.PayloadTemplate': ("Object template of the aircraft the carrier maintains.", "CarrierJet"),
'FlightDeckBehaviorModuleData.ReplacementDelay': ("Time (ms) to build a replacement for a lost aircraft.", "15000"),
'FlightDeckBehaviorModuleData.DockAnimationDelay': ("Delay (ms) for the docking animation.", "0"),
'FlightDeckBehaviorModuleData.LaunchWaveDelay': ("Delay (ms) between launch waves.", "2000"),
'FlightDeckBehaviorModuleData.LaunchRampDelay': ("Delay (ms) for the launch ramp to rise.", "500"),
'FlightDeckBehaviorModuleData.LowerRampDelay': ("Delay (ms) for the ramp to lower after launch.", "500"),
'FlightDeckBehaviorModuleData.CatapultFireDelay': ("Delay (ms) between the plane being ready and the catapult firing.", "300"),
})

M['PoisonedBehavior'] = dict(
 sum="Reacts to POISON damage by re-damaging the object periodically for a while (damage over time).",
 body="""When the object takes POISON damage, it becomes poisoned: every <b>PoisonDamageInterval</b> it takes the same amount of damage again (as UNRESISTABLE so it doesn't re-trigger), until <b>PoisonDuration</b> has passed since the last poison dose. New doses refresh the duration. Healing cures the poison. Infantry usually have it for GLA toxin weapons.""",
 ex="""Behavior = PoisonedBehavior ModuleTag_Poisoned
  PoisonDamageInterval = 100
  PoisonDuration       = 3000
End""")
F.update({
'PoisonedBehaviorModuleData.PoisonDamageInterval': ("Time (ms) between repeated poison damage ticks.", "100"),
'PoisonedBehaviorModuleData.PoisonDuration': ("How long (ms) the poisoning lasts after the most recent poison hit.", "3000"),
})

M['RebuildHoleBehavior'] = dict(
 sum="GLA rebuild hole: spawns a worker that automatically reconstructs the destroyed building.",
 body="""Used on the 'hole' object created by RebuildHoleExposeDie when a GLA structure dies. After <b>WorkerRespawnDelay</b> it spawns a <b>WorkerObjectName</b> that rebuilds the original structure on the spot for free. The hole regenerates <b>HoleHealthRegen%PerSecond</b> of its health; destroying the hole (or the worker, which is respawned) prevents the rebuild.""",
 ex="""Behavior = RebuildHoleBehavior ModuleTag_Rebuild
  WorkerObjectName        = GLAInfantryWorker
  WorkerRespawnDelay      = 5000
  HoleHealthRegen%PerSecond = 10%
End""")
F.update({
'RebuildHoleBehaviorModuleData.WorkerObjectName': ("Object template of the worker that rebuilds the structure.", "GLAInfantryWorker"),
'RebuildHoleBehaviorModuleData.WorkerRespawnDelay': ("Time (ms) before a (new) worker spawns.", "5000"),
'RebuildHoleBehaviorModuleData.HoleHealthRegen%PerSecond': ("Percent of max health the hole regains per second.", "10%"),
})

M['SupplyWarehouseCripplingBehavior'] = dict(
 sum="Supply warehouse disables itself when REALLYDAMAGED and slowly self-heals.",
 body="""When the warehouse reaches the REALLYDAMAGED state it becomes disabled (can't give supplies). It starts healing <b>SelfHealAmount</b> every <b>SelfHealDelay</b> once it has not been damaged for <b>SelfHealSupression</b>, and re-enables when back above really-damaged.""",
 ex="""Behavior = SupplyWarehouseCripplingBehavior ModuleTag_Cripple
  SelfHealSupression = 3000
  SelfHealDelay      = 1000
  SelfHealAmount     = 5
End""")
F.update({
'SupplyWarehouseCripplingBehaviorModuleData.SelfHealSupression': ("Time (ms) since last damage before self-healing may start.", "3000"),
'SupplyWarehouseCripplingBehaviorModuleData.SelfHealDelay': ("Time (ms) between heal ticks.", "1000"),
'SupplyWarehouseCripplingBehaviorModuleData.SelfHealAmount': ("Health restored per tick.", "5"),
})

M['TechBuildingBehavior'] = dict(
 sum="Tech building: captured neutral structures that pulse an FX while owned and revert to neutral when destroyed.",
 body="""Used on capturable tech buildings (oil derrick, hospital, artillery platform…). While owned by a player it plays <b>PulseFX</b> every <b>PulseFXRate</b>. When it 'dies' it does not get destroyed: it goes back to neutral (rubble state) and can be recaptured after repair.""",
 ex="""Behavior = TechBuildingBehavior ModuleTag_Tech
  PulseFX     = FX_TechBuildingPulse
  PulseFXRate = 2000
End""")
F.update({
'TechBuildingBehaviorModuleData.PulseFX': ("FXList played periodically while the building is owned.", "FX_TechBuildingPulse"),
'TechBuildingBehaviorModuleData.PulseFXRate': ("Interval (ms) between pulses.", "2000"),
})

M['MinefieldBehavior'] = dict(
 sum="Land mine logic: detonates on contact, supports 'virtual' multi-mine objects, regeneration and creator-linked decay.",
 body="""Put on mine objects. When an object whose relationship matches <b>DetonatedBy</b> collides with it, the mine fires <b>DetonationWeapon</b>. A single mine object can represent several mines (<b>NumVirtualMines</b>): each detonation uses one and the mine is destroyed when none are left; its health also represents remaining mines. <b>Regenerates</b> mines recreate themselves (e.g. from a regenerating mine upgrade). <b>StopsRegenAfterCreatorDies</b>/<b>DegenPercentPerSecondAfterCreatorDies</b> make mines wither when the building that generated them is gone. Workers/dozers don't trigger unless <b>WorkersDetonate</b>.""",
 ex="""Behavior = MinefieldBehavior ModuleTag_Mine
  DetonationWeapon = LandMineWeapon
  DetonatedBy      = ENEMIES NEUTRAL
  NumVirtualMines  = 3
  Regenerates      = No
  WorkersDetonate  = No
  StopsRegenAfterCreatorDies = Yes
  DegenPercentPerSecondAfterCreatorDies = 5%
End""")
F.update({
'MinefieldBehaviorModuleData.DetonationWeapon': ("Weapon fired when the mine is triggered.", "LandMineWeapon"),
'MinefieldBehaviorModuleData.DetonatedBy': ("Relationships that trigger the mine: ALLIES, ENEMIES, NEUTRAL.", "ENEMIES NEUTRAL"),
'MinefieldBehaviorModuleData.StopsRegenAfterCreatorDies': ("If Yes, regeneration stops once the object that created the mine is dead.", "Yes"),
'MinefieldBehaviorModuleData.Regenerates': ("If Yes the mine regenerates and cannot be killed normally (only disarmed/detonated).", "No"),
'MinefieldBehaviorModuleData.WorkersDetonate': ("If Yes, workers and dozers also trigger it.", "No"),
'MinefieldBehaviorModuleData.CreatorDeathCheckRate': ("How often (ms) to check whether the creator is still alive.", "1000"),
'MinefieldBehaviorModuleData.ScootFromStartingPointTime': ("If non-zero, the mine slides from its spawn point to its final position over this time (ms) (used by mine-laying projectiles).", "0"),
'MinefieldBehaviorModuleData.NumVirtualMines': ("Number of mines this object represents.", "3"),
'MinefieldBehaviorModuleData.RepeatDetonateMoveThresh': ("Distance an object must move on the mine before it can trigger another detonation.", "1.0"),
'MinefieldBehaviorModuleData.DegenPercentPerSecondAfterCreatorDies': ("Percent of max health lost per second once the creator died.", "5%"),
'MinefieldBehaviorModuleData.CreationList': ("OCL created when the mine detonates.", "OCL_MineDebris"),
})

M['BattleBusSlowDeathBehavior'] = dict(
 sum="Battle Bus two-stage death: first 'death' throws it in the air and leaves a usable empty hulk, second death is final.",
 body="""SlowDeathBehavior variant for the GLA Battle Bus. On the first (intentional) death the bus is thrown into the air with <b>ThrowForce</b>, plays <b>FXStartUndeath</b>/<b>OCLStartUndeath</b>, hits passengers for <b>PercentDamageToPassengers</b>, lands (<b>FXHitGround</b>/<b>OCLHitGround</b>) and becomes an immobile bunker hulk that its passengers can still fire from. If the hulk is empty for <b>EmptyHulkDestructionDelay</b> it kills itself. The second death uses the normal SlowDeath behaviour. Combine with UndeadBody.""",
 ex="""Behavior = BattleBusSlowDeathBehavior ModuleTag_BusDeath
  DeathTypes             = ALL
  FXStartUndeath         = FX_BattleBusStartUndeath
  OCLStartUndeath        = OCL_BattleBusDebris
  FXHitGround            = FX_BattleBusHitGround
  ThrowForce             = 10
  PercentDamageToPassengers = 50%
  EmptyHulkDestructionDelay = 3000
  DestructionDelay       = 500
End""")
F.update({
'BattleBusSlowDeathBehaviorModuleData.FXStartUndeath': ("FXList when the bus is thrown into the air (first death).", "FX_BattleBusStartUndeath"),
'BattleBusSlowDeathBehaviorModuleData.OCLStartUndeath': ("OCL at the start of the throw.", "OCL_BattleBusDebris"),
'BattleBusSlowDeathBehaviorModuleData.FXHitGround': ("FXList when it lands.", "FX_BattleBusHitGround"),
'BattleBusSlowDeathBehaviorModuleData.OCLHitGround': ("OCL when it lands.", ""),
'BattleBusSlowDeathBehaviorModuleData.ThrowForce': ("How hard the bus is thrown upward.", "10"),
'BattleBusSlowDeathBehaviorModuleData.PercentDamageToPassengers': ("Damage to passengers (percent of their max health) when thrown.", "50%"),
'BattleBusSlowDeathBehaviorModuleData.EmptyHulkDestructionDelay': ("If non-zero, the hulk kills itself after being empty this long (ms).", "3000"),
})

M['JetSlowDeathBehavior'] = dict(
 sum="Jet crash sequence: rolls and falls from the sky, secondary explosion, hits the ground, final blow-up.",
 body="""SlowDeathBehavior for airplanes. If killed on the ground (parked) it uses <b>FXOnGroundDeath</b>/<b>OCLOnGroundDeath</b> and dies normally. If killed in the air, it plays the initial death FX/OCL, keeps flying while rolling (<b>RollRate</b>, changing by <b>RollRateDelta</b>) and losing lift (<b>FallHowFast</b>), plays a secondary explosion after <b>DelaySecondaryFromInitialDeath</b>, then on hitting the ground plays the hit-ground effects, pitches (<b>PitchRate</b>) and after <b>DelayFinalBlowUpFromHitGround</b> does the final explosion. <b>DeathLoopSound</b> plays throughout.""",
 ex="""Behavior = JetSlowDeathBehavior ModuleTag_JetDeath
  DeathTypes          = ALL
  FXOnGroundDeath     = FX_JetOnGroundDeath
  OCLOnGroundDeath    = OCL_JetOnGroundDeath
  FXInitialDeath      = FX_JetInitialDeath
  DelaySecondaryFromInitialDeath = 1000
  FXSecondary         = FX_JetSecondaryExplosion
  FXHitGround         = FX_JetHitGround
  OCLHitGround        = OCL_JetHitGroundDebris
  DelayFinalBlowUpFromHitGround = 1000
  FXFinalBlowUp       = FX_JetFinalBlowUp
  DeathLoopSound      = JetDeathLoop
  RollRate            = 0.1
  RollRateDelta       = 98%
  PitchRate           = 0.05
  FallHowFast         = 40%
End""")
F.update({
'JetSlowDeathBehaviorModuleData.FXOnGroundDeath': ("FXList when the jet is destroyed while on the ground.", "FX_JetOnGroundDeath"),
'JetSlowDeathBehaviorModuleData.OCLOnGroundDeath': ("OCL when destroyed on the ground.", "OCL_JetOnGroundDeath"),
'JetSlowDeathBehaviorModuleData.FXInitialDeath': ("FXList at the moment of death in the air.", "FX_JetInitialDeath"),
'JetSlowDeathBehaviorModuleData.OCLInitialDeath': ("OCL at the moment of death in the air.", ""),
'JetSlowDeathBehaviorModuleData.DelaySecondaryFromInitialDeath': ("Time (ms) from initial death to the secondary event.", "1000"),
'JetSlowDeathBehaviorModuleData.FXSecondary': ("FXList of the secondary mid-air explosion.", "FX_JetSecondaryExplosion"),
'JetSlowDeathBehaviorModuleData.OCLSecondary': ("OCL of the secondary event.", ""),
'JetSlowDeathBehaviorModuleData.FXHitGround': ("FXList when the wreck hits the ground.", "FX_JetHitGround"),
'JetSlowDeathBehaviorModuleData.OCLHitGround': ("OCL when the wreck hits the ground.", "OCL_JetHitGroundDebris"),
'JetSlowDeathBehaviorModuleData.DelayFinalBlowUpFromHitGround': ("Time (ms) from hitting the ground to the final explosion.", "1000"),
'JetSlowDeathBehaviorModuleData.FXFinalBlowUp': ("FXList of the final explosion.", "FX_JetFinalBlowUp"),
'JetSlowDeathBehaviorModuleData.OCLFinalBlowUp': ("OCL of the final explosion.", ""),
'JetSlowDeathBehaviorModuleData.DeathLoopSound': ("Looping sound during the fall.", "JetDeathLoop"),
'JetSlowDeathBehaviorModuleData.RollRate': ("Initial roll speed while falling.", "0.1"),
'JetSlowDeathBehaviorModuleData.RollRateDelta': ("Per-frame multiplier on the roll rate (e.g. 98% slowly reduces it).", "98%"),
'JetSlowDeathBehaviorModuleData.PitchRate': ("Spin speed on the pitch axis after hitting the ground.", "0.05"),
'JetSlowDeathBehaviorModuleData.FallHowFast': ("Fraction of gravity used to reduce lift (how fast it drops).", "40%"),
})

M['RailroadBehavior'] = dict(
 sum="Train car / locomotive physics following a waypoint railroad track, crushing things in its path.",
 body="""A PhysicsBehavior subclass for trains. The locomotive (<b>IsLocomotive</b> = Yes) follows map waypoints whose names start with <b>PathPrefixName</b>, pulls carriages (<b>CarriageTemplateName</b>, repeatable, in order), accelerates up to <b>SpeedMax</b>, brakes at stations and waits <b>WaitAtStationTime</b>. Objects on the track are hit: above <b>KillSpeedMin</b> they are killed, with metal/meaty bounce sounds. Running/clickety-clack/whistle sounds are played while moving. If it derails, <b>CrashFXTemplateName</b> is used.""",
 ex="""Behavior = RailroadBehavior ModuleTag_Train
  IsLocomotive         = Yes
  PathPrefixName       = Railroad
  CarriageTemplateName = TrainBoxcar
  CarriageTemplateName = TrainBoxcar
  SpeedMax             = 4
  Acceleration         = 1.01
  Braking              = 0.99
  WaitAtStationTime    = 10000
  KillSpeedMin         = 1.0
  RunningSound         = TrainRunning
  WhistleSound         = TrainWhistle
End""")
F.update({
'RailroadBehaviorModuleData.PathPrefixName': ("Prefix of the map waypoints that form the track.", "Railroad"),
'RailroadBehaviorModuleData.CrashFXTemplateName': ("FX template used when the train crashes/derails.", "FX_TrainCrash"),
'RailroadBehaviorModuleData.IsLocomotive': ("Yes for the engine that drives the train; carriages use No.", "Yes"),
'RailroadBehaviorModuleData.CarriageTemplateName': ("Carriage object template attached behind the locomotive. Repeat for each carriage in order.", "TrainBoxcar"),
'RailroadBehaviorModuleData.BigMetalBounceSound': ("Sound when it hits a big metal object.", "TrainHitMetalBig"),
'RailroadBehaviorModuleData.SmallMetalBounceSound': ("Sound when it hits a small metal object.", "TrainHitMetalSmall"),
'RailroadBehaviorModuleData.MeatyBounceSound': ("Sound when it hits infantry.", "TrainHitMeaty"),
'RailroadBehaviorModuleData.RunningGarrisonSpeedMax': ("Max speed while carrying garrisoned units.", "1.0"),
'RailroadBehaviorModuleData.KillSpeedMin': ("Minimum speed at which objects struck by the train are killed.", "1.0"),
'RailroadBehaviorModuleData.SpeedMax': ("Top speed.", "4"),
'RailroadBehaviorModuleData.Acceleration': ("Per-frame speed multiplier while accelerating (>1).", "1.01"),
'RailroadBehaviorModuleData.Braking': ("Per-frame speed multiplier while braking (<1).", "0.99"),
'RailroadBehaviorModuleData.WaitAtStationTime': ("Time (ms) to wait at each station.", "10000"),
'RailroadBehaviorModuleData.RunningSound': ("Looping engine sound while moving.", "TrainRunning"),
'RailroadBehaviorModuleData.ClicketyClackSound': ("Rail clack sound.", "TrainClack"),
'RailroadBehaviorModuleData.WhistleSound': ("Whistle sound.", "TrainWhistle"),
'RailroadBehaviorModuleData.Friction': ("Rolling friction multiplier.", "0.97"),
})

M['SpawnBehavior'] = dict(
 sum="Creates and maintains a group of slave units (drones, Stinger soldiers, Angry Mob members) and replaces lost ones.",
 body="""Keeps <b>SpawnNumber</b> slaves of type <b>SpawnTemplateName</b> (repeat to cycle through several templates) alive. <b>InitialBurst</b> creates some immediately; after that one lost slave is replaced every <b>SpawnReplaceDelay</b>. Slaves normally attack what the master attacks unless <b>SlavesHaveFreeWill</b>. With <b>SpawnedRequireSpawner</b> the slaves die with the master (drones), <b>AggregateHealth</b> shows the combined health of all slaves on the master (Stinger Site), <b>ExitByBudding</b> creates new ones on top of existing ones (mob), <b>OneShot</b> spawns once only, <b>CanReclaimOrphans</b> adopts orphaned slaves. <b>PropagateDamageTypesToSlavesWhenExisting</b> forwards damage of the listed types to slaves instead of the master.""",
 ex="""Behavior = SpawnBehavior ModuleTag_Spawn
  SpawnNumber          = 3
  SpawnReplaceDelay    = 15000
  SpawnTemplateName    = GLAInfantryStingerSoldier
  OneShot              = No
  CanReclaimOrphans    = No
  AggregateHealth      = Yes
  SpawnedRequireSpawner= Yes
  InitialBurst         = 3
End""")
F.update({
'SpawnBehaviorModuleData.SpawnNumber': ("Number of slaves to maintain.", "3"),
'SpawnBehaviorModuleData.SpawnReplaceDelay': ("Time (ms) to replace one lost slave.", "15000"),
'SpawnBehaviorModuleData.OneShot': ("If Yes, spawn once and never replace.", "No"),
'SpawnBehaviorModuleData.CanReclaimOrphans': ("If Yes, orphaned slaves of the same type can be adopted instead of spawning new ones.", "No"),
'SpawnBehaviorModuleData.AggregateHealth': ("If Yes, the master's health bar shows the combined health of the slaves.", "Yes"),
'SpawnBehaviorModuleData.ExitByBudding': ("If Yes, new slaves are created on top of an existing slave rather than at the master.", "No"),
'SpawnBehaviorModuleData.SpawnTemplateName': ("Slave object template. Repeat to alternate between several templates.", "GLAInfantryStingerSoldier"),
'SpawnBehaviorModuleData.SpawnedRequireSpawner': ("If Yes, slaves are destroyed when the master dies.", "Yes"),
'SpawnBehaviorModuleData.PropagateDamageTypesToSlavesWhenExisting': ("Damage of these types taken by the master is redirected to slaves while any exist.", "NONE +SNIPER"),
'SpawnBehaviorModuleData.InitialBurst': ("Number of slaves created immediately at start, ignoring the delay.", "3"),
'SpawnBehaviorModuleData.SlavesHaveFreeWill': ("If Yes, slaves pick their own targets instead of copying the master's.", "No"),
})

M['DestroyDie'] = dict(
 sum="Default die module: simply removes the object from the world when it dies.",
 body="""The most basic die module — when the object dies (matching DeathTypes etc.) it is destroyed immediately with no corpse, FX or effects. Most objects that have no SlowDeathBehavior use this so that they actually go away. Only the DieMux filter fields apply.""",
 ex="""Behavior = DestroyDie ModuleTag_Destroy
  DeathTypes = ALL
End""")

M['FXListDie'] = dict(
 sum="Plays an FXList when the object dies (optionally only after an upgrade).",
 body="""A die module that plays <b>DeathFX</b> at the dying object, oriented to the object if <b>OrientToObject</b> = Yes. It's an upgrade module too: with StartsActive = No and TriggeredBy the death FX only plays once the upgrade is present.""",
 ex="""Behavior = FXListDie ModuleTag_DeathFX
  DeathTypes     = ALL
  DeathFX        = FX_GenericInfantryDeath
  OrientToObject = Yes
End""")
F.update({
'FXListDieModuleData.StartsActive': ("If Yes (default) active without an upgrade.", "Yes"),
'FXListDieModuleData.DeathFX': ("FXList played on death.", "FX_GenericInfantryDeath"),
'FXListDieModuleData.OrientToObject': ("If Yes the FX is oriented like the object.", "Yes"),
})

M['CrushDie'] = dict(
 sum="Infantry/vehicle crushed death: plays crush sounds and sets the correct crushed model state.",
 body="""Handles CRUSHED deaths (e.g. a car run over by a tank). Depending on whether the crusher ran over the front, back or entire object, it sets FRONTCRUSHED/BACKCRUSHED model conditions and plays <b>FrontEndCrushSound</b>, <b>BackEndCrushSound</b> or <b>TotalCrushSound</b> with the given percent chance.""",
 ex="""Behavior = CrushDie ModuleTag_Crush
  DeathTypes            = NONE +CRUSHED +SPLATTED
  TotalCrushSound       = CarCrushTotal
  BackEndCrushSound     = CarCrushBack
  FrontEndCrushSound    = CarCrushFront
  TotalCrushSoundPercent    = 100
  BackEndCrushSoundPercent  = 50
  FrontEndCrushSoundPercent = 50
End""")
F.update({
'CrushDieModuleData.TotalCrushSound': ("Sound when fully crushed.", "CarCrushTotal"),
'CrushDieModuleData.BackEndCrushSound': ("Sound when the back end is crushed.", "CarCrushBack"),
'CrushDieModuleData.FrontEndCrushSound': ("Sound when the front end is crushed.", "CarCrushFront"),
'CrushDieModuleData.TotalCrushSoundPercent': ("Chance (0-100) the total crush sound plays.", "100"),
'CrushDieModuleData.BackEndCrushSoundPercent': ("Chance (0-100) the back crush sound plays.", "50"),
'CrushDieModuleData.FrontEndCrushSoundPercent': ("Chance (0-100) the front crush sound plays.", "50"),
})

M['DamDie'] = dict(
 sum="Dam death: when the dam dies the water flood (map water wave) is released.",
 body="""Special-purpose die module for map dams: on death it enables the map's waveguide/flood objects so the water rushes out. The TimeForFullHeal field is parsed but not used by the logic.""",
 ex="""Behavior = DamDie ModuleTag_Dam
  DeathTypes = ALL
End""")
F.update({'DamDieModuleData.TimeForFullHeal': ("Parsed but unused.", "")})

M['CreateCrateDie'] = dict(
 sum="On death, may create a crate (money, veterancy, unit...) according to CrateData rules.",
 body="""References one or more crate definitions from Crate.ini via <b>CrateData</b> (repeatable). When the object dies, each crate definition checks its own conditions (killer, veterancy, chance…) and may spawn its crate object at the death location. Used for salvage and bounty crates.""",
 ex="""Behavior = CreateCrateDie ModuleTag_Crate
  DeathTypes = ALL
  CrateData  = SalvageCrateData
End""")
F.update({'CreateCrateDieModuleData.CrateData': ("Name of a CrateData definition (from Crate.ini) to evaluate on death. Repeatable.", "SalvageCrateData")})

M['CreateObjectDie'] = dict(
 sum="Creates an ObjectCreationList when the object dies (hulks, debris, replacement objects).",
 body="""On death, runs <b>CreationList</b> at the dying object. With <b>TransferPreviousHealth</b> the new object receives the health the old one had before the killing blow (used for 'transform on death'), and <b>TransferSelection</b> keeps it selected.""",
 ex="""Behavior = CreateObjectDie ModuleTag_Hulk
  DeathTypes   = ALL -CRUSHED
  CreationList = OCL_CrusaderHulk
End""")
F.update({
'CreateObjectDieModuleData.CreationList': ("ObjectCreationList created on death.", "OCL_CrusaderHulk"),
'CreateObjectDieModuleData.TransferPreviousHealth': ("Give the created object the health the dying object had before the killing hit.", "No"),
'CreateObjectDieModuleData.TransferSelection': ("If the dying object was selected, select the created object.", "No"),
})

M['EjectPilotDie'] = dict(
 sum="Ejects a pilot (veteran vehicles/aircraft) on death, using different OCLs in the air or on the ground.",
 body="""When a vehicle or aircraft with at least VETERAN rank dies (see VeterancyLevels), a pilot is created using <b>AirCreationList</b> when airborne (usually a parachute) or <b>GroundCreationList</b> on the ground. The pilot is invulnerable for <b>InvulnerableTime</b>. USA pilots can then enter empty vehicles to give them veterancy.""",
 ex="""Behavior = EjectPilotDie ModuleTag_EjectPilot
  DeathTypes         = ALL -CRUSHED -SPLATTED
  ExemptStatus       = HIJACKED
  VeterancyLevels    = ALL -REGULAR
  AirCreationList    = OCL_EjectPilotViaParachute
  GroundCreationList = OCL_EjectPilotOnGround
  InvulnerableTime   = 3000
End""")
F.update({
'EjectPilotDieModuleData.AirCreationList': ("OCL used when the object dies while airborne.", "OCL_EjectPilotViaParachute"),
'EjectPilotDieModuleData.GroundCreationList': ("OCL used when the object dies on the ground.", "OCL_EjectPilotOnGround"),
'EjectPilotDieModuleData.InvulnerableTime': ("Time (ms) the ejected pilot is invulnerable.", "3000"),
})

M['SpecialPowerCompletionDie'] = dict(
 sum="Tells the script engine that a special power has completed when this object dies.",
 body="""Placed on objects created by a special power (e.g. the bomber that delivers a payload). When the object dies/is removed, the script engine is notified that <b>SpecialPowerTemplate</b> has completed for the owning player, so map scripts can react to 'special power X completed'.""",
 ex="""Behavior = SpecialPowerCompletionDie ModuleTag_SPComplete
  SpecialPowerTemplate = SuperweaponCarpetBomb
End""")
F.update({'SpecialPowerCompletionDieModuleData.SpecialPowerTemplate': ("Special power to report as completed.", "SuperweaponCarpetBomb")})

M['RebuildHoleExposeDie'] = dict(
 sum="GLA structure death: creates a rebuild hole that will reconstruct the building.",
 body="""When the structure dies, a hole object (<b>HoleName</b>, with <b>HoleMaxHealth</b>) is created in its place; the hole's RebuildHoleBehavior then rebuilds the structure. With <b>TransferAttackers</b> units attacking the building retarget the hole.""",
 ex="""Behavior = RebuildHoleExposeDie ModuleTag_Hole
  DeathTypes        = ALL
  HoleName          = GLAHoleBarracks
  HoleMaxHealth     = 100
  TransferAttackers = Yes
End""")
F.update({
'RebuildHoleExposeDieModuleData.HoleName': ("Object template of the hole created.", "GLAHoleBarracks"),
'RebuildHoleExposeDieModuleData.HoleMaxHealth': ("Max health of the created hole.", "100"),
'RebuildHoleExposeDieModuleData.TransferAttackers': ("If Yes, attackers switch their attack to the hole.", "Yes"),
})

M['UpgradeDie'] = dict(
 sum="On death, removes an object upgrade from the producer that created this object (e.g. Battlemaster drone, Overlord add-on).",
 body="""Objects created by an object-upgrade (drones, add-ons) carry this so that when they die the parent object loses <b>UpgradeToRemove</b>, allowing the player to buy it again.""",
 ex="""Behavior = UpgradeDie ModuleTag_UpgradeDie
  DeathTypes      = ALL
  UpgradeToRemove = Upgrade_AmericaBattleDrone
End""")
F.update({'UpgradeDieModuleData.UpgradeToRemove': ("Object upgrade removed from the producer when this object dies.", "Upgrade_AmericaBattleDrone")})

M['KeepObjectDie'] = dict(
 sum="Die module that keeps the object in the world as rubble instead of removing it.",
 body="""For objects (mainly civilian buildings) that should stay as rubble when destroyed and have no other die module to handle it. Without it, such objects would linger in an odd state. Only DieMux filter fields.""",
 ex="""Behavior = KeepObjectDie ModuleTag_Keep
  DeathTypes = ALL
End""")
