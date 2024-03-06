Slang Examples
==============

This directory contains small example programs showing how to use the Slang language, compiler, and API.

* The [`hello-world`](hello-world/) example shows a minimal example of using Slang shader code more or less like HLSL.

* The [`sphere-diff-texturing`](sphere-diff-texturing/) example shows how Slang's support automatic differentiation.

Most of the examples presented here use a software layer called `gfx` (exposed via `slang-gfx.h`) to abstract over the differences between various target APIs/platforms (D3D11, D3D12, OpenGL, Vulkan, CUDA, and CPU).
Using `gfx` is not a requirement for using Slang, but it provides a concrete example of how tight integration of Slang's features into a GPU abstraction layer can provide for a clean and usable application programming model.
