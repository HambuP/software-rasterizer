# Process & Research

<p class="subtitle">How the project actually went — day by day, in the words written right after each session.</p>

---

This page is the behind-the-scenes of the pipeline pages. Rather than summarizing or rewriting, what follows is pulled directly from the dev log kept in Notion — written in ~5 minutes at the end of each coding session, while it was still fresh.

---

## April 18 — Day 1

Set up the full environment: repository, CMake, SDL3, CLion. The project started.

---

## April 19 — One very long day

> *"Went from installing SDL3 to having all the MVP matrices ready. Made the z-buffer, interpolated with colors, added all the transformation operations for the pipeline. Today was the day I really learned the full rasterizer pipeline. I always applied dot product and cross product blindly — now I see their exact function. I derived each MVP matrix from scratch for the first time: rotation from the unit circle, the camera matrix from the three axes, the projection matrix by solving a system of equations with near and far planes. Those had always been copy-paste for me."*

**What was left:** perspective divide, viewport transform, first cube.

---

## April 20 — The first cube

Finished the MVP pipeline. The first cube appeared — epileptic colors, back faces visible, but there.

> *"Learned that MVP multiplication order matters enormously. Also that a single missing zero in a rotation matrix breaks the entire cube. And that you need to clear the renderer every frame — otherwise you get abstract art."*

---

## April 22 — First clean cube

A short session. Fixed the z-buffer sign inversion and the renderer clearing issue from the day before. The cube rotated correctly.

> *"Today the work was simply finally rasterizing the first colored, rotating cube in motion. And it worked."*

---

## April 25 — OBJ & BMP parsers

> *"Learned a lot about the BMP format: header has metadata at specific byte positions, colors are BGR not RGB, rows go bottom-to-top, and each row is padded to a multiple of 4 bytes. For OBJ: reading line by line, splitting by spaces and slashes, assigning each token to its list."*

---

## April 26 — Textures, culling, crash

> *"Implemented texture interpolation with perspective correction. Added frustum and backface culling. Then tried loading a random model and it crashed immediately — copying the entire material every frame. Fixed by passing as const pointer: 8 bytes instead of megabytes per frame."*

---

## April 28–29 — Multithreading + Phong research

> *"Switching from Debug to Release mode multiplied FPS by 5. Finished multithreading — not as hard as it sounds. Started Phong research: even for multiple lights, it's a simple sum of values."*

---

## May 5–7 — Phong shading

> *"Multiple objects were ignoring each other's depth — two separate z-buffers. Fixed by passing a shared global z-buffer. Then: Phong implemented but lighting frozen when rotating — normals not being transformed with the model. Fixed, and now the skull looks genuinely ominous."*

---

## May 13 — WASD, toon shading, the drunk camera

> *"Realized Phong was basically flat shading — interpolating normals wrong. Fixed. Toon shading done quickly. Then the camera: the lookAt matrix wasn't transposed, and I was mixing OpenGL and DirectX conventions. Fixed both and made everything consistent."*

---

## May 14 — Shadow mapping and the final scene

> *"Probably the longest day of the project. Shadow mapping implemented — structurally simpler than expected, basically another z-buffer. The hard part was the spotlight: perspective projection, radial lerp attenuation, distance attenuation. Used std::variant for multiple light types. Set up the scene, recorded the final videos. Project done."*

**Total: ~26 days of active development.** April 18 → May 14.

---

## The handwritten notes

Every concept went through paper before code. Derivations of the projection matrix, the lookAt decomposition, the yaw/pitch formulas, the perspective-correct interpolation proof — the reMarkable tablet was the first step for all of it. The notes are messy and jump between topics. That's what working through a problem actually looks like.

<p class="img-caption" style="text-align:center; margin-top:1rem;">
  📄 <a href="../../assets/img/brainstorm_remarkable.pdf" target="_blank">View handwritten notes (PDF)</a>
</p>

<div class="page-nav">
  <a href="../bugs/" class="page-nav-btn prev">← Bugs — Hall of Fame</a>
  <a href="../references/" class="page-nav-btn next">References →</a>
</div>