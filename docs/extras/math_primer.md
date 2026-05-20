# Math Primer — Vectors & Matrices

<p class="subtitle">Everything you need to read the pipeline pages — no prior knowledge required.</p>

---

This page covers the math foundations used throughout the rasterizer: vectors, matrices, and the operations between them. If you've never seen a matrix before, start here. If you already know this, skip straight to the [pipeline](../pipeline/01_framebuffer.md).

## Vectors

A vector is just a list of numbers. In 3D graphics, vectors typically have 2, 3, or 4 components:

\[ \mathbf{v} = \begin{pmatrix} x \\ y \\ z \end{pmatrix} \]

A 3D vector represents a point or direction in space. `x` is left/right, `y` is up/down, `z` is forward/back.

**Adding vectors** — add component by component:

\[ \begin{pmatrix} 1 \\ 2 \\ 3 \end{pmatrix} + \begin{pmatrix} 4 \\ 5 \\ 6 \end{pmatrix} = \begin{pmatrix} 5 \\ 7 \\ 9 \end{pmatrix} \]

**Scaling a vector** — multiply every component by a number:

\[ 2 \cdot \begin{pmatrix} 1 \\ 2 \\ 3 \end{pmatrix} = \begin{pmatrix} 2 \\ 4 \\ 6 \end{pmatrix} \]

**Length (magnitude)** — how long the vector is:

\[ |\mathbf{v}| = \sqrt{x^2 + y^2 + z^2} \]

**Normalizing** — scale a vector so its length becomes exactly 1. Useful whenever you only care about direction:

\[ \hat{\mathbf{v}} = \frac{\mathbf{v}}{|\mathbf{v}|} \]

**Dot product** — multiply each pair of components and sum them up. The result is a single number:

\[ \mathbf{a} \cdot \mathbf{b} = a_x b_x + a_y b_y + a_z b_z \]

The dot product tells you how much two vectors point in the same direction. If both are unit vectors, the result is the cosine of the angle between them — 1 means same direction, 0 means perpendicular, −1 means opposite.

**Cross product** — takes two 3D vectors and produces a third vector that is perpendicular to both:

\[ \mathbf{a} \times \mathbf{b} = \begin{pmatrix} a_y b_z - a_z b_y \\ a_z b_x - a_x b_z \\ a_x b_y - a_y b_x \end{pmatrix} \]

---

## Matrices

A matrix is a rectangular grid of numbers. In 3D graphics we mostly use 4×4 matrices — 4 rows and 4 columns, 16 numbers total:

\[ M = \begin{pmatrix} m_{00} & m_{01} & m_{02} & m_{03} \\ m_{10} & m_{11} & m_{12} & m_{13} \\ m_{20} & m_{21} & m_{22} & m_{23} \\ m_{30} & m_{31} & m_{32} & m_{33} \end{pmatrix} \]

A matrix represents a transformation — something you do to a vector. Scale it, rotate it, translate it.

### Matrix × Vector

This is the most important operation. To multiply a 4×4 matrix by a vector, you take the **dot product of each row with the vector**:

\[ M \mathbf{v} = \begin{pmatrix} \text{row}_0 \cdot \mathbf{v} \\ \text{row}_1 \cdot \mathbf{v} \\ \text{row}_2 \cdot \mathbf{v} \\ \text{row}_3 \cdot \mathbf{v} \end{pmatrix} \]

Each component of the result is one row's dot product with the input. The first row controls what x becomes. The second row controls y. And so on. This is why the rotation matrix page can say "the first row gives the x' formula" — because the first row of the matrix, dotted with [x, y, z, 1], gives exactly the equation for x'.

A concrete example — scaling by 2 in X only:

\[ \begin{pmatrix} 2 & 0 & 0 & 0 \\ 0 & 1 & 0 & 0 \\ 0 & 0 & 1 & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix} \begin{pmatrix} 3 \\ 5 \\ 1 \\ 1 \end{pmatrix} = \begin{pmatrix} 2 \cdot 3 + 0 \cdot 5 + \ldots \\ 0 \cdot 3 + 1 \cdot 5 + \ldots \\ \ldots \\ \ldots \end{pmatrix} = \begin{pmatrix} 6 \\ 5 \\ 1 \\ 1 \end{pmatrix} \]

### Matrix × Matrix

To combine two transformations into one matrix, you multiply the matrices together. The result is a new matrix that applies both transformations at once.

To get the element at row i, column j of the result: take row i from the left matrix and column j from the right matrix, and compute their dot product.

\[ (AB)_{ij} = \sum_k A_{ik} \cdot B_{kj} \]

**Order matters.** <span class="accent-red">A × B ≠ B × A.</span> Rotating then translating gives a completely different result than translating then rotating. Always apply transformations right to left — the rightmost matrix acts first.

### The Identity Matrix

The identity matrix is the "do nothing" matrix — multiplying any vector by it gives the same vector back:

\[ I = \begin{pmatrix} 1 & 0 & 0 & 0 \\ 0 & 1 & 0 & 0 \\ 0 & 0 & 1 & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix} \]

It's the starting point when combining transformations.

### Transpose

The transpose of a matrix flips it across the diagonal — rows become columns:

\[ M^T_{ij} = M_{ji} \]

### Inverse

The inverse of a matrix M, written M⁻¹, is the matrix that "undoes" M. Multiplying them gives the identity:

\[ M \cdot M^{-1} = I \]

If M rotated something clockwise, M⁻¹ rotates it counter-clockwise. If M translated to (5, 0, 0), M⁻¹ translates to (−5, 0, 0).

For **orthogonal matrices** (matrices whose columns are unit vectors that are all perpendicular to each other), the inverse equals the transpose: M⁻¹ = M^T. This is a very useful shortcut — computing the transpose is trivial, computing the full inverse is expensive.

---

## Why 4D vectors for 3D graphics

You might notice that vectors in the pipeline have 4 components (x, y, z, **w**), not 3. That's because translation can't be expressed as a 3×3 matrix multiplication — it's not a linear transformation. Adding the 4th component w = 1 for points and w = 0 for directions lets us encode translation in the 4th column of a 4×4 matrix, and all transformations (including translation) become matrix multiplications.

This coordinate system is called **homogeneous coordinates**, and it's the reason 4×4 matrices are standard in 3D graphics.

---

With these tools, every matrix in the pipeline section should make sense. The model matrix scales, rotates, and translates objects. The view matrix moves the world into camera space. The projection matrix maps the 3D scene onto a 2D screen. All of them are just matrices — grids of numbers that transform vectors.

<div class="page-nav">
  <a href="../extras/bugs.md" class="page-nav-btn prev">← Bugs</a>
  <a href="../extras/references.md" class="page-nav-btn next">References →</a>
</div>