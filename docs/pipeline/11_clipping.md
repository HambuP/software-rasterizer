# Near-Plane Clipping

<p class="subtitle">What to do with a triangle that is partly behind the camera.</p>

---

## The problem

On May 20, a few days after "finishing" the project, I went back to it and realized something was missing. Frustum culling only threw away triangles that were <span class="accent-red">completely</span> outside the view. But a triangle can be half in front of the camera and half behind it — walk into a wall, or stand inside a room with big floor triangles, and it happens all the time.

Those triangles can't be culled, because part of them is visible. And they can't be drawn as they are either. A vertex behind the camera has <span class="accent-red">w ≤ 0</span> in clip space. Dividing by a negative w flips its x and y to the other side of the screen, and w = 0 is a division by zero. The triangle gets stretched across the screen, inverted, or both.

Without clipping, all the work that went into the near plane of the projection matrix was almost for nothing.

## Z frustum culling first

The first step was the easy one: adding Z to frustum culling. It's the same test as X and Y — if all three vertices are past the near plane (z < −w) or past the far plane (z > w), the triangle is discarded. I had tried this before and failed, but only because my conventions (z+ or z−, left or right handed) weren't defined. Once they were, it was a copy-paste of the X/Y checks.

## Clipping in clip space

Clipping happens in <span class="accent-gold">clip space</span>, after multiplying by the MVP and **before** the perspective divide. There are two reasons:

- The near plane is just <span class="accent-sage">z = −w</span> there, so "is this vertex in front of the near plane?" is a single comparison: `z >= -w`.
- Clip space is still a linear space. Nothing has been divided by w yet, so moving along an edge in a straight line is just a lerp between its two vertices. After the divide, that's no longer true.

First, count how many vertices are behind the near plane:

```cpp
int puntos_afuera = 0;
if (v1.z < -v1.w) puntos_afuera++;
if (v2.z < -v2.w) puntos_afuera++;
if (v3.z < -v3.w) puntos_afuera++;
```

## The three cases

- **0 outside:** the normal path. Divide by w, go to the screen, rasterize.
- **1 outside:** cutting one corner off a triangle leaves a <span class="accent-gold">quadrilateral</span>. It gets split into **2 triangles**.
- **2 outside:** cutting two corners off leaves a <span class="accent-gold">smaller triangle</span> — **1 triangle**.

(3 outside never gets here — Z culling already threw it away.)

## Finding the new vertices

Each edge that crosses the near plane needs a new vertex exactly on the plane. Take `d = z + w` for each vertex: it's positive in front of the near plane, negative behind it, and exactly 0 on it. Since clip space is linear, `d` changes linearly along the edge, so the crossing point is where it hits 0:

\[ t = \frac{d_{in}}{d_{in} - d_{out}} \]

The new vertex is a lerp between the inside vertex and the outside one:

```cpp
float t = (in.z + in.w) / ((in.z + in.w) - (out.z + out.w));

Vec4 new_clip = in * (1 - t) + out * t;   // position in clip space
Vec2 new_uv   = uv_in * (1 - t) + uv_out * t;
Vec4 new_nor  = nor_in * (1 - t) + nor_out * t;
Vec4 new_pos  = pos_in * (1 - t) + pos_out * t;  // world position, for lighting
```

The same `t` works for <span class="accent-sage">every attribute</span>: UVs, normals and the world position used for Phong. The new vertices then go through the normal path — perspective divide, viewport transform, rasterization — like any other vertex.

## The most stupid bug of the whole project

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">BUG</span>
    <span class="bug-title">Clipping destroys everything — Vec4 × scalar ignored w</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The three cases were worked out, the logic looked right, and still every clipped triangle shrank and left artifacts everywhere.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>My <code>Vec4 * float</code> operator scaled x, y and z, but not w. So in <code>in*(1-t) + out*t</code>, the new w came out as roughly <code>w_in + w_out</code>. A w that's too big makes x/w and y/w too small after the divide, which pulls every new vertex toward the center of the screen. That's the shrinking.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Scale w too. One hour of debugging, one character of code. When the logic is right and nothing works, look at the operators and primitives.</span>
    </div>
  </div>
</div>

## Clipping the light's camera too

A few days later, while building the final room scene, a piece of wall started showing up inside the shadows, even though the wall was behind the light. The cause was the same problem from a different point of view: the shadow pass is a full rasterization from the light, and the walls are a few huge triangles, so one of their vertices was crossing <span class="accent-red">the light's near plane</span>.

The fix was the same clipping code, now in the shadow pass. The lesson: <span class="accent-gold">the light is just another camera</span>. Anything that can go wrong for the camera can go wrong for it.

## Making it fast

Right after clipping worked, the scene was running at **15 FPS** on my old laptop. I tried a profiler for the first time, but it didn't help much, so I looked at the structure of the code first. Three changes:

1. **1/w once per vertex, not per pixel.** The inverse depth of a vertex doesn't change across the triangle, so it's computed once before the pixel loop. **+2 FPS.**
2. **Persistent lists.** The triangle list and the thread list were being created every frame. Now they're members of the renderer and just get `clear()`ed. **+2 FPS.**
3. **Only build clipping data when it's needed.** I was creating four `std::vector`s for *every* triangle, even though they're only used when a vertex crosses the near plane. Moving them inside that condition and turning them into `std::array`s (no heap allocation): **+20 FPS.**

From 15 to ~39 FPS — almost 3× with three small changes, no profiler needed. The lesson: look at the structure of the pipeline first. Most of the speed was in work that didn't need to happen at all.

## What's not perfect

Clipped triangles currently skip backface culling — the check is commented out for the two clipping cases. It doesn't affect the image (the z-buffer still resolves it), but it means some hidden triangles near the camera get rasterized for nothing.

<div class="page-nav">
  <a href="../10_shadows/" class="page-nav-btn prev">← Shadow Mapping</a>
  <a href="../../extras/bugs/" class="page-nav-btn next">Bugs — Hall of Fame →</a>
</div>
