# References

<p class="subtitle">Everything that helped — with a line on why.</p>

---

## Video

**[Sebastian Lague — Building a Software Rasterizer](https://www.youtube.com/watch?v=yyJ-hdISgnw)**
The original inspiration for this project. Revisited several times during development to clarify concepts in the pipeline, especially perspective-correct interpolation for texture mapping.

**[Brendan Galea — 3D Projection](https://youtu.be/U0_ONQQ5ZNM)**
Clarified how the projection matrix works: what the aspect ratio and focal length actually do, and how to use the near and far planes to define the depth function. Clear and complete.

**[Mike Shack — SDL3 Setup](https://youtu.be/XHWZyZyj7vA)**
Quick visual walkthrough of the SDL3 window and rendering setup. Useful for getting started fast.

**[Mr Mouse Math — Computer Graphics MVP Model](https://www.youtube.com/watch?v=a_rX4xfYcy4)**
Helped clarify the view matrix — specifically where the three camera axes come from and how rotating a point aligns with the camera's coordinate system.

**[OBJ File Format Explained](https://youtu.be/iClme2zsg3I)**
Short and clear explanation of the OBJ format. Confirmed the structure before implementing the parser.

---

## Articles & Documentation

**[Scratchapixel — Rasterization Algorithm](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/overview-rasterization-algorithm.html)**
The best explanation of barycentric coordinates and the rasterization pipeline found during the project. Clear derivations with good visualizations.

**[BMP File Format Specification](https://en.wikipedia.org/wiki/BMP_file_format)**
The reference used to understand the BMP header: where the pixel data starts, width, height, and the row padding calculation.

**[Phong Reflection Model](https://en.wikipedia.org/wiki/Phong_reflection_model)**
Reference for the Phong shading formula — ambient, diffuse, specular, and the shininess exponent.

**[cppreference — std::vector](https://en.cppreference.com/w/cpp/container/vector)**
Quick reference for C++ vectors used as the framebuffer structure.

**[cppreference — std::istringstream](https://en.cppreference.com/w/cpp/io/basic_istringstream)**
Understanding token-by-token parsing for the OBJ and MTL file readers.

**[cppreference — std::fstream](https://en.cppreference.com/w/cpp/io/basic_fstream)**
Reading binary files (BMP) and text files (OBJ/MTL) in C++.

**[cppreference — std::thread](https://en.cppreference.com/w/cpp/thread/thread)**
Reference for implementing multithreading — thread creation, hardware_concurrency(), and join().

**[cppreference — C++ Classes](https://en.cppreference.com/w/cpp/language/class)**
Quick refresher on C++ class structure, used for Mesh, Render, and Material.

---

## Tools

**[SDL3](https://wiki.libsdl.org/SDL3/FrontPage)** — Window management, input, and the texture/renderer pipeline.

**[CLion](https://www.jetbrains.com/clion/)** — IDE. CMake integration and the run configuration working directory setting saved a lot of debugging time.

**[reMarkable](https://remarkable.com/)** — Tablet used for all handwritten derivations. Every matrix and formula was drawn here before being coded.

**[Claude](https://claude.ai)** — Used throughout to design and build the interactive visualizations embedded in this documentation. Also used as a thinking partner for the MkDocs setup.

<div class="page-nav">
  <a href="../process/" class="page-nav-btn prev">← Process & Research</a>
  <a href="../math_primer/" class="page-nav-btn next">Math Primer →</a>
</div>