# CLAUDE.md

Guidance for coding agents (and new developers) working in this repository.

## What this is

Carbon Trinity: Fenris Creations' rendering engine for the Carbon game engine. C++, built per graphics
API as Python extension modules named `_trinity_<api>[_<flavor>]` (for example
`_trinity_dx12_trinitydev`; on Windows the extension suffix is `.pyd`, while on macOS/Linux it's typically `.so`). The engine is scripted from Python through the "Blue" exposure
layer (see below). The primary consumer is Fenris's internal game codebase, which imports it as
`import trinity`; the repo can also be built standalone (public headers in `trinity/Include/`).

## Repository structure

- `trinity/` - main engine library: meshes, models, effects, particles, lights, post-process,
  raytracing, resources. `Eve/` holds game-layer classes, `Include/` the public headers.
- `trinityal/` - graphics abstraction layer (RHI) with `dx11/`, `dx12/`, `metal/`, `stub/`
  backends and a GoogleTest suite in `tests/`.
- `shadercompiler/` - HLSL-to-backend effect compiler, with its own `tests/` and `python/`
  bindings.
- `cmake/` - CCP build helper modules (`Ccp*.cmake`).
- `vendor/` - git submodules only: `microsoft/vcpkg` and `carbonengine/vcpkg-registry`.
- `python/upgrade_scripts/` - content-migration helper scripts.

Naming conventions: `Tr2*` core engine classes, `Tri*` low-level math/device primitives, `Eve*`
game-layer classes, `*AL` / `*ALDx11` / `*ALDx12` in trinityal, `I*` interfaces.

No shader source or game content lives in this repo; the only shaders are test assets under
`trinityal/tests/`.

## Building

vcpkg manifest mode using the vendored submodules as toolchain. Before the first configure run
`git submodule update --init --recursive`.

Blessed Windows dev recipe:

```
cmake --preset x64-windows-trinitydev -DBUILD_DX11=ON -DBUILD_DX12=ON
cmake --build .cmake-build-x64-windows-trinitydev --config TrinityDev --target trinity_dx11 trinity_dx12 trinity_stub
```

Presets follow `<arch>-<os>-<flavor>` (see `CMakePresets.json`); flavors are Debug / TrinityDev /
Internal / Release. macOS presets build the metal backend. Personal presets belong in
`CMakeUserPresets.json`, which is gitignored. Binary dirs are `.cmake-build-<preset>`.

Tripwires:

- `BUILD_DX11` / `BUILD_DX12` / `BUILD_METAL` default OFF. A bare configure builds only the stub
  backend; if your dx targets are missing, this is why.
- The Windows generator is Visual Studio (multi-config). `--config` is mandatory on every build
  and must match the preset's flavor, or you silently build the wrong configuration.
- CMake writes status messages and warnings to stderr. PowerShell 5.1 with
  `$ErrorActionPreference = 'Stop'` turns that into a spurious NativeCommandError even on a
  successful run; judge success by `$LASTEXITCODE`, not by the presence of stderr output.

## The Blue exposure layer

Most engine classes `Foo.{h,cpp}` have a sibling `Foo_Blue.cpp` (about 500 in the repo) defining
Foo's Python surface. Only members mapped there (`MAP_ATTRIBUTE`, `MAP_METHOD_AND_WRAP`,
`MAP_INTERFACE`) are reachable from Python. When editing any class that has a `_Blue.cpp` sibling:

1. A new public member intended for Python use MUST be mapped in the `_Blue.cpp`, or it is
   invisible from Python. The symptom is "my change has no effect" despite a clean build and
   deploy. Do not reflexively expose every new member either: exposure is a public-API
   commitment, so confirm the member is actually meant for Python before mapping it.
2. Renaming or removing a C++ member leaves a stale `_Blue.cpp` entry that breaks the build.
   Update the `_Blue.cpp` in the same change.
3. Exposed names AND their semantics are a public API with consumers outside this repo. Before
   renaming, removing, or reducing an exposed attribute to a no-op, check the consuming python
   codebases (grep the consuming game workspace if you have one); if you cannot check, flag the
   change as potentially breaking in the PR. A silently no-op'd attribute passes every build here
   and breaks consumers you cannot see.

Match the style of nearby `_Blue.cpp` files when adding entries.

## Reaching the game

Built modules carry a flavor postfix (for example `_trinity_dx12_trinitydev.pyd`). A host
application importing the unsuffixed module name fails with `ModuleNotFoundError` unless it is
launched in the matching build flavor.

With `INSTALL_TO_MONOLITH=ON` the build installs into a vendor-style layout consumed by the game
workspace. For a local build to actually load there, the workspace's `carbon.json` must pin
carbon-trinity to the local drop (not a tagged release), and the workspace's `updateBinaries.py`
step must be re-run after every build to copy the DLLs into the runtime location.

Stale deployed DLLs are the most common cause of "my change isn't showing in-game". Verify the
deployed binaries are fresh (and the `carbon.json` pin is `local`) before debugging the code.

## Verification

Before declaring a change complete:

1. Build all three Windows targets: `trinity_dx11 trinity_dx12 trinity_stub`. A single-target
   build misses stale `_Blue.cpp` entries and per-backend breaks.
2. When touching `trinityal/` or `shadercompiler/`, build and run the matching GoogleTest targets
   (for example `TrinityALTest_dx11`, `TrinityALTest_dx12`).
3. clang-format the diff before pushing; CI (`.github/workflows/cpp_format.yml`) hard-fails PRs
   on formatting.
4. `trinity/` itself has no unit tests. Rendering changes are verified visually in a host
   application; a successful compile is not verification. Report explicitly what was and was not
   verified.

## Gotchas

- An effect's resolved path may end in a suffix like `.sm_depth`. This is internal permutation
  naming, not technique semantics: `sm` does not mean shadow-map or depth-only.
- An effect with empty `parameters` / `resources` / `options` lists is not necessarily broken;
  minimal vertex-driven shaders legitimately expose nothing. Cross-check `effectResource.isGood`
  and whether the object actually renders before concluding a shader is missing.
