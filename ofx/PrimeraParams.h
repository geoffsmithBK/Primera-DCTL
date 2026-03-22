// Primera OFX — Scene-to-display primary grading with integrated display rendering.
// Copyright (C) 2024-2026  Primera contributors
//
// Display rendering derived from OpenDRT v1.1.0 by Jed Smith
// https://github.com/jedypod/open-display-transform
// Copyright (C) Jed Smith, licensed under GPLv3
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

// Parameter name constants
static const char* kParamBypass      = "bypass";
static const char* kParamInputTF     = "inputTF";
static const char* kParamExposure    = "exposure";
static const char* kParamBlackPt     = "blackPoint";
static const char* kParamTemp        = "temp";
static const char* kParamTint        = "tint";
static const char* kParamContrast    = "contrast";
static const char* kParamPivot       = "pivot";
static const char* kParamShadows     = "shadows";
static const char* kParamHighlights  = "highlights";
static const char* kParamRollOff     = "rollOff";
static const char* kParamSaturation  = "saturation";
static const char* kParamPreserveLum = "preserveLuma";
static const char* kParamRedHue      = "redHue";
static const char* kParamRedDen      = "redDen";
static const char* kParamYelHue      = "yelHue";
static const char* kParamYelDen      = "yelDen";
static const char* kParamGrnHue      = "grnHue";
static const char* kParamGrnDen      = "grnDen";
static const char* kParamCynHue      = "cynHue";
static const char* kParamCynDen      = "cynDen";
static const char* kParamBluHue      = "bluHue";
static const char* kParamBluDen      = "bluDen";
static const char* kParamMagHue      = "magHue";
static const char* kParamMagDen      = "magDen";
static const char* kParamShowChart   = "showChart";
static const char* kParamShowOverlay = "showOverlay";

// OpenDRT parameter name constants
static const char* kParamEnableDRT       = "enableDRT";
static const char* kParamInputGamut      = "inputGamut";
static const char* kParamLookPreset      = "lookPreset";
static const char* kParamTonescalePreset = "tonescalePreset";
static const char* kParamDisplayEncoding = "displayEncoding";
static const char* kParamCreativeWP      = "creativeWP";
static const char* kParamCWPRange        = "cwpRange";
static const char* kParamSurround        = "surround";
static const char* kParamPeakLum         = "peakLuminance";
static const char* kParamGreyLum         = "greyLuminance";
static const char* kParamHDRGreyBoost    = "hdrGreyBoost";
static const char* kParamHDRPurity       = "hdrPurity";

// GPU params struct — must match Metal shader definition exactly.
// Use only int and float members (no float3) to avoid alignment ambiguity.
struct PrimeraGPUParams {
    int width;
    int height;
    int bypass;
    int inputTF;
    float exposure;
    float blackPoint;
    float temp;
    float tint;
    float contrast;
    float pivot;
    float shadows;
    float highlights;
    float rollOff;
    float saturation;
    int preserveLuma;
    float redHue;
    float redDen;
    float yelHue;
    float yelDen;
    float grnHue;
    float grnDen;
    float cynHue;
    float cynDen;
    float bluHue;
    float bluDen;
    float magHue;
    float magDen;
    int showChart;
    int showOverlay;

    // --- OpenDRT ---
    int enableDRT;

    // Display settings (resolved from presets on CPU)
    int displayGamut;   // 0=Rec.709, 1=P3D65, 2=Rec.2020, 3=P3-D60, 4=P3-DCI, 5=XYZ
    int eotf;           // 0=Linear, 1=sRGB 2.2, 2=Rec.1886 2.4, 3=DCI 2.6, 4=PQ, 5=HLG
    int cwp;            // 0=D93, 1=D75, 2=D65, 3=D60, 4=D55, 5=D50
    int tn_su;          // surround: 0=dark, 1=dim, 2=average
    float cwp_lm;       // creative whitepoint range

    // Enable flags (resolved from look preset)
    int tn_hcon_enable;
    int tn_lcon_enable;
    int pt_enable;
    int ptl_enable;
    int ptm_enable;
    int brl_enable;
    int brlp_enable;
    int hc_enable;
    int hs_rgb_enable;
    int hs_cmy_enable;

    // Render space
    float rs_sa;
    float rs_rw;
    float rs_bw;

    // Tonescale offset (needed per-pixel for offset step)
    float tn_off;

    // Pre-computed tonescale constants (CPU-side)
    float ts_s;
    float ts_p;         // surround-compensated contrast power
    float ts_m2;
    float ts_dsc;
    float ts_x0;
    float ts_s1;
    float s_Lp100;
    float tn_toe;       // for final tonescale inverse toe
    float pt_cmp_Lf;

    // Pre-computed contrast_low constants
    float lcon_m;
    float lcon_w;
    float lcon_cnst_sc;

    // Pre-computed contrast_high
    float hcon_p;
    float tn_hcon_pv;
    float tn_hcon_st;

    // Purity limit low
    float pt_lml;
    float pt_lml_r;
    float pt_lml_g;
    float pt_lml_b;

    // Purity limit high
    float pt_lmh;
    float pt_lmh_r;
    float pt_lmh_b;

    // Purity compress low (softplus)
    float ptl_c;
    float ptl_m;
    float ptl_y;

    // Mid-range purity
    float ptm_low;
    float ptm_low_rng;
    float ptm_low_st;
    float ptm_high;
    float ptm_high_rng;
    float ptm_high_st;

    // Brilliance (pre-tonescale)
    float brl;
    float brl_r;
    float brl_g;
    float brl_b;
    float brl_rng;
    float brl_st;

    // Post brilliance
    float brlp;
    float brlp_r;
    float brlp_g;
    float brlp_b;

    // Hue contrast
    float hc_r;
    float hc_r_rng;

    // Hue shift RGB
    float hs_r;
    float hs_r_rng;
    float hs_g;
    float hs_g_rng;
    float hs_b;
    float hs_b_rng;

    // Hue shift CMY
    float hs_c;
    float hs_c_rng;
    float hs_m;
    float hs_m_rng;
    float hs_y;
    float hs_y_rng;

    // Pre-computed input gamut -> P3-D65 matrix
    float mtxInToWork[9];
};
