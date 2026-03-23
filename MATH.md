# Primera DCTL — Mathematical Reference

Desmos-ready expressions for every function in Primera.dctl. Paste directly into [Desmos](https://www.desmos.com/calculator) to visualize.

Desmos conventions used below:
- `\left\{condition: value\right\}` for piecewise
- `\log_{b}(x)` for log base b; `\ln(x)` for natural log
- Slider parameters shown as `p = 0` (click the slider in Desmos to sweep)

---

## 1. Transfer Functions (Log ↔ Linear)

### ARRI LogC3 (EI 800)

**Decode (log → linear):**

```
a = 5.555556
b = 0.052272
c = 0.247190
d = 0.385537
e = 5.367655
f = 0.092809
\operatorname{cut} = e \cdot 0.010591 + f

y = \left\{x > \operatorname{cut}: \frac{10^{(x - d)/c} - b}{a},\ \frac{x - f}{e}\right\}
```

**Encode (linear → log):**

```
y = \left\{x > 0.010591: c \cdot \log_{10}(a \cdot x + b) + d,\ e \cdot x + f\right\}
```

### ARRI LogC4

**Decode (log → linear):**

```
a = \frac{262144 - 16}{117.45}
b = \frac{1023 - 95}{1023}
c = \frac{95}{1023}
s = \frac{7 \cdot \ln(2) \cdot 2^{7 - 14c/b}}{a \cdot b}
t = \frac{2^{14(-c/b) + 6} - 64}{a}

y = \left\{x \ge 0: \frac{2^{14 \cdot ((x - c)/b) + 6} - 64}{a},\ x \cdot s + t\right\}
```

**Encode (linear → log):**

```
y = \left\{x \ge t: \frac{\log_2(a \cdot x + 64) - 6}{14} \cdot b + c,\ \frac{x - t}{s}\right\}
```

### REDLog3G10 (v3 / IPP2)

**Decode (log → linear):**

```
a = 0.224282
b = 155.975327
c = 0.01
g = 15.1927

y = \left\{x < 0: \frac{x}{g} - c,\ \frac{10^{x/a} - 1}{b} - c\right\}
```

**Encode (linear → log):**

```
y = \left\{x + c < 0: (x + c) \cdot g,\ a \cdot \log_{10}((x + c) \cdot b + 1)\right\}
```

### Sony S-Log3

**Decode (log → linear):**

```
\operatorname{cut} = \frac{171.2102946929}{1023}

y = \left\{x \ge \operatorname{cut}: 10^{(x \cdot 1023 - 420)/261.5} \cdot 0.19 - 0.01,\ \frac{(x \cdot 1023 - 95) \cdot 0.01125}{171.2102946929 - 95}\right\}
```

**Encode (linear → log):**

```
y = \left\{x \ge 0.01125: \frac{420 + \log_{10}\left(\frac{x + 0.01}{0.19}\right) \cdot 261.5}{1023},\ \frac{x \cdot \frac{171.2102946929 - 95}{0.01125} + 95}{1023}\right\}
```

### ACEScct

**Decode (log → linear):**

```
\operatorname{cut} = 0.155251141552511
s = 10.5402377416545
o = 0.0729055341958355

y = \left\{x > \operatorname{cut}: 2^{x \cdot 17.52 - 9.72},\ \frac{x - o}{s}\right\}
```

**Encode (linear → log):**

```
y = \left\{x > 0.0078125: \frac{\log_2(x) + 9.72}{17.52},\ s \cdot x + o\right\}
```

### DaVinci Intermediate

**Decode (log → linear):**

```
A = 0.0075
B = 7
C = 0.07329248
M = 10.44426855
\operatorname{cut} = 0.02740668

y = \left\{x \le \operatorname{cut}: \frac{x}{M},\ 2^{x/C - B} - A\right\}
```

**Encode (linear → log):**

```
y = \left\{x \le 0.00262409: x \cdot M,\ C \cdot (\log_2(x + A) + B)\right\}
```

### Cineon

**Decode (log → linear):**

```
o = 0.0108

y = \frac{10^{(x \cdot 1023 - 685)/300} - o}{1 - o}
```

**Encode (linear → log):**

```
y = \frac{\log_{10}(x \cdot (1 - o) + o) \cdot 300 + 685}{1023}
```

---

## 2. Grading Operations

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

## 3. Tetrahedral Interpolation — Hue & Density

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

---

## 4. Rec.709 Luminance

```
L = 0.2126 \cdot R + 0.7152 \cdot G + 0.0722 \cdot B
```
