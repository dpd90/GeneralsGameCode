# Module Codex generator

Builds `generals-module-reference.html` (repo root), a searchable reference covering every module
registered in `GeneralsMD/.../ModuleFactory.cpp` and `Core/GameEngineDevice/.../W3DModuleFactory.cpp`.

## Rebuild

```
python3 scripts/module-codex/extract.py   # scans the source -> modules_raw.json (~1 min)
python3 scripts/module-codex/build.py     # writes ../../generals-module-reference.html
```

## After adding a new module

1. Register it as usual (ModuleFactory / W3DModuleFactory). The extractor picks it up automatically,
   together with its FieldParse table, inherited fields, defaults and code comments.
2. Run both scripts. `build.py` lists any module that has no written description yet. The module still
   appears on the page with its extracted fields and a placeholder text.
3. Add a description in `desc/` (a new `d08.py` is fine), following the existing shape:
   - `M['ModuleName'] = dict(sum=..., body=..., ex=...)` gives a one-line summary, an HTML description and an INI example.
   - `F['ModuleDataClass.Field'] = ("what it does", "example value")` adds one entry per field.
4. For a V2 module that builds on a vanilla one, add it to `PAIRS` in `build.py` so the page offers
   the "Compare with ..." button. A name ending in `V2` is automatically badged as mod-original.
   Other mod-original names go in `MOD_EXTRA`.

Fields missing from `F` fall back to the vanilla counterpart's description (for V2 copies), then to
a field with the same name on another module, then to the source comment.
