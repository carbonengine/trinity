# Carbon trinity
Rendering engine for the Carbon Game Engine

## 🛠️ Building

### Prerequisites

- CMake 3.31 or newer
- Visual Studio with the **v141 (VS 2017) C++ build tools** component
- GitHub SSH access — the dependencies are submodules cloned over SSH

```powershell
git clone --recurse-submodules <url>
```

Or if already cloned then `git submodule update --init --recursive`

### Generate a solution

Generate a solution, then you can handle the rest from you IDE for local dev workflows:

```powershell
cmake --preset x64-windows-internal -A x64 -T v141
```

This will generate an .slnx at `.cmake-build-<preset-name>/`. Run `cmake --list-presets` to see the presets.

### Working with Monolith

To install to a trinity next to the rest of the engine components, add the destination when generating the solution.

```powershell
cmake --preset x64-windows-internal -A x64 -T v141 `
  -DINSTALL_TO_MONOLITH=ON `
  -DCMAKE_INSTALL_PREFIX="<vendor-folder>"
```

- `-A x64` is required, otherwise you get a 32-bit solution
- `-T` must match `VCPKG_PLATFORM_TOOLSET` in your preset's triplet
- `-G`, `-A` and `-T` apply only to a **new** build folder — delete it to change them
- Open the generated solution, not the repo folder. Opening the folder makes Visual Studio
  reconfigure the same build directory and drop these settings

> **Configure fails on a missing `/scripts/toolchains/windows.cmake`?**
> Set `PATH_TO_VCPKG_ROOT` in your environment to `<repo>/vendor/github.com/microsoft/vcpkg`.

### Options

All are `OFF` by default, so a plain build has no renderer backend. Pass them as `-D` when
generating; each one pulls extra vcpkg packages on that configure.

| Option | Effect |
| --- | --- |
| `BUILD_DX11` | DirectX 11 targets |
| `BUILD_DX12` | DirectX 12 targets |
| `BUILD_METAL` | Metal targets |
| `BUILD_SHADER_COMPILER` | Build the shader compiler |
| `WITH_GRANNY` | Granny `.gr2` support |

## 🤝 Contributing
Contribution follows the standard GIT PR model.

By submitting a pull request or otherwise contributing to this project, you agree to license your contribution under the [MIT License](LICENSE.md) License, and you confirm that you have the right to do so.

## 📄 License and Legal Notices

© 2026 CCP Games

This software is provided by CCP Games. See [NOTICE](NOTICE.md) for included 3rd party code.

Trademark Notice: CCP Games is a trademark of CCP ehf.

This project is licensed under the [MIT License](LICENSE.md). Nothing in the [MIT License](LICENSE.md) grants any rights to CCP Games' trademarks or game content.
