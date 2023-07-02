# LumiEngine [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
A simple Vulkan renderer.

## Features

- [ ] Shadow map
- [ ] PBR
- [ ] SSAO
- [ ] Defered shading
- [ ] MSAA
- [ ] vsync

## Prerequisites
- Visual Studio >= 2022
- CMake >= 3.20
- Vulkan SDK >= 1.3.250.0

## Build

```shell
git clone https://github.com/LumiOwO/LumiEngine.git --recursive
cd LumiEngine
cmake -S . -B build
```
Then open the Visual Studio solution in the build directory and build it manually.


## License
[MIT License](LICENSE)

## References
- [Vulkan Guide](https://vkguide.dev/)

## Todos

- [x] Index buffer
- [ ] Instanced rendering
- [ ] 整理坐标系
    - glm的z轴向外为正
    - vulkan ndc的y轴向下为正
    - depth vulkan [0,1], opengl [-1,1]

- materials
    - buffer在material里构建
    - descripor也在material里
    - material->bind_pipeline
    - material->bind_descriptor
    - material->update_buffer(update_descriptor)
        - per frame
        - per object
    - static Material::Construct<T>() { (assert derived); return new T; }
    - 用 unordered map 来反射