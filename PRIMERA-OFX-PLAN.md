# Primera OFX Plugin Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a bespoke OFX plugin for DaVinci Resolve Studio that combines Primera's primary grading operations with OpenDRT's display rendering transform in a single, self-contained scene-to-display pipeline, targeting Apple Silicon / Metal.

**Architecture:** The plugin accepts scene-referred (log or linear) input, applies Primera's grading chain (photometric ops in linear, perceptual ops in log), then hands off to an integrated OpenDRT display rendering (norm-based tonescale, purity compression, gamut mapping, EOTF). All pixel processing runs in a single Metal compute kernel dispatch. The OFX host code handles parameter definitions, UI organization across 4 pages, GPU dispatch, and diagnostic overlay drawing.

**Tech Stack:** C++ (OFX host code), Objective-C++ (Metal bridge), Metal Shading Language (GPU kernels), Make (build system). OFX SDK shipped with DaVinci Resolve. Target: macOS arm64+x86_64 universal binary.

---

## Context

Primera exists as a DCTL (776 lines) that provides comprehensive primary grading within Resolve's node pipeline. However, a DCTL cannot encapsulate the display rendering transform — it sits either before or after Resolve's color management, never straddling it. This means Primera's highlight roll-off can get double-compressed by the DRT, its saturation can get clipped by gamut mapping, and the colorist grades blind to the final rendering.

The solution: an OFX plugin that owns the entire scene-to-display path. Primera's grading operations feed directly into OpenDRT's display rendering, with parametric control over both. The colorist grades *through* the rendering, as Filmlight's Baselight does. The DCTL continues to exist as a lightweight companion for quick work within Resolve's existing color management.

OpenDRT (Jed Smith, GPLv3) is the DRT component because its design philosophy is neutral/unopinionated, placing creative control in the colorist's hands rather than imposing a "look." Its norm-based tonescale preserves chromaticity, its purity compression provides smooth gamut management, and it targets multiple display standards (SDR, HDR PQ/HLG, multiple gamuts).

## Critical Files

| File | Role |
|------|------|
| `/Users/gsmith/work/Primera-DCTL/Primera.dctl` | All grading algorithms to port (lines 124-775) |
| `/tmp/OpenDRT.dctl` | OpenDRT algorithms to port (1210 lines, GPLv3) |
| `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/GainPlugin/GainPlugin.cpp` | OFX plugin structure template |
| `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/GainPlugin/MetalKernel.mm` | Metal pipeline cache + dispatch template |
| `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/GainPlugin/Makefile` | Build system template |
| `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/Support/` | OFX C++ support library (headers + source) |

## File Structure

All new files live in `/Users/gsmith/work/Primera-DCTL/ofx/`:

```
ofx/
├── Makefile                  — Build system (adapted from GainPlugin)
├── PrimeraPlugin.h           — Plugin factory class declaration
├── PrimeraPlugin.cpp         — OFX entry, params, render dispatch, overlay
├── PrimeraParams.h           — Param name constants + GPU params struct
├── MetalKernel.mm            — Metal shader compilation, pipeline cache, dispatch
│                               (shader source embedded as inline C string,
│                                segmented into labeled sections)
└── Info.plist                — Bundle metadata (optional, added in Task 8)
```

**Key design decisions:**

1. **Inline shader source** (like GainPlugin) rather than separate .metal files. Avoids runtime file I/O and xcrun build complexity. The ~2500-line shader string is segmented into concatenated string literals with section comments.

2. **Packed parameter struct** passed as a single `setBytes` call at Metal buffer index 1, rather than individual `setBytes` per parameter. The struct is defined identically in `PrimeraParams.h` (C++) and at the top of the Metal shader source. All members are `int` or `float` — no `float3` — to avoid alignment ambiguity between C++ and Metal.

3. **Gamut matrices baked as constants** in the shader source (ported from the DCTL's 15 input gamuts + output gamuts + CAT02 whitepoint matrices). The CPU selects which matrices to write into the params struct based on user choice params. This avoids runtime JSON parsing.

4. **Single kernel dispatch** — the entire Primera grade + OpenDRT render pipeline executes in one `dispatchThreadgroups` call. No intermediate textures needed since everything is per-pixel.

---

## Task 0: Scaffold — Passthrough Plugin with Metal Dispatch

**Files:**
- Create: `ofx/Makefile`
- Create: `ofx/PrimeraPlugin.h`
- Create: `ofx/PrimeraPlugin.cpp`
- Create: `ofx/PrimeraParams.h`
- Create: `ofx/MetalKernel.mm`

- [ ] **Step 1: Create Makefile**

Adapt from GainPlugin Makefile. Key changes:
- Target: `PrimeraOFX`
- Metal only (no CUDA, no OpenCL objects)
- OFX SDK paths: `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/OpenFX-1.4/include` and `.../Support/include`
- Support library source: `.../Support/Library/*.cpp`
- Frameworks: `-framework Metal -framework AppKit`
- Architectures: `-arch arm64 -arch x86_64`
- Bundle dir: `PrimeraOFX.ofx.bundle/Contents/MacOS/`
- Install target: `cp -fr PrimeraOFX.ofx.bundle /Library/OFX/Plugins`

- [ ] **Step 2: Create PrimeraParams.h**

Minimal version with just the GPU params struct:
```cpp
#pragma once

struct PrimeraGPUParams {
    int width;
    int height;
    int bypass;
};
```

- [ ] **Step 3: Create PrimeraPlugin.h**

Plugin factory declaration (same pattern as GainPlugin.h):
```cpp
#pragma once
#include "ofxsImageEffect.h"

class PrimeraPluginFactory : public OFX::PluginFactoryHelper<PrimeraPluginFactory> {
public:
    PrimeraPluginFactory();
    virtual void load() {}
    virtual void unload() {}
    virtual void describe(OFX::ImageEffectDescriptor& p_Desc);
    virtual void describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum p_Context);
    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle p_Handle, OFX::ContextEnum p_Context);
};
```

- [ ] **Step 4: Create MetalKernel.mm**

Follow GainPlugin MetalKernel.mm exactly:
- Inline shader: a passthrough kernel that copies RGBA input to output
- Pipeline cache with mutex (global `PipelineQueueMap`)
- `RunMetalKernel(void* p_CmdQ, int p_Width, int p_Height, const PrimeraGPUParams* p_Params, const float* p_Input, float* p_Output)`
- Buffer bindings: input=0, output=8, width=11, height=12, params struct=1

- [ ] **Step 5: Create PrimeraPlugin.cpp**

Full OFX plugin following GainPlugin.cpp structure:
- `kPluginIdentifier = "com.primera.PrimeraOFX"`, version 0.1.0, grouping "Primera"
- `PrimeraProcessor` class inheriting `OFX::ImageProcessor`, overriding `processImagesMetal()` only
- `PrimeraPlugin` class inheriting `OFX::ImageEffect` with render/setupAndProcess/isIdentity
- `describe()`: Metal support, `setNoSpatialAwareness(true)`, filter+general contexts, float RGBA
- `describeInContext()`: source clip, output clip, one page "Controls" with a single "Bypass" boolean
- `OFX::Plugin::getPluginIDs()` registration

- [ ] **Step 6: Build and verify**

```bash
cd /Users/gsmith/work/Primera-DCTL/ofx && make
sudo make install
```
Launch Resolve Studio. Add "Primera" OFX to a node (Effects Library > OpenFX > Primera). Image should pass through unmodified.

- [ ] **Step 7: Commit**

```bash
git add ofx/
git commit -m "feat(ofx): scaffold passthrough OFX plugin with Metal dispatch"
```

---

## Task 1: Transfer Functions and Input Parameters

**Files:**
- Modify: `ofx/PrimeraParams.h`
- Modify: `ofx/PrimeraPlugin.cpp`
- Modify: `ofx/MetalKernel.mm` (shader source)

- [ ] **Step 1: Extend PrimeraParams.h with input params**

Add `inputTF` (int) to the GPU params struct. Add parameter name constants:
```cpp
static const char* kParamInputTF = "inputTF";
```

- [ ] **Step 2: Add transfer function choice param to PrimeraPlugin.cpp**

In `describeInContext()`, create Page "Input" with a Choice param for Input Transfer Function. 8 options matching Primera DCTL: ARRI LogC3, ARRI LogC4, REDLog3G10, Sony S-Log3, ACEScct, Pines2, DaVinci Intermediate, Cineon.

In `PrimeraPlugin` constructor, `fetchChoiceParam(kParamInputTF)`.
In `setupAndProcess()`, read the choice value into `PrimeraGPUParams.inputTF`.

- [ ] **Step 3: Port all 8 transfer functions to Metal shader**

Port `logc3_to_lin`/`lin_to_logc3`, `logc4_to_lin`/`lin_to_logc4`, `redlog3g10_to_lin`/`lin_to_redlog3g10`, `slog3_to_lin`/`lin_to_slog3`, `acescct_to_lin`/`lin_to_acescct`, `pines2_to_lin`/`lin_to_pines2`, `di_to_lin`/`lin_to_di`, `cineon_to_lin`/`lin_to_cineon` from Primera.dctl lines 124-304. Port `decode()`/`encode()` dispatch functions (lines 498-524).

DCTL-to-Metal translation table (complete):
| DCTL | Metal |
|------|-------|
| `_powf` | `pow` |
| `_log10f` | `log10` |
| `_log2f` | `log2` |
| `_expf` | `exp` |
| `_exp10f` | `pow(10.0f, x)` (Metal has no `exp10`) |
| `_fmaxf` | `max` |
| `_fminf` | `min` |
| `_fabs` | `fabs` (use `fabs` not `abs` for float — `abs` is integer in Metal) |
| `_sinf` | `sin` |
| `_cosf` | `cos` |
| `_sqrtf` | `sqrt` |
| `_floorf` | `floor` |
| `_tanhf` | `tanh` |
| `_atan2f` | `atan2` |
| `make_float3()` | `float3()` |
| `make_float2()` | `float2()` |
| `float3x3` | use `float mtx[9]` (flat array, avoid Metal's `float3x3` for struct packing) |
| `hypotf3(v)` | `length(v)` (Metal's built-in `length()` for `float3`) |
| `hypotf2(v)` | `length(v)` (Metal's built-in `length()` for `float2`) |

Kernel: decode via selected TF, then immediately re-encode (roundtrip). Output should equal input.

- [ ] **Step 4: Build, install, verify roundtrip**

Load on a LogC3 clip with Resolve timeline set to LogC3. Select "ARRI LogC3" in the plugin. Image should be unchanged. Select a wrong TF — image should look wrong, confirming the decode/encode is active.

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(ofx): add transfer function decode/encode for 8 log formats"
```

---

## Task 2: Scene-Linear Grading Operations

**Files:**
- Modify: `ofx/PrimeraParams.h`
- Modify: `ofx/PrimeraPlugin.cpp`
- Modify: `ofx/MetalKernel.mm` (shader source)

- [ ] **Step 1: Extend params struct and add UI parameters**

Add to struct: `float exposure, blackPoint, temp, tint`.
Add to `describeInContext()`: Page "Grade", Group "Exposure" (Exposure stops, Black Point), Group "Balance" (Temp, Tint). Ranges match DCTL exactly.

- [ ] **Step 2: Port scene-linear operations to Metal shader**

Port `soft_black_point()` from Primera.dctl lines 356-361.
In kernel, after decode and before encode:
- Exposure: `rgb *= pow(2.0, params.exposure);`
- Black Point: `soft_black_point()` calls (lines 662-668)
- Temp/Tint: per-channel gains (lines 673-675)

- [ ] **Step 3: Build, install, verify**

Adjust exposure — image brightens/darkens. Adjust temp — warm/cool shift. Adjust black point — shadows lift/lower with smooth toe. Compare against Primera DCTL with identical settings.

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(ofx): add exposure, black point, temp/tint in scene-linear"
```

---

## Task 3: Log-Domain Grading Operations

**Files:**
- Modify: `ofx/PrimeraParams.h`
- Modify: `ofx/PrimeraPlugin.cpp`
- Modify: `ofx/MetalKernel.mm` (shader source)

- [ ] **Step 1: Extend params and UI**

Add to struct: `float contrast, pivot, shadows, highlights, rollOff, saturation; int preserveLuma`.
Add to "Grade" page: Group "Tone" (Contrast, Pivot, Shadows, Highlights, Roll Off), Group "Saturation" (Saturation, Preserve Luma checkbox).

- [ ] **Step 2: Port log-domain operations to Metal shader**

Port from Primera.dctl:
- `rolling_contrast()` (lines 404-417)
- `shadow_fill()` (lines 366-372)
- `highlight_gain()` (lines 377-383)
- `soft_highlight()` (lines 389-402) — Metal has `tanh()` built in
- `rgb_to_hsv()`, `hsv_to_rgb()`, `luminance()` (lines 310-351)
- Saturation logic (lines 721-752)

Signal chain in kernel: decode → scene-linear ops → encode → contrast/pivot → shadows → highlights → roll off → saturation → output.

- [ ] **Step 3: Build, install, verify**

Full Primera grading pipeline active. Apply identical settings in DCTL and OFX plugin on the same clip. Outputs should be visually indistinguishable.

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(ofx): add contrast, shadows, highlights, roll off, saturation"
```

---

## Task 4: Per-Color Hue/Density and Stop Chart

**Files:**
- Modify: `ofx/PrimeraParams.h`
- Modify: `ofx/PrimeraPlugin.cpp`
- Modify: `ofx/MetalKernel.mm` (shader source)

- [ ] **Step 1: Extend params and UI**

Add 12 per-color floats to struct (redHue, redDen, yelHue, yelDen, ..., magHue, magDen).
Add `int showChart`.
Add to "Grade" page: Group "Per-Color" (collapsed via `setOpen(false)`), 12 double params.
Add Page "Diagnostics": Show Chart boolean.

- [ ] **Step 2: Port tetrahedral interpolation to Metal shader**

Port from Primera.dctl:
- `tetra_corner()` (lines 426-443) — Rodrigues rotation
- `tetra_interp()` (lines 448-492) — 6-tetrahedra interpolation
- Per-color dispatch logic (lines 756-771)

- [ ] **Step 3: Port stop chart to Metal shader**

Port `glyph_row()`, `tf_name_char()`, `tf_name_len()` (lines 42-118) and the chart rendering logic (lines 533-648). Chart bypasses all processing when enabled.

- [ ] **Step 4: Build, install, verify**

Enable "Show Chart" — should match DCTL chart exactly. Test per-color: rotate red hue, observe red objects shift. This completes Primera DCTL feature parity.

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(ofx): add per-color hue/density and stop chart (Primera parity)"
```

---

## Task 5: OpenDRT Core — Tonescale, Gamut Conversion, EOTF

**Files:**
- Modify: `ofx/PrimeraParams.h`
- Modify: `ofx/PrimeraPlugin.cpp`
- Modify: `ofx/MetalKernel.mm` (shader source)

This is the largest task. It adds the OpenDRT display rendering pipeline.

- [ ] **Step 1: Extend params struct with DRT fields**

```cpp
// OpenDRT
int enableDRT;
int inputGamut;       // for gamut conversion to working space
int displayGamut;     // output gamut
int displayEOTF;      // output EOTF (0=Linear, 1=sRGB 2.2, 2=Rec.1886 2.4, 3=DCI 2.6, 4=PQ, 5=HLG)
int creativeWP;       // creative whitepoint
int surround;         // 0=Dark, 1=Dim, 2=Average — modulates tonescale power
int tonescalePreset;  // selects from OpenDRT's 14 tonescale presets
float peakLuminance;  // nits
float greyLuminance;  // nits
float hdrGreyBoost;
float hdrPurity;
// Pre-computed tonescale constants (CPU-side, NOT per-pixel)
float ts_s;           // tonescale scale parameter
float ts_p;           // tonescale power (adjusted for surround)
float ts_m2;          // inverse toe scale
float ts_dsc;         // display encoding scale
float ts_x0;          // 0.18 + offset
float ts_s1;          // HDR-lerped scale for purity
float s_Lp100;        // scene-linear scale at Lp=100
float tn_toe;         // toe compression amount
float tn_off;         // scene-linear offset
// Pre-computed matrices (CPU fills based on gamut selections)
float mtxInToWork[9];   // input gamut -> working space (P3-D65)
float mtxWorkToDisp[9]; // working space -> display gamut
float mtxCWP[9];        // creative whitepoint CAT02
float cwpScale;         // whitepoint normalization
```

- [ ] **Step 2: Add Input page gamut param and Render page params to PrimeraPlugin.cpp**

Page "Input" (extend from Task 1):
- Input Gamut (choice: 15 options — XYZ, AP0, AP1, P3-D65, Rec.2020, Rec.709, ARRI WG3, ARRI WG4, RED WG, Sony SGamut3, SGamut3.Cine, V-Gamut, E-Gamut, E-Gamut2, DaVinci WG)

**Note:** Input Transfer Function and Input Gamut must correspond to the source material (e.g., LogC3 + ARRI WG3, S-Log3 + SGamut3). The plugin does not enforce this — the colorist must select the correct combination. Mismatched settings produce incorrect color. Consider adding a tooltip hint on the Input Gamut param explaining this.

Page "Render":
- Enable DRT (boolean, default true)
- Group "Tonescale":
  - Tonescale Preset (choice: 14 presets from OpenDRT — Standard, ACES 1.x, ACES 2.0, etc.; default Standard)
  - Display Peak Luminance (100-4000, default 100)
  - Display Grey Luminance (3-25, default 10)
  - HDR Grey Boost (0-1, default 0.13)
- Group "Purity": HDR Purity (0-1, default 0.5)
- Display Gamut (choice: Rec.709, P3-D65, P3-D60, P3-DCI, Rec.2020 P3-limited, XYZ-DCI)
- Display EOTF (choice: Linear, sRGB 2.2, Rec.1886 2.4, DCI 2.6, PQ ST.2084, HLG)
- Surround (choice: Dark, Dim, Average; default Dim — modulates tonescale power via `ts_p = tn_con / (1 + surround * 0.05)`)
- Creative Whitepoint (choice: D65, D60, D55, D50)
- Look Preset (choice: Standard, Arriba, Sylvan, Colorful, Aery, Dystopic, Umbra, Base)

- [ ] **Step 3: Bake gamut matrices into shader and CPU selection logic**

Port all 15 input gamut matrices from OpenDRT DCTL (lines ~200-400 in the DCTL). Port output gamut matrices and CAT02 whitepoint matrices. Bake as `constant float` arrays in the Metal shader.

In `setupAndProcess()`, select the appropriate matrices based on choice params and write them into the params struct's `mtxInToWork[9]`, `mtxWorkToDisp[9]`, `mtxCWP[9]` fields. This means the per-pixel shader just reads pre-computed matrices rather than branching on gamut selection.

- [ ] **Step 4: Port OpenDRT core math functions to Metal shader**

From OpenDRT DCTL:
- `compress_hyperbolic_power(x, s, p)` — `pow(x / (x + s), p)`
- `compress_toe_quadratic(x, toe)` — `x * x / (x + toe)`, plus inverse
- `opponent(rgb)` — `float2(r - b, g - (r + b) / 2.0)`
- `gauss_window(x, w)` — `exp(-x * x / w)`
- Matrix multiply: `float3 mtx_mul(float* m, float3 v)` — 3x3 matrix times float3
- PQ EOTF: `eotf_pq()` — ST.2084 inverse
- HLG EOTF: `eotf_hlg()` — BT.2100 inverse

- [ ] **Step 5: Port OpenDRT tonescale pipeline to Metal shader**

Port the core signal chain from OpenDRT DCTL lines 936-1190:
1. Input gamut → P3-D65 (matrix multiply)
2. Render space saturation (weighted luminance desaturation, default rs_sa=0.35)
3. Offset (default 0.005)
4. Tonescale norm: `sqrt(r*r + g*g + b*b) / sqrt(3.0)`
5. RGB ratio decomposition: `rgb / tsn`
6. Hyperbolic compression: `compress_hyperbolic_power(tsn, ts_s, ts_p)` — uses pre-computed constants from params struct
7. Return from ratios: `rgb *= tsn`
8. Clamp [0, 1]
9. Inverse EOTF (Linear passthrough, power, PQ, or HLG)

**Tonescale constraint pre-computation (CPU-side, in `setupAndProcess()`):**
Lines 943-959 of OpenDRT compute ~10 values (`ts_x1`, `ts_y1`, `ts_x0`, `ts_y0`, `ts_s0`, `ts_p`, `ts_s10`, `ts_m1`, `ts_m2`, `ts_s`, `ts_dsc`, `pt_cmp_Lf`, `s_Lp100`, `ts_s1`) that depend only on params, not pixel values. These MUST be computed on the CPU once per render call and passed to the shader via the params struct. This avoids redundant per-pixel computation of `pow()`, `log2()`, etc. The CPU code implements the same formulas from lines 943-959 using standard `<cmath>` functions.

Skip brilliance, hue shifts, and purity compression for this task — those come in Task 6. Use OpenDRT's default constants where needed.

- [ ] **Step 6: Connect Primera output to OpenDRT input**

In the kernel, after Primera's grading chain output (which is in log space), decode back to scene-linear, then feed into the OpenDRT pipeline. The full kernel signal chain becomes:

```
decode(input, TF) → exposure/bp/temp/tint → encode(TF)
→ contrast/shadows/highlights/rolloff → saturation → tetra interp
→ decode(graded, TF) → [OpenDRT: gamut convert → tonescale → EOTF] → output
```

- [ ] **Step 7: Build, install, verify**

Load on a wide-gamut clip. Enable DRT. Image should appear display-rendered: highlights compressed, shadows shaped, gamut mapped. Compare against Jed Smith's OpenDRT OFX plugin (installed at `/Library/OFX/Plugins/OpenDRT.ofx.bundle/`) with matched tonescale settings. The basic tonal rendering should be similar even without purity/hue refinements.

- [ ] **Step 8: Commit**

```bash
git commit -m "feat(ofx): integrate OpenDRT tonescale and gamut mapping (GPLv3)"
```

---

## Task 6: OpenDRT Perceptual — Purity, Hue Shifts, Brilliance

**Files:**
- Modify: `ofx/PrimeraParams.h`
- Modify: `ofx/PrimeraPlugin.cpp` (optional: Look Preset choice param)
- Modify: `ofx/MetalKernel.mm` (shader source)

- [ ] **Step 1: Port hue angle and achromatic distance calculations**

From OpenDRT DCTL lines 985-1013:
- Opponent space: `opp = opponent(rgb)`
- Achromatic distance: `ach_d = length(opp) / 2.0`, smoothed with `compress_toe_quadratic`
- Hue angle: `atan2(opp.x, opp.y)` rotated so red = 0.0
- RGB hue angle windows: `gauss_window()` at specific offsets
- CMY hue angle windows

- [ ] **Step 2: Port brilliance (pre-tonescale)**

From OpenDRT DCTL lines 1016-1022:
- Per-hue intensity scaling via `pow(2.0, brl_exf * ...)` modulated by `ach_d` and `tsn`
- Use default constants: `brl=0.0`, `brl_r=-2.5`, `brl_g=-1.5`, `brl_b=-1.5`, `brl_rng=0.5`, `brl_st=0.35`

- [ ] **Step 3: Port contrast low and contrast high**

From OpenDRT DCTL:
- `compress_toe_cubic()` for contrast low (lines 1024-1036)
- `contrast_high()` for contrast high (lines 1038-1044)
- Use default constants: `tn_lcon=0.0`, `tn_hcon=0.0` (both disabled by default in Standard preset)

- [ ] **Step 4: Port hue contrast R**

From OpenDRT DCTL lines 1062-1072:
- Red hue compression toward primary on low end, expansion on high end
- Use default: `hc_r=1.0`, `hc_r_rng=0.3`

- [ ] **Step 5: Port hue shifts RGB and CMY**

From OpenDRT DCTL lines 1078-1100:
- RGB shifts: R toward yellow, G toward yellow, B toward cyan as intensity increases
- CMY shifts: C toward blue, M toward blue, Y toward red as intensity decreases
- Use defaults: `hs_r=0.6`, `hs_g=0.35`, `hs_b=0.66`, `hs_c=0.25`, `hs_m=0.0`, `hs_y=0.0`

- [ ] **Step 6: Port purity compression (3 stages)**

From OpenDRT DCTL lines 1104-1133:
1. Purity limit low: `ptf = 1.0 - pow(tsn_pt, pt_lml_p)` with per-channel modulation
2. Purity limit high: `ptf = pow(ptf, pt_lmh_p)` modulated by `ach_d`
3. Mid purity: Gaussian-modulated boost/reduce via `exp(-2 * ach_d^2 / st)`
4. Lerp to achromatic: `rgb = rgb * ptf + 1.0 - ptf`

- [ ] **Step 7: Port inverse render space, post brilliance, purity softclip**

From OpenDRT DCTL lines 1135-1155:
- Inverse render space: `rgb = (sat_L * rs_sa - rgb) / (rs_sa - 1.0)`
- `display_gamut_whitepoint()` function (creative whitepoint application)
- Post brilliance (lines 1143-1152)
- Purity softclip: `softplus()` per-channel (line 1155)

- [ ] **Step 8: Add Look Preset choice param (optional)**

Choice param on Render page: Standard (default), plus other OpenDRT presets.
Each preset is a set of constant values for brilliance, hue shift, purity compression rates, and contrast params. A switch in `setupAndProcess()` fills a `DRTLookParams` sub-struct within `PrimeraGPUParams`.

- [ ] **Step 9: Build, install, verify**

Test with saturated wide-gamut content. Purity compression should smoothly desaturate highlights. Skin tones should maintain hue through exposure changes. Compare against Jed's OpenDRT OFX — renders should now match closely with Standard preset.

- [ ] **Step 10: Commit**

```bash
git commit -m "feat(ofx): complete OpenDRT perceptual pipeline (purity, hue, brilliance)"
```

---

## Task 7: Diagnostic Overlay

**Files:**
- Modify: `ofx/PrimeraPlugin.cpp` (add overlay interact, diagnostics page)
- Modify: `ofx/PrimeraParams.h`

- [ ] **Step 1: Implement overlay interact class in PrimeraPlugin.cpp**

Add `PrimeraOverlayInteract` class inheriting `OFX::OverlayInteract`:
- Override `draw(const OFX::DrawArgs& args)`
- Access params via `_effect` pointer to read tonescale parameters
- Use `OFX::Private::gDrawSuite` for drawing (same pattern as GainPlugin lines 329-349)

- [ ] **Step 2: Draw tonescale curve overlay**

When "Show Tonescale Overlay" is enabled:
- Compute 64 points on the tonescale curve (CPU-side, using the same hyperbolic compression formula)
- Draw as a polyline in the bottom-right corner (~200x200 logical pixels)
- Mark 18% grey on X axis, display grey on Y axis
- Mark peak luminance on Y axis
- Use `drawText` for labels: "18%", "Lg", "Lp"
- Use host's standard overlay color via `getColour(kOfxStandardColourOverlayActive)`

- [ ] **Step 3: Wire overlay into plugin descriptor**

In `describe()`: `p_Desc.setOverlayInteractDescriptor(new PrimeraOverlayDescriptor())`

Add `PrimeraOverlayDescriptor` class: `DefaultEffectOverlayDescriptor<PrimeraOverlayDescriptor, PrimeraOverlayInteract>`

- [ ] **Step 4: Build, install, verify**

Enable overlay. Small tonescale plot visible in viewer. Adjust peak luminance — curve ceiling changes. Adjust grey luminance — midpoint shifts.

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(ofx): add tonescale diagnostic overlay"
```

---

## Task 8: Polish and Production Readiness

**Files:**
- Modify: `ofx/PrimeraPlugin.cpp`
- Modify: `ofx/MetalKernel.mm`
- Create: `ofx/Info.plist`

- [ ] **Step 1: Refine isIdentity()**

Return true (passthrough) when all Primera grade params are at defaults AND DRT is disabled. Check all params at render time via `getValueAtTime(p_Args.time)`.

- [ ] **Step 2: Verify animation support**

Audit all `getValueAtTime()` calls use `p_Args.time`, not current time. This ensures keyframed parameters animate correctly.

- [ ] **Step 3: Add error handling to MetalKernel.mm**

Check `newLibraryWithSource:` and `newComputePipelineStateWithFunction:` for errors. Log shader compilation failures with `fprintf(stderr, "Primera OFX: ...")`. On failure, fall through gracefully (output zeros or passthrough).

- [ ] **Step 4: Add license headers**

All files containing OpenDRT-derived code: GPLv3 header citing Jed Smith.
Primera-original code: appropriate header noting the GPLv3 dependency.
Create `ofx/LICENSE` explaining the dual-license situation.

- [ ] **Step 5: Create Info.plist**

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key><string>PrimeraOFX.ofx</string>
    <key>CFBundlePackageType</key><string>BNDL</string>
    <key>CFBundleVersion</key><string>0.1.0</string>
</dict>
</plist>
```

- [ ] **Step 6: Build universal binary and full verification**

```bash
make clean && make
sudo make install
```

Verification matrix:
- Load in Resolve Studio on Apple Silicon
- Test on LogC3 ARRI footage, REDLog3G10 RED footage, S-Log3 Sony footage
- Test SDR output (Rec.709, 2.4 gamma)
- Test HDR output (P3-D65, PQ, 1000 nits peak)
- Verify LUT baking: right-click node → Generate 3D LUT (requires `setNoSpatialAwareness(true)`)
- Verify Grade page + Render page parameters all function
- Compare against standalone OpenDRT OFX + Primera DCTL combination

- [ ] **Step 7: Commit**

```bash
git commit -m "feat(ofx): polish, license headers, production readiness"
```

---

## Parameter Summary

| Page | Group | Parameter | Type | Default | Range |
|------|-------|-----------|------|---------|-------|
| Input | — | Input Transfer Function | choice | LogC3 | 8 options |
| Input | — | Input Gamut | choice | ARRI WG3 | 15 options |
| Grade | Exposure | Exposure | double | 0.0 | [-6, 6] |
| Grade | Exposure | Black Point | double | 0.0 | [-0.05, 0.05] |
| Grade | Balance | Temp | double | 0.0 | [-1, 1] |
| Grade | Balance | Tint | double | 0.0 | [-1, 1] |
| Grade | Tone | Contrast | double | 1.0 | [0.5, 2.0] |
| Grade | Tone | Pivot | double | 0.0 | [-0.2, 0.2] |
| Grade | Tone | Shadows | double | 0.0 | [-1, 1] |
| Grade | Tone | Highlights | double | 0.0 | [-1, 1] |
| Grade | Tone | Roll Off | double | 0.0 | [0, 2] |
| Grade | Saturation | Saturation | double | 0.0 | [-1, 1] |
| Grade | Saturation | Preserve Luma | bool | false | — |
| Grade | Per-Color | Red/Yel/Grn/Cyn/Blu/Mag Hue | 6x double | 0.0 | [-1, 1] |
| Grade | Per-Color | Red/Yel/Grn/Cyn/Blu/Mag Density | 6x double | 0.0 | [-1, 1] |
| Render | — | Enable DRT | bool | true | — |
| Render | Tonescale | Tonescale Preset | choice | Standard | 14 options |
| Render | Tonescale | Display Peak Luminance | double | 100.0 | [100, 4000] |
| Render | Tonescale | Display Grey Luminance | double | 10.0 | [3, 25] |
| Render | Tonescale | HDR Grey Boost | double | 0.13 | [0, 1] |
| Render | Purity | HDR Purity | double | 0.5 | [0, 1] |
| Render | — | Display Gamut | choice | Rec.709 | 6 options |
| Render | — | Display EOTF | choice | Rec.1886 2.4 | 6 options (incl. Linear) |
| Render | — | Surround | choice | Dim | 3 options |
| Render | — | Creative Whitepoint | choice | D65 | 4 options |
| Render | — | Look Preset | choice | Standard | 8 options |
| Diagnostics | — | Show Chart | bool | false | — |
| Diagnostics | — | Show Tonescale Overlay | bool | false | — |
| **Total** | | | | | **41 params** |

**Notes:**
- Input TF and Input Gamut must correspond (e.g., LogC3 + ARRI WG3). The plugin does not enforce this.
- Tonescale Preset selects from OpenDRT's 14 tonescale presets (shape parameters: contrast, shoulder clip, toe, offset). Look Preset selects from 8 look presets (perceptual parameters: brilliance, hue shifts, purity rates).
- Surround modulates the tonescale power: `ts_p = tn_con / (1 + surround * 0.05)` where surround is 0 (Dark), 1 (Dim), or 2 (Average).
- Display EOTF "Linear" outputs scene-linear values (useful for further downstream processing or monitoring).
- Output clamp is always enabled (hardcoded to true). Values are clamped [0, 1] after EOTF application.

## Potential Risks

1. **Metal shader compilation time** — ~2500 lines compiled from string at first load. One-time cost (~100-500ms), cached per command queue. If problematic, pre-compile to `.metallib` via `xcrun -sdk macosx metal`.

2. **Struct alignment** — Metal requires 16-byte alignment for buffer bindings. Use only `int` and `float` members (no `float3`) to avoid padding mismatches between C++ `setBytes` and Metal. Verify with `static_assert(sizeof(PrimeraGPUParams) == EXPECTED)`.

3. **Numerical precision** — `pow(x/(x+s), p)` can lose precision for very small x (shadow region). Monitor for banding or artifacts in deep shadows. DCTL uses 32-bit float throughout, so parity is maintained.

4. **OpenDRT tonescale constraints** — Lines 943-959 compute per-frame constants that should ideally be pre-calculated on the CPU. Several depend only on params (not pixel values) and can be computed once in `setupAndProcess()` and passed to the shader, avoiding redundant per-pixel computation.

5. **GPLv3 licensing** — All OpenDRT-derived code must be clearly marked. Resolve before distribution. Options: accept GPLv3 for the whole plugin, clean-room reimplement from color science, or negotiate with Jed Smith.

6. **Inline shader navigation** — At ~2500 lines, the inline shader string in MetalKernel.mm needs a table-of-contents comment block at the top listing section offsets. Segment the string into labeled concatenated literals:
   ```cpp
   // === SECTION: Transfer Functions (lines ~1-200) ===
   "float logc3_to_lin(float x) { ... }\n"
   // === SECTION: Primera Grade (lines ~200-500) ===
   "float3 primera_grade(...) { ... }\n"
   // === SECTION: OpenDRT Core (lines ~500-1500) ===
   // etc.
   ```
