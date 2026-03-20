# Primera OFX Plugin Architecture

## 1. The DRT Encapsulation Question

No, we can't — not fully. And the reason is architectural, not preferential.

A DCTL in Resolve occupies a fixed position in the signal chain: it's a per-pixel node that sits either before or after Resolve's color management (RCM, ACES, or the DaVinci color pipeline). It cannot *straddle* the display transform. This means:

- **If Primera sits before the DRT:** It operates in scene-referred space (good for photometric operations), but its highlight roll-off gets *double-compressed* by the DRT's own shoulder. Its saturation adjustments may get clipped by the DRT's gamut mapping. The colorist is grading blind to what the rendering will do — they're painting on a canvas that will be resized after they finish.

- **If Primera sits after the DRT:** It operates in display-referred space, which means scene-linear exposure, black point, and color balance operations are no longer physically meaningful. The signal has already been rendered — manipulating it now is like adjusting a print, not a negative.

- **The split workflow** (some operations before, some after the DRT) requires *two* nodes, breaking the "all-in-one" premise of Primera, and the colorist has to mentally model the DRT's behavior as the boundary between them.

Siragusano's argument is exactly this: **the rendering is not a post-process — it's the medium.** Filmlight built this understanding into Baselight's architecture: you grade *through* a rendering, and the rendering adapts to the display target. Separating "grading" from "rendering" is a legacy of pipelines where they were physically separate stages (grade the negative, print to stock, project). In a digital pipeline, they can be — and arguably should be — unified.

So the answer to your question is: **to fully realize what we've outlined in FIRST_PRINCIPLES.md, the tool must own the scene-to-display path.** It needs to:

1. Accept scene-referred linear (or log-encoded) input
2. Apply photometric operations in scene-linear
3. Apply perceptual operations in a well-chosen log/perceptual space
4. Apply its own display rendering (tone mapping, gamut mapping, chromaticity management) with parametric control
5. Output display-referred values for the target display (SDR Rec.709, HDR PQ/HLG at various peak luminances, P3-D65 theatrical, etc.)

This doesn't mean we abandon the DCTL — Primera as it exists is a solid, practical primary tool for colorists who are working within Resolve's existing color management. But the comprehensive tool we're describing is a different thing: it's a self-contained rendering pipeline that *replaces* rather than nests within the host's color management.

## 2. OFX: Yes, and Here's Why It's the Right Move

DCTL served its purpose as a prototyping language — it let us validate the math quickly, iterate on the signal chain, and confirm that the operations feel right. But its limitations are now the constraint:

**What DCTL can't do:**
- Custom UI (limited to sliders, checkboxes, combo boxes — no curve editors, no color wheels, no draggable points, no parameter grouping/tabs)
- Multi-pass rendering (everything is single-pass, per-pixel)
- Texture/LUT loading (no 3D LUT support, no external file I/O)
- State persistence (no frame-to-frame memory, no temporal operations)
- Spatial awareness (no kernel access to neighboring pixels — relevant for grain, sharpening, spatial gamut mapping)
- Custom overlay drawing on the viewer

**What OFX gives us:**
- Full parameter API with pages, groups, custom labels, conditional visibility
- Custom overlay rendering on the viewer (for interactive controls like draggable pivot points, gamut boundary visualizations, Garvin-style diagnostics)
- Metal compute shaders for GPU acceleration (directly relevant to your Apple Silicon target)
- Multi-pass rendering (render to intermediate textures, then composite — needed for things like spatial gamut mapping or grain synthesis)
- 3D LUT support for baked display transforms or photochemical emulation profiles
- Source clip access (for temporal operations like auto-exposure or shot matching, if we ever go there)

**Apple Silicon / Metal / MPS targeting:**

This is a clean initial target. Resolve on macOS uses Metal for its GPU compute path. An OFX plugin targeting Apple Silicon can:

- Use Metal compute shaders (`.metal` files compiled to Metal shader libraries) for all pixel-processing kernels
- Use MPS where useful (e.g., matrix operations for color space transforms, though at per-pixel scale we'd likely write our own kernels)
- Build arm64-native via Xcode/clang with no legacy x86 baggage
- Leverage Apple Silicon's unified memory architecture — no GPU upload/download cost for intermediate buffers, which makes multi-pass rendering essentially free from a memory-transfer perspective

The architecture I'd propose:

```
OFX Plugin Bundle (.ofx.bundle)
├── Metal shader library (.metallib)
│   ├── Transfer function kernels (log↔linear)
│   ├── Photometric operation kernels (exposure, balance, flare)
│   ├── Perceptual operation kernels (contrast, shadows, highlights, sat)
│   ├── Tetrahedral interpolation kernel
│   ├── Display rendering kernel (tone map, gamut map)
│   └── Diagnostic overlay kernels
├── Plugin source (.cpp)
│   ├── OFX parameter definitions and UI layout
│   ├── Render action (dispatch Metal compute)
│   ├── Display transform configuration
│   └── Overlay drawing (viewer UI)
└── Resources
    ├── Default display transform profiles (SDR, HDR1000, HDR4000, P3-DCI)
    └── Photochemical emulation profiles (optional)
```

The Primera DCTL would continue to exist as a lightweight companion — the "grab and go" version for quick primary work within Resolve's existing color management. The OFX plugin would be the full environment: scene-to-display, parametric rendering, comprehensive UI.

## 3. Display Rendering: OpenDRT Integration

### 3.1 Why OpenDRT

Jed Smith's [OpenDRT](https://github.com/jedypod/open-display-transform) (Open Display Rendering Transform) is an excellent fit as Primera OFX's display rendering component. Its design philosophy aligns directly with our architecture: **a neutral, unopinionated rendering that places creative control in the colorist's hands** rather than baking in a strong aesthetic (as ACES 1.x RRT does). Primera's grading operations are the creative layer; the DRT should be transparent infrastructure that faithfully delivers scene intent to the display.

### 3.2 OpenDRT Signal Chain

OpenDRT's internal pipeline:

```
Input gamut → XYZ D65 (working space)
  → Render space saturation
  → Scene-linear offset
  → Brilliance (pre-tonescale)
  → Contrast Low (shadow/midtone piecewise)
  → Tonescale (hyperbolic power compression + quadratic toe)
  → Contrast High (highlight piecewise)
  → Purity compression (progressive desaturation — gamut management)
  → Mid Purity (midtone saturation correction)
  → Post-Brilliance
  → Hue shifts (RGB + CMY, Abney effect compensation)
  → Display gamut conversion
  → Creative whitepoint (CAT02-based)
  → Inverse EOTF
  → Output clamp
```

### 3.3 Key Technical Properties

**Norm-based tonescale.** The core tone curve is a hyperbolic power compression (extended Reinhard family):

```
y = s_y * (x / (x + s_x))^p
```

Combined with a quadratic toe for shadows: `f(x) = x^2 / (x + toe)`. Critically, this is applied to **Rec.2020 luminance** (`Y = 0.2627*R + 0.6780*G + 0.0593*B`), not per-channel. This preserves chromaticity through the tone curve — no per-channel hue shifts in highlights. This directly implements Slomka's principle: compress luminance, preserve chromaticity.

**Purity compression instead of hard gamut mapping.** Rather than clipping at the gamut boundary, OpenDRT uses progressive desaturation — reducing saturation as intensity increases. This operates in an opponent color space: `opponent(rgb) = (R-B, G-(R+B)/2)`. Three stages:
1. **Purity Compress High** — per-channel purity reduction at high intensity (separate R, G, B rates)
2. **Purity Compress Low** — increases tonality in extremely pure inputs, smoothing saturated light source gradients
3. **Mid Purity** — corrects undersaturation in midtone yellows/cyans, prevents chalky highlights

This is Siragusano's smooth gamut mapping philosophy implemented via a different mechanism but to the same end: colors fade toward white naturally rather than clipping.

**Well-behaved path to white.** The purity compression ensures saturated colors desaturate smoothly toward the achromatic axis as they brighten. Hue shift controls (per RGB primary and CMY secondary) compensate for the Abney effect — the perceptual hue shift that occurs with intensity changes.

**Parametric rendering.** The "StickShift" variant exposes the full parameter set: tonescale shape (`tn_Lg` grey luminance, `tn_con` contrast, `tn_sh` shoulder clip, `tn_toe` shadow compression), purity behavior per channel, brilliance per hue (RGB+CMY), hue correction, and creative whitepoint via CAT02. This gives us the parametric DRT control we identified as necessary in FIRST_PRINCIPLES.md.

**Multi-display targeting.** SDR (sRGB ~2.2, Rec.1886 2.4, DCI 2.6), HDR (PQ, HLG at configurable peak luminance 100-4000 nits), multiple output gamuts (Rec.709, P3-D65, P3-D60, P3-DCI, Rec.2020). The `tn_Lp` (peak luminance) parameter adapts the tonescale ceiling to the target display.

### 3.4 Input Expectations

OpenDRT accepts scene-referred linear RGB in any supported wide gamut:
- ARRI Wide Gamut 3/4, Sony SGamut3, RED Wide Gamut
- ACEScg (AP1), ACES AP0
- Rec.2020, Rec.709, P3-D65
- Filmlight E-Gamut, Panasonic V-Gamut, DaVinci Wide Gamut

Internal working space is XYZ D65 (via 3x3 matrix from input gamut). This means Primera's photometric operations (exposure, balance, flare) happen in the camera's native linear gamut, then we hand off to OpenDRT which converts to its working space for rendering.

### 3.5 What We Port

OpenDRT exists only as DCTL and Nuke implementations — no Metal, GLSL, or C/C++. We port the algorithms to Metal compute shaders. The math is entirely per-pixel (no spatial operations): matrix multiplies, power functions, ratio calculations, piecewise curves. Clean port.

### 3.6 Licensing

OpenDRT is **GPLv3** (copyleft). Derivative works must be GPLv3 with source available. Options when distribution becomes relevant:

1. **Accept GPLv3** — release Primera OFX as open source (precedent: Blender, etc.)
2. **Clean-room reimplementation** — implement from the color science (norm-based tone mapping, opponent-space desaturation, purity compression) rather than from the DCTL code. The underlying techniques are not copyrightable.
3. **Contact Jed Smith** — discuss dual licensing or permissive exception for non-competing integration.
4. **Modular separation** — architect the DRT as a loadable component with a clean API boundary.

**For now: proceed with a direct port, flag the license boundary clearly in code, resolve before distribution.**

### 3.7 Comparison: OpenDRT vs. ACES 2.0 DRT

| Aspect | OpenDRT | ACES 2.0 DRT |
|--------|---------|--------------|
| Philosophy | Neutral, author-control-oriented | Opinionated "filmic" reference rendering |
| Highlight color | Predictable (red light on yellow → orange) | Tendency toward red/pink shifts in highlights |
| Gamut management | Progressive desaturation (purity compression) | Explicit gamut compression algorithm |
| Tonescale | Hyperbolic power + quadratic toe | Low highlight contrast, strong toe |
| Invertibility | Designed to be invertible | Limited |
| Input flexibility | Any wide-gamut scene-linear | ACES-centric pipeline |
| Customization | Extensive parametric control | Limited user parameters |

OpenDRT's neutrality and parametric nature make it the better foundation for a tool where the colorist — not the rendering — defines the aesthetic.
