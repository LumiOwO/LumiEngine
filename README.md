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