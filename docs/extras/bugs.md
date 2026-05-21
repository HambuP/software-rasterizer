# Bugs — Hall of Fame

<p class="subtitle">Every bug that cost more than 30 minutes, taught something, or was just absurd enough to remember.</p>

---

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">01</span>
    <span class="bug-title">30 seconds to draw a single triangle</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The window loaded but took literally 30 seconds to draw one triangle, nearly crashing every frame.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>Two problems. First, <code>SDL_UpdateTexture</code> was being called inside the per-pixel loop — uploading the full texture to the GPU on every single pixel write. Second, the framebuffer was being passed by value, so the entire array was copied every frame.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Move <code>SDL_UpdateTexture</code> outside the pixel loop. Add a single <code>&</code> to pass the framebuffer by reference. One character fix for the second one.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">02</span>
    <span class="bug-title">Z-buffer working but completely wrong — barycentric assigned backwards</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Two intersecting triangles with depth — the z-buffer was active but behaved unpredictably, ignoring actual depth values.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>When interpolating, the edge function for edge AB was being multiplied by the depth of A — but the edge function for AB represents the area of the sub-triangle opposite to C. It should be multiplied by C's depth.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Each edge function value maps to the vertex it's <em>opposite</em> to, not the one it's named after.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">03</span>
    <span class="bug-title">Moving the cube in Z sends it flying in the rotation direction</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Translating the cube along Z would send it in a completely wrong direction depending on its current rotation.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The MVP matrix was assembled in the wrong order. The projection matrix was being applied first instead of last.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Reverse the multiplication order: Projection × View × Model.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">04</span>
    <span class="bug-title">Only one face of the cube renders — a missing zero</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>After fixing the MVP, only a single face of the cube appeared. The pipeline seemed correct.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The rotation X matrix in <code>math.hpp</code> had only 15 elements — a single missing <code>0</code>.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Add the zero. 30 minutes hunting for a typo.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">05</span>
    <span class="bug-title">Back faces draw on top of front faces — z-buffer inverted</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Rotating the cube revealed that back faces were always drawn on top of front faces.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The z-buffer was reading <code>-w</code> instead of <code>+w</code>, inverting all depth comparisons.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Negate the sign: store <code>+w</code> so closer objects have smaller depth values.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">06</span>
    <span class="bug-title">Abstract art — renderer not cleared between frames</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Rotating and moving the cube produced a layered painting — every previous frame stayed on screen.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The framebuffer and z-buffer were cleared each frame, but <code>SDL_RenderClear</code> was never called. SDL kept the previous frame underneath.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Add <code>SDL_RenderClear(renderer)</code> at the start of every frame.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">07</span>
    <span class="bug-title">Black window — working directory, missing push_backs, wrong filenames</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>After implementing the OBJ parser, nothing appeared on screen.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>Several problems stacked: wrong working directory, the MTL path being reset on every line read, filenames using <code>\\</code> instead of <code>/</code> and <code>.jpg</code> instead of <code>.bmp</code>, and faces never being appended to the mesh list.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Print statements everywhere to locate each error one by one.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">08</span>
    <span class="bug-title">OBJ indices start at 1, C++ vectors start at 0</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The mesh loaded but the geometry was completely destroyed — vertices in the wrong positions, faces twisted.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>OBJ files use 1-based indices. Accessing <code>vertices[index]</code> without subtracting 1 reads the wrong vertex every time.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span><code>vertices[index - 1]</code>.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">09</span>
    <span class="bug-title">Back faces on top again — the w sign flip that worked, then didn't</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Same symptom as bug #05 — back faces drawing on top — but this time with a real OBJ file.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The <code>-w</code> fix from bug #05 had only worked for the manually-constructed cube. Real OBJ files have different conventions, so negating w broke it again.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Revert from <code>-1</code> to <code>+1</code>. The previous fix was masking the real problem.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">10</span>
    <span class="bug-title">UV texture distortion — linear interpolation in screen space</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The cube faces showed correct textures, but they warped and stretched as the cube rotated.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>UV coordinates were being interpolated linearly in screen space, which ignores depth. A vertex at depth 10 and one at depth 1 are treated the same.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Perspective-correct interpolation: interpolate <code>UV/w</code> and <code>1/w</code> separately, then divide. Since <code>1/w</code> is linear in screen space, this corrects for depth.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">11</span>
    <span class="bug-title">Crash when loading a detailed model — texture copied every frame</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Loading a low-poly Jeep instead of the Minecraft cube caused an immediate crash — stack overflow or memory access violation.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The entire texture — over a million pixels — was being copied into the rasterizer every frame. The tiny Minecraft texture hid this; the Jeep's larger texture exposed it immediately.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Pass the material as a <code>const*</code> pointer instead of by value. 8 bytes instead of megabytes per frame.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">12</span>
    <span class="bug-title">Multiple objects — one always renders on top regardless of depth</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>With two objects in the scene, changing the render order always made one appear in front of the other, ignoring actual depth.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>Each object had its own z-buffer. Each one only compared depth against itself, not against the other objects in the scene.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>A single global z-buffer shared across all objects and passed as a parameter to the render function.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">13</span>
    <span class="bug-title">Phong shading frozen — normals not rotating with the object</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The lighting looked correct at first, but as the object rotated, the shadows stayed fixed — as if painted on once and never updated.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The original OBJ normals and vertex positions were being used for lighting, not the transformed ones. The rotation wasn't being applied to the normals.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Apply the rotation matrix to normals and the full model matrix to vertex positions before passing them to the rasterizer.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">14</span>
    <span class="bug-title">Phong shading looks flat — barycentric order wrong again</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The object rotated correctly and lighting responded, but even on detailed models the triangles were clearly visible — no smooth gradient.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>Normals and 3D positions were being interpolated, but assigned to the wrong barycentric coordinates — same mistake as the first color interpolation bug.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Match the barycentric order to the one used for UV interpolation. 3 characters changed.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">15</span>
    <span class="bug-title">Drunk camera — lookAt not inverted</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>Moving the camera made everything spin erratically. Looking left felt like going right. Nothing was consistent.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The lookAt matrix was the camera-to-world transform, not the world-to-camera (view) transform. It was moving the camera into the world instead of moving the world into camera space. Also mixing OpenGL and DirectX conventions for the forward vector.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Use the inverse (transposed rotation part) and fix the sign conventions consistently.</span>
    </div>
  </div>
</div>

<div class="bug-card">
  <div class="bug-header">
    <span class="bug-tag">16</span>
    <span class="bug-title">Spotlight not visible — gimbal lock in lookAt</span>
  </div>
  <div class="bug-body">
    <div class="bug-row">
      <span class="bug-label">What happened</span>
      <span>The directional light worked. The spotlight produced no light and no shadow.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Cause</span>
      <span>The spotlight was pointing straight down — perfectly parallel to the world up vector used to compute the right vector in lookAt. A cross product of two parallel vectors is zero, breaking the entire matrix.</span>
    </div>
    <div class="bug-row">
      <span class="bug-label">Fix</span>
      <span>Short-term: tilt the light slightly off vertical. Long-term: handle the parallel case in lookAt by switching to a different up reference vector when forward ≈ up.</span>
    </div>
  </div>
</div>

<div class="page-nav">
  <a href="../../pipeline/10_shadows/" class="page-nav-btn prev">← Shadow Mapping</a>
  <a href="../process/" class="page-nav-btn next">Process & Research →</a>
</div>