# Primera-DCTL

A "micro" version of a more ambitious project, presented here in simplified DCTL form. Primera-DCTL is an all-in-one "primary+" color grading DCTL for DaVinci Resolve Studio. It brings together my favorite ways of adjusting:

- Exposure
- Black point
- Color balance ("temp & tint")
- Contrast / Pivot
- Shadows / Highlights
- Highlight roll-off
- Saturation
- Per-color hue and density

Primera-DCTL is intended to be used as a "macro," or sub-graph, encapsulating the first several nodes in a fixed node tree that a colorist (me, in this case) would typically start with when grading at the clip level in Resolve. It applies the most typical adjustments I will tend to make and in the order of operations I've come to prefer after many years of color grading.

## Supported Transfer Functions

It is designed to be used in the context of a fully color-managed workflow and supports the most common transfer functions I encounter in my day-to-day work:

- ARRI LogC3
- ARRI LogC4
- REDLog3G10
- Sony S-Log3
- ACEScct
- Pines2 (based on Josh Pines' log2 encoding specification)
- DaVinci Intermediate
- Cineon

## How Each Stage Works

Each slider, or group of sliders, tackles a primary color grading operation using a preferential approach. This helps save time when setting up the "head" of my clip-level node tree in preparation for the most time-consuming aspect of grading which, for me, are per-clip adjustments.

### 1. Exposure

Primera-DCTL uses linear gain (`Gain = 2^n`) to apply mathematically "correct" +/- exposure adjustments in photographic stops. As such, this operation is performed in linear light-space before being applied to the currently selected transfer function.

### 2. Black Point

Scene-linear flare correction, modeled on the physical phenomenon of veiling glare: a constant amount of stray light is subtracted from the signal in linear light-space. An exponential soft toe near zero ensures values compress smoothly toward black rather than hard-clipping. Dragging the slider left (negative) lowers the black point; dragging right raises it.

### 3. Color Balance ("Temp & Tint")

Color balance is applied as per-channel linear gain in linear light-space, alongside exposure, within a single decode/encode round trip. The Temp slider shifts the blue-yellow axis (R and B gain inversely) and the Tint slider shifts the green-magenta axis (G gain), emulating an adjustment to the camera's white balance at the time of shooting.

### 4. Contrast / Pivot

Contrast is applied as a rolling power curve in log space using `pow(x/pivot, contrast) * pivot`, which produces a natural S-curve that is self-limiting at the extremes. The pivot automatically adapts to the encoded mid-gray (0.18 linear) of the currently selected transfer function. The Pivot slider allows shifting the pivot point up or down, effectively serving as a midtone brightness control.

### 5. Shadows / Highlights

Shadows and Highlights are symmetrical regional gain controls operating in log space. Shadows applies gain constrained below mid-grey using a smoothstep blend, effectively acting as a fill light for recovering shadow detail after contrast or black point adjustments. Highlights applies gain constrained above mid-grey, offering control over the upper tonal range. Both use stop-based gain (`2^n`) consistent with the Exposure slider.

### 6. Roll Off

Highlight roll-off using tanh compression. A single "smart" slider where 0-1 moves the compression knee from white down toward mid-grey, and 1-2 progressively lowers the highlight ceiling for increasingly aggressive roll-off. The slope at the knee is always preserved for a seamless join with uncompressed values.

### 7. Saturation

Primera-DCTL's approach to increasing saturation works by applying positive linear gain values to the saturation channel of the image in HSV space. Negative saturation, i.e. moving the slider below the "0" point on its scale, works via decimal multiplication in RGB space. Both operations can optionally attempt to preserve the image's perceptual "brightness" via the "Preserve Luminance" checkbox.

### 8. Per-Color Hue and Density

These sliders represent the "+" part of "primary+" in the initial description of Primera-DCTL above. Six pairs of Hue and Density sliders — one for each of the primary and secondary colors (Red, Yellow, Green, Cyan, Blue, Magenta) — provide per-color control via tetrahedral interpolation of the RGB cube.

Hue shifts are implemented as true Rodrigues rotations of each cube corner's chromatic component around the achromatic axis, giving perceptually uniform rotation across the entire hue wheel. Each slider's +/-1.0 range maps to +/-60 degrees of rotation (one sextant), allowing any color to be pushed all the way to its neighbor. Density adds a uniform luminance offset to a given color region — positive brightens, negative darkens.

The tetrahedral decomposition itself follows the standard Sakamoto method (6 tetrahedra along the black-white diagonal, selected by channel sort order), inspired by Steve Yedlin's work on display preparation and the many community DCTL implementations it spawned.

## Stop Chart

A built-in diagnostic chart can be toggled via the "Show Chart" checkbox at the bottom of the UI. Inspired by Walter Volpatto's exposure chart, it replaces the image with:

- **Upper half**: 17 vertical columns representing -8 to +8 photographic stops relative to mid-grey (0.18 linear), each displayed at its log-encoded value for the selected transfer function. Stops that fall outside the encoding's range clip naturally to black or white, visually indicating the usable dynamic range.
- **Lower half**: A smooth gradient ramp from 0 to 1 in the selected log encoding, overlaid with the transfer function name and its mid-grey code value rendered in a blocky bitmap font.

The chart outputs raw log-encoded values, so it responds correctly to whatever display transform is downstream (ACES, CST, OpenDRT, etc.).

## Compatibility

Primera-DCTL can also be used as a starting point for look development when applied at the group or timeline level in Resolve. It does not encapsulate any transforms itself, but has been tested and used with the image formation methods I use most frequently:

- Jed Smith's OpenDRT
- Resolve FX ACES Transform
- Resolve FX Color Space Transform
