# Primera-DCTL

A "lite" version of a more ambitious project, presented here in simplified DCTL form. Primera-DCTL is a many-in-one "primary+" color grading DCTL for DaVinci Resolve Studio. It brings together my favorite ways of adjusting:

- Exposure
- Black point (aka "flare")
- Color balance ("temp & tint")
- Contrast / Pivot
- Shadows / Highlights (w/o spatial operations)
- Highlight roll-off
- Saturation
- Six-color hue and density

Primera-DCTL is intended to encapsulate the first 'n' nodes in a fixed node tree that a colorist (me, in this case) would typically start with when grading at the clip level in Resolve. It applies the most typical adjustments I will tend to make and in the order of operations I've come to prefer after many years of color grading.

## Supported Transfer Functions

Primera-DCTL is designed to be used in the context of a fully color-managed workflow and supports the most common transfer functions I encounter in my day-to-day work:

- ARRI LogC3
- ARRI LogC4
- REDLog3G10
- Sony S-Log3
- ACEScct
- DaVinci Intermediate
- Cineon

## How Each Stage Works

Each slider, or group of sliders, tackles a primary color grading operation using a preferential approach. This helps save time when setting up the "head" of my clip-level node tree in preparation for the most time-consuming aspect of grading which (for me) are per-clip adjustments.

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

Highlight roll-off using `tanh` compression. A single "smart" slider where 0-1 moves the compression knee from white down toward mid-grey, and 1-2 progressively lowers the highlight ceiling for increasingly aggressive roll-off. The slope at the knee is always preserved for a seamless join with uncompressed values.

### 7. Saturation

Another "smart" or "combo" slider. Increasing saturation works by applying positive linear gain values to the saturation channel of the image in HSV space. Negative saturation, i.e. moving the slider below the "0" point on its scale, works via decimal multiplication in RGB space. Both operations can optionally attempt to preserve the image's perceptual brightness via the "Preserve Luminance" checkbox.

### 8. Six-Color Hue and Density

These sliders represent the "+" part of "primary+" in the initial description of Primera-DCTL above. Six pairs of Hue and Density sliders — one for each of the primary and secondary colors (Red, Yellow, Green, Cyan, Blue, Magenta) — provide per-color control via tetrahedral interpolation of the RGB cube.

Hue shifts are implemented as true <a href="https://en.wikipedia.org/wiki/Rodrigues'_rotation_formula">"Rodrigues rotations</a> of each cube corner's chromatic component around the achromatic axis, giving perceptually uniform rotation across the entire hue wheel. Each slider's +/-1.0 range maps to +/-60 degrees of rotation (one sextant), allowing any color to be pushed all the way to its neighbor. Density adds a uniform luminance offset to a given color region — positive brightens, negative darkens.

The tetrahedral decomposition itself follows the standard <a href="https://patents.google.com/patent/US4511989A">Sakamoto method</a> (6 tetrahedra along the black-white diagonal, selected by channel sort order). Inspired by <a href="https://www.yedlin.net/DisplayPrepDemo/DispPrepDemoFollowup.html">Steve Yedlin's</a> work on display preparation and the many community DCTL implementations it spawned.

## Stop Chart

A built-in diagnostic chart can be toggled via the "Show Chart" checkbox at the bottom of the UI. Inspired by <a href="https://www.youtube.com/watch?v=ymr4wyo7GcA">Walter Volpatto's</a> exposure chart, it replaces the image with:

- **Upper half**: 17 vertical columns representing -8 to +8 photographic stops relative to mid-grey (0.18 linear), each displayed at its log-encoded value for the selected transfer function. Stops that fall outside the encoding's range clip naturally to black or white, visually indicating the usable dynamic range.
- **Lower half**: A smooth gradient ramp from 0 to 1 in the selected log encoding, overlaid with the transfer function name and its mid-grey code value. The rectangle behind the text is a constant mid-grey value for the displayed Log curve, handy for adding a mid-grey point to a curve with the qualifier tool.

The chart outputs raw log-encoded values, so it responds correctly to whatever display transform is downstream (ACES, CST, OpenDRT, etc.).

## Beyond Clip-Level Grading

Primera-DCTL can also be used as a starting point for look development when applied at the group or timeline level in Resolve, or as part of a fixed node tree. It does not encapsulate any transforms itself, but has been tested and used with the image formation methods I use most frequently:

- Jed Smith's <a href="https://github.com/jedypod/open-display-transform/tree/main">OpenDRT</a>
- Juan-Pablo Zambrano's <a href="https://github.com/JuanPabloZambrano/DCTL/tree/main/2499_DRT">2499 DRT</a>
- Resolve FX <a href="https://github.com/aces-aswf">ACES Transform</a>
- Resolve FX Color Space Transform

## Inspiration

Primera-DCTL wouldn't exist without the contributions, and generously shared knowledge, of many named and unnamed members of the "Color-Concerned Community":

- <a href="https://antlerpost.com/colour-spaces/">Nick Shaw</a>
- <a href="https://github.com/jedypod">Jed Smith</a>
- <a href="https://github.com/baldavenger">Paul Dore</a>
- <a href="https://x.com/Sir_Daniele">Daniele Siragusano</a>
- <a href="https://github.com/https://github.com/thatcherfreeman">Thatcher Freeman</a>
- <a href="https://github.com/hotgluebanjo">Calvin Silly</a>
- <a href="https://www.youtube.com/@KaurH">Kaur Hendrikson</a>
- <a href="https://blog.kasson.com/about-2/patents-and-papers-about-color/">J.M. Kasson</a>
