M = {}
F = {}

F.update({
'W3DModelDrawModuleData.InitialRecoilSpeed': ("Speed at which recoil bones move back when a weapon fires.", "100"),
'W3DModelDrawModuleData.MaxRecoilDistance': ("Maximum distance a recoil bone moves back.", "3"),
'W3DModelDrawModuleData.RecoilDamping': ("Damping of the recoil motion.", "0.4"),
'W3DModelDrawModuleData.RecoilSettleSpeed': ("Speed at which recoil bones return to rest.", "6"),
'W3DModelDrawModuleData.OkToChangeModelColor': ("If Yes, the model is tinted with the house (player) colour.", "Yes"),
'W3DModelDrawModuleData.AnimationsRequirePower': ("If Yes (default), animations pause while the object is underpowered.", "Yes"),
'W3DModelDrawModuleData.ParticlesAttachedToAnimatedBones': ("If Yes, particle systems on bones follow the animated bone positions.", "No"),
'W3DModelDrawModuleData.MinLODRequired': ("Minimum game detail level (LOW, MEDIUM, HIGH) required to use this draw module at all (used for optional detail models).", "LOW"),
'W3DModelDrawModuleData.ProjectileBoneFeedbackEnabledSlots': ("Weapon slots whose launch-bone projectiles are hidden/shown according to the current clip ammo.", "PRIMARY SECONDARY"),
'W3DModelDrawModuleData.DefaultConditionState': ("Begins the default ConditionState sub-block (must come first), used when no other state matches. Block ends with End.", "(sub-block)"),
'W3DModelDrawModuleData.ConditionState': ("Begins a ConditionState sub-block for a set of model condition flags, e.g. 'ConditionState = DAMAGED'. See sub-block fields.", "(sub-block)"),
'W3DModelDrawModuleData.AliasConditionState': ("Makes another set of condition flags reuse the previous ConditionState block, e.g. 'AliasConditionState = DAMAGED MOVING'.", "REALLYDAMAGED"),
'W3DModelDrawModuleData.TransitionState': ("Begins a TransitionState sub-block played when switching between two states with the given TransitionKeys: 'TransitionState = keyA keyB'.", "(sub-block)"),
'W3DModelDrawModuleData.TrackMarks': ("Texture of tread/tyre marks left on the ground.", "EXTireTrack.tga"),
'W3DModelDrawModuleData.ExtraPublicBone': ("Additional bone names made public (accessible by other systems, e.g. FX/weapons). Repeatable.", "Flare01"),
'W3DModelDrawModuleData.AttachToBoneInAnotherModule': ("Draw this module's model attached to a bone of another draw module on the same object.", "Turret01"),
'W3DModelDrawModuleData.IgnoreConditionStates': ("Model condition flags this draw module ignores when choosing a state.", "PREATTACK_A"),
'W3DModelDrawModuleData.ReceivesDynamicLights': ("If Yes (default), the model is lit by dynamic lights (explosions etc.).", "Yes"),
'ModelConditionInfo.Model': ("W3D model name used in this state (NONE for invisible).", "AVCrusader"),
'ModelConditionInfo.Turret': ("Bone that rotates for the main turret.", "Turret01"),
'ModelConditionInfo.TurretArtAngle': ("Angle offset of the turret as authored in the art.", "0"),
'ModelConditionInfo.TurretPitch': ("Bone that pitches for the main turret.", "Barrel01"),
'ModelConditionInfo.TurretArtPitch': ("Pitch offset of the turret as authored.", "0"),
'ModelConditionInfo.AltTurret': ("Bone that rotates for the alternate turret.", "Turret02"),
'ModelConditionInfo.AltTurretArtAngle': ("Art angle offset of the alt turret.", "0"),
'ModelConditionInfo.AltTurretPitch': ("Pitch bone for the alt turret.", "Barrel02"),
'ModelConditionInfo.AltTurretArtPitch': ("Art pitch offset of the alt turret.", "0"),
'ModelConditionInfo.ShowSubObject': ("Subobjects to show in this state (space-separated).", "Antenna01"),
'ModelConditionInfo.HideSubObject': ("Subobjects to hide in this state.", "Rocket01 Rocket02"),
'ModelConditionInfo.WeaponFireFXBone': ("<WeaponSlot> <BoneBase>: bones where the weapon's fire FX play (BoneBase01, 02...).", "PRIMARY Muzzle"),
'ModelConditionInfo.WeaponRecoilBone': ("<WeaponSlot> <BoneBase>: bones that recoil when firing.", "PRIMARY Barrel"),
'ModelConditionInfo.WeaponMuzzleFlash': ("<WeaponSlot> <BoneBase>: muzzle-flash subobjects shown briefly when firing.", "PRIMARY MuzzleFX"),
'ModelConditionInfo.WeaponLaunchBone': ("<WeaponSlot> <BoneBase>: bones where projectiles are launched from.", "PRIMARY Muzzle"),
'ModelConditionInfo.WeaponHideShowBone': ("<WeaponSlot> <BoneBase>: projectile subobjects hidden/shown with clip ammo.", "SECONDARY Missile"),
'ModelConditionInfo.Animation': ("Animation to play in this state: <model.anim> [frames]. Repeat for random choice.", "AVCrusader.AVCrusader"),
'ModelConditionInfo.IdleAnimation': ("Idle animation(s) randomly played when the main animation completes.", "AVCrusader.Idle"),
'ModelConditionInfo.AnimationMode': ("How the animation plays: ONCE, LOOP, ONCE_BACKWARDS, LOOP_BACKWARDS, MANUAL, PING_PONG...", "LOOP"),
'ModelConditionInfo.TransitionKey': ("Name used to match TransitionState blocks when entering/leaving this state.", "Trans_Deployed"),
'ModelConditionInfo.WaitForStateToFinishIfPossible': ("Transition key to wait for so this state's animation finishes before switching.", "Trans_Deployed"),
'ModelConditionInfo.Flags': ("Animation flags: RANDOMSTART, START_FRAME_FIRST, START_FRAME_LAST, ADJUST_HEIGHT_BY_CONSTRUCTION_PERCENT, PRISTINE_BONE_POS_IN_FINAL_FRAME, MAINTAIN_FRAME_ACROSS_STATES...", "RANDOMSTART"),
'ModelConditionInfo.ParticleSysBone': ("<BoneName> <ParticleSystem>: attach a particle system to a bone in this state. Repeatable.", "Smoke01 SmokeTrail"),
'ModelConditionInfo.AnimationSpeedFactorRange': ("<min> <max>: random speed multiplier for the animation.", "0.9 1.1"),
})

M['W3DDefaultDraw'] = dict(
 sum="Placeholder draw module that draws a default box/marker; used for objects without real art.",
 body="""Draws nothing meaningful (a debug placeholder). Used on logic-only or test objects that still need a Draw module. No fields.""",
 ex="""Draw = W3DDefaultDraw ModuleTag_Draw
End""")

M['W3DDebrisDraw'] = dict(
 sum="Draw module for debris objects created by OCLs; the model is set by the OCL at creation time.",
 body="""Used on generic debris objects. The OCL's CreateDebris nugget tells this module which model/animation to use (ModelNames, AnimationSet), so a single debris object template can show many different chunks. No INI fields of its own.""",
 ex="""Draw = W3DDebrisDraw ModuleTag_Draw
End""")

M['W3DModelDraw'] = dict(
 sum="Main 3D model draw module: picks the model/animation for each model condition state, turrets, weapon bones, recoil, tracks.",
 body="""The workhorse Draw module. You define a <b>DefaultConditionState</b> and any number of <b>ConditionState</b> blocks, each keyed by a set of model condition flags (DAMAGED, MOVING, ATTACKING, REALLYDAMAGED, SNOW, NIGHT, USER_1…). Each block sets the <b>Model</b>, <b>Animation</b>(s) and mode, turret bones, which subobjects to show/hide, and weapon bone names (fire FX, launch, recoil, muzzle flash) per weapon slot. The best-matching block for the object's current flags is used. <b>AliasConditionState</b> reuses the previous block for other flags, <b>TransitionState</b> plays animations between states (via TransitionKey). Other fields control recoil, house colour, track marks, extra public bones and LOD.""",
 ex="""Draw = W3DModelDraw ModuleTag_Draw
  OkToChangeModelColor = Yes
  TrackMarks = EXTireTrack.tga
  DefaultConditionState
    Model              = AVCrusader
    Turret             = Turret01
    WeaponFireFXBone   = PRIMARY Muzzle
    WeaponRecoilBone   = PRIMARY Barrel
    WeaponMuzzleFlash  = PRIMARY MuzzleFX
    WeaponLaunchBone   = PRIMARY Muzzle
  End
  ConditionState = REALLYDAMAGED
    Model = AVCrusader_D
  End
  AliasConditionState = REALLYDAMAGED MOVING
  ConditionState = RUBBLE
    Model = AVCrusader_D
    ParticleSysBone = Smoke01 SmokeBlack
  End
End""")

M['W3DLaserDraw'] = dict(
 sum="Draws laser beams (multi-layered textured beams, optional arc) for laser objects driven by LaserUpdate.",
 body="""Renders the beam between the laser's endpoints: <b>NumBeams</b> concentric beams interpolating from <b>InnerBeamWidth</b>/<b>InnerColor</b> to <b>OuterBeamWidth</b>/<b>OuterColor</b>, textured with <b>Texture</b> (scrolling at <b>ScrollRate</b>, tiled if <b>Tile</b>). The beam is at full intensity for <b>MaxIntensityLifetime</b>, then fades over <b>FadeLifetime</b>. With <b>Segments</b> > 1 and <b>ArcHeight</b> the beam curves (e.g. electric arcs).""",
 ex="""Draw = W3DLaserDraw ModuleTag_Draw
  NumBeams       = 3
  InnerBeamWidth = 0.5
  OuterBeamWidth = 3.0
  InnerColor     = R:255 G:255 B:255 A:255
  OuterColor     = R:255 G:0 B:0 A:128
  MaxIntensityLifetime = 300
  FadeLifetime   = 200
  Texture        = EXLaser.tga
  ScrollRate     = -2.5
  Tile           = Yes
  Segments       = 1
End""")
F.update({
'W3DLaserDrawModuleData.NumBeams': ("Number of concentric beams drawn.", "3"),
'W3DLaserDrawModuleData.InnerBeamWidth': ("Width of the innermost beam.", "0.5"),
'W3DLaserDrawModuleData.OuterBeamWidth': ("Width of the outermost beam.", "3.0"),
'W3DLaserDrawModuleData.InnerColor': ("Colour of the innermost beam (R:G:B:A).", "R:255 G:255 B:255 A:255"),
'W3DLaserDrawModuleData.OuterColor': ("Colour of the outermost beam.", "R:255 G:0 B:0 A:128"),
'W3DLaserDrawModuleData.MaxIntensityLifetime': ("Time (ms) at full intensity.", "300"),
'W3DLaserDrawModuleData.FadeLifetime': ("Time (ms) to fade out afterwards.", "200"),
'W3DLaserDrawModuleData.Texture': ("Beam texture.", "EXLaser.tga"),
'W3DLaserDrawModuleData.ScrollRate': ("Texture scroll speed along the beam.", "-2.5"),
'W3DLaserDrawModuleData.Tile': ("If Yes the texture tiles along the beam length.", "Yes"),
'W3DLaserDrawModuleData.Segments': ("Number of segments (for curved/arced beams).", "1"),
'W3DLaserDrawModuleData.ArcHeight': ("Height of the arc when Segments > 1.", "0"),
'W3DLaserDrawModuleData.SegmentOverlapRatio': ("Overlap between segments to hide seams.", "0"),
'W3DLaserDrawModuleData.TilingScalar': ("Scale of the texture tiling.", "1.0"),
})

F.update({
'W3DTankDrawModuleData.TreadDebrisLeft': ("Particle system spawned behind the left tread while moving.", "TrackDebrisDirtLeft"),
'W3DTankDrawModuleData.TreadDebrisRight': ("Particle system spawned behind the right tread.", "TrackDebrisDirtRight"),
'W3DTankDrawModuleData.TreadAnimationRate': ("Tread texture scroll per second (1.0 = full texture width).", "2.0"),
'W3DTankDrawModuleData.TreadPivotSpeedFraction': ("Below this fraction of max speed, treads animate in opposite directions when turning (pivot).", "0.6"),
'W3DTankDrawModuleData.TreadDriveSpeedFraction': ("Below this fraction of max speed the treads stop animating.", "0.3"),
})
M['W3DTankDraw'] = dict(
 sum="W3DModelDraw for tracked vehicles: scrolling tread textures and tread debris particles.",
 body="""Everything W3DModelDraw does, plus: the tread meshes' texture UVs scroll with movement (<b>TreadAnimationRate</b>), counter-rotating when pivoting slowly (<b>TreadPivotSpeedFraction</b>), and tread debris particle systems (<b>TreadDebrisLeft/Right</b>) are emitted while moving.""",
 ex="""Draw = W3DTankDraw ModuleTag_Draw
  OkToChangeModelColor = Yes
  TrackMarks           = EXTankTrack.tga
  TreadAnimationRate   = 2.0
  TreadDebrisLeft      = TrackDebrisDirtLeft
  TreadDebrisRight     = TrackDebrisDirtRight
  DefaultConditionState
    Model  = AVCrusader
    Turret = Turret01
    WeaponFireFXBone = PRIMARY Muzzle
  End
End""")

M['W3DOverlordTankDraw'] = dict(
 sum="W3DTankDraw for the Overlord: explicitly draws the rider (add-on) right after itself.",
 body="""Same as W3DTankDraw, but draws the OverlordContain rider (Gattling/Propaganda/Bunker add-on) immediately after itself and propagates hidden state to it, so the add-on stays in sync with the tank.""",
 ex="""Draw = W3DOverlordTankDraw ModuleTag_Draw
  OkToChangeModelColor = Yes
  TreadAnimationRate   = 2.0
  DefaultConditionState
    Model  = NVOverlord
    Turret = Turret01
  End
End""")

F.update({
'W3DTruckDrawModuleData.Dust': ("Particle system of dust kicked up while driving.", "RocketBuggyDust"),
'W3DTruckDrawModuleData.DirtSpray': ("Particle system of dirt sprayed by the wheels.", "RocketBuggyDirtSpray"),
'W3DTruckDrawModuleData.PowerslideSpray': ("Particle system when powersliding.", "RocketBuggyDirtPowerSlide"),
'W3DTruckDrawModuleData.LeftFrontTireBone': ("Bone of the left front tyre (rotates and steers).", "Tire01"),
'W3DTruckDrawModuleData.RightFrontTireBone': ("Bone of the right front tyre.", "Tire02"),
'W3DTruckDrawModuleData.LeftRearTireBone': ("Bone of the left rear tyre.", "Tire03"),
'W3DTruckDrawModuleData.RightRearTireBone': ("Bone of the right rear tyre.", "Tire04"),
'W3DTruckDrawModuleData.MidLeftFrontTireBone': ("Extra tyre bone (for up to 8 tyres).", "Tire05"),
'W3DTruckDrawModuleData.MidRightFrontTireBone': ("Extra tyre bone.", "Tire06"),
'W3DTruckDrawModuleData.MidLeftRearTireBone': ("Extra tyre bone.", "Tire07"),
'W3DTruckDrawModuleData.MidRightRearTireBone': ("Extra tyre bone.", "Tire08"),
'W3DTruckDrawModuleData.MidLeftMidTireBone': ("Extra middle tyre bone.", ""),
'W3DTruckDrawModuleData.MidRightMidTireBone': ("Extra middle tyre bone.", ""),
'W3DTruckDrawModuleData.TireRotationMultiplier': ("Tyre rotation per unit of speed.", "0.2"),
'W3DTruckDrawModuleData.PowerslideRotationAddition': ("Extra tyre spin while powersliding.", "0.3"),
'W3DTruckDrawModuleData.CabBone': ("Cab bone for articulated trucks.", "Cab"),
'W3DTruckDrawModuleData.TrailerBone': ("Trailer bone for articulated trucks.", "Trailer"),
'W3DTruckDrawModuleData.CabRotationMultiplier': ("How much the cab turns relative to steering.", "0.5"),
'W3DTruckDrawModuleData.TrailerRotationMultiplier': ("How much the trailer swings.", "0.5"),
'W3DTruckDrawModuleData.RotationDamping': ("Damping of cab/trailer rotation.", "0.7"),
})
M['W3DTruckDraw'] = dict(
 sum="W3DModelDraw for wheeled vehicles: spinning/steering tyres, suspension, dust/dirt sprays, articulated cab/trailer.",
 body="""Everything W3DModelDraw does, plus: tyre bones rotate with speed (<b>TireRotationMultiplier</b>), front tyres steer, suspension bounces, dust/dirt/powerslide particle systems play, and articulated trucks bend at <b>CabBone</b>/<b>TrailerBone</b>.""",
 ex="""Draw = W3DTruckDraw ModuleTag_Draw
  OkToChangeModelColor = Yes
  Dust               = RocketBuggyDust
  DirtSpray          = RocketBuggyDirtSpray
  PowerslideSpray    = RocketBuggyDirtPowerSlide
  LeftFrontTireBone  = Tire01
  RightFrontTireBone = Tire02
  LeftRearTireBone   = Tire03
  RightRearTireBone  = Tire04
  TireRotationMultiplier = 0.2
  PowerslideRotationAddition = 0.3
  DefaultConditionState
    Model = GVTechnical
  End
End""")

M['W3DOverlordTruckDraw'] = dict(
 sum="W3DTruckDraw that draws its contained rider immediately after itself (wheeled Overlord-style carriers).",
 body="""Same as W3DTruckDraw, plus the Overlord rider-drawing behaviour (draws and hides/shows the contained add-on in sync with itself).""",
 ex="""Draw = W3DOverlordTruckDraw ModuleTag_Draw
  LeftFrontTireBone  = Tire01
  RightFrontTireBone = Tire02
  DefaultConditionState
    Model = MyCarrierTruck
  End
End""")

M['W3DOverlordAircraftDraw'] = dict(
 sum="W3DModelDraw for aircraft carrying a portable-structure rider (Helix): draws the rider right after itself.",
 body="""Same as W3DModelDraw plus the Overlord rider-drawing behaviour, used by the China Helix so its add-on (Gattling, Bunker, Propaganda) is drawn attached.""",
 ex="""Draw = W3DOverlordAircraftDraw ModuleTag_Draw
  DefaultConditionState
    Model = NVHelix
  End
End""")

M['W3DProjectileStreamDraw'] = dict(
 sum="Draws a textured ribbon connecting all projectiles of a stream (Toxin/Flame sprays), with ProjectileStreamUpdate.",
 body="""Renders a strip of <b>Texture</b> of <b>Width</b> through the projectiles tracked by ProjectileStreamUpdate, tiled <b>TileFactor</b> times and scrolling at <b>ScrollRate</b>, with at most <b>MaxSegments</b> segments.""",
 ex="""Draw = W3DProjectileStreamDraw ModuleTag_Draw
  Texture     = EXToxicStream.tga
  Width       = 5
  TileFactor  = 1
  ScrollRate  = 1.5
  MaxSegments = 20
End""")
F.update({
'W3DProjectileStreamDrawModuleData.Texture': ("Texture of the stream ribbon.", "EXToxicStream.tga"),
'W3DProjectileStreamDrawModuleData.Width': ("Width of the ribbon.", "5"),
'W3DProjectileStreamDrawModuleData.TileFactor': ("How many times the texture tiles along the ribbon.", "1"),
'W3DProjectileStreamDrawModuleData.ScrollRate': ("Texture scroll speed.", "1.5"),
'W3DProjectileStreamDrawModuleData.MaxSegments': ("Maximum number of segments drawn.", "20"),
})

M['W3DPoliceCarDraw'] = dict(
 sum="W3DTruckDraw with flashing police-light dynamic lights.",
 body="""Same fields as W3DTruckDraw; additionally creates a flashing red/blue dynamic light on the car (civilian police cars).""",
 ex="""Draw = W3DPoliceCarDraw ModuleTag_Draw
  LeftFrontTireBone  = Tire01
  RightFrontTireBone = Tire02
  DefaultConditionState
    Model = CVPoliceCar
  End
End""")

M['W3DRopeDraw'] = dict(
 sum="Draws a rope (Chinook rappel ropes). Configured at runtime by the code that creates the rope.",
 body="""Used on the rope object created by ChinookAIUpdate; length, width, colour and wobble are set by that module at runtime. No INI fields.""",
 ex="""Draw = W3DRopeDraw ModuleTag_Draw
End""")

M['W3DScienceModelDraw'] = dict(
 sum="W3DModelDraw that only draws if the local player owns a given science.",
 body="""Same as W3DModelDraw, but invisible unless the local player has <b>RequiredScience</b> (e.g. showing special markers only to players with a certain general's power).""",
 ex="""Draw = W3DScienceModelDraw ModuleTag_Draw
  RequiredScience = SCIENCE_SomePower
  DefaultConditionState
    Model = MyMarker
  End
End""")
F.update({'W3DScienceModelDrawModuleData.RequiredScience': ("Local player must have this science for the model to draw.", "SCIENCE_SomePower")})

M['W3DSupplyDraw'] = dict(
 sum="W3DModelDraw that hides supply-box bones as the warehouse/pile is depleted.",
 body="""Same as W3DModelDraw; as supplies are taken, bones named <b>SupplyBonePrefix</b>01…NN are hidden proportionally, so the pile visibly shrinks.""",
 ex="""Draw = W3DSupplyDraw ModuleTag_Draw
  SupplyBonePrefix = SUPPLY
  DefaultConditionState
    Model = CBSupplyDock
  End
End""")
F.update({'W3DSupplyDrawModuleData.SupplyBonePrefix': ("Prefix of the supply-box bones hidden as supplies run out.", "SUPPLY")})

M['W3DDependencyModelDraw'] = dict(
 sum="W3DModelDraw for riders/add-ons: only draws when its container draws, attached to a bone of the container.",
 body="""Used by Overlord/Helix add-ons and OverlordContainV2 riders: the model is drawn only when the container tells it to (right after itself), attached to <b>AttachToBoneInContainer</b> on the container's model.""",
 ex="""Draw = W3DDependencyModelDraw ModuleTag_Draw
  AttachToBoneInContainer = TURRETPOS
  DefaultConditionState
    Model = NVOverlordGattling
  End
End""")
F.update({'W3DDependencyModelDrawModuleData.AttachToBoneInContainer': ("Bone on the container's model to attach this model to.", "TURRETPOS")})

M['W3DTracerDraw'] = dict(
 sum="Draws bullet tracers (short bright streaks). Configured by the weapon/FX that creates them.",
 body="""Used on tracer objects; length, width, colour and speed are set at runtime by the creating code (Tracer FX nugget). No INI fields.""",
 ex="""Draw = W3DTracerDraw ModuleTag_Draw
End""")

M['W3DTankTruckDraw'] = dict(
 sum="Draw module for vehicles with both treads and wheels (half-tracks): combines W3DTankDraw and W3DTruckDraw.",
 body="""Has the truck fields (dust, dirt spray, tyre bones, tyre rotation) and the tank fields (tread debris, tread animation) at the same time.""",
 ex="""Draw = W3DTankTruckDraw ModuleTag_Draw
  Dust               = TankDust
  LeftFrontTireBone  = Tire01
  RightFrontTireBone = Tire02
  TireRotationMultiplier = 0.2
  TreadAnimationRate = 2.0
  TreadDebrisLeft    = TrackDebrisDirtLeft
  TreadDebrisRight   = TrackDebrisDirtRight
  DefaultConditionState
    Model = MyHalftrack
  End
End""")
for k in ['Dust','DirtSpray','PowerslideSpray','LeftFrontTireBone','RightFrontTireBone','LeftRearTireBone','RightRearTireBone','MidLeftFrontTireBone','MidRightFrontTireBone','MidLeftRearTireBone','MidRightRearTireBone','TireRotationMultiplier','PowerslideRotationAddition']:
    F['W3DTankTruckDrawModuleData.'+k] = F['W3DTruckDrawModuleData.'+k]
for k in ['TreadDebrisLeft','TreadDebrisRight','TreadAnimationRate','TreadPivotSpeedFraction','TreadDriveSpeedFraction']:
    F['W3DTankTruckDrawModuleData.'+k] = F['W3DTankDrawModuleData.'+k]

M['W3DTreeDraw'] = dict(
 sum="Efficient batched tree drawing with push-aside and topple behaviour.",
 body="""Map trees use this instead of W3DModelDraw; trees are batched for performance. Units driving through push the tree aside (<b>MoveOutwardTime</b>, <b>MoveInwardTime</b>, <b>MoveOutwardDistanceFactor</b>). With <b>DoTopple</b> it can topple like ToppleUpdate (FX, stump, velocity/accel/bounce) and then sink (<b>SinkDistance</b> over <b>SinkTime</b>). <b>DarkeningFactor</b> darkens the tree, <b>DoShadow</b> enables its shadow.""",
 ex="""Draw = W3DTreeDraw ModuleTag_Draw
  ModelName    = PTOak01
  TextureName  = PTOak01.tga
  MoveOutwardTime = 300
  MoveInwardTime  = 1500
  MoveOutwardDistanceFactor = 1.0
  DoTopple     = Yes
  ToppleFX     = FX_TreeFall
  StumpName    = TreeStumpSmall
  KillWhenFinishedToppling = Yes
  SinkDistance = 20
  SinkTime     = 10000
  DoShadow     = Yes
End""")
F.update({
'W3DTreeDrawModuleData.ModelName': ("W3D model of the tree.", "PTOak01"),
'W3DTreeDrawModuleData.TextureName': ("Texture of the tree.", "PTOak01.tga"),
'W3DTreeDrawModuleData.MoveOutwardTime': ("Time (ms) to bend away when pushed by a unit.", "300"),
'W3DTreeDrawModuleData.MoveInwardTime': ("Time (ms) to spring back.", "1500"),
'W3DTreeDrawModuleData.MoveOutwardDistanceFactor': ("How far the tree bends away.", "1.0"),
'W3DTreeDrawModuleData.DarkeningFactor': ("Amount the tree colour is darkened.", "0"),
'W3DTreeDrawModuleData.ToppleFX': ("FXList when it starts toppling.", "FX_TreeFall"),
'W3DTreeDrawModuleData.BounceFX': ("FXList when it bounces on the ground.", "FX_TreeBounce"),
'W3DTreeDrawModuleData.StumpName': ("Object created as the stump.", "TreeStumpSmall"),
'W3DTreeDrawModuleData.KillWhenFinishedToppling': ("Kill the tree once toppled.", "Yes"),
'W3DTreeDrawModuleData.DoTopple': ("If Yes, the tree can topple when hit by vehicles.", "Yes"),
'W3DTreeDrawModuleData.InitialVelocityPercent': ("Initial topple angular velocity (percent).", "20%"),
'W3DTreeDrawModuleData.InitialAccelPercent': ("Topple angular acceleration (percent).", "1%"),
'W3DTreeDrawModuleData.BounceVelocityPercent': ("Velocity kept after bouncing (percent).", "30%"),
'W3DTreeDrawModuleData.MinimumToppleSpeed': ("Minimum toppling speed.", "0.5"),
'W3DTreeDrawModuleData.SinkDistance': ("How far the toppled tree sinks into the ground.", "20"),
'W3DTreeDrawModuleData.SinkTime': ("Time (ms) to sink after toppling.", "10000"),
'W3DTreeDrawModuleData.DoShadow': ("If Yes, the tree casts a shadow.", "Yes"),
})

M['W3DPropDraw'] = dict(
 sum="Very cheap batched draw for static props (rocks, debris) that never animate.",
 body="""Draws a static model <b>ModelName</b> through the batched prop renderer. No condition states, no animation — just a fast static prop.""",
 ex="""Draw = W3DPropDraw ModuleTag_Draw
  ModelName = PRRock01
End""")
F.update({'W3DPropDrawModuleData.ModelName': ("W3D model of the prop.", "PRRock01")})

M['W3DPersistentAnimModelDraw'] = dict(
 sum="W3DModelDraw whose animations keep playing while the object is disabled (EMP, subdued, hacked, paralyzed, unmanned) for chosen types.",
 body="""Mod-original. Every W3DModelDraw field works unchanged. Normally the engine freezes animation when an object is disabled by HACKED, PARALYZED, EMP, SUBDUED or UNMANNED, regardless of AnimationsRequirePower. This module overrides the pause request and silently ignores it when every active disable type is in <b>AnimateThroughDisabledTypes</b> (default ALL), so e.g. a stunned unit can keep playing its idle/glow animation. Other pause reasons (types not listed, or underpowered with AnimationsRequirePower) pass through normally. Only those five types matter; listing others is accepted but inert.""",
 ex="""Draw = W3DPersistentAnimModelDraw ModuleTag_Draw
  AnimateThroughDisabledTypes = ALL -HACKED
  DefaultConditionState
    Model         = MyStunUnit
    Animation     = MyStunUnit.Idle
    AnimationMode = LOOP
  End
End""")
F.update({'W3DPersistentAnimModelDrawModuleData.AnimateThroughDisabledTypes': ("Disabled types that should NOT pause this draw's animation (ALL/NONE/+X/-X). Default ALL. Only HACKED, PARALYZED, EMP, SUBDUED, UNMANNED have an effect.", "ALL -HACKED")})

M['W3DWheeledTankDraw'] = dict(
 sum="W3DTankDraw plus skid-steer wheel bones that spin together when driving and counter-rotate when turning.",
 body="""Mod-original. Everything W3DTankDraw does (turret, tread debris, tread UV scrolling) plus two wheel-bone lists: <b>LeftTireBones</b> and <b>RightTireBones</b>. Driving forward/backward spins both sides together with speed × <b>TireRotationMultiplier</b>; while turning (including pivoting in place) a counter-rotation term is added on top, so the sides spin in opposite directions like a skid-steered vehicle. Use for wheeled vehicles that steer by differential wheel speed (no steering angle/suspension, unlike W3DTruckDraw).""",
 ex="""Draw = W3DWheeledTankDraw ModuleTag_Draw
  TireRotationMultiplier = 0.2
  LeftTireBones  = Tire01 Tire02 Tire03 Tire04
  RightTireBones = Tire05 Tire06 Tire07 Tire08
  TreadDebrisLeft  = TrackDebrisDirtLeft
  TreadDebrisRight = TrackDebrisDirtRight
  DefaultConditionState
    Model  = MyWheeledAPC
    Turret = Turret01
  End
End""")
F.update({
'W3DWheeledTankDrawModuleData.TireRotationMultiplier': ("Radians of wheel rotation per frame per unit of speed; also used (not scaled by speed) as the counter-rotation amount while turning.", "0.2"),
'W3DWheeledTankDrawModuleData.LeftTireBones': ("Space-separated left-side wheel bones (any count).", "Tire01 Tire02 Tire03 Tire04"),
'W3DWheeledTankDrawModuleData.RightTireBones': ("Space-separated right-side wheel bones (any count).", "Tire05 Tire06 Tire07 Tire08"),
})

M['W3DBreakApartPieceDraw'] = dict(
 sum="Draw module for BreakApartDeathBehaviorV2 debris pieces: shows exactly one subobject of a reference model, set at spawn.",
 body="""Mod-original. Has no INI fields. The object it's on is spawned by BreakApartDeathBehaviorV2 (BreakApartPieceOCL / RemainPieceOCL); right after spawning, that module tells this draw which reference model and which bone/subobject to display. It clones the whole reference model, hides every other subobject, and positions it exactly like the dying unit so the one visible part lines up where it was, then follows the piece as it is flung by physics. The remainder variant shows everything that was not broken off. Put this on the generic debris object template used by the OCL.""",
 ex="""Object BreakApartPiece
  KindOf = NO_COLLIDE
  Draw = W3DBreakApartPieceDraw ModuleTag_Draw
  End
  Behavior = PhysicsBehavior ModuleTag_Physics
    Mass = 5
    AllowBouncing = Yes
    KillWhenRestingOnGround = Yes
  End
  Behavior = LifetimeUpdate ModuleTag_Life
    MinLifetime = 6000
    MaxLifetime = 8000
  End
  Body = InactiveBody ModuleTag_Body
  End
End""")

M['W3DOverlordWheeledTankDraw'] = dict(
 sum="W3DWheeledTankDraw + W3DOverlordTankDraw: wheeled skid-steer draw that also draws its Overlord rider(s).",
 body="""Mod-original. Everything W3DWheeledTankDraw does (W3DTankDraw + differential Left/RightTireBones spin) plus the Overlord rider drawing (draws contained rider(s) right after itself, propagates hidden state), including OverlordContainV2's multiple visible riders. No new fields.""",
 ex="""Draw = W3DOverlordWheeledTankDraw ModuleTag_Draw
  TireRotationMultiplier = 0.2
  LeftTireBones  = Tire01 Tire02 Tire03
  RightTireBones = Tire04 Tire05 Tire06
  DefaultConditionState
    Model  = MyWheeledOverlord
    Turret = Turret01
  End
End""")
