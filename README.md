# Slang Differential for Inverse Rendering
## Overview

This project aims to bridge the gap between high-level differential programming and real-time graphics rendering, focusing on inverse rendering applications. By translating and extending the capabilities of the NDiffr library into Slang—a shading language designed for real-time graphics—we leverage automatic differentiation (AD) to facilitate the development of differentiable rendering techniques. This approach not only democratizes access to advanced rendering algorithms but also streamlines the process of integrating AD into existing graphics pipelines.

## Installation and Compilation

For detailed instructions on how to install and compile the project, please refer to the document located at `docs/building.md`. This document provides comprehensive steps to ensure a successful setup of the development environment and compilation of the project.

## Usage

To utilize the project's functionalities, you can directly execute the tests and examples provided. Currently, the primary example to demonstrate the capabilities of this project is `sphere-texture-diff`, which can be found under the `examples/sphere-texture-diff` directory.

To execute an example, navigate to the corresponding example folder and run the release executable from there (found in  `bin/platform/relase/`. Platform refers to system , for example, windowsx86 ). For instance, to run the `sphere-diff-texture` example, you should execute it from within the `examples/sphere-texture-diff` folder.

This approach ensures that all necessary resources and context are correctly loaded, allowing for a smooth and accurate demonstration of the project's differential rendering capabilities.

## Background

### Inverse Rendering

Inverse rendering refers to the process of deducing scene properties (like geometry, lighting, and materials) from images. It is a critical technique in computer vision and graphics, enabling photorealistic rendering, augmented reality, and scene understanding. Traditional methods often involve heuristics or optimization techniques that can be computationally expensive and difficult to generalize.

### Automatic Differentiation

Automatic differentiation is a set of techniques to numerically evaluate the derivative of a function specified by a computer program. AD exploits the fact that every program, no matter how complex, executes a sequence of elementary arithmetic operations and functions. By applying the chain rule repeatedly to these operations, AD tools can compute derivatives of arbitrary order efficiently, allowing for gradient-based optimization of parameters.

### NDiffr Library

The NDiffr library represents a pioneering effort to apply AD within the context of differentiable rendering. It provides a Python-based framework for defining rendering operations and computing gradients with respect to scene parameters. However, its utilization in real-time applications is limited due to the overhead associated with Python and the lack of integration with existing real-time rendering engines.

## Project Approach

### Translation to Slang

Slang, with its robust ecosystem and close integration with real-time rendering APIs, presents an ideal platform for extending NDiffr's capabilities. By translating NDiffr's functionalities into Slang, we enable differentiable rendering directly within shader code, allowing for unprecedented integration with graphics applications and engines.

A key focus of our project is on differentiable texturing—a technique that allows for the optimization of texture parameters through gradient descent. This is achieved by implementing differentiable versions of texture sampling operations in Slang, thus enabling the computation of gradients with respect to texture parameters. Our approach not only supports traditional 2D textures but also extends to more complex shading models and procedural texturing techniques.


## Slang Differential for Inverse Rendering

### Differentiable Texturing

#### Overview

A key focus of our project is on differentiable texturing—a technique that allows for the optimization of texture parameters through gradient descent. This is achieved by implementing differentiable versions of texture sampling operations in Slang, thus enabling the computation of gradients with respect to texture parameters. Our approach not only supports traditional 2D textures but also extends to more complex shading models and procedural texturing techniques.
Differential texturing is a cornerstone in differentiable rendering, enabling the optimization of textures through backpropagation. In our project, we extend this concept to leverage mipmapping, a technique for handling texture resolutions across varying distances, to enhance performance and reduce aliasing. This integration is pivotal for inverse rendering tasks, allowing for a nuanced adjustment of textures that react to environmental conditions and camera perspectives.

#### Technical Deep Dive

##### Texture Resources and Mipmaps
In graphics, textures are not flat images but resources with multiple levels of detail (LODs), known as mipmaps. Each mipmap level halves the resolution of the texture, providing a pyramid of textures from full resolution down to a single pixel. This hierarchy allows GPUs to select the appropriate level of detail based on the distance of a surface from the camera, minimizing computational overhead and improving rendering quality.

##### Differentiable Mipmapping
Our implementation introduces the concept of differentiable mipmapping. We modify the traditional mipmapping process to be aware of gradients, thus enabling the adjustment of texture details in a gradient descent optimization loop. This is particularly useful in inverse rendering scenarios, where we aim to reconstruct scene properties from images. By optimizing mip levels alongside texture values, we can ensure that the texture not only appears correct at a fixed distance but adapts optimally across changes in viewpoint and distance.

##### Accumulated Buffer for Gradient Propagation
A key innovation in our approach is the use of an accumulated buffer that stores gradients for each mipmap level during backpropagation. This buffer captures how changes in texture at various levels of detail affect the final rendering outcome, allowing for a nuanced optimization process that considers the full spectrum of texture detail.

##### Gradient Propagation Through Texture Sampling
In the forward pass, texture sampling involves selecting the appropriate mipmap level and blending between levels based on the distance to the camera. During the backward pass, we propagate gradients through these sampling operations. This requires a careful reinterpretation of how sampling decisions influence rendering outcomes, necessitating the derivation of gradients with respect to both texture values and mipmap level selections.

##### Pipelines and Texture Resources
To facilitate this complex process, we leverage Slang's capabilities to define custom pipelines and texture resources that are compatible with differential operations. Our pipelines are designed to carry forward rendering operations while capturing necessary derivatives, feeding into the accumulated buffer. Texture resources, on the other hand, are extended to support gradient accumulation and retrieval, making them first-class citizens in the optimization loop.

#### Conclusion
The integration of differential texturing with mipmapping represents a significant advancement in the field of differentiable rendering. It not only enhances the visual fidelity of inverse rendering results but also introduces a new level of detail management in the optimization of textures for real-time graphics. Our approach opens new avenues for research and application in material design, photorealistic rendering, and beyond.



## Application in Inverse Rendering

By leveraging differentiable rendering techniques, our project facilitates inverse rendering tasks directly within the rendering pipeline. This integration allows for real-time feedback and optimization of scene parameters, opening up new possibilities in visual effects, scene reconstruction, and material design.
Future Work

Looking ahead, we plan to expand the project's capabilities to include differentiable geometry and lighting models. Additionally, we aim to explore the application of our techniques in virtual and augmented reality environments, where the ability to adapt and optimize rendering parameters in real-time can significantly enhance user experience and immersion.





