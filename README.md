# Primera-DCTL

A "micro" version of a more ambitious project, presented here in simplified DCTL form. Primera-DCTL is an all-in-one "primary+" color grading DCTL for DaVinci Resolve Studio. It brings together my favorite ways of adjusting:

- Exposure
- Color balance ("temp & tint")
- Contrast / Pivot
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

### 2. Color Balance ("Temp & Tint")

Color balance is applied as per-channel linear gain in linear light-space, alongside exposure, within a single decode/encode round trip. The Temp slider shifts the blue-yellow axis (R and B gain inversely) and the Tint slider shifts the green-magenta axis (G gain), emulating an adjustment to the camera's white balance at the time of shooting.

### 3. Contrast / Pivot

Contrast is applied as a rolling power curve in log space using `pow(x/pivot, contrast) * pivot`, which produces a natural S-curve that is self-limiting at the extremes. The pivot automatically adapts to the encoded mid-gray (0.18 linear) of the currently selected transfer function. An additional Pivot Offset slider allows shifting the pivot point up or down, effectively serving as a midtone brightness control.

### 4. Saturation

Primera-DCTL's approach to increasing saturation works by applying positive linear gain values to the saturation channel of the image in HSV space. Negative saturation, i.e. moving the slider below the "0" point on its scale, works via decimal multiplication in RGB space. Both operations can optionally attempt to preserve the image's perceptual "brightness" via the "Preserve Luminance" checkbox.

### 5. Per-Color Hue and Density

These sliders represent the "+" part of "primary+" in the initial description of Primera-DCTL above. Six pairs of Hue and Density sliders — one for each of the primary and secondary colors (Red, Yellow, Green, Cyan, Blue, Magenta) — provide per-color control via tetrahedral interpolation of the RGB cube.

Hue shifts are implemented as true Rodrigues rotations of each cube corner's chromatic component around the achromatic axis, giving perceptually uniform rotation across the entire hue wheel. Each slider's ±1.0 range maps to ±60° of rotation (one sextant), allowing any color to be pushed all the way to its neighbor. Density adds a uniform luminance offset to a given color region — positive brightens, negative darkens.

The tetrahedral decomposition itself follows the standard Sakamoto method (6 tetrahedra along the black-white diagonal, selected by channel sort order), inspired by Steve Yedlin's work on display preparation and the many community DCTL implementations it spawned.

## Compatibility

Primera-DCTL can also be used as a starting point for look development when applied at the group or timeline level in Resolve. It does not encapsulate any transforms itself, but has been tested and used with the image formation methods I use most frequently:

- Jed Smith's OpenDRT
- Resolve FX ACES Transform
- Resolve FX Color Space Transform
