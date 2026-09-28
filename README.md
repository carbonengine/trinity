# Carbon trinity
Rendering engine for the Carbon Game Engine, the technology behind EVE Online and EVE Frontier.

Trinity is a C++ renderer with DirectX 11, DirectX 12 and Metal backends, a shader compiler, and a Python
exposure layer through Blue. It is one of the [Carbon Engine components](https://carbonengine.github.io/documentation/components.html).

## 🛠️ Building

### Prerequisites

- Git, with SSH access to GitHub. The two submodules clone over HTTPS, but the `carbon-*` packages that vcpkg
  builds from our registry are fetched over SSH (`git@github.com:carbonengine/...`).
- CMake 3.31 or newer (required by `CMakePresets.json`).
- Windows: Visual Studio with the **v141 (VS 2017) C++ build tools** component. The toolset is pinned by the
  registry triplets, so it must match the `-T` argument below.
- macOS: Xcode command line tools.

```powershell
git clone --recurse-submodules https://github.com/carbonengine/trinity.git
```

Or if already cloned then `git submodule update --init --recursive`

Dependencies come through vcpkg: `vendor/github.com/microsoft/vcpkg` plus the
[Carbon vcpkg registry](https://github.com/carbonengine/vcpkg-registry) for the `carbon-*` components and a few SDKs.
The first configure builds them, which takes a while.

### Generate a solution

Generate a solution, then you can handle the rest from your IDE for local dev workflows:

```powershell
cmake --preset x64-windows-internal -A x64 -T v141
```

This will generate an .slnx at `.cmake-build-<preset-name>/`. Run `cmake --list-presets` to see the presets
(`x64-windows-*`, `arm64-osx-*`, `x64-osx-*`, each in `internal`, `release`, `debug` and `trinitydev` flavours).

Or build from the command line:

```powershell
cmake --build .cmake-build-x64-windows-internal --config Release
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

### Working with Monolith

To install trinity next to the rest of the engine components, add the destination when generating the solution:

```powershell
cmake --preset x64-windows-internal -A x64 -T v141 `
  -DINSTALL_TO_MONOLITH=ON `
  -DCMAKE_INSTALL_PREFIX="<vendor-folder>"
```

## 🤝 Contributing

Contributions are welcome. Please read the Carbon Engine [contributing guide](https://github.com/carbonengine/.github/blob/main/CONTRIBUTING.md) before opening an issue or pull request. It covers the workflow, the CLA and the pull request template, and applies to every `carbonengine` repository. Please also follow the [Code of Conduct](https://github.com/carbonengine/.github/blob/main/CODE_OF_CONDUCT.md), and report security issues privately as described in the [Security Policy](https://github.com/carbonengine/.github/blob/main/SECURITY.md) rather than in a public issue.

By submitting a pull request or otherwise contributing to this project, you agree to license your contribution under the [MIT License](LICENSE.md), and you confirm that you have the right to do so.

## 📄 License and Legal Notices

© 2026 Fenris Creations

This software is provided by Fenris Creations. See [NOTICE](NOTICE.md) for included 3rd party code.

Trademark Notice: Fenris Creations is a trademark of CCP ehf.

This project is licensed under the [MIT License](LICENSE.md). Nothing in the [MIT License](LICENSE.md) grants any rights to Fenris Creations' trademarks or game content.
