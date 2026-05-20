# 3D Software <span class="accent-red">Rasterizer</span>

<p class="subtitle">Built from scratch in C++ using SDL3 — no engines, no graphics APIs, no shortcuts.</p>

<figure class="hero-figure">
  <img src="assets/img/hero.png" alt="Final render" class="hero-img"/>
  <figcaption>Final render — Phong (left) and Toon shading (right), two lights, shadow mapping, ~30 FPS on CPU.</figcaption>
</figure>

---

## What is this

Every pixel, every frame on screen — every triangle, shadow, and reflection — is the result of a complex process that transforms objects with 3D coordinates into colored pixels. Game engines abstract this entire process, and GPUs accelerate it in hardware. This project aims to understand exactly what happens at each step — and the only way to do that is to build it in software: <span class="accent-red">every mathematical operation happening in plain C++ code, with nothing hidden underneath</span>.

A software rasterizer takes a 3D scene, applies a series of mathematical transformations, and writes the final colors directly into a framebuffer — <span class="accent-gold">the list of pixels that ends up painted on screen</span>. All of it without graphics APIs like OpenGL or Vulkan. Just math, memory, and a lot of debugging.

## Why from scratch

The question that started all of this: <span class="accent-gold">how does a flat screen trick our eyes into seeing depth?</span> The only way to truly answer it is to build the thing yourself. Not use it — build it.

Every concept in this project was understood before it was coded. Before writing a single line, I made sure I could explain it, draw it, and derive it on paper. If I couldn't reconstruct the reasoning from scratch, I didn't implement it yet. <span class="accent-red">Nothing was copied without understanding where it came from.</span>

The result is a renderer I can explain from first principles — and this documentation is the attempt to do exactly that: explain each concept as clearly and intuitively as possible.

## What it does

<div class="feature-grid">
  <div class="feature-item">
    <span class="feature-tag">Math</span>
    <span>Custom Vec2 · Vec3 · Vec4 · Mat4 from scratch</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Rasterization</span>
    <span>Barycentric coords · perspective-correct interpolation</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Depth</span>
    <span>Z-buffer with per-pixel depth testing</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Pipeline</span>
    <span>Model · View · Projection — full MVP from scratch</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Parsing</span>
    <span>OBJ + BMP parser — zero external libraries</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Camera</span>
    <span>Free WASD + mouse look · yaw · pitch</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Textures</span>
    <span>UV mapping · perspective-correct sampling</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Culling</span>
    <span>Backface · frustum culling</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Lighting</span>
    <span>Phong · directional + spot lights · toon shading</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Shadows</span>
    <span>Shadow mapping · bias · spotlight perspective</span>
  </div>
  <div class="feature-item">
    <span class="feature-tag">Threads</span>
    <span>Multithreaded rasterizer across CPU cores</span>
  </div>
</div>

## Stack

<div class="stack-grid">
  <div class="stack-item">
    <span class="stack-label">Language</span>
    <span class="stack-val">C++17</span>
  </div>
  <div class="stack-item">
    <span class="stack-label">Windowing</span>
    <span class="stack-val">SDL3</span>
  </div>
  <div class="stack-item">
    <span class="stack-label">Build</span>
    <span class="stack-val">CMake</span>
  </div>
  <div class="stack-item">
    <span class="stack-label">IDE</span>
    <span class="stack-val">CLion</span>
  </div>
  <div class="stack-item">
    <span class="stack-label">Dependencies</span>
    <span class="stack-val">SDL3 only — everything else is written from scratch</span>
  </div>
</div>

## How to read this

Each section covers one stage of the pipeline **in the order it was built**. The goal is educational: every concept is explained from first principles, with interactive visualizations embedded directly in the page to make the math tangible before looking at the code.

Each page also shows **how the process actually went** — including the bugs, the dead ends, and the moments where something only made sense after drawing it on paper. The [Process & Research](extras/process.md) section has the actual handwritten notes from a reMarkable tablet, and [Bugs — Hall of Fame](extras/bugs.md) collects the most interesting failures.

The interactive visualizations were designed by me and built with [Claude](https://claude.ai) — each one targets a concept I found genuinely hard to understand the first time.

<div class="nav-cards">
  <a href="pipeline/01_framebuffer/" class="nav-card">
    <span class="nav-card-label">Pipeline</span>
    <span class="nav-card-title">Start from the beginning →</span>
    <span class="nav-card-sub">From the first pixel on screen to shadow mapping — every stage explained.</span>
  </a>
  <a href="extras/bugs/" class="nav-card">
    <span class="nav-card-label">Extras</span>
    <span class="nav-card-title">Bugs, notes & references →</span>
    <span class="nav-card-sub">The failures, the handwritten math, and everything that helped along the way.</span>
  </a>
</div>

<p class="github-link"><a href="https://github.com/HambuP/3D-Rasterizer-in-Cpp-using-SDL3">View source on GitHub →</a></p>