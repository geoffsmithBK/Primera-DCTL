# Primera DCTL — Mathematical Reference

Desmos-ready expressions for the grading operations in Primera.dctl. Paste directly into [Desmos](https://www.desmos.com/calculator) to visualize.

Desmos conventions used below:
- `\left\{condition: value\right\}` for piecewise
- `\log_{b}(x)` for log base b; `\ln(x)` for natural log
- Slider parameters shown as `p = 0` (click the slider in Desmos to sweep)

---

## 1. Grading Operations

### Exposure

Linear gain in photographic stops, applied in scene-linear space.

```
p = 0
y = x \cdot 2^p
```

### Black Point (Soft Flare Correction)

Subtracts a constant (veiling glare model), with an exponential soft toe near zero to avoid hard clipping. C1 continuous at the knee.

```
p = 0
k = 0.005
\operatorname{corrected}(x) = x - (-p)

y = \left\{x \le 0: x,\ \operatorname{corrected}(x) \ge k: \operatorname{corrected}(x),\ k \cdot e^{\operatorname{corrected}(x)/k - 1}\right\}
```

### Color Balance (Temp & Tint)

Per-channel linear gain. Temp shifts blue↔yellow; Tint shifts green↔magenta.

```
T = 0
R_{\operatorname{gain}} = 2^T
B_{\operatorname{gain}} = 2^{-T}
G_{\operatorname{gain}} = 2^{\operatorname{Tint}}
```

### Contrast (Rolling Power Curve)

Symmetric power curve in log space that pivots around encoded mid-grey. Self-limiting at extremes — approaches 0 and 1 asymptotically.

```
c = 1.0
p_v = 0.42
o = 0

\operatorname{pivot} = p_v + o

y = \left\{x \le 0: x,\ x \ge 1: x,\ x \le \operatorname{pivot}: \left(\frac{x}{\operatorname{pivot}}\right)^c \cdot \operatorname{pivot},\ 1 - \left(\frac{1-x}{1-\operatorname{pivot}}\right)^c \cdot (1 - \operatorname{pivot})\right\}
```

*Note: `p_v = 0.42` approximates LogC3 mid-grey; adjust per TF.*

### Shadows (Fill Light)

Stop-based gain constrained below mid-grey via smoothstep blend. Full effect at black, identity at mid-grey.

```
p = 0
m = 0.42
g = 2^p

t(x) = \frac{x}{m}
S(x) = t(x)^2 \cdot (3 - 2 \cdot t(x))

y = \left\{x \le 0: x,\ x \ge m: x,\ x \cdot (g + (1 - g) \cdot S(x))\right\}
```

### Highlights

Mirror of shadows — stop-based gain constrained above mid-grey. Identity at mid-grey, full effect at white.

```
p = 0
m = 0.42
g = 2^p

t(x) = \frac{x - m}{1 - m}
S(x) = t(x)^2 \cdot (3 - 2 \cdot t(x))

y = \left\{x \le m: x,\ x \ge 1: x,\ x \cdot (1 + (g - 1) \cdot S(x))\right\}
```

### Roll Off (Tanh Highlight Compression)

Single slider: 0–1 moves the knee from white down to mid-grey; 1–2 increases compression strength (lowers the ceiling). Slope is preserved at the knee.

```
a = 0.5
m = 0.42

k_a = \min(a, 1)
s = 1 + \max(a - 1, 0) \cdot 2

\operatorname{knee} = 1 - k_a \cdot (1 - m)
R = 1 - \operatorname{knee}

y = \left\{x \le 0: x,\ x \le \operatorname{knee}: x,\ \operatorname{knee} + R \cdot \frac{\tanh\left(\frac{(x - \operatorname{knee})}{R} \cdot s\right)}{s}\right\}
```

### Saturation (Positive — HSV S-Channel Gain)

Applies `2^p` gain to the saturation channel in HSV space, clamped to 1.0.

```
p = 0
S_{\operatorname{out}} = \min(S_{\operatorname{in}} \cdot 2^p,\ 1)
```

### Saturation (Negative — RGB Desaturation)

Mixes each channel toward luminance.

```
p = -0.5
L = 0.2126 \cdot R + 0.7152 \cdot G + 0.0722 \cdot B
f = 1 + p

R_{\operatorname{out}} = L + (R - L) \cdot f
```

---

## 2. Tetrahedral Interpolation — Hue & Density

### Rodrigues Rotation (Hue Shift per Cube Corner)

Each cube corner's chromatic component is rotated around the achromatic axis `(1,1,1)/sqrt(3)` by `slider * 60 degrees`.

```
\theta = p \cdot \frac{\pi}{3}
k = \frac{1}{\sqrt{3}}

\bar{v} = \frac{R + G + B}{3}

c_R = R - \bar{v},\quad c_G = G - \bar{v},\quad c_B = B - \bar{v}

c_R' = c_R \cos\theta + k(c_B - c_G)\sin\theta
c_G' = c_G \cos\theta + k(c_R - c_B)\sin\theta
c_B' = c_B \cos\theta + k(c_G - c_R)\sin\theta
```

Output corner: `(c_R' + mean + density, c_G' + mean + density, c_B' + mean + density)`

### Tetrahedral Interpolation (Sakamoto Method)

Six tetrahedra along the black–white diagonal, selected by channel sort order. For the case `R >= G >= B` using Red and Yellow corners:

```
\operatorname{out}_R = R \cdot \operatorname{Rc}_R + G \cdot (\operatorname{Yc}_R - \operatorname{Rc}_R) + B \cdot (1 - \operatorname{Yc}_R)
```

*(Analogous for G, B channels and other sort orders.)*
