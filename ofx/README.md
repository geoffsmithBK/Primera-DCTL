# Primera OFX

A native OFX plugin for DaVinci Resolve Studio that combines **scene-to-display primary grading** with an integrated **display rendering transform** (OpenDRT v1.1.0). Primera OFX replaces the need for separate grading nodes and a downstream display transform — the colorist grades *through* the rendering in a single plugin.

## What It Does

Primera OFX provides a unified signal chain:

1. **Scene-linear photometric operations** — exposure (stop-based), black point (flare subtraction), color balance (temp & tint)
2. **Log-space perceptual operations** — contrast/pivot, shadows/highlights, highlight roll-off (tanh), saturation, per-color hue & density (tetrahedral interpolation with Rodrigues rotation)
3. **Display rendering** (OpenDRT) — norm-based tonescale, progressive purity compression (gamut management), brilliance, hue shift compensation, creative whitepoint (CAT02), multi-display output targeting
4. **Built-in diagnostics** — procedural stop chart and interactive tonescale overlay rendered directly in the viewer

All pixel processing runs in **Metal compute shaders** on the GPU.

## DCTL vs. OFX

The companion [Primera DCTL](../Primera.dctl) remains a lightweight, single-file tool for quick primary grading within Resolve's existing color management. The OFX plugin is the full-featured version.

### Advantages of OFX over DCTL

- **Integrated display rendering.** Grades *through* the DRT rather than before or after it — no double-compression of highlights, no display-referred hacks. The plugin owns the entire scene-to-display path.
- **Grouped parameter UI.** Parameters are organized into logical pages (Primary, Color, Display, Diagnostics) with labeled groups, rather than a flat list of sliders.
- **Viewer overlays.** Tonescale curve and stop chart are drawn as native OFX overlays on the viewer.
- **Multi-display targeting.** A single plugin instance can output to SDR (sRGB, Rec.1886, DCI 2.6), HDR (PQ, HLG) at configurable peak luminance, across Rec.709, P3-D65, P3-DCI, Rec.2020, and other gamuts.
- **Look presets.** Curated OpenDRT look presets (Base, Neutral, etc.) that configure the rendering's aesthetic in one click.
- **Universal binary.** Builds as a fat `arm64 + x86_64` bundle.

### Tradeoffs

- **Build step required.** The DCTL is a single file you drop into Resolve's LUT folder. The OFX plugin must be compiled from source.
- **macOS only** (currently). The Metal compute path targets Apple Silicon and Intel Macs. No CUDA/OpenCL kernels yet.
- **GPLv3 license.** The OFX plugin incorporates OpenDRT (GPLv3), so the entire plugin is GPLv3. The DCTL carries no such obligation.
- **Resolve Studio required.** OFX plugins are not supported in the free version of DaVinci Resolve.

## Requirements

- **macOS** 15.x (Sequoia) or later — tested on macOS 26.3.x (Tahoe)
- **Xcode Command Line Tools** (provides `clang++`, `make`, and the Metal toolchain)
- **DaVinci Resolve Studio** 19.x or later — tested on 20.3.x
- **OpenFX SDK** source (headers and support library)

### Xcode Command Line Tools

The build uses `clang++` with `-framework Metal` and `-framework AppKit`. If you have Xcode installed, you already have everything needed. Otherwise:

```bash
xcode-select --install
```

No separate Metal toolchain installation is required — the Metal framework headers and libraries ship with Xcode / Command Line Tools.

### OpenFX SDK

The Makefile expects three directories containing the OpenFX API headers and support library source. These are provided by the [OpenFX SDK](https://github.com/AcademySoftwareFoundation/openfx):

| Directory      | Contents                              | SDK path                       |
|----------------|---------------------------------------|--------------------------------|
| `OFXInc/`      | OFX C API headers                     | `openfx/include/`              |
| `OFXSupportInc/`| OFX C++ Support library headers      | `openfx/Support/include/`      |
| `OFXSupport/`  | OFX C++ Support library sources       | `openfx/Support/Library/`      |

Clone the SDK and create symlinks:

```bash
# Clone the OpenFX SDK (anywhere convenient)
git clone https://github.com/AcademySoftwareFoundation/openfx.git ../openfx

# Create symlinks from the ofx/ directory
cd ofx/
ln -s ../openfx/include         OFXInc
ln -s ../openfx/Support/include OFXSupportInc
ln -s ../openfx/Support/Library OFXSupport
```

The symlinks are listed in `.gitignore` — they are machine-local and not committed to the repository.

## Building

From the `ofx/` directory:

```bash
make
```

This compiles the C++ plugin source and Metal kernel into a universal (`arm64 + x86_64`) binary, then assembles the bundle:

```
PrimeraOFX.ofx.bundle/
├── Contents/
│   ├── Info.plist
│   └── MacOS/
│       └── PrimeraOFX.ofx
```

### Install

```bash
make install
```

Copies the bundle to `/Library/OFX/Plugins/`, where Resolve scans for OFX plugins at launch. You may need `sudo` depending on directory permissions.

### Development cycle

```bash
make dev
```

Runs `clean` → build → `install` in one step. Restart Resolve after installing to load the updated plugin.

### Clean

```bash
make clean
```

Removes all build artifacts (`.o`, `.ofx`, `.ofx.bundle/`).

## Usage in Resolve

After installation, Primera appears under **OpenFX → Primera → Primera** in Resolve's Effects panel. Apply it as a node effect in the Color page. When the integrated display rendering is enabled (`Enable DRT` checkbox), the plugin replaces whatever display transform would normally be downstream — set Resolve's timeline color space to bypass or match your source footage.

## License

GPLv3 — see [LICENSE](LICENSE). Display rendering derived from [OpenDRT v1.1.0](https://github.com/jedypod/open-display-transform) by Jed Smith.
