# Slang Differential Rendering: Texture Optimization and 2D Gaussian Splatting

A high-performance implementation of automatic differentiation for inverse rendering tasks using the Slang shading language, with applications in texture optimization and 2D Gaussian Splatting (2DGS).

## Overview

This project bridges the gap between differential programming and real-time graphics by implementing automatic differentiation (AD) in Slang - a shading language designed for high-performance, cross-platform rendering. By extending the capabilities of the NDiffr library into Slang, we enable gradient-based optimization directly within shader code, making inverse rendering more accessible and significantly faster.

Key features:
- **High-performance differential rendering**: 116× faster than NVIDIA Falcor for texturing and 5× faster than PyTorch Slang implementations for 2DGS
- **Automatic differentiation in shader code**: Compute gradients of rendering operations with respect to scene parameters
- **Real-time inverse rendering**: Optimize scene properties through gradient descent within the rendering pipeline
- **Unified framework**: Support for both texture optimization and 2D Gaussian Splatting in one codebase

## Applications

### Differential Texturing

Our implementation provides a differentiable texturing system that supports mipmapping and gradient-based optimization. The framework:

- Enables texture parameter optimization through gradient descent
- Implements differentiable mipmapping for better detail management
- Uses an accumulated buffer for efficient gradient propagation
- Allows real-time feedback during the optimization process

The texture optimization example (`examples/sphere-texture-diff`) demonstrates learning a texture from a reference texture through inverse rendering, achieving significantly faster convergence than previous methods.

### 2D Gaussian Splatting (2DGS)

The 2DGS implementation (`examples/2dgs`) represents scenes with 2D Gaussian primitives (oriented elliptical disks) rather than 3D Gaussians. This approach:

- Provides more accurate geometry representation during rendering
- Achieves superior performance compared to 3D Gaussian Splatting
- Enables high-quality, differentiable surface reconstruction
- Facilitates real-time rendering and optimization

Our implementation uses advanced optimization techniques including tile-based processing, efficient memory management, and parallel workgroup coordination to achieve significant performance improvements.

## Installation and Compilation

For detailed instructions on how to install and compile the project, please refer to the document located at `docs/building.md`. This document provides comprehensive steps to ensure a successful setup of the development environment and compilation of the project.

## Usage

### Running Examples

To use the project's functionalities, you can execute the provided examples:

1. **Differential Texturing** (`examples/sphere-texture-diff`): Demonstrates texture optimization through inverse rendering
2. **2D Gaussian Splatting** (`examples/2dgs`): Showcases high-performance 2DGS implementation

To run an example, navigate to the corresponding example folder and execute the release executable from there (found in `bin/platform/release/`). For instance:

```bash
# For differential texturing example
cd examples/sphere-texture-diff
../../bin/windows-x64/release/sphere-texture-diff

# For 2D Gaussian Splatting example
cd examples/2dgs
../../bin/windows-x64/release/2dgs
```

This approach ensures that all necessary resources and context are correctly loaded.

### Differential Texturing Output

When running the differential texturing example, you'll see three components in the output:

- **Left Quad (Learnt Texture)**: The dynamically optimized texture resulting from the inverse rendering process
- **Top Right Quad (Reference Texture)**: The target texture used as the reference
- **Bottom Right Quad (Loss Texture)**: A visualization of the per-pixel difference between the learnt and reference textures

Each frame applies a random model-view-projection matrix to test the robustness of the learnt texture.

![Differential Texture Learning Output](examples/sphere-texture-diff/sphere-diff.png)

*Fig 1: Output of the differential texturing example showing learnt texture (left), reference texture (top right), and loss visualization (bottom right).*

### 2D Gaussian Splatting Output

The 2DGS example demonstrates:

- Real-time rendering of a scene represented by 2D Gaussian primitives
- Efficient optimization of Gaussian parameters through differential rendering
- Accurate geometry reconstruction and high-quality visual results

![2D Gaussian Splatting Results](Captura%20de%20pantalla%202025-05-15%20a%20las%2013.40.54.png)

*Fig 2: Output of the 2D Gaussian Splatting example showing the optimized scene with accurate geometry representation.*

## Technical Details

### Automatic Differentiation in Slang

Our implementation leverages Slang's programming model to implement automatic differentiation directly within shader code. This enables:

- Computing derivatives of complex rendering operations
- Propagating gradients through the entire rendering pipeline
- Optimizing scene parameters through gradient-based methods

### Performance Optimizations

Key optimizations that contribute to the exceptional performance include:

- Efficient gradient calculation and propagation
- GPU-accelerated parallel processing
- Memory-optimized data structures
- Tile-based processing for improved cache coherence

## Future Work

Future development will focus on:

- Extending the framework to support additional rendering techniques
- Improving the scalability for larger scenes
- Integrating with existing rendering engines and frameworks
- Supporting more complex material models and lighting effects

## Citation

If you use this code in your research, please cite:

```
@software{slang_differential,
  author = {Your Name},
  title = {Slang Differential for Inverse Rendering},
  year = {2025},
  url = {https://github.com/yourusername/slang-differential}
}
```

## License

[Your License Information]
