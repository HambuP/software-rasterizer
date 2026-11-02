<div align="center">

<img src="docs/assets/img/Final%20HERO%20Image.png" alt="Two skulls in a library: Phong shading on the left, toon shading on the right, lit by two lights with shadow mapping" width="100%"/>

# Software Rasterizer

**A 3D renderer that runs entirely on the CPU, written from scratch in C++ to understand how 3D graphics actually work.**

[Documentation](https://hambup.me) · [Bugs Hall of Fame](https://hambup.me/extras/bugs/) · [Handwritten notes](https://hambup.me/extras/process/)

</div>

---

## Why I built this

This project started in Lua, with one goal: understanding how 3D graphics work. I moved to C++ because Lua was getting complicated, and I wanted to start over in a proper environment with the right tools. Honestly, it was pure curiosity.

That curiosity turned into the thing I want to focus on: graphics. Building the base of 3D rendering didn't just teach me how the pipeline works. It taught me how linear algebra is actually used, how to structure a project, how to fix bugs, how to optimize, and how to take an idea and turn it into something huge. I built every part of it from scratch, so I can explain any part of it to anyone, and that's what I love most: teaching.

## In motion

<table>
  <tr>
    <td width="50%"><img src="docs/assets/img/Escena%20FINAL%20CROPPED.gif" alt="Skull scene rendering in real time" width="100%"/></td>
    <td width="50%"><img src="docs/assets/img/Backrooms%20FINAL%20CROPPED.gif" alt="Walking through the Backrooms scene" width="100%"/></td>
  </tr>
  <tr>
    <td align="center"><sub>Phong vs toon shading, two lights, real-time shadow mapping</sub></td>
    <td align="center"><sub>The Backrooms: a full downloaded scene, ~70K triangles</sub></td>
  </tr>
</table>

**The Backrooms test.** To prove the rasterizer isn't tied to the scenes I built for it, I found a full Backrooms model online, imported it, and ran it here. Getting the scale and the look right took some work, but it shows what the renderer can handle. The lighting in this scene is baked into its textures, so this one shows off geometry, texturing and clipping. The skull scene is where my own lighting and shadows are.

## Features

Everything below is my own code. SDL3 only opens the window and shows the finished framebuffer.

**Pipeline**
- Math library from scratch: `Vec2`, `Vec3`, `Vec4`, `Mat4`
- Full Model · View · Projection transform, with every matrix derived on paper first
- Near-plane triangle clipping, frustum rejection and back-face culling
- Barycentric rasterization with perspective-correct interpolation
- Per-pixel z-buffer

**Lighting and shading**
- Per-pixel Phong shading (ambient, diffuse, specular)
- Toon shading with banded light and outlines, switchable per object
- Directional light and spotlight (cone and attenuation)
- Shadow mapping for both: orthographic for the directional light, perspective for the spotlight, with bias against shadow acne and shadow maps that only re-render when a light changes

**Assets**
- OBJ parser with MTL materials (quads and n-gons are split into triangles)
- BMP decoder written from scratch, 24-bit and 32-bit

**Performance and interaction**
- Multithreaded: the screen and the shadow maps are split into bands of rows, one per CPU core
- First-person camera: WASD and mouse look

## Performance

Release build at 800×600 on a TODO-CPU.

| Scene | Triangles | FPS |
|---|---|---|
| Jeep model | ~5K | ~400 |
| Default scene (in the repo), 2 lights with shadows | 22 | ~150 |
| Two skulls + room, 2 lights with shadows | ~160K | ~150 |
| Backrooms | ~70K | ~100 |

The default scene has 22 triangles and the skull scene has 160K, yet both run at about the same speed. Most of the frame goes to shading every pixel on screen, not to the number of triangles.

## Run it

### Option 1: download (Windows)

Grab the latest zip from [Releases](https://github.com/HambuP/software-rasterizer/releases), unzip it, and run `Rasterizer.exe`. Nothing else to install.

### Option 2: build from source (Windows, Linux, macOS)

You need [CMake](https://cmake.org/download/) 3.24+ and a C++17 compiler. You don't need to install SDL3; CMake downloads and builds it automatically the first time (this takes a minute or two).

```bash
git clone https://github.com/HambuP/software-rasterizer.git
cd software-rasterizer
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run it **from the repo root**, so it can find the `obj/` folder:

```bash
./build/Rasterizer              # Linux / macOS
build\Rasterizer.exe            # Windows (MinGW)
build\Release\Rasterizer.exe    # Windows (Visual Studio)
```

Build in Release. A Debug build runs several times slower.

On Linux, SDL3 needs your system's X11/Wayland development packages. See SDL's [Linux notes](https://github.com/libsdl-org/SDL/blob/main/docs/README-linux.md).

Tested with MinGW GCC 15.2 on Windows and GCC 13 on Linux. macOS and MSVC should work but haven't been tested yet.

### Controls

| Key | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| Mouse | Look around |
| `Alt` + `F4` | Quit |

## Limitations

- **Textures must be BMP** (24- or 32-bit). If a model ships with PNG or JPG textures, convert them first.
- **OBJ faces need positions, UVs and normals** (`f v/vt/vn`). Models without UVs or normals won't load.
- **The scene lives in `main.cpp`.** There's no scene file or menu, so loading another model means editing the code and recompiling.
- **Fixed 800×600 window.**
- **The models in the screenshots aren't in the repo.** Running it opens the default scene: a grass block on a wooden floor in front of a library wall, lit by the same two lights with shadows.

## Documentation

Every stage of the pipeline is explained at **[hambup.me](https://hambup.me)**, from the first pixel to shadow mapping, with interactive visualizations, my handwritten notes, and the [Bugs Hall of Fame](https://hambup.me/extras/bugs/): everything that broke along the way and why.

**A note on the code:** the comments are in Spanish, my first language. It's how I think through problems, so that's how I wrote them down. The documentation is all in English.

## What's next

A ray tracer: same goal, the other way of making an image.

## Credits

Models shown in the screenshots (not included in the repo):

- Skull: [Skull v3](https://free3d.com/3d-model/skull-v3--785914.html) by printable_models on Free3D
- Backrooms: [Backrooms VR](https://sketchfab.com/3d-models/backrooms-vr-1a5c397f0a43408fa38b09ea5c041149) by VTX on Sketchfab. I converted its textures to BMP and edited its materials to run here.

Window and input by [SDL3](https://github.com/libsdl-org/SDL).

## License

[MIT](LICENSE)
