M = {}
F = {}

M['ExperienceScalarUpgrade'] = dict(
 sum="Upgrade that increases how fast the object gains experience.",
 body="""When triggered, <b>AddXPScalar</b> is added to the object's experience gain multiplier (e.g. 0.5 = 50% more XP per kill). Used by training upgrades like USA Advanced Training.""",
 ex="""Behavior = ExperienceScalarUpgrade ModuleTag_Training
  TriggeredBy = Upgrade_AmericaAdvancedTraining
  AddXPScalar = 1.0
End""")
F.update({'ExperienceScalarUpgradeModuleData.AddXPScalar': ("Amount added to the experience gain multiplier (1.0 = double XP).", "1.0")})

M['MaxHealthUpgrade'] = dict(
 sum="Upgrade that increases the object's maximum health.",
 body="""When triggered, adds <b>AddMaxHealth</b> to max health. <b>ChangeType</b> decides what happens to current health: SAME_CURRENTHEALTH (default, current unchanged), PRESERVE_RATIO (keep the same percentage), ADD_CURRENT_HEALTH_TOO (current health grows by the same amount).""",
 ex="""Behavior = MaxHealthUpgrade ModuleTag_Health
  TriggeredBy  = Upgrade_ExtraArmor
  AddMaxHealth = 100
  ChangeType   = ADD_CURRENT_HEALTH_TOO
End""")
F.update({
'MaxHealthUpgradeModuleData.AddMaxHealth': ("Amount of max health added.", "100"),
'MaxHealthUpgradeModuleData.ChangeType': ("How current health reacts: SAME_CURRENTHEALTH, PRESERVE_RATIO, ADD_CURRENT_HEALTH_TOO.", "ADD_CURRENT_HEALTH_TOO"),
})

M['LockWeaponCreate'] = dict(
 sum="On creation, locks the object's weapon choice to one slot.",
 body="""A create module that locks the weapon set to <b>SlotToLock</b> when the object is created (used by objects created with a specific weapon to use, e.g. certain Zero Hour units/objects).""",
 ex="""Behavior = LockWeaponCreate ModuleTag_Lock
  SlotToLock = SECONDARY
End""")
F.update({'LockWeaponCreateModuleData.SlotToLock': ("Weapon slot locked on creation (PRIMARY/SECONDARY/TERTIARY).", "SECONDARY")})

M['PreorderCreate'] = dict(
 sum="On creation, sets the PREORDER model condition if the player preordered (bonus visuals).",
 body="""A create module that applies the PREORDER model condition to the object when the game was a preorder edition — a cosmetic hook (e.g. special flags on buildings). No fields.""",
 ex="""Behavior = PreorderCreate ModuleTag_Preorder
End""")

M['SupplyCenterCreate'] = dict(
 sum="On creation, registers a new supply center with every player's resource AI.",
 body="""Required on supply centers so all AI players' resource gathering logic knows about it. No fields.""",
 ex="""Behavior = SupplyCenterCreate ModuleTag_SCCreate
End""")

M['SupplyWarehouseCreate'] = dict(
 sum="On creation, registers a supply warehouse/pile with every player's resource AI.",
 body="""Required on supply warehouses/docks so AI players see them as supply sources. No fields.""",
 ex="""Behavior = SupplyWarehouseCreate ModuleTag_SWCreate
End""")

M['SpecialPowerCreate'] = dict(
 sum="On construction complete, starts the recharge countdown of the building's special powers.",
 body="""Put on buildings with special powers (superweapons, command center powers) so that their timers start counting when the building is finished, not when placement starts. No fields.""",
 ex="""Behavior = SpecialPowerCreate ModuleTag_SPCreate
End""")

M['GrantUpgradeCreate'] = dict(
 sum="On creation (or build complete), grants an upgrade to the object/player.",
 body="""When the object is created (for structures: when construction completes) it grants <b>UpgradeToGrant</b> — player upgrades to the player, object upgrades to itself. Skipped if the object has any <b>ExemptStatus</b> (e.g. UNDER_CONSTRUCTION to wait for completion).""",
 ex="""Behavior = GrantUpgradeCreate ModuleTag_Grant
  UpgradeToGrant = Upgrade_GLAFakeBuilding
  ExemptStatus   = UNDER_CONSTRUCTION
End""")
F.update({
'GrantUpgradeCreateModuleData.UpgradeToGrant': ("Upgrade granted on creation.", "Upgrade_GLAFakeBuilding"),
'GrantUpgradeCreateModuleData.ExemptStatus': ("Don't grant while any of these statuses are set.", "UNDER_CONSTRUCTION"),
})

M['VeterancyGainCreate'] = dict(
 sum="On creation, gives the object a starting veterancy level if the player has a science.",
 body="""If the player owns <b>ScienceRequired</b>, newly created objects start at <b>StartingLevel</b> (VETERAN, ELITE, HEROIC). This is how General's promotions like 'Veteran Tanks' work.""",
 ex="""Behavior = VeterancyGainCreate ModuleTag_Vet
  StartingLevel   = VETERAN
  ScienceRequired = SCIENCE_TankVeteran
End""")
F.update({
'VeterancyGainCreateModuleData.StartingLevel': ("Veterancy level set at creation (REGULAR, VETERAN, ELITE, HEROIC).", "VETERAN"),
'VeterancyGainCreateModuleData.ScienceRequired': ("Science the player needs for the bonus.", "SCIENCE_TankVeteran"),
})

F.update({'DamageModuleData.DamageTypes': ("Damage types this damage module reacts to (parsed, but most damage modules use their own filters).", "ALL")})

M['BoneFXDamage'] = dict(
 sum="Damage module companion of BoneFXUpdate: notifies it when the body damage state changes.",
 body="""Needed alongside BoneFXUpdate. It listens for body damage state transitions and tells BoneFXUpdate to switch to the effect set of the new state.""",
 ex="""Behavior = BoneFXDamage ModuleTag_BoneFXDamage
End""")

M['TransitionDamageFX'] = dict(
 sum="Plays FX, OCLs and particle systems at bones when the object transitions to a new damage state.",
 body="""When the body state changes to Damaged, ReallyDamaged or Rubble, up to 12 FXLists, 12 OCLs and 12 particle systems configured for that state play once at their bones (e.g. explosion + smoke plume when a building becomes badly damaged). Entry syntax: <code>ReallyDamagedFXList1 = Loc: X:0 Y:0 Z:10 FXList:FX_StructureMediumDamage</code> (or <code>Bone:BoneName</code> with RandomBone:Yes), OCLs use <code>OCL:</code>, particles <code>PSys:</code>. <b>DamageFXTypes/DamageOCLTypes/DamageParticleTypes</b> filter by the damage type that caused the transition.""",
 ex="""Behavior = TransitionDamageFX ModuleTag_Transition
  DamagedParticleSystem1       = Bone:Smoke01 RandomBone:No PSys:StructureTransitionSmallSmoke
  ReallyDamagedFXList1         = Loc: X:0 Y:0 Z:10 FXList:FX_StructureMediumDamage
  ReallyDamagedParticleSystem1 = Bone:Fire01 RandomBone:No PSys:StructureTransitionMediumFire
  RubbleOCL1                   = Loc: X:0 Y:0 Z:0 OCL:OCL_StructureRubbleDebris
End""")
F.update({
'TransitionDamageFXModuleData.DamageFXTypes': ("Damage types whose transitions play the FXLists.", "ALL"),
'TransitionDamageFXModuleData.DamageOCLTypes': ("Damage types whose transitions create the OCLs.", "ALL"),
'TransitionDamageFXModuleData.DamageParticleTypes': ("Damage types whose transitions create the particle systems.", "ALL"),
})

M['ContainedTransitionDamageFXV2'] = dict(
 sum="Drop-in TransitionDamageFX that also fires on contained riders (e.g. Overlord add-ons), whose damage state is copied without notification.",
 body="""Mod-original. Riders inside an OverlordContain get their damage state copied from the carrier without their damage modules being notified, so a normal TransitionDamageFX on the rider never fires. This module takes the EXACT same fields as TransitionDamageFX, runs the same effect code, and additionally polls its body's damage state every <b>CheckInterval</b>; if the state changed silently, it runs the transition itself, using the container's last DamageInfo while contained so the DamageFX/OCL/Particle type filters work as on the container. Normal notifications update the tracked state first, so effects never play twice. Can replace TransitionDamageFX on any object.""",
 ex="""Behavior = ContainedTransitionDamageFXV2 ModuleTag_Transition
  CheckInterval                = 100
  ReallyDamagedParticleSystem1 = Bone:Fire01 RandomBone:No PSys:SmallFire
  RubbleFXList1                = Loc: X:0 Y:0 Z:5 FXList:FX_AddonDestroyed
End""")
F.update({'ContainedTransitionDamageFXV2ModuleData.CheckInterval': ("Time (ms) between body-state polls (min 1 frame).", "100")})

M['SwitchStateWhenDamagedBehaviorV2'] = dict(
 sum="Automatically triggers the companion SwitchStateV2 when the object takes qualifying damage.",
 body="""Mod-original, a pure DamageModule (no per-frame update). When a hit of a type in <b>DamageTypes</b> has actual (post-armor) damage >= <b>DamageAmount</b> (or <= with <b>GreaterOrEqual</b> = No), it calls the SwitchStateV2 on the same object sharing <b>SpecialPowerTemplate</b>. That always drives toward AlteredState: with SwitchStateV2 Lifetime = 0 it's a one-time permanent switch; with Lifetime > 0 every qualifying hit re-arms the revert countdown, so sustained damage keeps it Altered. Can be upgrade-gated (StartsActive = No + TriggeredBy); upgrades are permanent unlocks.""",
 ex="""Behavior = SwitchStateV2 ModuleTag_Switch
  SpecialPowerTemplate = SpecialPower_SwitchState
  ConditionStateType   = USER_2
  Lifetime             = 5000
End
Behavior = SwitchStateWhenDamagedBehaviorV2 ModuleTag_DamageTrigger
  SpecialPowerTemplate = SpecialPower_SwitchState
  DamageTypes          = NONE +EXPLOSION +FLAME
  DamageAmount         = 25
  GreaterOrEqual       = Yes
  StartsActive         = Yes
End""")
F.update({
'SwitchStateWhenDamagedBehaviorV2ModuleData.StartsActive': ("Yes (default): active immediately. No: inactive until a TriggeredBy upgrade completes.", "Yes"),
'SwitchStateWhenDamagedBehaviorV2ModuleData.SpecialPowerTemplate': ("Must match the SpecialPowerTemplate of the target SwitchStateV2 (+ optional SwitchStateV2Activate).", "SpecialPower_SwitchState"),
'SwitchStateWhenDamagedBehaviorV2ModuleData.DamageTypes': ("Damage types that can trigger the switch (NONE/ALL/+X/-X syntax).", "NONE +EXPLOSION +FLAME"),
'SwitchStateWhenDamagedBehaviorV2ModuleData.DamageAmount': ("Threshold compared against the actual post-armor damage of a single hit.", "25"),
'SwitchStateWhenDamagedBehaviorV2ModuleData.GreaterOrEqual': ("Yes: trigger when damage >= DamageAmount. No: trigger when damage <= DamageAmount.", "Yes"),
})

M['FireWeaponCollide'] = dict(
 sum="Fires a weapon at anything that collides with the object (e.g. burning ground, electrified fences).",
 body="""Whenever another object collides with this one, <b>CollideWeapon</b> is fired at it (every frame of contact, or only once with <b>FireOnce</b>). <b>RequiredStatus</b>/<b>ForbiddenStatus</b> gate it on this object's status.""",
 ex="""Behavior = FireWeaponCollide ModuleTag_Collide
  CollideWeapon = FirewallContactWeapon
  FireOnce      = No
End""")
F.update({
'FireWeaponCollideModuleData.CollideWeapon': ("Weapon fired at colliding objects.", "FirewallContactWeapon"),
'FireWeaponCollideModuleData.FireOnce': ("If Yes, fire only once per object instead of every frame of contact.", "No"),
'FireWeaponCollideModuleData.RequiredStatus': ("Only fire while this object has all these statuses.", ""),
'FireWeaponCollideModuleData.ForbiddenStatus': ("Don't fire while this object has any of these statuses.", ""),
})

M['SquishCollide'] = dict(
 sum="Lets infantry be crushed (squished) by vehicles that drive over them.",
 body="""When a crusher (vehicle with enough CrusherLevel vs. this object's CrushableLevel) collides with the object, it is killed with the CRUSHED death type. No fields.""",
 ex="""Behavior = SquishCollide ModuleTag_Squish
End""")

F.update({
'CrateCollideModuleData.RequiredKindOf': ("Only objects with one of these KindOfs can pick the crate up.", "INFANTRY VEHICLE"),
'CrateCollideModuleData.ForbiddenKindOf': ("Objects with any of these KindOfs can't pick it up.", "PROJECTILE"),
'CrateCollideModuleData.ForbidOwnerPlayer': ("If Yes, the player whose unit died to create the crate can't pick it up.", "No"),
'CrateCollideModuleData.BuildingPickup': ("If Yes, structures can pick it up (bypassing the AI requirement).", "No"),
'CrateCollideModuleData.HumanOnly': ("If Yes, only human players can pick it up (missions).", "No"),
'CrateCollideModuleData.AllowMultiPickup': ("If Yes, several objects can pick it up in the same frame.", "No"),
'CrateCollideModuleData.PickupScience': ("Only players with this science can pick it up.", ""),
'CrateCollideModuleData.ExecuteFX': ("FXList played when the crate is collected.", "FX_CratePickup"),
'CrateCollideModuleData.ExecuteAnimation': ("2D animation played at the crate location.", "MoneyPickUp"),
'CrateCollideModuleData.ExecuteAnimationTime': ("Duration (seconds) of the animation.", "3.0"),
'CrateCollideModuleData.ExecuteAnimationZRise': ("How much the animation rises while playing.", "25"),
'CrateCollideModuleData.ExecuteAnimationFades': ("If Yes the animation fades out.", "Yes"),
})

M['HealCrateCollide'] = dict(
 sum="Crate that fully heals everything owned by the player who picks it up.",
 body="""On pickup, all of the collector's player's objects are healed to full. Uses only the common crate fields.""",
 ex="""Behavior = HealCrateCollide ModuleTag_Crate
  RequiredKindOf = INFANTRY VEHICLE
  ForbiddenKindOf= PROJECTILE
  ExecuteFX      = FX_HealCrate
End""")

M['MoneyCrateCollide'] = dict(
 sum="Crate that gives money to the player who picks it up.",
 body="""On pickup gives <b>MoneyProvided</b>, plus <b>UpgradedBoost</b> extra for players owning a given upgrade (<code>UpgradeType:Upgrade_X Boost:N</code>).""",
 ex="""Behavior = MoneyCrateCollide ModuleTag_Crate
  RequiredKindOf = INFANTRY VEHICLE
  ForbidOwnerPlayer = Yes
  MoneyProvided  = 1000
  ExecuteAnimation = MoneyPickUp
  ExecuteAnimationTime = 3.0
  ExecuteAnimationZRise = 25
End""")
F.update({
'MoneyCrateCollideModuleData.MoneyProvided': ("Money given on pickup.", "1000"),
'MoneyCrateCollideModuleData.UpgradedBoost': ("Extra money with an upgrade: UpgradeType:<Upgrade> Boost:<amount>. Repeatable.", "UpgradeType:Upgrade_GLAWorkerShoes Boost:100"),
})

M['ShroudCrateCollide'] = dict(
 sum="Crate that reveals the entire map for the player who picks it up.",
 body="""On pickup clears the shroud of the whole map for the collector's player. Common crate fields only.""",
 ex="""Behavior = ShroudCrateCollide ModuleTag_Crate
  RequiredKindOf = INFANTRY VEHICLE
End""")

M['UnitCrateCollide'] = dict(
 sum="Crate that gives the collector some free units.",
 body="""On pickup creates <b>UnitCount</b> units of <b>UnitName</b> for the collector's player near the crate.""",
 ex="""Behavior = UnitCrateCollide ModuleTag_Crate
  RequiredKindOf = INFANTRY VEHICLE
  UnitCount      = 2
  UnitName       = GLAInfantryRebel
End""")
F.update({
'UnitCrateCollideModuleData.UnitCount': ("Number of units created.", "2"),
'UnitCrateCollideModuleData.UnitName': ("Object template of the units.", "GLAInfantryRebel"),
})

M['VeterancyCrateCollide'] = dict(
 sum="Crate that gives a veterancy level to the collector (or everything in range). Also used by pilots entering vehicles.",
 body="""On pickup grants one veterancy level to the collector, or to every friendly unit within <b>EffectRange</b>. <b>AddsOwnerVeterancy</b> gives the crate owner's veterancy instead of +1 (used by the USA pilot 'crate' when a pilot enters a vehicle, with <b>IsPilot</b> = Yes).""",
 ex="""Behavior = VeterancyCrateCollide ModuleTag_Crate
  RequiredKindOf     = VEHICLE
  EffectRange        = 0
  AddsOwnerVeterancy = Yes
  IsPilot            = Yes
End""")
F.update({
'VeterancyCrateCollideModuleData.EffectRange': ("If non-zero, all friendly units in this range gain a level.", "0"),
'VeterancyCrateCollideModuleData.AddsOwnerVeterancy': ("If Yes, the target gets the crate owner's (pilot's) veterancy level.", "Yes"),
'VeterancyCrateCollideModuleData.IsPilot': ("If Yes, behaves as a pilot entering a vehicle (pilot is consumed).", "Yes"),
})

M['ConvertToCarBombCrateCollide'] = dict(
 sum="GLA Terrorist 'crate' behaviour: entering a civilian car converts it into a car bomb.",
 body="""On the Terrorist: when he 'collides' with a valid car it becomes his car bomb (switches owner, activates its CARBOMB weapon set and AI), playing <b>FXList</b>. Common crate fields filter valid targets.""",
 ex="""Behavior = ConvertToCarBombCrateCollide ModuleTag_CarBomb
  RequiredKindOf = CAN_BE_CAR_BOMB
  ForbiddenKindOf= STRUCTURE
  FXList         = FX_CarBombConvert
End""")
F.update({'ConvertToCarBombCrateCollideModuleData.FXList': ("FXList played when the car is converted.", "FX_CarBombConvert")})

M['ConvertToHijackedVehicleCrateCollide'] = dict(
 sum="Hijacker 'crate' behaviour: entering an enemy vehicle steals it (kills the driver, switches owner).",
 body="""On the Hijacker: colliding with an eligible enemy vehicle transfers it to the hijacker's player and hides the hijacker inside (see HijackerUpdate).""",
 ex="""Behavior = ConvertToHijackedVehicleCrateCollide ModuleTag_Hijack
  RequiredKindOf  = VEHICLE
  ForbiddenKindOf = AIRCRAFT BOAT
End""")

M['SabotageCommandCenterCrateCollide'] = dict(
 sum="Saboteur effect: resets all general's power timers at an enemy command center.",
 body="""On the saboteur: entering an enemy Command Center resets every special power timer of that player's command center.""",
 ex="""Behavior = SabotageCommandCenterCrateCollide ModuleTag_Sabotage
  BuildingPickup = Yes
End""")
M['SabotageFakeBuildingCrateCollide'] = dict(
 sum="Saboteur effect: destroys an enemy fake building.",
 body="""On the saboteur: entering a GLA fake structure destroys it.""",
 ex="""Behavior = SabotageFakeBuildingCrateCollide ModuleTag_Sabotage
  BuildingPickup = Yes
End""")
M['SabotageInternetCenterCrateCollide'] = dict(
 sum="Saboteur effect: disables an enemy Internet Center (and its hackers) for a while.",
 body="""Entering an enemy Internet Center disables it for <b>SabotageDuration</b>.""",
 ex="""Behavior = SabotageInternetCenterCrateCollide ModuleTag_Sabotage
  BuildingPickup   = Yes
  SabotageDuration = 30000
End""")
F.update({'SabotageInternetCenterCrateCollideModuleData.SabotageDuration': ("Time (ms) the target is disabled.", "30000")})
M['SabotageMilitaryFactoryCrateCollide'] = dict(
 sum="Saboteur effect: disables an enemy factory's production for a while.",
 body="""Entering an enemy factory disables it for <b>SabotageDuration</b>.""",
 ex="""Behavior = SabotageMilitaryFactoryCrateCollide ModuleTag_Sabotage
  BuildingPickup   = Yes
  SabotageDuration = 30000
End""")
F.update({'SabotageMilitaryFactoryCrateCollideModuleData.SabotageDuration': ("Time (ms) the factory is disabled.", "30000")})
M['SabotagePowerPlantCrateCollide'] = dict(
 sum="Saboteur effect: shuts down an enemy power plant's power output for a while.",
 body="""Entering an enemy power plant causes its player to lose that plant's power for <b>SabotagePowerDuration</b>.""",
 ex="""Behavior = SabotagePowerPlantCrateCollide ModuleTag_Sabotage
  BuildingPickup        = Yes
  SabotagePowerDuration = 30000
End""")
F.update({'SabotagePowerPlantCrateCollideModuleData.SabotagePowerDuration': ("Time (ms) the power is cut.", "30000")})
M['SabotageSuperweaponCrateCollide'] = dict(
 sum="Saboteur effect: resets an enemy superweapon's countdown.",
 body="""Entering an enemy superweapon building resets its special power timer.""",
 ex="""Behavior = SabotageSuperweaponCrateCollide ModuleTag_Sabotage
  BuildingPickup = Yes
End""")
M['SabotageSupplyCenterCrateCollide'] = dict(
 sum="Saboteur effect: steals cash from the enemy by entering their supply center.",
 body="""Entering an enemy supply center transfers <b>StealCashAmount</b> from that player to the saboteur's player.""",
 ex="""Behavior = SabotageSupplyCenterCrateCollide ModuleTag_Sabotage
  BuildingPickup  = Yes
  StealCashAmount = 1000
End""")
F.update({'SabotageSupplyCenterCrateCollideModuleData.StealCashAmount': ("Cash stolen.", "1000")})
M['SabotageSupplyDropzoneCrateCollide'] = dict(
 sum="Saboteur effect: steals cash and resets the timer of an enemy Supply Drop Zone.",
 body="""Entering an enemy Supply Drop Zone steals <b>StealCashAmount</b> and resets its drop timer.""",
 ex="""Behavior = SabotageSupplyDropzoneCrateCollide ModuleTag_Sabotage
  BuildingPickup  = Yes
  StealCashAmount = 800
End""")
F.update({'SabotageSupplyDropzoneCrateCollideModuleData.StealCashAmount': ("Cash stolen.", "800")})

M['SalvageCrateCollide'] = dict(
 sum="GLA salvage crate: gives a weapon upgrade (salvager units), a veterancy level, or money.",
 body="""Dropped by destroyed vehicles when GLA salvagers kill them. When picked up: first a <b>WeaponChance</b> roll for a salvage weapon upgrade (only units with WEAPON_SALVAGER that are not yet fully upgraded), otherwise <b>LevelChance</b> for a veterancy level, otherwise <b>MoneyChance</b> for random money between <b>MinMoney</b> and <b>MaxMoney</b>.""",
 ex="""Behavior = SalvageCrateCollide ModuleTag_Salvage
  RequiredKindOf = SALVAGER
  WeaponChance   = 100%
  LevelChance    = 25%
  MoneyChance    = 75%
  MinMoney       = 25
  MaxMoney       = 75
End""")
F.update({
'SalvageCrateCollideModuleData.WeaponChance': ("Chance to give a weapon (salvage) upgrade if possible.", "100%"),
'SalvageCrateCollideModuleData.LevelChance': ("Chance to give a veterancy level if the weapon roll fails.", "25%"),
'SalvageCrateCollideModuleData.MoneyChance': ("Chance to give money if the weapon roll fails.", "75%"),
'SalvageCrateCollideModuleData.MinMoney': ("Minimum money given.", "25"),
'SalvageCrateCollideModuleData.MaxMoney': ("Maximum money given.", "75"),
})

# Bodies
F.update({
'ActiveBodyModuleData.MaxHealth': ("Maximum health.", "300"),
'ActiveBodyModuleData.InitialHealth': ("Health when created.", "300"),
'ActiveBodyModuleData.SubdualDamageCap': ("Maximum accumulated subdual (non-lethal disabling) damage. Reaching max health in subdual damage disables the object.", "600"),
'ActiveBodyModuleData.SubdualDamageHealRate': ("Interval (ms) at which accumulated subdual damage decreases.", "500"),
'ActiveBodyModuleData.SubdualDamageHealAmount': ("Subdual damage removed each interval.", "50"),
})

M['InactiveBody'] = dict(
 sum="Body with no health: the object is indestructible and ignores damage (props, markers, effects).",
 body="""Used on objects that should never be damaged or die from damage (decorative props, projectiles-helpers, markers). It has no health data. The object can still be destroyed by code/lifetime modules.""",
 ex="""Body = InactiveBody ModuleTag_Body
End""")

M['ActiveBody'] = dict(
 sum="Standard body: health, armor-mitigated damage, damage states, death, healing and subdual damage.",
 body="""The normal body for units. It has <b>MaxHealth</b>/<b>InitialHealth</b>, applies armor to incoming damage, computes the body damage state (PRISTINE / DAMAGED / REALLYDAMAGED / RUBBLE, thresholds from GameData), notifies damage modules and kills the object at 0 health. Also handles subdual damage (EMP-like non-lethal damage that disables the unit when it reaches MaxHealth, capped by <b>SubdualDamageCap</b>, bleeding off at <b>SubdualDamageHealAmount</b> per <b>SubdualDamageHealRate</b>).""",
 ex="""Body = ActiveBody ModuleTag_Body
  MaxHealth     = 480
  InitialHealth = 480
  SubdualDamageCap        = 960
  SubdualDamageHealRate   = 500
  SubdualDamageHealAmount = 50
End""")

M['HighlanderBody'] = dict(
 sum="ActiveBody that takes damage but cannot die from normal damage (stays at 1 HP); only UNRESISTABLE damage kills it.",
 body="""Same fields as ActiveBody. Damage is applied normally but health never drops below 1 unless the damage type is UNRESISTABLE (used by scripts or special kill weapons). Used for mission-critical objects.""",
 ex="""Body = HighlanderBody ModuleTag_Body
  MaxHealth     = 1000
  InitialHealth = 1000
End""")

M['ImmortalBody'] = dict(
 sum="ActiveBody whose health never drops below 1 (cannot die at all from damage).",
 body="""Same fields as ActiveBody. Takes damage normally but health is clamped at 1, so the object can't be killed by any damage (unlike HighlanderBody, not even unresistable).""",
 ex="""Body = ImmortalBody ModuleTag_Body
  MaxHealth     = 500
  InitialHealth = 500
End""")

M['ShieldedBody'] = dict(
 sum="ActiveBody that lets ShieldGeneratorUpdateV2 freeze the reported damage state while a shield is active.",
 body="""Mod-original. Identical to ActiveBody (same fields), but a companion module can temporarily freeze the BodyDamageType it reports to the Locomotor (movement penalties) and Drawable (DAMAGED/REALLYDAMAGED visuals), so temporary health effects don't make the unit visibly/mechanically snap between damage states. Real health, death and rubble are unaffected. Used by ShieldGeneratorUpdateV2 as a safety net; objects without it simply skip the freeze. Opt in per unit: <code>Body = ShieldedBody</code> instead of ActiveBody.""",
 ex="""Body = ShieldedBody ModuleTag_Body
  MaxHealth     = 500
  InitialHealth = 500
End""")

M['StructureBody'] = dict(
 sum="ActiveBody for buildings: tracks the constructing dozer/worker and structure-specific behaviour.",
 body="""Same fields as ActiveBody, used on structures. Additionally remembers which builder is constructing it and handles structure-specific interactions (repair by dozers, capture, sell).""",
 ex="""Body = StructureBody ModuleTag_Body
  MaxHealth     = 1500
  InitialHealth = 1500
End""")

M['HiveStructureBody'] = dict(
 sum="Structure body that redirects certain damage types to its spawned slaves (Stinger Site takes sniper/poison damage on its soldiers).",
 body="""Extends StructureBody. Damage of types in <b>PropagateDamageTypesToSlavesWhenExisting</b> is passed to one of its slaves (from SpawnBehavior) instead of the structure while slaves exist. Types in <b>SwallowDamageTypesIfSlavesNotExisting</b> (subset) are ignored entirely when no slaves exist, so e.g. snipers can't damage an empty Stinger Site.""",
 ex="""Body = HiveStructureBody ModuleTag_Body
  MaxHealth     = 1000
  InitialHealth = 1000
  PropagateDamageTypesToSlavesWhenExisting = NONE +SMALL_ARMS +SNIPER +POISON +RADIATION +SURRENDER +FLAME
  SwallowDamageTypesIfSlavesNotExisting    = NONE +SNIPER +POISON +RADIATION +SURRENDER
End""")
F.update({
'HiveStructureBodyModuleData.PropagateDamageTypesToSlavesWhenExisting': ("Damage types redirected to slaves while any exist.", "NONE +SMALL_ARMS +SNIPER +POISON"),
'HiveStructureBodyModuleData.SwallowDamageTypesIfSlavesNotExisting': ("Subset of the above that is ignored completely when no slaves exist.", "NONE +SNIPER +POISON"),
})

M['UndeadBody'] = dict(
 sum="Body with two lives: the first death is intercepted and the object continues with SecondLifeMaxHealth (Battle Bus).",
 body="""The first time the object would die, it instead gets the SECOND_LIFE status, its max health set to <b>SecondLifeMaxHealth</b> (full), and its SlowDeath modules can run a 'fake' death (see BattleBusSlowDeathBehavior). The second death is handled normally.""",
 ex="""Body = UndeadBody ModuleTag_Body
  MaxHealth           = 400
  InitialHealth       = 400
  SecondLifeMaxHealth = 200
End""")
F.update({'UndeadBodyModuleData.SecondLifeMaxHealth': ("Max (and current) health after the first death.", "200")})

M['CashHackSpecialPower'] = dict(
 sum="Special power that steals money from an enemy player (e.g. Black Lotus Cash Hack / Command Center).",
 body="""When used on an enemy structure, steals <b>MoneyAmount</b> from its owner; with sciences, <b>UpgradeMoneyAmount</b> (<code>Science MoneyAmount</code> pairs) overrides the amount.""",
 ex="""Behavior = CashHackSpecialPower ModuleTag_CashHack
  SpecialPowerTemplate = SpecialAbilityBlackLotusStealCashHack
  MoneyAmount          = 1000
  UpgradeMoneyAmount   = SCIENCE_CashHack2 2000
End""")
F.update({
'CashHackSpecialPowerModuleData.UpgradeMoneyAmount': ("Science-dependent amount: <Science> <Amount>. Repeatable.", "SCIENCE_CashHack2 2000"),
'CashHackSpecialPowerModuleData.MoneyAmount': ("Money stolen.", "1000"),
})

M['DefectorSpecialPower'] = dict(
 sum="Special power that makes a targeted enemy unit defect to your side.",
 body="""Click an enemy unit with the power's cursor and it switches to your team. <b>FatCursorRadius</b> makes the targeting cursor more forgiving.""",
 ex="""Behavior = DefectorSpecialPower ModuleTag_Defector
  SpecialPowerTemplate = SuperweaponDefector
  FatCursorRadius      = 20
End""")
F.update({'DefectorSpecialPowerModuleData.FatCursorRadius': ("Extra radius around the cursor used to find a target.", "20")})

M['DemoralizeSpecialPower'] = dict(
 sum="(Cut) power that demoralizes enemy units in an area; range/duration scale with prisoners held.",
 body="""Demoralizes enemies in a radius of <b>BaseRange</b> + <b>BonusRangePerCaptured</b> per prisoner (max <b>MaxRange</b>) for <b>BaseDuration</b> + bonus per prisoner (max <b>MaxDuration</b>), playing <b>FXList</b>. Part of the unused prisoner feature.""",
 ex="""Behavior = DemoralizeSpecialPower ModuleTag_Demoralize
  SpecialPowerTemplate     = SpecialPowerDemoralize
  BaseRange                = 100
  BonusRangePerCaptured    = 10
  MaxRange                 = 200
  BaseDuration             = 10000
  BonusDurationPerCaptured = 1000
  MaxDuration              = 20000
  FXList                   = FX_Demoralize
End""")
F.update({
'DemoralizeSpecialPowerModuleData.BaseRange': ("Base radius of the effect.", "100"),
'DemoralizeSpecialPowerModuleData.BonusRangePerCaptured': ("Extra radius per prisoner held.", "10"),
'DemoralizeSpecialPowerModuleData.MaxRange': ("Maximum radius.", "200"),
'DemoralizeSpecialPowerModuleData.BaseDuration': ("Base duration (ms).", "10000"),
'DemoralizeSpecialPowerModuleData.BonusDurationPerCaptured': ("Extra duration (ms) per prisoner.", "1000"),
'DemoralizeSpecialPowerModuleData.MaxDuration': ("Maximum duration (ms).", "20000"),
'DemoralizeSpecialPowerModuleData.FXList': ("FXList played at the target.", "FX_Demoralize"),
})

M['OCLSpecialPower'] = dict(
 sum="Generic special power that creates an ObjectCreationList at/toward the target (paradrops, airstrikes, artillery, A-10s…).",
 body="""The most common special power implementation. When fired at a target it creates <b>OCL</b> using <b>CreateLocation</b>: CREATE_AT_EDGE_NEAR_SOURCE (plane comes from the map edge near your base), CREATE_AT_EDGE_NEAR_TARGET, CREATE_AT_EDGE_FARTHEST_FROM_TARGET, CREATE_AT_LOCATION (directly at target), USE_OWNER_OBJECT or CREATE_ABOVE_LOCATION. <b>UpgradeOCL</b> (<code>Science OCL</code> pairs) swaps to a stronger OCL for higher general's ranks. <b>ReferenceObject</b> tells script placement what the final object is (e.g. GLA Sneak Attack tunnel), and <b>OCLAdjustPositionToPassable</b> moves the target to the nearest passable cell.""",
 ex="""Behavior = OCLSpecialPower ModuleTag_Paradrop
  SpecialPowerTemplate = SuperweaponParadropAmerica
  OCL                  = SUPERWEAPON_Paradrop1
  UpgradeOCL           = SCIENCE_Paradrop2 SUPERWEAPON_Paradrop2
  UpgradeOCL           = SCIENCE_Paradrop3 SUPERWEAPON_Paradrop3
  CreateLocation       = CREATE_AT_EDGE_NEAR_SOURCE
End""")
F.update({
'OCLSpecialPowerModuleData.UpgradeOCL': ("Rank-dependent OCL: <Science> <OCL>. Repeatable; the highest owned science wins.", "SCIENCE_Paradrop2 SUPERWEAPON_Paradrop2"),
'OCLSpecialPowerModuleData.OCL': ("ObjectCreationList created by the power.", "SUPERWEAPON_Paradrop1"),
'OCLSpecialPowerModuleData.CreateLocation': ("Where the OCL starts: CREATE_AT_EDGE_NEAR_SOURCE, CREATE_AT_EDGE_NEAR_TARGET, CREATE_AT_EDGE_FARTHEST_FROM_TARGET, CREATE_AT_LOCATION, USE_OWNER_OBJECT, CREATE_ABOVE_LOCATION.", "CREATE_AT_EDGE_NEAR_SOURCE"),
'OCLSpecialPowerModuleData.ReferenceObject': ("Template of the final object created, for script placement checks.", "GLATunnelNetwork"),
'OCLSpecialPowerModuleData.OCLAdjustPositionToPassable': ("If Yes, the target is moved to the nearest passable cell.", "No"),
})

M['FireWeaponPower'] = dict(
 sum="Special power that makes the unit fire a specific weapon (loaded from its weapon set) at the target.",
 body="""When activated, the unit fires its special-power weapon at the target up to <b>MaxShotsToFire</b> times (e.g. a unit ability like a one-off artillery barrage bound to a button).""",
 ex="""Behavior = FireWeaponPower ModuleTag_FirePower
  SpecialPowerTemplate = SpecialAbilityFireBarrage
  MaxShotsToFire       = 3
End""")
F.update({'FireWeaponPowerModuleData.MaxShotsToFire': ("Maximum number of shots fired per activation.", "3")})

M['SpecialAbility'] = dict(
 sum="Button side of unit special abilities; pairs with SpecialAbilityUpdate which does the work.",
 body="""The SpecialPowerModule that a unit's special-ability command button talks to (capture building, place charges, snipe, hijack…). It handles recharge and science gating, then hands off to the SpecialAbilityUpdate on the same object with the same <b>SpecialPowerTemplate</b>, which should have UpdateModuleStartsAttack semantics.""",
 ex="""Behavior = SpecialAbility ModuleTag_AbilityButton
  SpecialPowerTemplate     = SpecialAbilityBlackLotusCaptureBuilding
  UpdateModuleStartsAttack = Yes
  StartsReady              = Yes
End""")

M['SwitchStateV2Activate'] = dict(
 sum="Button-facing half of SwitchStateV2: the special power module a SPECIAL_POWER command button triggers.",
 body="""Mod-original. Does only standard special power bookkeeping (science gating, recharge, initiate sound) and forwards to the SwitchStateV2 on the same object with the same <b>SpecialPowerTemplate</b>, which performs the toggle. Needed because the command button only finds SpecialPowerModules, while the state logic must be an UpdateModule. All configuration lives on SwitchStateV2.""",
 ex="""Behavior = SwitchStateV2Activate ModuleTag_01
  SpecialPowerTemplate = SpecialPower_SwitchState
  StartsReady          = Yes
End""")

M['ShieldGeneratorActivateV2'] = dict(
 sum="Button-facing half of ShieldGeneratorUpdateV2: the special power the shield command button triggers.",
 body="""Mod-original. Standard special power bookkeeping only; forwards activation to the ShieldGeneratorUpdateV2 with the same <b>SpecialPowerTemplate</b>, which holds all shield configuration.""",
 ex="""Behavior = ShieldGeneratorActivateV2 ModuleTag_01
  SpecialPowerTemplate = SpecialPower_Shield
End""")

M['SpyVisionSpecialPower'] = dict(
 sum="Special power that activates SpyVisionUpdate: see through the eyes of all enemy units for a duration.",
 body="""When fired, the object's SpyVisionUpdate is turned on for <b>BaseDuration</b> (plus <b>BonusDurationPerCaptured</b> per prisoner, capped at <b>MaxDuration</b> — prisoner scaling is from the cut POW feature).""",
 ex="""Behavior = SpyVisionSpecialPower ModuleTag_SpyVision
  SpecialPowerTemplate = SpecialPowerSpyVision
  BaseDuration         = 30000
  MaxDuration          = 30000
End""")
F.update({
'SpyVisionSpecialPowerModuleData.BaseDuration': ("Duration (ms) of the spy vision.", "30000"),
'SpyVisionSpecialPowerModuleData.BonusDurationPerCaptured': ("Extra duration (ms) per prisoner held (unused feature).", "0"),
'SpyVisionSpecialPowerModuleData.MaxDuration': ("Maximum duration (ms).", "30000"),
})

M['CashBountyPower'] = dict(
 sum="Passive general's power: you receive a percentage of the cost of every enemy unit you kill.",
 body="""Once the science is purchased, the player gains <b>Bounty</b> percent of the build cost of each enemy object killed. <b>UpgradeBounty</b> (<code>Science Percent</code> pairs) gives higher percentages for higher ranks.""",
 ex="""Behavior = CashBountyPower ModuleTag_Bounty
  SpecialPowerTemplate = SuperweaponCashBounty
  Bounty               = 5%
  UpgradeBounty        = SCIENCE_CashBounty2 10%
  UpgradeBounty        = SCIENCE_CashBounty3 20%
End""")
F.update({
'CashBountyPowerModuleData.UpgradeBounty': ("Rank-dependent bounty: <Science> <Percent>. Repeatable.", "SCIENCE_CashBounty2 10%"),
'CashBountyPowerModuleData.Bounty': ("Percent of a killed enemy's cost given as bounty.", "5%"),
})

M['CleanupAreaPower'] = dict(
 sum="Ability that orders the unit to clean up all hazards (toxins, radiation) in an area (Ambulance 'Cleanup Area').",
 body="""Works with CleanupHazardUpdate: the unit moves up to <b>MaxMoveDistanceFromLocation</b> around the target location and cleans every hazard it finds until none remain.""",
 ex="""Behavior = CleanupAreaPower ModuleTag_Cleanup
  SpecialPowerTemplate        = SpecialAbilityAmbulanceCleanupArea
  MaxMoveDistanceFromLocation = 50
End""")
F.update({'CleanupAreaPowerModuleData.MaxMoveDistanceFromLocation': ("How far from the target point the unit may roam while cleaning.", "50")})

M['AnimatedParticleSysBoneClientUpdate'] = dict(
 sum="Client update that moves attached particle systems with animated bones.",
 body="""By default particle systems attached to bones stay at the bone's rest position. This client update re-positions them every frame to follow animated bones (e.g. smoke from a rotating part). No fields.""",
 ex="""ClientUpdate = AnimatedParticleSysBoneClientUpdate ModuleTag_AnimPSys
End""")

M['SwayClientUpdate'] = dict(
 sum="Client update that makes trees and plants sway in the wind.",
 body="""Applies a wind-driven sway to the object's model (visual only), using the global wind settings. No fields.""",
 ex="""ClientUpdate = SwayClientUpdate ModuleTag_Sway
End""")

M['BeaconClientUpdate'] = dict(
 sum="Client update for multiplayer beacons (map pings): radar pulses at intervals.",
 body="""Makes the placed beacon pulse on the radar every <b>RadarPulseFrequency</b> for <b>RadarPulseDuration</b>.""",
 ex="""ClientUpdate = BeaconClientUpdate ModuleTag_Beacon
  RadarPulseFrequency = 1000
  RadarPulseDuration  = 500
End""")
F.update({
'BeaconClientUpdateModuleData.RadarPulseFrequency': ("Interval (ms) between radar pulses.", "1000"),
'BeaconClientUpdateModuleData.RadarPulseDuration': ("Duration (ms) of each radar pulse.", "500"),
})
