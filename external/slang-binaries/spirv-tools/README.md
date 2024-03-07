# spirv-tools

These directories contain the following spirv-tools binaries for several OSs

- spirv-val
- spriv-dis

## Building

Follow the directions from the
[spirv-tools](https://github.com/KhronosGroup/SPIRV-Tools) documentation, (it's
a pretty vanilla CMake project with no dependencies).

As a convenience a Nix flake is here with the following targets defined:

- `cross.mingw32`, `cross.mingwW64` for Windows builds
- `static` for Linux static libmusl builds, available on `aarch64-linux`,
  `i686-linux` and `x86_64-linux`
