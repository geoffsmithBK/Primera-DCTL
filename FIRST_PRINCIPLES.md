# First Principles: Color Correction and Grading for Log-Encoded Digital Cinema

## 1. The Foundational Domain: Scene-Referred Linear Light

Everything begins with the photon count. A modern cinema camera sensor produces a scene-referred, linear-light signal where pixel value is proportional to exposure (within its photometric linearity range). This is the ground truth. Every operation we perform is either a transformation of this ground truth or a perceptual re-mapping of it.

The key insight that separates modern digital cinema color work from video-era thinking: **the camera negative is a radiometric measurement, not a picture.** LogC, S-Log3, REDLog3G10 — these are *storage encodings*, not looks. They exist to efficiently pack ~14+ stops of dynamic range into a code-value space that survives 10-bit or 12-bit quantization without visible banding. The encoding itself encodes no creative intent.

This is the Poynton principle at its most fundamental: understand *what the numbers represent* before you manipulate them. A code value of 0.5 in LogC4 and 0.5 in S-Log3 represent completely different scene luminances. The transfer function is not the image — it's the container.

## 2. Two Domains, Two Classes of Operation

Primera embodies this correctly, and it's worth making the principle explicit because it's where most grading tools go wrong:

**Scene-linear domain** (photometric operations):
- **Exposure** — multiplicative gain. Physically correct: doubling exposure = doubling photon count = 2x linear gain. This *must* happen in linear.
- **Color balance (temp/tint)** — per-channel linear gains. Equivalent to changing the spectral power distribution of the illuminant. Only physically meaningful in linear.
- **Black point / flare** — additive/subtractive offset in linear. Models lens flare, sensor black level, or optical path contamination. Subtracting a constant in linear is equivalent to removing a uniform veiling luminance.

**Log domain** (perceptual operations):
- **Contrast** — power functions or S-curves applied to log-encoded values. Because log encoding is approximately perceptually uniform (roughly proportional to lightness), contrast adjustments in log space affect perceived tonal distribution. The same power curve applied in linear would devastate shadows while barely touching highlights.
- **Shadows/highlights** — zonal adjustments. Operating in log means you're working in a space that roughly corresponds to how the eye perceives tonal steps. A smoothstep-blended approach is the right idea — it's a soft version of Ansel Adams' zone system.
- **Saturation** — inherently perceptual. Operating on chrominance relationships in a quasi-perceptual space.

## 3. The Evolution from Correction to Grading

### 3.1 Color Correction: The Measurement Discipline

**Color correction** in its original sense meant: make the negative match the scene. Neutral the gray card. Match the reference. This is a *measurement* discipline. It descends from the photochemical lab, where timing lights were adjusted to produce a one-light print that represented the DP's exposure intention.

### 3.2 The DI Revolution

Three convergences created color grading as a distinct craft:

**The digital intermediate (early 2000s)** — When *O Brother, Where Art Thou?* (2000) and *Amélie* (2001) demonstrated that the entire photochemical timing chain could be replaced with pixel-level control, it broke the constraint that "correction" was about *restoring* intent. Now it was about *creating* intent. The colorist became a co-author of the image, not a technician restoring it.

Bill Feightner's work at EFILM was foundational here. EFILM was among the first facilities to build a complete DI pipeline that could replace photochemical timing end-to-end for major studio features. Feightner understood something critical: the DI wasn't just a digitization of the existing process — it was a fundamentally new medium that happened to inherit the aesthetic vocabulary of photochemistry. His insight was that the pipeline needed to *both* faithfully reproduce photochemical behaviors (because filmmakers knew what they wanted in those terms) *and* offer capabilities that exceeded them. This dual mandate — backward compatibility with photochemical intuition, forward capability beyond it — shaped the architecture of Colorfront's tools (On-Set Dailies, Live) and remains a design principle worth preserving.

**Log-to-display pipeline control** — The Cineon system (Kodak, 1993) established that a 10-bit logarithmic encoding could represent the full density range of a film negative. This gave colorists — and the tools built by people like Josh Pines at Kodak and later at various facilities — a *scene-referred* starting point with enough dynamic range to re-interpret the exposure rather than merely correct it. Pines' encoding work (the Pines2 TF) is a direct descendant of this thinking: design a log curve that allocates code values where human perception is most sensitive.

**Display-referred awareness** — Bogdanowicz's and Poynton's work emphasizes that the output is not the input. The transform from scene-referred to display-referred is where creative intent lives. A colorist is implicitly authoring a tone-mapping function — not a fixed one like an OETF, but a *contextual* one that changes per shot, per scene, per emotional beat.

## 4. The Film Look as a Perceptual Local Maximum

### 4.1 The Hypothesis

Roughly a century of photochemical image-making created a set of visual characteristics that audiences have internalized not just as "cinema" but as a proxy for perceived image *quality* itself. The hypothesis: for the general population, photochemical image rendering occupies a **local maximum** in the space of possible image renderings — not necessarily the global optimum, but a stable attractor that's extraordinarily difficult to displace.

This is not nostalgia. It's a claim about human visual preference shaped by long exposure, and it has deep implications for how we design grading tools.

### 4.2 What Film Actually Does (and Why It Works)

Photochemical image capture and reproduction produces several interrelated characteristics:

**Highlight roll-off (the shoulder).** Film's response to increasing exposure is a gradual, asymptotic compression. There is no hard clip. Highlights don't "break" — they *fade*. This behavior is baked into the sensitometric curve of every negative stock ever manufactured. Crucially, the shoulder is not a simple function: different dye layers (cyan, magenta, yellow) have different shoulder onset points and shapes, which means highlight roll-off is accompanied by subtle hue shifts (typically toward warm tones as blue/green layers saturate first). This is the single most-cited characteristic in discussions of "the film look," and it's the one that digital cameras historically got most wrong — a hard-clipping sensor produces a fundamentally different highlight aesthetic than a soft-shouldering negative.

**Shadow rendering (the toe).** Film's toe region compresses deep shadows smoothly, maintaining separation and subtle color while never producing true black. This gives shadows a quality of *depth* — you sense luminance information below what you can explicitly resolve. Digital sensors, particularly early ones, produce noisy but linearly faithful shadow detail, which reads as "thin" or "electronic" without careful handling.

**Cross-channel coupling (inter-layer effects).** In a photochemical negative, the three dye layers are not independent. Silver halide development in one layer affects adjacent layers through chemical byproducts (inter-image effects) and through the physical overlap of spectral sensitivities. The result: changing exposure in one "channel" subtly shifts the others. This creates a color rendering where hue, saturation, and luminance are *coupled* in ways that per-channel digital operations do not reproduce. This coupling tends to produce palettes that feel *coherent* — colors shift together rather than independently.

**Gamut characteristics.** Print stocks (particularly Kodak Vision 2383 and its predecessors) impose a gamut that is neither Rec.709 nor P3, but a specific, irregular volume in color space. This gamut limitation is not a deficiency — it's an editor. Colors that fall outside the print stock's reproducible gamut are compressed or redirected in ways that the stock's dye chemistry dictates. The result is a *curated* palette where extreme saturations are impossible and all colors share a family resemblance.

**Grain as texture.** Film grain is signal-dependent, organic, and spatially random in a way that is distinct from digital noise (which is photon-counting noise at low levels and fixed-pattern at high levels). Grain provides a micro-texture that the eye interprets as *substance* — the image has a physical quality, a materiality.

### 4.3 Siragusano and the Display Rendering Problem

Daniele Siragusano's work at Filmlight — and particularly his contributions to the ACES Output Transform redesign (what became the foundation for the ACES 2.0 DRT) — directly confronts the film-look-as-local-maximum problem from the engineering side.

Siragusano's central insight: **the display rendering transform is where the "look of cinema" either lives or dies, and most digital rendering pipelines get it wrong by treating it as a simple per-channel tone curve.** The problems with naive per-channel rendering are precisely the problems that make digital images look "not like film":

1. **Hue shifts under exposure changes.** A per-channel sigmoid or S-curve, applied independently to R, G, B, changes the *ratio* of the channels as values approach the shoulder or toe. This means that a saturated red, as it gets brighter, doesn't just get brighter — it shifts toward yellow or orange as the red channel clips while green/blue continue to rise. Film doesn't do this (or does it in a controlled, aesthetically coherent way dictated by dye chemistry). Siragusano's work on chromaticity-preserving rendering addresses this directly: the display transform should compress *luminance* while preserving (or controllably shifting) *chromaticity*.

2. **Gamut mapping, not clipping.** When scene colors exceed the display's reproducible gamut, a naive pipeline clips them to the gamut boundary, producing hard edges in color space and visible artifacts (the "LED neon sign problem" — an in-gamut neon becomes a flat, dead patch of saturated primary). Siragusano's approach: compress toward the gamut boundary smoothly, preserving hue where possible, trading saturation for luminance accuracy. This is conceptually what print stock dye chemistry does — it's a smooth, analog gamut map.

3. **The path to white.** Under increasing exposure, a neutral (equal-energy) stimulus should remain neutral all the way to display peak. Film does this naturally — equal exposure increase across layers produces equal density increase. But a per-channel digital sigmoid can break neutrality if the channels have different headroom or different curve shapes above the nominal exposure range. Siragusano's rendering ensures that the "path to white" is chromaticity-stable.

The direct implication for tool design: **operations that happen before the display transform should be designed with awareness of what the display transform will do.** A highlight roll-off applied in scene-referred space, followed by a DRT that applies *its own* roll-off, may produce double-compression artifacts. The tool and the pipeline need to be designed in concert.

### 4.4 Feightner and the Photochemical Simulation Thesis

Bill Feightner's contribution is complementary to Siragusano's. Where Siragusano works forward from first principles ("what should the display rendering *do*?"), Feightner works backward from the photochemical reference ("what does film *actually produce*, and how do we reproduce it digitally?").

The Colorfront/EFILM approach treats the photochemical pipeline as a known-good rendering system. The negative stock's spectral sensitivity, the print stock's dye set, the projector's illuminant — each stage is a characterized transform, and the composition of all stages is the "film look" for that stock combination. Feightner's insight: **you can decompose the film look into component transforms and then recombine or modify them digitally, preserving the characteristics you want (cross-channel coupling, shoulder behavior, gamut compression) while gaining capabilities the photochemical process doesn't offer (selective color manipulation, spatial operations, unlimited "reprints").**

This has a practical implication for Primera and related tools: when we build operations like highlight roll-off, saturation, or color balance, we should consider whether the *combined behavior* of our operations reproduces (or intentionally departs from) the combined behavior of the photochemical pipeline. A tool that handles highlights beautifully but produces color relationships that no film stock would ever generate may feel "wrong" to an audience trained on a century of photochemistry — even if neither the audience nor the colorist can articulate *what* feels wrong.

### 4.5 Convergence: The Local Maximum

These threads converge on a specific claim:

The "film look" is a local maximum in preference space because:

1. **It's a complete, coherent rendering system.** Shoulder, toe, cross-channel coupling, gamut compression, grain — these aren't independent features. They're consequences of a unified physical/chemical process. Reproducing any one of them in isolation (e.g., adding film grain to a digitally-rendered image) doesn't capture the whole, because the perceptual coherence comes from the *interaction* of all elements.

2. **It's been optimized by a century of iteration.** Film stocks evolved in response to filmmaker and audience feedback. Kodak and Fuji didn't design their stocks in a vacuum — they iterated toward preference. The final generation of motion picture negative and print stocks (Vision3, Eterna) represent roughly 100 years of refinement toward what audiences find pleasing.

3. **It may align with properties of human vision.** The logarithmic response of film density to exposure mirrors the approximately logarithmic response of the human visual system to luminance (Weber-Fechner). The smooth shoulder mirrors the adaptation behavior of the eye under high-luminance conditions. The grain structure may activate spatial-frequency channels that contribute to perceived sharpness and "presence." These alignments may not be coincidental — they may be why the process was successful in the first place.

4. **Departures from it are perceptible as "digital" or "video."** When audiences describe an image as looking "video-ish" or "too digital," they are usually reacting to one or more of: hard highlight clipping, per-channel hue shifts in bright regions, excessive saturation uniformity (all colors at similar saturation, without the natural compression that print stock imposes), or a shadow rendering that is either too noisy or too clean (lacking grain's texture without substituting anything in its place).

The question for tool designers is not "how do we make everything look like film" but rather: **how do we give colorists control over the specific perceptual properties that film's process produces, so they can choose to emulate, depart from, or extend the photochemical rendering?**

This is the design philosophy that should guide Primera's evolution: not film emulation per se, but *parametric control over the properties that film got right by accident of chemistry, informed by an understanding of why those properties work perceptually.*

## 5. What the Colorists Do (and What It Implies for Tools)

### 5.1 Walter Volpatto

(Road to Perdition, Hugo, The Irishman)

Works with extremely controlled palettes. His approach implies that hue isolation and selective saturation control are primary tools, not secondary corrections. He treats the image as a composition of color masses — broad regions of related hue and saturation that are shaped together rather than pixel-by-pixel.

**Tool implication:** Tetrahedral interpolation is the right primitive for this — it manipulates color relationships without breaking the channel independence that hue-vs-hue curves can shatter. The six-vertex RGBCMY model maps naturally to how a colorist thinks about "warming the reds," "cooling the blues," or "desaturating the greens" as coherent color-mass operations.

### 5.2 Tom Poole

(Moonlight, If Beale Street Could Talk)

Known for deeply saturated, chromatically complex images where skin tones occupy a very specific and defended region of color space while allowing environments to push into expressive territory. His work on *Moonlight* is instructive: the palette shifts between warm and cool across acts, but skin tones maintain a consistent, luminous quality throughout — they're the anchor around which environmental color pivots.

**Tool implication:** Skin tone protection and qualification are not secondary concerns — they're primary architectural features. A grading tool should make it easy to define a "defended region" (typically a ~25-50 degree hue window around skin) that other operations respect. This could be implemented as a soft hue-angle mask that modulates the strength of operations like saturation, hue rotation, and even contrast.

### 5.3 Alastor Arnold

(The Batman, Dune: Part Two)

Extreme dynamic range manipulation, using the full sensor capture range to create images that feel simultaneously hyperreal and stylized. His work implies that highlight and shadow handling are not just tonal controls but *textural* ones — how highlights roll off defines the "feel" of a light source. The sodium-vapor-dominated palette of *The Batman* required highlight rendering that preserved the spectral character of practical light sources under extreme compression.

**Tool implication:** The shape of the shoulder curve matters as much as its onset point. Parametric control over roll-off shape (not just amount) is essential. A single `tanh` is a good starting point, but a more complete model would offer control over the rate of onset, the ultimate ceiling, and the shape of the transition between them. The Alexa 35's extended highlight handling (via REVEAL color science) gives Arnold more raw material to work with; tools need to match that range.

### 5.4 Matt Osborne

(The Gentlemen, Bodies Bodies Bodies)

Bold, high-contrast looks with deliberate color separation. His work often pushes primaries and secondaries into distinct, clearly delineated regions — a pop sensibility applied to cinema-quality origination.

**Tool implication:** Contrast and color interaction needs to be controllable — specifically, whether increasing contrast also increases apparent saturation (as it does in photochemical processes and in naive RGB contrast) or whether luminance and chrominance are treated independently. A "contrast affects saturation" parameter would let the colorist choose between photochemical-style coupled contrast and purely tonal contrast.

### 5.5 Ian Vertovec

(The Revenant, Birdman, Once Upon a Time in Hollywood)

Vertovec's work is notable for its integration of photochemical sensibility into an entirely digital pipeline. His approach to *The Revenant* — shot on the Alexa 65 in extreme natural light conditions — required maintaining the integrity of natural light's spectral character while compressing an enormous dynamic range into a theatrical presentation. He has spoken about the importance of understanding printer lights and photochemical timing as a *conceptual framework*, even when working entirely digitally.

**Tool implication:** Vertovec's practice suggests that exposure and color balance controls should be designed to *feel* like photochemical timing, even when they're mathematically different. Printer lights operate as multiplicative RGB gains with a specific, integer-step granularity (each point = roughly 1/8 stop, with a slightly different per-channel curve depending on the stock). A "printer light" mode that couples R/G/B gains with the step size and ratio behavior of a photochemical printer — rather than offering independent, continuous per-channel sliders — could be a valuable interface for colorists who think in those terms. Beyond interface: Vertovec's work demonstrates that the *constraint* of printer lights (coarse steps, coupled channels) was actually a feature, not a limitation — it forced changes to be coherent across channels, producing the coupled color shifts that read as "filmic."

### 5.6 Gregg Garvin

Known for precise, technically rigorous DI work. Garvin's approach emphasizes the colorist as a technical bridge between the DP's creative intent and the mathematical reality of the digital pipeline. He is meticulous about calibration, monitoring, and the perceptual accuracy of the grading environment.

**Tool implication:** Garvin's practice underscores the importance of *metrological* tools alongside creative ones. A DCTL designed for primary grading should include diagnostic capabilities: the stop chart in Primera is a start, but could be extended to include gamut coverage visualization, waveform-like overlays showing the distribution of values across the tonal range, and false-color modes that map scene-linear exposure zones to visible colors. The colorist needs to know *where they are* in the code-value space at all times — guessing is unacceptable when the difference between a good grade and a broken one is a few code values in the shadows.

### 5.7 Yvan Lucas

(Portrait of a Lady on Fire, Saint Omer, Anatomy of a Fall)

Lucas's work is characterized by a painterly, almost art-historical sensibility — color in his grades serves narrative and emotional functions that go well beyond "correction." His palette choices reference painting traditions (the warm chiaroscuro of *Portrait of a Lady on Fire* is not accidental; it evokes Baroque candlelight painting). He works with directors who think about color as a dramaturgical element.

**Tool implication:** Lucas's practice suggests that color grading tools should support *intentional* departures from photometric accuracy. Operations like "push all shadows toward blue" or "make highlights trend golden" are not corrections — they're authorial decisions about the emotional temperature of the image. These require tools that can shift color *relationships* (the hue of shadows relative to highlights, the warmth of midtones relative to both) rather than just adjusting individual tonal zones. A "color balance by zone" model — where the colorist can specify a hue/saturation vector for shadows, midtones, and highlights independently — maps directly to how Lucas works. This is related to but distinct from lift/gamma/gain: it's not about *luminance* per zone but about *chrominance* per zone.

## 6. The Technologists: Principles for Tool Architecture

### 6.1 Josh Pines — Encoding as Interface

Pines' work on log encodings (including the Pines2 curve) is fundamentally about **the encoding as a user interface**. The choice of log curve determines how code values map to perceptual steps, which in turn determines how *controls that operate in code-value space* feel to the operator. A curve that spaces code values evenly across perceptual steps makes a linear slider feel perceptually linear. A curve that compresses shadows (allocating fewer code values to deep shadows than to midtones) makes shadow adjustments feel "touchy" while highlight adjustments feel smooth.

**Design principle:** When building controls that operate in log space, consider whether the user's expectation is perceptual uniformity or photometric accuracy, and choose (or offer a choice of) operating space accordingly.

### 6.2 Joseph Slomka — Path to White and Rendering Intent

Slomka's work on display rendering — particularly his thinking about the "path to white" problem and per-channel vs. luminance-preserving operations — addresses the fundamental question: **what should happen to a color as it gets brighter?**

In physical reality, a colored surface under increasing illumination eventually overwhelms the visual system and is perceived as white (the Bezold-Brücke effect, ultimately — though Slomka's concern is the rendering pipeline, not the retinal response). The path from a saturated color to white can follow many trajectories through color space:
- **Per-channel sigmoid:** Each channel clips independently. A saturated red becomes yellow (G catches up to R before B does), then white. This is the naive digital approach and produces the hue shifts that read as "digital."
- **Constant-hue path:** The color desaturates toward white along a line of constant hue angle. This preserves chromatic identity but can look flat or "washed out" in highlights.
- **Film-like path:** The color follows a trajectory determined by the differential shoulder characteristics of the three dye layers. This produces *controlled, stock-specific* hue shifts that are aesthetically intentional. The warm highlight shift of Kodak stocks is a path-to-white characteristic.

**Design principle:** Highlight rendering operations (roll-off, soft clip, shoulder) should offer control over *how chromaticity changes during compression*, not just how luminance compresses. This may mean decomposing the signal into luminance and chromaticity, compressing luminance, and then applying a separate, controllable chromaticity trajectory.

### 6.3 Dr. Mitch Bogdanowicz — Scene-to-Display Mapping

Bogdanowicz's work emphasizes the *chain* from scene to display and the mathematical characterization of each link. His key contribution to our thinking: **every operation in the grading pipeline is implicitly a statement about where in the scene-to-display chain you believe you are operating.** An exposure adjustment in scene-linear is a statement about the scene. A contrast adjustment in log is a statement about perception. A saturation boost in display-referred space is a statement about the display.

**Design principle:** Tools should be explicit about their operating domain. Primera already does this (linear operations in linear, perceptual operations in log), but this could be made more rigorous by, for example, offering a mode where the tool sits after the display transform (for display-referred adjustments) or by explicitly tagging operations as "scene-referred" or "display-referred" so the colorist knows what they're adjusting.

### 6.4 Charles Poynton — Know What the Numbers Mean

Poynton's contribution is foundational: **confusion about what pixel values represent is the root cause of most color pipeline errors.** His relentless insistence on precision in terminology (gamma vs. transfer function, brightness vs. luminance, resolution vs. addressability) is not pedantry — it's engineering discipline.

For tool design, the Poynton principle means:
- Never apply a "gamma correction" without knowing which gamma (OETF? EOTF? encoding gamma? system gamma?).
- Never mix linear-light values with log-encoded values in the same operation.
- Never assume that "0 to 1" means the same thing in different encodings.
- Label controls with their actual units (stops, code values, scene-linear ratio) rather than arbitrary slider ranges.

### 6.5 Daniele Siragusano — The Rendering Transform as Creative Infrastructure

Beyond his specific contributions to ACES 2.0 (discussed above in Section 4.3), Siragusano's broader argument is that **the display rendering transform is not a "last step" — it is the creative infrastructure on which all upstream decisions are evaluated.** A colorist working in DaVinci Resolve with ACES is not grading "the image" — they are grading *the image as it will appear after the Output Transform*. The OT/DRT is as much a part of the creative intent as the grade itself.

This has a radical implication: **a grading tool that operates upstream of the display transform is only half a tool unless it accounts for what the display transform will do.** A highlight roll-off applied in scene-referred space, followed by a DRT that applies *its own* roll-off, may produce double-compression artifacts. Saturation boosted in log space may be clipped by the DRT's gamut mapping. The tool and the pipeline need to be co-designed.

Filmlight's approach (the Truelight system, now integrated throughout Baselight) takes this to its logical conclusion: the rendering is built into the tool, not applied after it. The colorist grades through a rendering, and the rendering adapts to the display target. This is philosophically different from the Resolve/ACES model where the colorist grades in a scene-referred space and the OT is applied separately.

**Design principle:** For tools like Primera that operate in scene-referred space, consider providing a *preview-aware* mode where the tool knows (or can be told) what display transform will follow, and adjusts its behavior accordingly — e.g., reducing the risk of double-compression in highlights or providing feedback about which values will be gamut-clipped by the downstream rendering.

## 7. Where Color Science Meets the Craft: Technical Frontiers

### 7.1 Chromaticity-Linear Operations vs. RGB-Channel Operations

Most grading tools (including Primera currently) operate on R, G, B independently. This is fast and simple but has a fundamental problem: channel-independent operations change both luminance *and* chromaticity simultaneously. Raising the red channel doesn't just make things warmer — it shifts hues and changes saturation in ways that depend on the starting color.

The alternative (explored in ACES 2.0, AgX, and in Slomka's work): operate in a space that separates luminance from chrominance. Candidate spaces:
- **CIE XYZ** (separate Y for luminance)
- **IPT or ICtCp** (perceptually uniform)
- **A "maxRGB" or norm-based decomposition** (luminance approximated by the maximum channel, chrominance by the ratio of channels to the max)

The question isn't "which color space" but "what invariant do you want to preserve during the operation?" If you want contrast to affect only perceived lightness, decompose into luminance + chrominance, modify luminance, recompose. If you want contrast to affect saturation as film would, operate on RGB channels and accept the coupling.

### 7.2 Open-Domain vs. Closed-Domain Operations

Primera currently clamps or guards against out-of-range values in several places. This is pragmatically necessary but theoretically limiting. Scene-linear values above 1.0 are not errors — they're highlights. Log-encoded values above 1.0 (possible in LogC4 and REDLog3G10) represent real scene content.

The frontier approach: design operations that are *unbounded* — they accept and produce any value, relying on a final display mapping to handle the domain reduction. This is the ACES philosophy, but it's also what colorists like Arnold are implicitly relying on when they manipulate extreme highlight values in the Alexa 35's extended range.

Siragusano's gamut-mapping work is relevant here: if operations are unbounded, the display transform must handle arbitrarily large and arbitrarily saturated values gracefully. This is exactly the problem the ACES 2.0 DRT was designed to solve.

### 7.3 The Tone-Mapping Question

Primera's current architecture applies all operations *before* the display transform (it operates in log, which is scene-referred). This is correct for primary grading. But there's an important question about whether some operations should happen *after* or *around* the display transform:

- **Pre-display:** scene-referred grading. "Change what the camera saw."
- **Post-display:** display-referred grading. "Change what the viewer sees."
- **Split:** grade scene-referred, apply display transform, then apply display-referred refinements.

Colorists like Arnold and Poole often work in a split model — primary work in a scene-referred space, then fine-tuning in the display-referred space (often on a Resolve node after the CST/ACES transform). Vertovec similarly: his photochemical-informed sensibility means he thinks about the image in terms of what it will look like on the print (display-referred), even when his initial manipulations are scene-referred. This suggests that a complete tool suite needs operations in *both* domains.

### 7.4 Perceptual Uniformity and the "Look" Transfer Function

The choice of log encoding affects how perceptual operations *feel*. Contrast in LogC3 feels different from contrast in S-Log3 because the log curves have different slopes and different allocations of code values to stops. Pines' encoding work addresses this directly: design a curve where equal code-value steps correspond to equal perceptual steps.

An interesting direction: rather than operating in the camera's native log, convert to a *canonical* perceptual space for all operations, then convert back. This would make the "feel" of controls camera-independent. DaVinci Intermediate exists partly for this reason, as does ACEScct. Filmlight's T-Log is another candidate — Siragusano designed it specifically to be a good working space for grading operations.

The tradeoff: an extra encode/decode pair per pixel, and the risk that the canonical space doesn't perfectly represent the capture characteristics of each camera (particularly in the extremes of the tonal range, where different sensors have different noise floors, highlight behavior, and spectral response).

### 7.5 Spectral Awareness

The ARRI Alexa 35 is significant because its new sensor and REVEAL color science produce a wider-gamut, more spectrally accurate capture than previous Alexas. Sony Venice 2 similarly. RED V-Raptor X with its dual-gain HDR sensor captures spectral information that previous REDs couldn't.

This means that operations designed for Rec.709-bounded color values may clip or distort wide-gamut content. Tetrahedral interpolation is relatively safe here (it's a piecewise-linear map of the unit cube), but HSV-based saturation is not — HSV assumes a bounded RGB cube and behaves unpredictably with out-of-gamut values.

### 7.6 The Photochemical Process as a Design Template

Drawing on Feightner and Vertovec's practices, we can identify the specific photochemical behaviors that are worth making available as parametric controls:

- **Printer-light-style color balance:** Coupled RGB gains with integer-step granularity and stock-specific per-channel curves.
- **Negative-stock shoulder emulation:** Per-channel highlight compression with differential onset points, producing the controlled hue shifts characteristic of specific negative stocks.
- **Print-stock gamut compression:** A smooth, perceptually motivated gamut mapping that constrains extreme saturations in a manner reminiscent of print dye chemistry.
- **Inter-layer coupling (cross-talk):** A controlled bleed between channels that produces the color-mass coherence of photochemical reproduction.
- **Density-based contrast:** Contrast applied as a density-domain operation (essentially: contrast in log space, but with the specific curve shape of a photochemical D-log-H characteristic rather than a generic power function).

These could be offered as a "photochemical mode" within the tool, or as a separate, specialized DCTL that sits alongside Primera.

## 8. Directions for Tool Development

Based on all of the above, the following directions emerge for extending Primera and/or building new tools:

### 8.1 Core Enhancements to Primera

1. **Luminance-chrominance separation** — Add an option to decompose into a luminance + chrominance representation before applying operations like contrast, shadows/highlights, and saturation. This gives the colorist control over whether contrast affects color (Osborne: yes, coupled; Poole: no, preserve skin tones; Lucas: selectively, by zone).

2. **Parametric shoulder/toe shaping** — Extend Roll Off beyond `tanh` to offer parametric control over the *shape* of the compression curve: onset point, rate, ceiling, and chromaticity path during compression (Slomka's path-to-white, Siragusano's chromaticity-preserving rendering, Arnold's textural highlights).

3. **Skin tone protection** — A defended hue/saturation region that operations like global saturation, contrast, and hue rotation respect. Could be implemented as a soft matte derived from hue angle (Poole's practice).

4. **Camera-independent perceptual space** — An option to internally convert to a canonical log space (DaVinci Intermediate, T-Log, or a custom Pines-like curve) for all perceptual operations, making control feel consistent regardless of source TF (Pines' encoding-as-interface principle).

5. **Norm-based saturation** — Replace or supplement HSV saturation with a norm-based approach (max(R,G,B) or Euclidean norm) that's more stable with wide-gamut content and doesn't produce HSV's edge-case artifacts.

6. **Zonal color balance** — Independent hue/saturation vectors for shadows, midtones, and highlights (Lucas's zone-based chromatic authorship, distinct from lift/gamma/gain which operate on luminance).

### 8.2 New Tools

7. **Photochemical emulation DCTL** — A separate tool implementing printer-light balance, negative-stock shoulder emulation, print-stock gamut mapping, and inter-layer coupling as parametric controls (Feightner/Vertovec photochemical thesis).

8. **Display-referred companion DCTL** — A tool designed to sit *after* the display transform for post-rendering refinements: fine contrast, selective saturation, and display-specific adjustments (Siragusano's rendering-aware grading, the split workflow used by Arnold and Poole).

9. **Diagnostic overlay DCTL** — Extended chart/diagnostic capabilities: false-color exposure mapping, gamut boundary visualization, waveform-style overlays, and zone identification (Garvin's metrological rigor).

10. **Scene-to-display bridge** — A tool that is aware of both the upstream encoding and the downstream display transform, providing feedback about values that will be clipped, compressed, or distorted by the rendering pipeline (Siragusano + Bogdanowicz: pipeline-aware operations).

## 9. Summary of Principles

1. **Know what the numbers mean.** (Poynton) Every operation must be designed for a specific signal domain.
2. **Operate in the right domain.** Photometric operations in linear. Perceptual operations in log. Never mix them.
3. **The display transform is part of the creative intent.** (Siragusano) Upstream tools must account for downstream rendering.
4. **The film look is a coherent system, not a checklist.** (Feightner) Emulating individual characteristics in isolation doesn't capture the whole.
5. **Encoding is interface.** (Pines) The choice of working space determines how controls feel to the operator.
6. **Constraint produces coherence.** (Vertovec) Coupled operations often produce more "filmic" results than independent-channel operations.
7. **Separate what you want to control independently.** (Slomka, Bogdanowicz) If the colorist needs to adjust luminance without affecting chrominance, the tool must decompose the signal accordingly.
8. **Defend the anchor.** (Poole) Skin tones and other perceptual anchors should be protected by default, not as an afterthought.
9. **Shape, not just amount.** (Arnold) The character of a curve matters as much as its magnitude.
10. **Color is dramaturgical.** (Lucas) Tools should support intentional departures from accuracy, not just corrections toward it.
