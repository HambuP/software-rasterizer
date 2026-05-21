# 3D Software Rasterizer in C++ using SDL3

A fully software-rendered 3D rasterizer built from scratch in C++ — no OpenGL, no Vulkan, no graphics APIs. Every mathematical operation runs on the CPU in plain code.

**[Documentation → hambup.me/rasterizer](https://hambup.me/rasterizer)**

![Final render](docs/assets/img/result_toon_vs_phong.gif)

---

## What it does

- Full MVP pipeline — Model, View, Projection matrices derived from scratch
- OBJ + BMP parser — zero external libraries beyond SDL3
- Perspective-correct UV texturing
- Phong lighting — ambient, diffuse, specular
- Toon / cel shading
- Shadow mapping — directional light (orthographic) + spotlight (perspective)
- Backface culling + frustum culling
- Multithreaded rasterizer (~400 FPS on a Jeep model)
- Free WASD camera with mouse look — yaw, pitch

## Stack

| | |
|---|---|
| Language | C++17 |
| Windowing | SDL3 |
| Build | CMake |
| IDE | CLion |
| Dependencies | SDL3 only |

## Build

```bash
git clone https://github.com/HambuP/3D-Rasterizer-in-Cpp-using-SDL3.git
cd 3D-Rasterizer-in-Cpp-using-SDL3
mkdir build && cd build
cmake ..
cmake --build .
```

Requires SDL3 installed. Set the working directory to the project root in your IDE before running.

## Documentation

The full pipeline is documented step by step at **[hambup.me/rasterizer](https://hambup.me/rasterizer)** — every section explains the math, the code, and what broke along the way.

## License

MIT