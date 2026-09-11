# GeneralsMod: new engine module checklist

> Personal dev note for this mod (not an upstream TheSuperHackers doc — see `AI_POLICY.md`/
> `CONTRIBUTING.md` for those). Check this whenever adding a new module class (a new
> `AIUpdate`/`Behavior`/`Update`-style `.h`+`.cpp` pair) to the engine. Every item below has
> already caused a real link error or runtime crash once in this mod — skipping one means
> redoing this debugging loop again.

Scope: this mod only touches the `GeneralsMD/` (Zero Hour) tree, plus `Core/` when the change is
genuinely shared infrastructure. `Generals/` (base game) is deliberately left alone unless a
change specifically calls for it.

Whenever a **new module class** is added (e.g. `AIUpdateInterfaceV2`, subclassing something like
`AIUpdateInterface`/`AIUpdateModuleData`, using `MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE` +
`MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA`), **all** of the following are required, or the game
either fails to link or asserts/crashes the first time the module is instantiated:

1. **CMakeLists.txt** — add both the new header (`Include/...`) and source (`Source/...`) file to
   `GeneralsMD/Code/GameEngine/CMakeLists.txt` (explicit file list, not a glob).
   **No inline comments on these lines.** CMakeLists.txt only understands `#` as a comment
   marker, not the `///<` C++-style comment used elsewhere in this mod (ModuleFactory.cpp, the
   `.inl` pool table, etc.) — appending `///< ...` directly after a filename with no separating
   newline makes CMake treat it as part of the same list token, so `target_sources` fails with
   `Cannot find source file: Include/.../ClassName.h///< ...` (the error message truncates the
   filename, e.g. shown as ending in `.h/<`). Keep CMakeLists.txt entries as bare paths, no
   trailing comment. (First hit: `HealAIUpdateV2`, 11/09/2026.)

2. **ModuleFactory.cpp** (`GeneralsMD/Code/GameEngine/Source/Common/Thing/ModuleFactory.cpp`) —
   `#include` the new header and add `addModule( NewClassName );` next to its base class's
   registration. Without this the module is never INI-selectable at all.

3. **`crc()` / `xfer()` / `loadPostProcess()` definitions** — `MAKE_STANDARD_MODULE_MACRO` only
   *declares* these three (as `protected virtual ... override`), it never defines them. Every
   concrete module `.cpp` must define all three itself, even as a trivial pass-through to the base
   class (see `DozerAIUpdate::crc/xfer/loadPostProcess` for the canonical pattern). Skipping this
   is a **link** error: `LNK2001: unresolved external symbol ...::crc/xfer/loadPostProcess`.
   Needs `#include "Common/Xfer.h"` in that `.cpp` (not pulled in transitively).

3b. **Destructor definition** — `MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE` also only *declares* the
   destructor (`protected virtual ~ClassName()`), same as crc/xfer/loadPostProcess above — it never
   defines it. If the module needs no custom cleanup, define it with `EMPTY_DTOR( ClassName )`
   (declared in `Common/GameMemory.h`; see `DozerPrimaryIdleState` in `DozerAIUpdate.cpp` for the
   pattern) instead of writing an empty `{ }` body by hand. Skipping this is a **link** error:
   `LNK2019: unresolved external symbol ...::~ClassName(void)` referenced from the scalar deleting
   destructor. (First hit: `HealAIUpdateV2`, 11/09/2026.)

4. **Memory pool size entry** — `MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ClassName, "ClassName" )`
   requires a matching row in the pool-size table, keyed by that exact string, or the game
   `DEBUG_CRASH`s the first time it tries to instantiate one (`"Initial size for pool X not found
   -- you should add it to MemoryInit.cpp"`). The real file is **not** `MemoryInit.cpp` (that name
   in the assert message is stale/wrong) — it's
   `Core/GameEngine/Source/Common/System/GameMemoryInitPools_GeneralsMD.inl`
   (there's a parallel `_Generals.inl` for the base game, selected by `#if RTS_ZEROHOUR` in
   `GameMemoryInit.cpp` — only touch the `_GeneralsMD.inl` one for this mod). Add:
   `{ "ClassName", initialCount, overflowCount },` — size it like a comparable existing module
   (e.g. a per-unit-type subclass like `DozerAIUpdate` is `32, 32`; the ubiquitous base
   `AIUpdateInterface` is `600, 32`). **This step has been missed three times now — check this
   file every single time a new `MEMORY_POOL_GLUE...` class is added.**

5. **`#include "GameLogic/Object.h"`** — if the new `.cpp` calls any member function on an
   `Object*` (e.g. `candidate->getBodyModule()`), the plain forward declaration of `Object` pulled
   in by most `GameLogic` headers is not enough; without the real `Object.h` include you get
   `C2027: use of undefined type 'Object'` (MSVC only reports it at the first place the full
   layout is actually needed, not at every use, so it can look like it "only" affects one line).

6. **`getObject()` const-correctness** — `Module` provides both `Object *getObject()` and
   `const Object *getObject() const`. If a member function is marked `const`, `getObject()`
   silently resolves to the `const` overload and any `Object *x = getObject();` in it fails to
   compile (`C2440`). Don't mark a module's own scan/helper methods `const` unless they genuinely
   need to be.

## If your module calls aiAttackObject()/aiForceAttackObject() with CMD_FROM_AI

If a module auto-triggers an attack (`aiAttackObject(target, shots, CMD_FROM_AI)`) on a target that
may be beyond current weapon range, and the object can be human-controlled, the unit will find the
target fine but **never physically move toward it** -- it just sits there. Root cause:
`AIAttackApproachTargetState::onEnter()` (`GeneralsMD/.../GameLogic/AI/AIStates.cpp`) deliberately
returns `STATE_FAILURE` (refusing to chase) for a `PLAYER_HUMAN`-controlled unit whenever
`ai->getLastCommandSource() == CMD_FROM_AI`, unless `ai->isAllowedToChase()` is set. This is by
design (auto-acquired *enemies* for a human player are meant to only fight back in place, not go
walkabout) but it silently also blocks any other engine-internal auto-attack, including
non-enemy uses like a heal/repair behavior. Two ways around it, both already used elsewhere in this
codebase -- pick one deliberately, don't just copy whichever a precedent file happens to use:
- Call `ai->setAllowedToChase( TRUE )` before issuing the attack (same pattern
  `AssaultTransportAIUpdate.cpp` uses for its own chasing passengers). Keeps `CMD_FROM_AI` semantics.
- Use a different, non-`CMD_FROM_AI` `CommandSourceType` for the auto-triggered attack, the way
  `DozerAIUpdate.cpp`'s mine-scan uses `CMD_FROM_DOZER` specifically to sidestep this check.
(First hit: the original `AIUpdateInterfaceV2` almost certainly died to this, unconfirmed at the
time since the module was reverted before it could be proven; confirmed and fixed in
`HealAIUpdateV2`, 11/09/2026.)

## Also, for context (already-established patterns, not new)

- New `DamageType` enum values must be appended immediately before `DAMAGE_NUM_TYPES` (never
  inserted mid-enum — shifts existing values, breaks save/replay compat), and
  `DamageTypeFlags::s_bitNameList[]` in `Damage.cpp` must get a matching new entry, same order,
  right before the closing `nullptr`.
- Same append-only-before-the-`_COUNT`/`_NUM_TYPES` rule applies to `WeaponSetType`
  (`GeneralsMD/Code/GameEngine/Include/GameLogic/WeaponSetType.h`) — used to gate a real weapon
  behind a `WeaponSet{ Conditions = ... }` INI block that only your own module activates (see
  `WEAPONSET_MINE_CLEARING_DETAIL` for `DozerMineDisarmingWeapon`, and `WEAPONSET_HEALING_DETAIL`
  for `HealAIUpdateV2`'s healing weapon — both exist so the weapon can't be manually ordered against
  an arbitrary target the rest of the time). Three places must stay in sync, same order, or the
  `static_assert` in `WeaponSet.cpp` fails to compile: the enum itself (before `WEAPONSET_COUNT`),
  `WeaponSetFlags::s_bitNameList[]` in `WeaponSet.cpp` (the INI-parseable name, before the closing
  `nullptr`), and `TheWeaponSetTypeToModelConditionTypeMap[]` in `WeaponSet.h` (before `};` — use
  `MODELCONDITION_INVALID` unless the flag should also swap a model condition). Note `WeaponSet.h`
  and `WeaponSet.cpp` are CRLF (`\r\n`) in this codebase, unlike most other files here — an LF-only
  edit still compiles but silently mixes line endings in the file, so match it.
- Precedent files worth re-reading when building a new "V2"-style module: `DozerAIUpdate.h/.cpp`
  (subclassing `AIUpdateInterface`, full macro/xfer pattern) and `FireOCLBehaviorV2.h/.cpp`
  (an earlier module of this exact kind added in this mod, 08/09/2026).

## If your module tracks "isAttacking()" across frames to detect when to stop

`getCurrentVictim()` returns `nullptr` once the victim is gone (destroyed, sold, captured, or
otherwise no longer resolvable) -- but `isAttacking()` can keep reporting `true` regardless. If your
per-frame check only reacts to victim state *when a victim exists* (e.g. "if victim != nullptr and
victim is at full health, stop attacking"), a nulled-out victim falls through every check untouched:
you never call `aiIdle()`, `isIdle()` never goes true again, any bored-scan gated on `isIdle()` never
gets another chance to run, and the unit sits in the attack state animating at a target that no
longer exists -- indefinitely. Always treat `victim == nullptr` while `isAttacking()` as its own
"stop attacking" case, not just the health/condition check on a live victim. (Symptom in
`HealAIUpdateV2`, 11/09/2026: unit kept playing its fire animation at empty space for the rest of
the game after its heal target vanished mid-heal.)

## Update: the engine's own "victim died, stop attacking" check turned out to be reliable -- the miss was ours

Follow-up to the "isAttacking() tracking" section above, and a correction to an earlier entry in
this same file. It was initially assumed `AIAttackFireWeaponState::update()`'s own
`if (!victim || victim->isEffectivelyDead()) return STATE_FAILURE;` only ran while that one specific
inner sub-state (aim -> approach/pursue -> fire -> wait-between-shots -> ...) was active, and that a
death during a different sub-state would slip through uncaught. Reading `AIAttackState::update()`
itself (the *outer* state, one level above all those inner sub-states) disproved that: it runs its
own `if (victim == nullptr || victim->isEffectivelyDead()) { ...; return STATE_SUCCESS; }` check
unconditionally, every single frame, regardless of which inner sub-state is currently active --
confirmed via diagnostic logging too (see Status below). The engine's native death-detection is not
the gap. The actual gap was on our side: a module that adds its own extra "stop attacking early"
condition (like heal-target-at-full-health, which the engine has no concept of) naturally only
force-clears animation flags on the transition *it* triggers -- and never on the transition the
engine triggers on its own. Don't add a redundant manual victim-null/dead check just to be safe --
it can't ever fire (the engine's outer per-frame check always gets there first), and it clutters the
one specific condition your own module actually needs to detect. Instead:

## The real fix: clear fire-loop flags unconditionally whenever isAttacking() is false, not just on your own stop transitions

The stuck-arm-animation symptom (unit keeps playing its firing/between-shots/reloading/preattack/
turret-rotate animation forever, well after the attack has actually ended) happens whenever
`isAttacking()` goes `false` **without** your own "stop attacking" code path having run that frame
-- which is exactly what happens every time the engine's own native death-detection (see above) ends
the attack on its own. Confirmed via diagnostic logging in `HealAIUpdateV2` (11/09/2026): the frame
the victim died, `isAttacking()` read `false` the very next time the module checked it, with no
`STOPPING`/`POST-STOP` log line from our own code in between -- proving our own stop-transition code
never ran for that case at all, so whatever it force-cleared was never reached. Fix: don't gate the
flag-clear on your own detected stop reason. In the module's per-frame `update()`, structure it as:

```cpp
if( isAttacking() )
{
    // ... your module's own extra stop conditions here (the engine already handles
    // victim-null/victim-died on its own, every frame -- see above) ...
}
else
{
    // unconditional: whoever ended the attack, however it ended, make sure the animation
    // flags actually reflect "not attacking" -- self->clearModelConditionFlags(...) on
    // PREATTACK_A / FIRING_A / BETWEEN_FIRING_SHOTS_A / RELOADING_A / TURRET_ROTATE, but
    // only if any are still set (cheap early-out, this runs every idle frame forever).
}
```
This is the one piece that actually matters. A module can still also force-clear immediately inside
its own stop-transition code (nicer for the one-frame-earlier visual snap, see `endHealAttack()` in
`HealAIUpdateV2.cpp`), but that's an optimization on top of the unconditional else-branch clear, not
a substitute for it.

## Gotcha: Object has no getter for its own ModelConditionFlags

`Object` only exposes setters/clearers for model conditions (`setModelConditionState`,
`clearModelConditionState`, `clearAndSetModelConditionFlags`, etc.) -- there is no
`Object::testModelConditionState()` or similar. The actual flags live on the `Drawable`
(`Drawable::getModelConditionFlags()` returns `const ModelConditionFlags&`, a `BitFlags<>` with the
usual `.test(flag)`), which makes sense since it's purely client-side/visual state. To read a flag
back, go through `object->getDrawable()->getModelConditionFlags().test(...)` -- and
`#include "GameClient/Drawable.h"` explicitly, since `GameLogic/Object.h` only forward-declares
`class Drawable`.

## Status: HealAIUpdateV2 arm-stuck-in-loop bug is RESOLVED (11/09/2026)

Root cause and fix are both described above. Of the three earlier rounds of C++ changes chased
while this was still unresolved (null-victim check, dead-victim check, then force-clearing flags
alongside `aiIdle()` -- all three folded into an `endHealAttack()` helper called only from our own
stop-transition checks), the null/dead-victim check turned out to be dead code once the real fix
landed -- it was removed. It could never fire: by the time our own per-frame check ran, the engine's
own outer unconditional check (see above) had already exited the attack state first, every time. Kept:
the full-health check (genuinely unique to this module -- the engine has no equivalent), and the
`endHealAttack()` helper itself (now called from just that one place). The temporary `DEBUG_LOG`
instrumentation that found the actual root cause has been stripped back out now that the fix is
confirmed working in-game -- see the logging practice note below for why it was added instantly in
the first place, and why it came back out promptly once its job was done.

## Practice: add debug logging at critical points the moment a feature is built, strip it once confirmed working

Don't wait for a bug report to start instrumenting. The moment a new module (or a non-trivial change
to an existing one) is built -- especially anything with multiple stop/transition conditions, like
an AI module's "why did I stop attacking/moving/doing X this frame" logic -- add `DEBUG_LOG` calls
at the handful of points that actually matter: state transitions, the specific per-frame checks that
decide whether to keep going or stop, and a warning-style log for "this looks like it shouldn't be
possible" states. Cheap to add up front; expensive to reconstruct later once the person reporting
the bug is three play sessions removed from it and can only describe the symptom in general terms.
The `HealAIUpdateV2` stuck-arm bug (11/09/2026) took three blind rounds of guess-and-check C++
fixes before diagnostic logging was added -- the log from a single repro then pinpointed the actual
root cause in one read. Add the logging *instantly* when the feature goes in, not as a last resort
after guessing has already failed a few times. Once the bug is fixed and confirmed (in-game, by the
user, not just "compiles and looks right"), strip the temporary logging back out in the same pass --
DEBUG_LOG calls left in permanently add per-frame overhead and log noise for behavior that's no
longer in question. A log line that's clearly permanent-and-useful (not tied to hunting one specific
bug) can stay; anything added just to catch this one issue should go.
