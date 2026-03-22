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

#include "PrimeraPlugin.h"
#include "PrimeraParams.h"

#include <cstdio>
#include <cmath>
#include <cstring>

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"
#include "ofxsLog.h"

#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#define kPluginName "Primera"
#define kPluginGrouping "Primera"
#define kPluginDescription "Primera: scene-to-display primary grading with integrated display rendering (OpenDRT v1.1.0)"
#define kPluginIdentifier "com.primera.PrimeraOFX"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 2

#define kSupportsTiles false
#define kSupportsMultiResolution false
#define kSupportsMultipleClipPARs false

////////////////////////////////////////////////////////////////////////////////
// Processor
////////////////////////////////////////////////////////////////////////////////

class PrimeraProcessor : public OFX::ImageProcessor
{
public:
    explicit PrimeraProcessor(OFX::ImageEffect& p_Instance);

    virtual void processImagesMetal();
    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);

    void setSrcImg(OFX::Image* p_SrcImg);
    void setParams(const PrimeraGPUParams& p_Params);

private:
    OFX::Image* _srcImg;
    PrimeraGPUParams _params;
};

PrimeraProcessor::PrimeraProcessor(OFX::ImageEffect& p_Instance)
    : OFX::ImageProcessor(p_Instance)
{
}

extern void RunMetalKernel(void* p_CmdQ, int p_Width, int p_Height,
                           const PrimeraGPUParams* p_Params,
                           const float* p_Input, float* p_Output);

void PrimeraProcessor::processImagesMetal()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    _params.width = width;
    _params.height = height;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunMetalKernel(_pMetalCmdQ, width, height, &_params, input, output);
}

void PrimeraProcessor::multiThreadProcessImages(OfxRectI p_ProcWindow)
{
    // CPU fallback — passthrough
    for (int y = p_ProcWindow.y1; y < p_ProcWindow.y2; ++y)
    {
        if (_effect.abort()) break;

        float* dstPix = static_cast<float*>(_dstImg->getPixelAddress(p_ProcWindow.x1, y));

        for (int x = p_ProcWindow.x1; x < p_ProcWindow.x2; ++x)
        {
            float* srcPix = static_cast<float*>(_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

            if (srcPix)
            {
                for (int c = 0; c < 4; ++c)
                    dstPix[c] = srcPix[c];
            }
            else
            {
                for (int c = 0; c < 4; ++c)
                    dstPix[c] = 0;
            }

            dstPix += 4;
        }
    }
}

void PrimeraProcessor::setSrcImg(OFX::Image* p_SrcImg)
{
    _srcImg = p_SrcImg;
}

void PrimeraProcessor::setParams(const PrimeraGPUParams& p_Params)
{
    _params = p_Params;
}

////////////////////////////////////////////////////////////////////////////////
// Plugin
////////////////////////////////////////////////////////////////////////////////

class PrimeraPlugin : public OFX::ImageEffect
{
public:
    explicit PrimeraPlugin(OfxImageEffectHandle p_Handle);

    virtual void render(const OFX::RenderArguments& p_Args);
    virtual bool isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime);

    void setupAndProcess(PrimeraProcessor& p_Processor, const OFX::RenderArguments& p_Args);

private:
    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;

    OFX::BooleanParam* m_Bypass;
    OFX::ChoiceParam*  m_InputTF;
    OFX::DoubleParam*  m_Exposure;
    OFX::DoubleParam*  m_BlackPt;
    OFX::DoubleParam*  m_Temp;
    OFX::DoubleParam*  m_Tint;
    OFX::DoubleParam*  m_Contrast;
    OFX::DoubleParam*  m_Pivot;
    OFX::DoubleParam*  m_Shadows;
    OFX::DoubleParam*  m_Highlights;
    OFX::DoubleParam*  m_RollOff;
    OFX::DoubleParam*  m_Saturation;
    OFX::BooleanParam* m_PreserveLum;
    OFX::DoubleParam*  m_RedHue;
    OFX::DoubleParam*  m_RedDen;
    OFX::DoubleParam*  m_YelHue;
    OFX::DoubleParam*  m_YelDen;
    OFX::DoubleParam*  m_GrnHue;
    OFX::DoubleParam*  m_GrnDen;
    OFX::DoubleParam*  m_CynHue;
    OFX::DoubleParam*  m_CynDen;
    OFX::DoubleParam*  m_BluHue;
    OFX::DoubleParam*  m_BluDen;
    OFX::DoubleParam*  m_MagHue;
    OFX::DoubleParam*  m_MagDen;
    OFX::BooleanParam* m_ShowChart;
    OFX::BooleanParam* m_ShowOverlay;

    // OpenDRT params
    OFX::BooleanParam* m_EnableDRT;
    OFX::ChoiceParam*  m_InputGamut;
    OFX::ChoiceParam*  m_LookPreset;
    OFX::ChoiceParam*  m_TonescalePreset;
    OFX::ChoiceParam*  m_DisplayEncoding;
    OFX::ChoiceParam*  m_CreativeWP;
    OFX::DoubleParam*  m_CWPRange;
    OFX::ChoiceParam*  m_Surround;
    OFX::DoubleParam*  m_PeakLum;
    OFX::DoubleParam*  m_GreyLum;
    OFX::DoubleParam*  m_HDRGreyBoost;
    OFX::DoubleParam*  m_HDRPurity;
};

PrimeraPlugin::PrimeraPlugin(OfxImageEffectHandle p_Handle)
    : ImageEffect(p_Handle)
{
    m_DstClip  = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip  = fetchClip(kOfxImageEffectSimpleSourceClipName);

    m_Bypass      = fetchBooleanParam(kParamBypass);
    m_InputTF     = fetchChoiceParam(kParamInputTF);
    m_Exposure    = fetchDoubleParam(kParamExposure);
    m_BlackPt     = fetchDoubleParam(kParamBlackPt);
    m_Temp        = fetchDoubleParam(kParamTemp);
    m_Tint        = fetchDoubleParam(kParamTint);
    m_Contrast    = fetchDoubleParam(kParamContrast);
    m_Pivot       = fetchDoubleParam(kParamPivot);
    m_Shadows     = fetchDoubleParam(kParamShadows);
    m_Highlights  = fetchDoubleParam(kParamHighlights);
    m_RollOff     = fetchDoubleParam(kParamRollOff);
    m_Saturation  = fetchDoubleParam(kParamSaturation);
    m_PreserveLum = fetchBooleanParam(kParamPreserveLum);
    m_RedHue      = fetchDoubleParam(kParamRedHue);
    m_RedDen      = fetchDoubleParam(kParamRedDen);
    m_YelHue      = fetchDoubleParam(kParamYelHue);
    m_YelDen      = fetchDoubleParam(kParamYelDen);
    m_GrnHue      = fetchDoubleParam(kParamGrnHue);
    m_GrnDen      = fetchDoubleParam(kParamGrnDen);
    m_CynHue      = fetchDoubleParam(kParamCynHue);
    m_CynDen      = fetchDoubleParam(kParamCynDen);
    m_BluHue      = fetchDoubleParam(kParamBluHue);
    m_BluDen      = fetchDoubleParam(kParamBluDen);
    m_MagHue      = fetchDoubleParam(kParamMagHue);
    m_MagDen      = fetchDoubleParam(kParamMagDen);
    m_ShowChart   = fetchBooleanParam(kParamShowChart);
    m_ShowOverlay = fetchBooleanParam(kParamShowOverlay);

    m_EnableDRT       = fetchBooleanParam(kParamEnableDRT);
    m_InputGamut      = fetchChoiceParam(kParamInputGamut);
    m_LookPreset      = fetchChoiceParam(kParamLookPreset);
    m_TonescalePreset = fetchChoiceParam(kParamTonescalePreset);
    m_DisplayEncoding = fetchChoiceParam(kParamDisplayEncoding);
    m_CreativeWP      = fetchChoiceParam(kParamCreativeWP);
    m_CWPRange        = fetchDoubleParam(kParamCWPRange);
    m_Surround        = fetchChoiceParam(kParamSurround);
    m_PeakLum         = fetchDoubleParam(kParamPeakLum);
    m_GreyLum         = fetchDoubleParam(kParamGreyLum);
    m_HDRGreyBoost    = fetchDoubleParam(kParamHDRGreyBoost);
    m_HDRPurity       = fetchDoubleParam(kParamHDRPurity);
}

void PrimeraPlugin::render(const OFX::RenderArguments& p_Args)
{
    if ((m_DstClip->getPixelDepth() == OFX::eBitDepthFloat) &&
        (m_DstClip->getPixelComponents() == OFX::ePixelComponentRGBA))
    {
        PrimeraProcessor processor(*this);
        setupAndProcess(processor, p_Args);
    }
    else
    {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

bool PrimeraPlugin::isIdentity(const OFX::IsIdentityArguments& p_Args,
                                OFX::Clip*& p_IdentityClip, double& p_IdentityTime)
{
    double t = p_Args.time;

    if (m_Bypass->getValueAtTime(t))
    {
        p_IdentityClip = m_SrcClip;
        p_IdentityTime = t;
        return true;
    }

    // Passthrough when all grade params are at defaults AND DRT is disabled
    bool drtOff = !m_EnableDRT->getValueAtTime(t);
    bool gradeIdentity =
        m_Exposure->getValueAtTime(t)    == 0.0  &&
        m_BlackPt->getValueAtTime(t)     == 0.0  &&
        m_Temp->getValueAtTime(t)        == 0.0  &&
        m_Tint->getValueAtTime(t)        == 0.0  &&
        m_Contrast->getValueAtTime(t)    == 1.0  &&
        m_Pivot->getValueAtTime(t)       == 0.0  &&
        m_Shadows->getValueAtTime(t)     == 0.0  &&
        m_Highlights->getValueAtTime(t)  == 0.0  &&
        m_RollOff->getValueAtTime(t)     == 0.0  &&
        m_Saturation->getValueAtTime(t)  == 0.0  &&
        m_RedHue->getValueAtTime(t)      == 0.0  &&
        m_RedDen->getValueAtTime(t)      == 0.0  &&
        m_YelHue->getValueAtTime(t)      == 0.0  &&
        m_YelDen->getValueAtTime(t)      == 0.0  &&
        m_GrnHue->getValueAtTime(t)      == 0.0  &&
        m_GrnDen->getValueAtTime(t)      == 0.0  &&
        m_CynHue->getValueAtTime(t)      == 0.0  &&
        m_CynDen->getValueAtTime(t)      == 0.0  &&
        m_BluHue->getValueAtTime(t)      == 0.0  &&
        m_BluDen->getValueAtTime(t)      == 0.0  &&
        m_MagHue->getValueAtTime(t)      == 0.0  &&
        m_MagDen->getValueAtTime(t)      == 0.0  &&
        !m_ShowChart->getValueAtTime(t);

    if (drtOff && gradeIdentity)
    {
        p_IdentityClip = m_SrcClip;
        p_IdentityTime = t;
        return true;
    }

    return false;
}

// ============================================================================
// OpenDRT matrix data and tonescale pre-computation
// ============================================================================

// 15 input gamut matrices: gamut -> XYZ (row-major, 9 floats each)
// Order: XYZ, AP0, AP1, P3-D65, Rec.2020, Rec.709, AWG3, AWG4, RWG, SGamut3,
//        SGamut3.Cine, V-Gamut, E-Gamut, E-Gamut2, DaVinci WG
static const float kInputGamutMatrices[15][9] = {
    {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}, // XYZ
    {0.93863094875f, -0.00574192055f, 0.017566898852f, 0.338093594922f, 0.727213902811f, -0.065307497733f, 0.000723121511f, 0.000818441849f, 1.0875161874f}, // AP0
    {0.652418717672f, 0.127179925538f, 0.170857283842f, 0.268064059194f, 0.672464478993f, 0.059471461813f, -0.00546992851f, 0.005182799977f, 1.08934487929f}, // AP1
    {0.486571133137f, 0.265667706728f, 0.198217317462f, 0.228974640369f, 0.691738605499f, 0.079286918044f, 0.0f, 0.045113388449f, 1.043944478035f}, // P3-D65
    {0.636958122253f, 0.144616916776f, 0.168880969286f, 0.262700229883f, 0.677998125553f, 0.059301715344f, 0.0f, 0.028072696179f, 1.060985088348f}, // Rec.2020
    {0.412390917540f, 0.357584357262f, 0.180480793118f, 0.212639078498f, 0.715168714523f, 0.072192311287f, 0.019330825657f, 0.119194783270f, 0.950532138348f}, // Rec.709
    {0.638007619284f, 0.214703856337f, 0.097744451431f, 0.291953779f, 0.823841041511f, -0.11579482051f, 0.002798279032f, -0.067034235689f, 1.15329370742f}, // AWG3
    {0.704858320407f, 0.12976029517f, 0.115837311474f, 0.254524176404f, 0.781477732712f, -0.036001909116f, 0.0f, 0.0f, 1.08905775076f}, // AWG4
    {0.735275208950f, 0.068609409034f, 0.146571278572f, 0.286694079638f, 0.842979073524f, -0.129673242569f, -0.079680845141f, -0.347343206406f, 1.516081929207f}, // RWG
    {0.706482713192f, 0.128801049791f, 0.115172164069f, 0.270979670813f, 0.786606411221f, -0.057586082034f, -0.009677845386f, 0.004600037493f, 1.09413555865f}, // SGamut3
    {0.599083920758f, 0.248925516115f, 0.102446490178f, 0.215075820116f, 0.885068501744f, -0.100144321859f, -0.032065849545f, -0.027658390679f, 1.14878199098f}, // SGamut3.Cine
    {0.679644469878f, 0.15221141244f, 0.118600044733f, 0.26068555009f, 0.77489446333f, -0.03558001342f, -0.009310198218f, -0.004612467044f, 1.10298041602f}, // V-Gamut
    {0.705396831036f, 0.164041340351f, 0.081017754972f, 0.280130714178f, 0.820206701756f, -0.100337378681f, -0.103781513870f, -0.072907261550f, 1.265746593475f}, // E-Gamut
    {0.736477700184f, 0.130739651087f, 0.083238575781f, 0.275069984406f, 0.828017790216f, -0.103087774621f, -0.124225154248f, -0.087159767391f, 1.3004426724f}, // E-Gamut2
    {0.700622320175f, 0.148774802685f, 0.101058728993f, 0.274118483067f, 0.873631775379f, -0.147750422359f, -0.098962903023f, -0.137895315886f, 1.325916051865f}, // DaVinci WG
};

// XYZ -> P3-D65 working space
static const float kXYZtoP3[9] = {
    2.4934969119f, -0.9313836179f, -0.4027107845f,
    -0.8294889696f, 1.7626640603f, 0.0236246858f,
    0.0358458302f, -0.0761723893f, 0.9568845240f
};

// 3x3 matrix multiply C = A * B (row-major)
static void mtx_mul_3x3(float* C, const float* A, const float* B) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            C[i*3+j] = 0.0f;
            for (int k = 0; k < 3; ++k)
                C[i*3+j] += A[i*3+k] * B[k*3+j];
        }
}

// CPU-side compress_toe_quadratic (matches shader)
static float cpu_compress_toe_quadratic(float x, float toe, int invert) {
    if (toe == 0.0f) return x;
    if (invert == 0) return x * x / (x + toe);
    return (x + sqrtf(x * (4.0f * toe + x))) / 2.0f;
}

// CPU-side compress_toe_cubic (matches shader)
static float cpu_compress_toe_cubic(float x, float m, float w, int inv) {
    if (m == 1.0f) return x;
    float x2 = x * x;
    if (inv == 0) {
        return x * (x2 + m * w) / (x2 + w);
    } else {
        float p0 = x2 - 3.0f * m * w;
        float p1 = 2.0f * x2 + 27.0f * w - 9.0f * m * w;
        float p2 = powf(sqrtf(x2 * p1 * p1 - 4 * p0 * p0 * p0) / 2.0f + x * p1 / 2.0f, 1.0f / 3.0f);
        return p0 / (3.0f * p2) + p2 / 3.0f + x / 3.0f;
    }
}

// ============================================================================
// Look Preset Data (matches OpenDRT v1.1.0 HEAD exactly)
// ============================================================================
// Each preset sets: tn_con, tn_sh, tn_toe, tn_off,
//   tn_hcon_enable, tn_hcon, tn_hcon_pv, tn_hcon_st,
//   tn_lcon_enable, tn_lcon, tn_lcon_w,
//   cwp, cwp_lm,
//   rs_sa, rs_rw, rs_bw,
//   pt_enable (always 1 for all presets),
//   pt_lml, pt_lml_r, pt_lml_g, pt_lml_b, pt_lmh, pt_lmh_r, pt_lmh_b,
//   ptl_enable, ptl_c, ptl_m, ptl_y,
//   ptm_enable, ptm_low, ptm_low_rng, ptm_low_st, ptm_high, ptm_high_rng, ptm_high_st,
//   brl_enable, brl, brl_r, brl_g, brl_b, brl_rng, brl_st,
//   brlp_enable, brlp, brlp_r, brlp_g, brlp_b,
//   hc_enable, hc_r, hc_r_rng,
//   hs_rgb_enable, hs_r, hs_r_rng, hs_g, hs_g_rng, hs_b, hs_b_rng,
//   hs_cmy_enable, hs_c, hs_c_rng, hs_m, hs_m_rng, hs_y, hs_y_rng

struct LookPresetData {
    float tn_con, tn_sh, tn_toe, tn_off;
    int tn_hcon_enable; float tn_hcon, tn_hcon_pv, tn_hcon_st;
    int tn_lcon_enable; float tn_lcon, tn_lcon_w;
    int cwp; float cwp_lm;
    float rs_sa, rs_rw, rs_bw;
    float pt_lml, pt_lml_r, pt_lml_g, pt_lml_b;
    float pt_lmh, pt_lmh_r, pt_lmh_b;
    int ptl_enable; float ptl_c, ptl_m, ptl_y;
    int ptm_enable; float ptm_low, ptm_low_rng, ptm_low_st, ptm_high, ptm_high_rng, ptm_high_st;
    int brl_enable; float brl, brl_r, brl_g, brl_b, brl_rng, brl_st;
    int brlp_enable; float brlp, brlp_r, brlp_g, brlp_b;
    int hc_enable; float hc_r, hc_r_rng;
    int hs_rgb_enable; float hs_r, hs_r_rng, hs_g, hs_g_rng, hs_b, hs_b_rng;
    int hs_cmy_enable; float hs_c, hs_c_rng, hs_m, hs_m_rng, hs_y, hs_y_rng;
};

static const LookPresetData kLookPresets[8] = {
    // 0: Standard
    { 1.66f, 0.5f, 0.003f, 0.005f,
      0, 0.0f, 1.0f, 4.0f,
      0, 0.0f, 0.5f,
      2, 0.25f,
      0.35f, 0.25f, 0.55f,
      0.25f, 0.5f, 0.0f, 0.1f, 0.25f, 0.5f, 0.0f,
      1, 0.06f, 0.08f, 0.06f,
      1, 0.4f, 0.25f, 0.5f, -0.8f, 0.35f, 0.4f,
      1, 0.0f, -2.5f, -1.5f, -1.5f, 0.5f, 0.35f,
      1, -0.5f, -1.25f, -1.25f, -0.25f,
      1, 1.0f, 0.3f,
      1, 0.6f, 0.6f, 0.35f, 1.0f, 0.66f, 1.0f,
      1, 0.25f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f },
    // 1: Arriba
    { 1.05f, 0.5f, 0.1f, 0.01f,
      0, 0.0f, 1.0f, 4.0f,
      1, 1.5f, 0.2f,
      2, 0.25f,
      0.35f, 0.25f, 0.55f,
      0.25f, 0.45f, 0.0f, 0.1f, 0.25f, 0.25f, 0.0f,
      1, 0.06f, 0.08f, 0.06f,
      1, 1.0f, 0.4f, 0.5f, -0.8f, 0.66f, 0.6f,
      1, 0.0f, -2.5f, -1.5f, -1.5f, 0.5f, 0.35f,
      1, 0.0f, -1.7f, -2.0f, -0.5f,
      1, 1.0f, 0.3f,
      1, 0.6f, 0.8f, 0.35f, 1.0f, 0.66f, 1.0f,
      1, 0.15f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f },
    // 2: Sylvan
    { 1.6f, 0.5f, 0.01f, 0.01f,
      0, 0.0f, 1.0f, 4.0f,
      1, 0.25f, 0.75f,
      2, 0.25f,
      0.25f, 0.25f, 0.55f,
      0.15f, 0.5f, 0.15f, 0.1f, 0.25f, 0.15f, 0.15f,
      1, 0.05f, 0.08f, 0.05f,
      1, 0.5f, 0.5f, 0.5f, -0.8f, 0.5f, 0.5f,
      1, -1.0f, -2.0f, -2.0f, 0.0f, 0.25f, 0.25f,
      1, -1.0f, -0.5f, -0.25f, -0.25f,
      1, 1.0f, 0.4f,
      1, 0.6f, 1.15f, 0.8f, 1.25f, 0.6f, 1.0f,
      1, 0.25f, 0.25f, 0.25f, 0.5f, 0.35f, 0.5f },
    // 3: Colorful
    { 1.5f, 0.5f, 0.003f, 0.003f,
      0, 0.0f, 1.0f, 4.0f,
      1, 0.4f, 0.5f,
      2, 0.25f,
      0.35f, 0.25f, 0.55f,
      0.5f, 1.0f, 0.0f, 0.5f, 0.15f, 0.15f, 0.15f,
      1, 0.05f, 0.06f, 0.05f,
      1, 0.8f, 0.5f, 0.4f, -0.8f, 0.4f, 0.4f,
      1, 0.0f, -1.25f, -1.25f, -0.25f, 0.3f, 0.5f,
      1, -0.5f, -1.25f, -1.25f, -0.5f,
      1, 1.0f, 0.4f,
      1, 0.5f, 0.8f, 0.35f, 1.0f, 0.5f, 1.0f,
      1, 0.25f, 1.0f, 0.0f, 1.0f, 0.25f, 1.0f },
    // 4: Aery
    { 1.15f, 0.5f, 0.04f, 0.006f,
      0, 0.0f, 0.0f, 0.5f,
      1, 0.5f, 2.0f,
      1, 0.25f,
      0.25f, 0.2f, 0.5f,
      0.0f, 0.5f, 0.15f, 0.1f, 0.0f, 0.1f, 0.0f,
      1, 0.05f, 0.08f, 0.05f,
      1, 0.8f, 0.35f, 0.5f, -0.9f, 0.5f, 0.3f,
      1, -3.0f, 0.0f, 0.0f, 1.0f, 0.8f, 0.15f,
      1, -1.0f, -1.0f, -1.0f, 0.0f,
      1, 0.5f, 0.25f,
      1, 0.6f, 1.0f, 0.35f, 2.0f, 0.5f, 1.5f,
      1, 0.35f, 1.0f, 0.25f, 1.0f, 0.35f, 0.5f },
    // 5: Dystopic
    { 1.6f, 0.5f, 0.01f, 0.008f,
      1, 0.25f, 0.0f, 1.0f,
      1, 1.0f, 0.75f,
      3, 0.25f,
      0.2f, 0.25f, 0.55f,
      0.15f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      1, 0.05f, 0.08f, 0.05f,
      1, 0.25f, 0.25f, 0.8f, -0.8f, 0.6f, 0.25f,
      1, -2.0f, -2.0f, -2.0f, 0.0f, 0.35f, 0.35f,
      1, 0.0f, -1.0f, -1.0f, -1.0f,
      1, 1.0f, 0.25f,
      1, 0.7f, 1.33f, 1.0f, 2.0f, 0.75f, 2.0f,
      1, 1.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.765f },
    // 6: Umbra
    { 1.8f, 0.5f, 0.001f, 0.015f,
      0, 0.0f, 1.0f, 4.0f,
      1, 1.0f, 1.0f,
      5, 0.25f,
      0.35f, 0.25f, 0.55f,
      0.0f, 0.5f, 0.0f, 0.15f, 0.25f, 0.25f, 0.0f,
      1, 0.05f, 0.06f, 0.05f,
      1, 0.4f, 0.35f, 0.66f, -0.6f, 0.45f, 0.45f,
      1, -2.0f, -4.5f, -3.0f, -4.0f, 0.35f, 0.3f,
      1, 0.0f, -2.0f, -1.0f, -0.5f,
      1, 1.0f, 0.35f,
      1, 0.66f, 1.0f, 0.5f, 2.0f, 0.85f, 2.0f,
      1, 0.0f, 1.0f, 0.25f, 1.0f, 0.66f, 0.66f },
    // 7: Base
    { 1.66f, 0.5f, 0.003f, 0.005f,
      0, 0.0f, 1.0f, 4.0f,
      0, 0.0f, 0.5f,
      2, 0.25f,
      0.35f, 0.25f, 0.55f,
      0.5f, 0.5f, 0.15f, 0.15f, 0.8f, 0.5f, 0.0f,
      1, 0.05f, 0.06f, 0.05f,
      0, 0.0f, 0.5f, 0.5f, 0.0f, 0.5f, 0.5f,
      0, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.35f,
      1, -0.5f, -1.6f, -1.6f, -0.8f,
      0, 0.0f, 0.25f,
      0, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
      0, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f },
};

// ============================================================================
// Tonescale Preset Data (overrides tonescale params only)
// ============================================================================
struct TonescalePresetData {
    float tn_con, tn_sh, tn_toe, tn_off;
    int tn_hcon_enable; float tn_hcon, tn_hcon_pv, tn_hcon_st;
    int tn_lcon_enable; float tn_lcon, tn_lcon_w;
};

// Index 0 = "USE LOOK PRESET" (no override), indices 1-13 override tonescale
static const TonescalePresetData kTonescalePresets[14] = {
    // 0: USE LOOK PRESET (dummy, not applied)
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    // 1: Low Contrast
    { 1.4f, 0.5f, 0.003f, 0.005f, 0, 0.0f, 1.0f, 4.0f, 0, 0.0f, 0.5f },
    // 2: Medium Contrast
    { 1.66f, 0.5f, 0.003f, 0.005f, 0, 0.0f, 1.0f, 4.0f, 0, 0.0f, 0.5f },
    // 3: High Contrast
    { 1.4f, 0.5f, 0.003f, 0.005f, 0, 0.0f, 1.0f, 4.0f, 1, 1.0f, 0.5f },
    // 4: Arriba Tonescale
    { 1.05f, 0.5f, 0.1f, 0.01f, 0, 0.0f, 1.0f, 4.0f, 1, 1.5f, 0.2f },
    // 5: Sylvan Tonescale
    { 1.6f, 0.5f, 0.01f, 0.01f, 0, 0.0f, 1.0f, 4.0f, 1, 0.25f, 0.75f },
    // 6: Colorful Tonescale
    { 1.5f, 0.5f, 0.003f, 0.003f, 0, 0.0f, 1.0f, 4.0f, 1, 0.4f, 0.5f },
    // 7: Aery Tonescale
    { 1.15f, 0.5f, 0.04f, 0.006f, 0, 0.0f, 0.0f, 0.5f, 1, 0.5f, 2.0f },
    // 8: Dystopic Tonescale
    { 1.6f, 0.5f, 0.01f, 0.008f, 1, 0.25f, 0.0f, 1.0f, 1, 1.0f, 0.75f },
    // 9: Umbra Tonescale
    { 1.8f, 0.5f, 0.001f, 0.015f, 0, 0.0f, 1.0f, 4.0f, 1, 1.0f, 1.0f },
    // 10: ACES-1.x
    { 1.0f, 0.35f, 0.02f, 0.0f, 1, 0.55f, 0.0f, 2.0f, 1, 1.13f, 1.0f },
    // 11: ACES-2.0
    { 1.15f, 0.5f, 0.04f, 0.0f, 0, 1.0f, 1.0f, 1.0f, 0, 1.0f, 0.6f },
    // 12: Marvelous Tonescape
    { 1.5f, 0.5f, 0.003f, 0.01f, 1, 0.25f, 0.0f, 4.0f, 1, 1.0f, 1.0f },
    // 13: DaGrinchi Tonegroan
    { 1.2f, 0.5f, 0.02f, 0.0f, 0, 0.0f, 1.0f, 1.0f, 0, 0.0f, 0.6f },
};

// ============================================================================
// Display Encoding Preset Data
// ============================================================================
struct DisplayEncodingData {
    int tn_su;          // surround compensation value for tonescale
    int display_gamut;  // 0=Rec.709, 1=P3D65, 2=Rec.2020, 3=P3-D60, 4=P3-DCI, 5=XYZ
    int eotf;           // 0=Linear, 1=sRGB 2.2, 2=Rec.1886 2.4, 3=DCI 2.6, 4=PQ, 5=HLG
};

static const DisplayEncodingData kDisplayEncodings[9] = {
    { 1, 0, 2 },  // 0: Rec.1886
    { 2, 0, 1 },  // 1: sRGB Display
    { 2, 1, 1 },  // 2: Display P3
    { 0, 3, 3 },  // 3: P3-D60 DCI
    { 0, 4, 3 },  // 4: P3-DCI
    { 0, 5, 3 },  // 5: DCI P3 XYZ
    { 0, 2, 4 },  // 6: Rec.2100 PQ
    { 0, 2, 5 },  // 7: Rec.2100 HLG
    { 0, 1, 4 },  // 8: Dolby PQ
};

// ============================================================================
// Pre-compute tonescale constants from DRT params (matches OpenDRT DCTL exactly)
// ============================================================================
static void computeTonescaleConstants(PrimeraGPUParams& p,
    float tn_Lp, float tn_gb, float pt_hdr, float tn_Lg,
    float tn_con, float tn_sh, float tn_toe, float tn_off,
    int eotf, int tn_su)
{
    float ts_x1 = powf(2.0f, 6.0f * tn_sh + 4.0f);
    float ts_y1 = tn_Lp / 100.0f;
    float ts_x0 = 0.18f + tn_off;
    float ts_y0 = (tn_Lg / 100.0f) * (1.0f + tn_gb * log2f(ts_y1));
    float ts_s0 = cpu_compress_toe_quadratic(ts_y0, tn_toe, 1);
    float ts_p = tn_con / (1.0f + (float)tn_su * 0.05f);
    float ts_s10 = ts_x0 * (powf(ts_s0, -1.0f / tn_con) - 1.0f);
    float ts_m1 = ts_y1 / powf(ts_x1 / (ts_x1 + ts_s10), tn_con);
    float ts_m2_val = cpu_compress_toe_quadratic(ts_m1, tn_toe, 1);
    float ts_s_val = ts_x0 * (powf(ts_s0 / ts_m2_val, -1.0f / tn_con) - 1.0f);

    // Display scale factor
    float ts_dsc_val = (eotf == 4) ? 0.01f : (eotf == 5) ? 0.1f : 100.0f / tn_Lp;

    // Purity compression interpolation
    float pt_cmp_Lf = pt_hdr * fminf(1.0f, (tn_Lp - 100.0f) / 900.0f);

    // Scene-linear scale at 100 nits
    float s_Lp100_val = ts_x0 * (powf(tn_Lg / 100.0f, -1.0f / tn_con) - 1.0f);

    // Final HDR-lerped tonescale factor
    float ts_s1_val = ts_s_val * pt_cmp_Lf + s_Lp100_val * (1.0f - pt_cmp_Lf);

    p.ts_s = ts_s_val;
    p.ts_p = ts_p;
    p.ts_m2 = ts_m2_val;
    p.ts_dsc = ts_dsc_val;
    p.ts_x0 = ts_x0;
    p.ts_s1 = ts_s1_val;
    p.s_Lp100 = s_Lp100_val;
    p.tn_toe = tn_toe;
    p.tn_off = tn_off;
    p.pt_cmp_Lf = pt_cmp_Lf;
}

// ============================================================================
// setupAndProcess: resolve presets and fill GPU params
// ============================================================================
void PrimeraPlugin::setupAndProcess(PrimeraProcessor& p_Processor,
                                     const OFX::RenderArguments& p_Args)
{
    std::unique_ptr<OFX::Image> dst(m_DstClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth();
    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();

    std::unique_ptr<OFX::Image> src(m_SrcClip->fetchImage(p_Args.time));
    OFX::BitDepthEnum srcBitDepth = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    if ((srcBitDepth != dstBitDepth) || (srcComponents != dstComponents))
    {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }

    double t = p_Args.time;

    PrimeraGPUParams params;
    memset(&params, 0, sizeof(params));

    params.bypass = m_Bypass->getValueAtTime(t) ? 1 : 0;

    int tfVal = 0;
    m_InputTF->getValueAtTime(t, tfVal);
    params.inputTF = tfVal;

    params.exposure    = static_cast<float>(m_Exposure->getValueAtTime(t));
    params.blackPoint  = static_cast<float>(m_BlackPt->getValueAtTime(t));
    params.temp        = static_cast<float>(m_Temp->getValueAtTime(t));
    params.tint        = static_cast<float>(m_Tint->getValueAtTime(t));
    params.contrast    = static_cast<float>(m_Contrast->getValueAtTime(t));
    params.pivot       = static_cast<float>(m_Pivot->getValueAtTime(t));
    params.shadows     = static_cast<float>(m_Shadows->getValueAtTime(t));
    params.highlights  = static_cast<float>(m_Highlights->getValueAtTime(t));
    params.rollOff     = static_cast<float>(m_RollOff->getValueAtTime(t));
    params.saturation  = static_cast<float>(m_Saturation->getValueAtTime(t));
    params.preserveLuma = m_PreserveLum->getValueAtTime(t) ? 1 : 0;
    params.redHue      = static_cast<float>(m_RedHue->getValueAtTime(t));
    params.redDen      = static_cast<float>(m_RedDen->getValueAtTime(t));
    params.yelHue      = static_cast<float>(m_YelHue->getValueAtTime(t));
    params.yelDen      = static_cast<float>(m_YelDen->getValueAtTime(t));
    params.grnHue      = static_cast<float>(m_GrnHue->getValueAtTime(t));
    params.grnDen      = static_cast<float>(m_GrnDen->getValueAtTime(t));
    params.cynHue      = static_cast<float>(m_CynHue->getValueAtTime(t));
    params.cynDen      = static_cast<float>(m_CynDen->getValueAtTime(t));
    params.bluHue      = static_cast<float>(m_BluHue->getValueAtTime(t));
    params.bluDen      = static_cast<float>(m_BluDen->getValueAtTime(t));
    params.magHue      = static_cast<float>(m_MagHue->getValueAtTime(t));
    params.magDen      = static_cast<float>(m_MagDen->getValueAtTime(t));
    params.showChart   = m_ShowChart->getValueAtTime(t) ? 1 : 0;
    params.showOverlay = m_ShowOverlay->getValueAtTime(t) ? 1 : 0;

    // --- OpenDRT params ---
    params.enableDRT = m_EnableDRT->getValueAtTime(t) ? 1 : 0;

    if (params.enableDRT) {
        int gamutVal = 0; m_InputGamut->getValueAtTime(t, gamutVal);
        int lookVal = 0; m_LookPreset->getValueAtTime(t, lookVal);
        int tsPresetVal = 0; m_TonescalePreset->getValueAtTime(t, tsPresetVal);
        int dispEncVal = 0; m_DisplayEncoding->getValueAtTime(t, dispEncVal);
        int cwpPresetVal = 0; m_CreativeWP->getValueAtTime(t, cwpPresetVal);
        int surroundVal = 0; m_Surround->getValueAtTime(t, surroundVal);
        float peakLum = static_cast<float>(m_PeakLum->getValueAtTime(t));
        float greyLum = static_cast<float>(m_GreyLum->getValueAtTime(t));
        float hdrGreyBoost = static_cast<float>(m_HDRGreyBoost->getValueAtTime(t));
        float hdrPurity = static_cast<float>(m_HDRPurity->getValueAtTime(t));
        float cwpRange = static_cast<float>(m_CWPRange->getValueAtTime(t));

        // Step 1: Apply look preset
        int lp = (lookVal >= 0 && lookVal < 8) ? lookVal : 0;
        const LookPresetData& look = kLookPresets[lp];

        // Tonescale params from look preset
        float tn_con = look.tn_con;
        float tn_sh = look.tn_sh;
        float tn_toe = look.tn_toe;
        float tn_off = look.tn_off;
        int tn_hcon_enable = look.tn_hcon_enable;
        float tn_hcon = look.tn_hcon;
        float tn_hcon_pv = look.tn_hcon_pv;
        float tn_hcon_st = look.tn_hcon_st;
        int tn_lcon_enable = look.tn_lcon_enable;
        float tn_lcon = look.tn_lcon;
        float tn_lcon_w = look.tn_lcon_w;

        // CWP from look preset
        int cwp = look.cwp;
        float cwp_lm = look.cwp_lm;

        // Render space from look preset
        params.rs_sa = look.rs_sa;
        params.rs_rw = look.rs_rw;
        params.rs_bw = look.rs_bw;

        // All perceptual params from look preset
        params.pt_enable = 1; // always enabled in preset mode
        params.pt_lml = look.pt_lml; params.pt_lml_r = look.pt_lml_r;
        params.pt_lml_g = look.pt_lml_g; params.pt_lml_b = look.pt_lml_b;
        params.pt_lmh = look.pt_lmh; params.pt_lmh_r = look.pt_lmh_r; params.pt_lmh_b = look.pt_lmh_b;
        params.ptl_enable = look.ptl_enable;
        params.ptl_c = look.ptl_c; params.ptl_m = look.ptl_m; params.ptl_y = look.ptl_y;
        params.ptm_enable = look.ptm_enable;
        params.ptm_low = look.ptm_low; params.ptm_low_rng = look.ptm_low_rng;
        params.ptm_low_st = look.ptm_low_st;
        params.ptm_high = look.ptm_high; params.ptm_high_rng = look.ptm_high_rng;
        params.ptm_high_st = look.ptm_high_st;
        params.brl_enable = look.brl_enable;
        params.brl = look.brl; params.brl_r = look.brl_r;
        params.brl_g = look.brl_g; params.brl_b = look.brl_b;
        params.brl_rng = look.brl_rng; params.brl_st = look.brl_st;
        params.brlp_enable = look.brlp_enable;
        params.brlp = look.brlp; params.brlp_r = look.brlp_r;
        params.brlp_g = look.brlp_g; params.brlp_b = look.brlp_b;
        params.hc_enable = look.hc_enable;
        params.hc_r = look.hc_r; params.hc_r_rng = look.hc_r_rng;
        params.hs_rgb_enable = look.hs_rgb_enable;
        params.hs_r = look.hs_r; params.hs_r_rng = look.hs_r_rng;
        params.hs_g = look.hs_g; params.hs_g_rng = look.hs_g_rng;
        params.hs_b = look.hs_b; params.hs_b_rng = look.hs_b_rng;
        params.hs_cmy_enable = look.hs_cmy_enable;
        params.hs_c = look.hs_c; params.hs_c_rng = look.hs_c_rng;
        params.hs_m = look.hs_m; params.hs_m_rng = look.hs_m_rng;
        params.hs_y = look.hs_y; params.hs_y_rng = look.hs_y_rng;

        // Step 2: Tonescale preset override (index 0 = use look preset, no override)
        int tp = (tsPresetVal >= 0 && tsPresetVal < 14) ? tsPresetVal : 0;
        if (tp > 0) {
            const TonescalePresetData& ts = kTonescalePresets[tp];
            tn_con = ts.tn_con; tn_sh = ts.tn_sh; tn_toe = ts.tn_toe; tn_off = ts.tn_off;
            tn_hcon_enable = ts.tn_hcon_enable; tn_hcon = ts.tn_hcon;
            tn_hcon_pv = ts.tn_hcon_pv; tn_hcon_st = ts.tn_hcon_st;
            tn_lcon_enable = ts.tn_lcon_enable; tn_lcon = ts.tn_lcon; tn_lcon_w = ts.tn_lcon_w;
        }

        // Step 3: Creative white preset override
        // 0 = USE LOOK PRESET, 1=D93, 2=D75, 3=D65, 4=D60, 5=D55, 6=D50
        if (cwpPresetVal > 0) {
            cwp_lm = cwpRange;
            if (cwpPresetVal == 1) cwp = 0;      // D93
            else if (cwpPresetVal == 2) cwp = 1;  // D75
            else if (cwpPresetVal == 3) cwp = 2;  // D65
            else if (cwpPresetVal == 4) cwp = 3;  // D60
            else if (cwpPresetVal == 5) cwp = 4;  // D55
            else if (cwpPresetVal == 6) cwp = 5;  // D50
        }

        // Step 4: Display encoding preset
        int de = (dispEncVal >= 0 && dispEncVal < 9) ? dispEncVal : 0;
        const DisplayEncodingData& enc = kDisplayEncodings[de];
        int tn_su = enc.tn_su;
        params.displayGamut = enc.display_gamut;
        params.eotf = enc.eotf;
        params.tn_su = tn_su;
        params.cwp = cwp;
        params.cwp_lm = cwp_lm;

        // Surround override from user (modifies tn_su)
        // The surround param dropdown: 0=Dark, 1=Dim, 2=Average
        // In OpenDRT, tn_su is set by display encoding preset, then user can override
        // We treat the surround dropdown as an override:
        params.tn_su = surroundVal;

        // Step 5: Store enable flags and pre-compute
        params.tn_hcon_enable = tn_hcon_enable;
        params.tn_lcon_enable = tn_lcon_enable;
        params.tn_off = tn_off;

        // Pre-compute tonescale constants
        computeTonescaleConstants(params, peakLum, hdrGreyBoost, hdrPurity,
                                  greyLum, tn_con, tn_sh, tn_toe, tn_off,
                                  params.eotf, params.tn_su);

        // Pre-compute contrast_high power
        params.hcon_p = powf(2.0f, tn_hcon);
        params.tn_hcon_pv = tn_hcon_pv;
        params.tn_hcon_st = tn_hcon_st;

        // Pre-compute contrast_low constants
        if (tn_lcon_enable) {
            float lcon_m = powf(2.0f, -tn_lcon);
            float lcon_w = tn_lcon_w / 4.0f;
            lcon_w *= lcon_w;
            float lcon_cnst_sc = cpu_compress_toe_cubic(params.ts_x0, lcon_m, lcon_w, 1) / params.ts_x0;
            params.lcon_m = lcon_m;
            params.lcon_w = lcon_w;
            params.lcon_cnst_sc = lcon_cnst_sc;
        }

        // Pre-compute input gamut -> P3-D65 matrix
        int gIdx = (gamutVal >= 0 && gamutVal < 15) ? gamutVal : 0;
        mtx_mul_3x3(params.mtxInToWork, kXYZtoP3, kInputGamutMatrices[gIdx]);
    }

    p_Processor.setDstImg(dst.get());
    p_Processor.setSrcImg(src.get());
    p_Processor.setGPURenderArgs(p_Args);
    p_Processor.setRenderWindow(p_Args.renderWindow);
    p_Processor.setParams(params);
    p_Processor.process();
}

////////////////////////////////////////////////////////////////////////////////
// Overlay Interact — tonescale curve visualization
////////////////////////////////////////////////////////////////////////////////

PrimeraOverlayInteract::PrimeraOverlayInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
    : OFX::OverlayInteract(handle)
    , _effect(effect)
{
}

// CPU-side tonescale evaluation (matches GPU pipeline)
static float evalTonescale(float x, float ts_s, float ts_p, float ts_m2,
                           float ts_x0, float tn_toe, float ts_dsc)
{
    float n = x / (x + ts_s);
    float y = (n < 0.0f) ? n : powf(n, ts_p);
    y *= ts_m2;
    // Quadratic toe
    if (tn_toe > 0.0f)
        y = y * y / (y + tn_toe);
    // Display scale
    y *= ts_dsc;
    return y;
}

bool PrimeraOverlayInteract::draw(const OFX::DrawArgs& args)
{
    OFX::BooleanParam* showOverlay = _effect->fetchBooleanParam(kParamShowOverlay);
    if (!showOverlay || !showOverlay->getValueAtTime(args.time))
        return false;

    OFX::BooleanParam* enableDRT = _effect->fetchBooleanParam(kParamEnableDRT);
    if (!enableDRT || !enableDRT->getValueAtTime(args.time))
        return false;

    // Fetch tonescale parameters to replicate the curve
    OFX::ChoiceParam* lookPresetP = _effect->fetchChoiceParam(kParamLookPreset);
    OFX::ChoiceParam* tsPresetP = _effect->fetchChoiceParam(kParamTonescalePreset);
    OFX::ChoiceParam* dispEncP = _effect->fetchChoiceParam(kParamDisplayEncoding);
    OFX::ChoiceParam* surroundP = _effect->fetchChoiceParam(kParamSurround);
    OFX::DoubleParam* peakLumP = _effect->fetchDoubleParam(kParamPeakLum);
    OFX::DoubleParam* greyLumP = _effect->fetchDoubleParam(kParamGreyLum);
    OFX::DoubleParam* hdrGreyP = _effect->fetchDoubleParam(kParamHDRGreyBoost);
    OFX::DoubleParam* hdrPurityP = _effect->fetchDoubleParam(kParamHDRPurity);

    double t = args.time;
    int lookVal = 0; lookPresetP->getValueAtTime(t, lookVal);
    int tsVal = 0; tsPresetP->getValueAtTime(t, tsVal);
    int dispVal = 0; dispEncP->getValueAtTime(t, dispVal);
    int surVal = 0; surroundP->getValueAtTime(t, surVal);
    float peakLum = static_cast<float>(peakLumP->getValueAtTime(t));
    float greyLum = static_cast<float>(greyLumP->getValueAtTime(t));
    float hdrGrey = static_cast<float>(hdrGreyP->getValueAtTime(t));
    float hdrPurity = static_cast<float>(hdrPurityP->getValueAtTime(t));

    // Resolve look and tonescale presets
    int lp = (lookVal >= 0 && lookVal < 8) ? lookVal : 0;
    const LookPresetData& look = kLookPresets[lp];
    float tn_con = look.tn_con, tn_sh = look.tn_sh, tn_toe = look.tn_toe, tn_off = look.tn_off;

    if (tsVal > 0 && tsVal < 14) {
        const TonescalePresetData& ts = kTonescalePresets[tsVal];
        tn_con = ts.tn_con; tn_sh = ts.tn_sh; tn_toe = ts.tn_toe; tn_off = ts.tn_off;
    }

    // Resolve display encoding for eotf/surround
    int de = (dispVal >= 0 && dispVal < 9) ? dispVal : 0;
    int eotf = kDisplayEncodings[de].eotf;

    // Compute tonescale constants
    PrimeraGPUParams tp;
    memset(&tp, 0, sizeof(tp));
    computeTonescaleConstants(tp, peakLum, hdrGrey, hdrPurity, greyLum,
                              tn_con, tn_sh, tn_toe, tn_off, eotf, surVal);

    // Overlay geometry — bottom-right corner, 200x200 pixels
    // Use pixel scale to convert to canonical coords
    double psx = args.pixelScale.x;
    double psy = args.pixelScale.y;
    double plotW = 220.0 * psx;
    double plotH = 220.0 * psy;
    double margin = 30.0 * psx;
    double marginY = 30.0 * psy;

    // Position at bottom-left of viewport
    double x0 = margin;
    double y0 = marginY;
    double x1 = x0 + plotW;
    double y1 = y0 + plotH;

    // Semi-transparent background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    glBegin(GL_QUADS);
    glVertex2d(x0, y0); glVertex2d(x1, y0);
    glVertex2d(x1, y1); glVertex2d(x0, y1);
    glEnd();

    // Grid lines (subtle)
    glColor4f(0.35f, 0.35f, 0.35f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 1; i < 4; ++i) {
        double gy = y0 + (y1 - y0) * i / 4.0;
        glVertex2d(x0, gy); glVertex2d(x1, gy);
        double gx = x0 + (x1 - x0) * i / 4.0;
        glVertex2d(gx, y0); glVertex2d(gx, y1);
    }
    glEnd();

    // Axes
    glColor4f(0.6f, 0.6f, 0.6f, 0.8f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x0, y0); glVertex2d(x1, y0);
    glVertex2d(x1, y1); glVertex2d(x0, y1);
    glEnd();

    // Draw tonescale curve
    // X-axis: scene-linear exposure (0 to ~16 stops above 18%)
    // Y-axis: display-referred output (0 to 1)
    const int nPts = 128;
    float maxExposure = powf(2.0f, 6.0f * tn_sh + 4.0f) * 2.0f; // a bit beyond shoulder

    glColor4f(0.9f, 0.65f, 0.2f, 1.0f); // warm orange curve
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= nPts; ++i) {
        float frac = static_cast<float>(i) / static_cast<float>(nPts);
        // Log-distributed X sampling for better shadow detail
        float sceneLin = maxExposure * powf(frac, 3.0f);
        float displayVal = evalTonescale(sceneLin, tp.ts_s, tp.ts_p, tp.ts_m2,
                                         tp.ts_x0, tp.tn_toe, tp.ts_dsc);
        displayVal = fminf(displayVal, 1.0f);

        double px = x0 + (x1 - x0) * frac;
        double py = y0 + (y1 - y0) * static_cast<double>(displayVal);
        glVertex2d(px, py);
    }
    glEnd();

    // Mark 18% grey on X axis
    float grey18_frac = powf(0.18f / maxExposure, 1.0f / 3.0f);
    if (grey18_frac > 0.0f && grey18_frac < 1.0f) {
        float greyOut = evalTonescale(0.18f, tp.ts_s, tp.ts_p, tp.ts_m2,
                                      tp.ts_x0, tp.tn_toe, tp.ts_dsc);
        greyOut = fminf(greyOut, 1.0f);

        double gx = x0 + (x1 - x0) * grey18_frac;
        double gy = y0 + (y1 - y0) * greyOut;

        // Vertical dashed line from axis to curve
        glColor4f(0.5f, 0.7f, 0.9f, 0.7f);
        glLineWidth(1.0f);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(2, 0xAAAA);
        glBegin(GL_LINES);
        glVertex2d(gx, y0); glVertex2d(gx, gy);
        glVertex2d(x0, gy); glVertex2d(gx, gy);
        glEnd();
        glDisable(GL_LINE_STIPPLE);

        // Dot at grey point
        glColor4f(0.5f, 0.7f, 0.9f, 1.0f);
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex2d(gx, gy);
        glEnd();
    }

    glDisable(GL_BLEND);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Factory
////////////////////////////////////////////////////////////////////////////////

using namespace OFX;

PrimeraPluginFactory::PrimeraPluginFactory()
    : OFX::PluginFactoryHelper<PrimeraPluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
{
}

void PrimeraPluginFactory::describe(OFX::ImageEffectDescriptor& p_Desc)
{
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
    p_Desc.setPluginGrouping(kPluginGrouping);
    p_Desc.setPluginDescription(kPluginDescription);

    p_Desc.addSupportedContext(eContextFilter);
    p_Desc.addSupportedContext(eContextGeneral);

    p_Desc.addSupportedBitDepth(eBitDepthFloat);

    p_Desc.setSingleInstance(false);
    p_Desc.setHostFrameThreading(false);
    p_Desc.setSupportsMultiResolution(kSupportsMultiResolution);
    p_Desc.setSupportsTiles(kSupportsTiles);
    p_Desc.setTemporalClipAccess(false);
    p_Desc.setRenderTwiceAlways(false);
    p_Desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);

    p_Desc.setSupportsMetalRender(true);

    p_Desc.setNoSpatialAwareness(true);

    p_Desc.setOverlayInteractDescriptor(new PrimeraOverlayDescriptor());
}

void PrimeraPluginFactory::describeInContext(OFX::ImageEffectDescriptor& p_Desc,
                                              OFX::ContextEnum /*p_Context*/)
{
    ClipDescriptor* srcClip = p_Desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    ClipDescriptor* dstClip = p_Desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // --- Page: Input ---
    PageParamDescriptor* inputPage = p_Desc.definePageParam("Input");

    ChoiceParamDescriptor* inputTF = p_Desc.defineChoiceParam(kParamInputTF);
    inputTF->setLabels("Input Transfer Function", "Input Transfer Function", "Input Transfer Function");
    inputTF->setHint("Log encoding of the source material");
    inputTF->appendOption("ARRI LogC3");
    inputTF->appendOption("ARRI LogC4");
    inputTF->appendOption("REDLog3G10");
    inputTF->appendOption("Sony S-Log3");
    inputTF->appendOption("ACEScct");
    inputTF->appendOption("Pines2");
    inputTF->appendOption("DaVinci Intermediate");
    inputTF->appendOption("Cineon");
    inputTF->setDefault(0);
    inputPage->addChild(*inputTF);

    ChoiceParamDescriptor* inputGamut = p_Desc.defineChoiceParam(kParamInputGamut);
    inputGamut->setLabels("Input Gamut", "Input Gamut", "Input Gamut");
    inputGamut->setHint("Color gamut of the source material");
    inputGamut->appendOption("XYZ");
    inputGamut->appendOption("ACES AP0");
    inputGamut->appendOption("ACES AP1");
    inputGamut->appendOption("P3-D65");
    inputGamut->appendOption("Rec 2020");
    inputGamut->appendOption("Rec 709");
    inputGamut->appendOption("ARRI Wide Gamut 3");
    inputGamut->appendOption("ARRI Wide Gamut 4");
    inputGamut->appendOption("RED Wide Gamut");
    inputGamut->appendOption("Sony SGamut3");
    inputGamut->appendOption("Sony SGamut3Cine");
    inputGamut->appendOption("Panasonic V-Gamut");
    inputGamut->appendOption("Filmlight E-Gamut");
    inputGamut->appendOption("Filmlight E-Gamut2");
    inputGamut->appendOption("DaVinci Wide Gamut");
    inputGamut->setDefault(6); // AWG3
    inputPage->addChild(*inputGamut);

    // --- Page: Grade ---
    PageParamDescriptor* gradePage = p_Desc.definePageParam("Grade");

    // Group: Exposure
    GroupParamDescriptor* expGroup = p_Desc.defineGroupParam("exposureGroup");
    expGroup->setLabels("Exposure", "Exposure", "Exposure");
    gradePage->addChild(*expGroup);

    DoubleParamDescriptor* exposure = p_Desc.defineDoubleParam(kParamExposure);
    exposure->setLabels("Exposure", "Exposure", "Exposure");
    exposure->setHint("Exposure adjustment in stops");
    exposure->setDefault(0.0);
    exposure->setRange(-6.0, 6.0);
    exposure->setDisplayRange(-4.0, 4.0);
    exposure->setIncrement(0.01);
    exposure->setDoubleType(eDoubleTypePlain);
    exposure->setParent(*expGroup);
    gradePage->addChild(*exposure);

    DoubleParamDescriptor* blackPt = p_Desc.defineDoubleParam(kParamBlackPt);
    blackPt->setLabels("Black Point", "Black Point", "Black Point");
    blackPt->setHint("Scene-linear black point offset with soft toe");
    blackPt->setDefault(0.0);
    blackPt->setRange(-0.05, 0.05);
    blackPt->setDisplayRange(-0.05, 0.05);
    blackPt->setIncrement(0.001);
    blackPt->setDoubleType(eDoubleTypePlain);
    blackPt->setParent(*expGroup);
    gradePage->addChild(*blackPt);

    // Group: Balance
    GroupParamDescriptor* balGroup = p_Desc.defineGroupParam("balanceGroup");
    balGroup->setLabels("Balance", "Balance", "Balance");
    gradePage->addChild(*balGroup);

    DoubleParamDescriptor* temp = p_Desc.defineDoubleParam(kParamTemp);
    temp->setLabels("Temp", "Temp", "Temp");
    temp->setHint("Color temperature shift (blue-yellow axis) in stops");
    temp->setDefault(0.0);
    temp->setRange(-1.0, 1.0);
    temp->setDisplayRange(-1.0, 1.0);
    temp->setIncrement(0.01);
    temp->setDoubleType(eDoubleTypePlain);
    temp->setParent(*balGroup);
    gradePage->addChild(*temp);

    DoubleParamDescriptor* tint = p_Desc.defineDoubleParam(kParamTint);
    tint->setLabels("Tint", "Tint", "Tint");
    tint->setHint("Tint shift (green-magenta axis) in stops");
    tint->setDefault(0.0);
    tint->setRange(-1.0, 1.0);
    tint->setDisplayRange(-1.0, 1.0);
    tint->setIncrement(0.01);
    tint->setDoubleType(eDoubleTypePlain);
    tint->setParent(*balGroup);
    gradePage->addChild(*tint);

    // Group: Tone
    GroupParamDescriptor* toneGroup = p_Desc.defineGroupParam("toneGroup");
    toneGroup->setLabels("Tone", "Tone", "Tone");
    gradePage->addChild(*toneGroup);

    DoubleParamDescriptor* contrast = p_Desc.defineDoubleParam(kParamContrast);
    contrast->setLabels("Contrast", "Contrast", "Contrast");
    contrast->setHint("Log-domain contrast (power curve around pivot)");
    contrast->setDefault(1.0);
    contrast->setRange(0.5, 2.0);
    contrast->setDisplayRange(0.5, 2.0);
    contrast->setIncrement(0.01);
    contrast->setDoubleType(eDoubleTypePlain);
    contrast->setParent(*toneGroup);
    gradePage->addChild(*contrast);

    DoubleParamDescriptor* pivot = p_Desc.defineDoubleParam(kParamPivot);
    pivot->setLabels("Pivot", "Pivot", "Pivot");
    pivot->setHint("Contrast pivot offset from mid-grey");
    pivot->setDefault(0.0);
    pivot->setRange(-0.2, 0.2);
    pivot->setDisplayRange(-0.2, 0.2);
    pivot->setIncrement(0.005);
    pivot->setDoubleType(eDoubleTypePlain);
    pivot->setParent(*toneGroup);
    gradePage->addChild(*pivot);

    DoubleParamDescriptor* shadows = p_Desc.defineDoubleParam(kParamShadows);
    shadows->setLabels("Shadows", "Shadows", "Shadows");
    shadows->setHint("Shadow fill (gain below mid-grey in log)");
    shadows->setDefault(0.0);
    shadows->setRange(-1.0, 1.0);
    shadows->setDisplayRange(-1.0, 1.0);
    shadows->setIncrement(0.01);
    shadows->setDoubleType(eDoubleTypePlain);
    shadows->setParent(*toneGroup);
    gradePage->addChild(*shadows);

    DoubleParamDescriptor* highlights = p_Desc.defineDoubleParam(kParamHighlights);
    highlights->setLabels("Highlights", "Highlights", "Highlights");
    highlights->setHint("Highlight gain (above mid-grey in log)");
    highlights->setDefault(0.0);
    highlights->setRange(-1.0, 1.0);
    highlights->setDisplayRange(-1.0, 1.0);
    highlights->setIncrement(0.01);
    highlights->setDoubleType(eDoubleTypePlain);
    highlights->setParent(*toneGroup);
    gradePage->addChild(*highlights);

    DoubleParamDescriptor* rollOff = p_Desc.defineDoubleParam(kParamRollOff);
    rollOff->setLabels("Roll Off", "Roll Off", "Roll Off");
    rollOff->setHint("Highlight soft clip / roll-off amount");
    rollOff->setDefault(0.0);
    rollOff->setRange(0.0, 2.0);
    rollOff->setDisplayRange(0.0, 2.0);
    rollOff->setIncrement(0.01);
    rollOff->setDoubleType(eDoubleTypePlain);
    rollOff->setParent(*toneGroup);
    gradePage->addChild(*rollOff);

    // Group: Saturation
    GroupParamDescriptor* satGroup = p_Desc.defineGroupParam("satGroup");
    satGroup->setLabels("Saturation", "Saturation", "Saturation");
    gradePage->addChild(*satGroup);

    DoubleParamDescriptor* saturation = p_Desc.defineDoubleParam(kParamSaturation);
    saturation->setLabels("Saturation", "Saturation", "Saturation");
    saturation->setHint("Saturation adjustment in log space");
    saturation->setDefault(0.0);
    saturation->setRange(-1.0, 1.0);
    saturation->setDisplayRange(-1.0, 1.0);
    saturation->setIncrement(0.01);
    saturation->setDoubleType(eDoubleTypePlain);
    saturation->setParent(*satGroup);
    gradePage->addChild(*saturation);

    BooleanParamDescriptor* preserveLum = p_Desc.defineBooleanParam(kParamPreserveLum);
    preserveLum->setLabels("Preserve Luma", "Preserve Luma", "Preserve Luma");
    preserveLum->setHint("Restore pre-saturation luminance after saturation change");
    preserveLum->setDefault(false);
    preserveLum->setParent(*satGroup);
    gradePage->addChild(*preserveLum);

    // Group: Per-Color
    GroupParamDescriptor* colorGroup = p_Desc.defineGroupParam("perColorGroup");
    colorGroup->setLabels("Per-Color", "Per-Color", "Per-Color");
    colorGroup->setOpen(false);
    gradePage->addChild(*colorGroup);

    struct { const char* name; const char* label; } colorParams[] = {
        {kParamRedHue, "Red Hue"},    {kParamRedDen, "Red Density"},
        {kParamYelHue, "Yellow Hue"}, {kParamYelDen, "Yellow Density"},
        {kParamGrnHue, "Green Hue"},  {kParamGrnDen, "Green Density"},
        {kParamCynHue, "Cyan Hue"},   {kParamCynDen, "Cyan Density"},
        {kParamBluHue, "Blue Hue"},   {kParamBluDen, "Blue Density"},
        {kParamMagHue, "Magenta Hue"},{kParamMagDen, "Magenta Density"},
    };
    for (int i = 0; i < 12; ++i)
    {
        DoubleParamDescriptor* cp = p_Desc.defineDoubleParam(colorParams[i].name);
        cp->setLabels(colorParams[i].label, colorParams[i].label, colorParams[i].label);
        cp->setDefault(0.0);
        cp->setRange(-1.0, 1.0);
        cp->setDisplayRange(-1.0, 1.0);
        cp->setIncrement(0.01);
        cp->setDoubleType(eDoubleTypePlain);
        cp->setParent(*colorGroup);
        gradePage->addChild(*cp);
    }

    // --- Page: OpenDRT ---
    // Ordered to match OpenDRT DCTL: display settings, then presets
    PageParamDescriptor* renderPage = p_Desc.definePageParam("OpenDRT");

    BooleanParamDescriptor* enableDRT = p_Desc.defineBooleanParam(kParamEnableDRT);
    enableDRT->setLabels("Enable OpenDRT", "Enable OpenDRT", "Enable OpenDRT");
    enableDRT->setHint("Enable OpenDRT display rendering transform");
    enableDRT->setDefault(true);
    renderPage->addChild(*enableDRT);

    DoubleParamDescriptor* peakLum = p_Desc.defineDoubleParam(kParamPeakLum);
    peakLum->setLabels("Display Peak Luminance", "Display Peak Luminance", "Display Peak Luminance");
    peakLum->setHint("Peak luminance of the display in nits");
    peakLum->setDefault(100.0);
    peakLum->setRange(100.0, 4000.0);
    peakLum->setDisplayRange(100.0, 2000.0);
    peakLum->setIncrement(10.0);
    peakLum->setDoubleType(eDoubleTypePlain);
    renderPage->addChild(*peakLum);

    DoubleParamDescriptor* hdrGrey = p_Desc.defineDoubleParam(kParamHDRGreyBoost);
    hdrGrey->setLabels("HDR Grey Boost", "HDR Grey Boost", "HDR Grey Boost");
    hdrGrey->setHint("Boost grey level for HDR displays");
    hdrGrey->setDefault(0.13);
    hdrGrey->setRange(0.0, 1.0);
    hdrGrey->setDisplayRange(0.0, 1.0);
    hdrGrey->setIncrement(0.01);
    hdrGrey->setDoubleType(eDoubleTypePlain);
    renderPage->addChild(*hdrGrey);

    DoubleParamDescriptor* hdrPurity = p_Desc.defineDoubleParam(kParamHDRPurity);
    hdrPurity->setLabels("HDR Purity", "HDR Purity", "HDR Purity");
    hdrPurity->setHint("Purity compression factor for HDR");
    hdrPurity->setDefault(0.5);
    hdrPurity->setRange(0.0, 1.0);
    hdrPurity->setDisplayRange(0.0, 1.0);
    hdrPurity->setIncrement(0.01);
    hdrPurity->setDoubleType(eDoubleTypePlain);
    renderPage->addChild(*hdrPurity);

    DoubleParamDescriptor* greyLum = p_Desc.defineDoubleParam(kParamGreyLum);
    greyLum->setLabels("Display Grey Luminance", "Display Grey Luminance", "Display Grey Luminance");
    greyLum->setHint("Grey luminance of the display in nits");
    greyLum->setDefault(10.0);
    greyLum->setRange(3.0, 25.0);
    greyLum->setDisplayRange(3.0, 25.0);
    greyLum->setIncrement(0.1);
    greyLum->setDoubleType(eDoubleTypePlain);
    renderPage->addChild(*greyLum);

    ChoiceParamDescriptor* surround = p_Desc.defineChoiceParam(kParamSurround);
    surround->setLabels("Surround", "Surround", "Surround");
    surround->setHint("Viewing surround luminance");
    surround->appendOption("Dark");
    surround->appendOption("Dim");
    surround->appendOption("Average");
    surround->setDefault(1); // Dim
    renderPage->addChild(*surround);

    // Look Preset
    ChoiceParamDescriptor* lookPreset = p_Desc.defineChoiceParam(kParamLookPreset);
    lookPreset->setLabels("Look Preset", "Look Preset", "Look Preset");
    lookPreset->setHint("OpenDRT look preset - sets the overall creative character");
    lookPreset->appendOption("Standard");
    lookPreset->appendOption("Arriba");
    lookPreset->appendOption("Sylvan");
    lookPreset->appendOption("Colorful");
    lookPreset->appendOption("Aery");
    lookPreset->appendOption("Dystopic");
    lookPreset->appendOption("Umbra");
    lookPreset->appendOption("Base");
    lookPreset->setDefault(0);
    renderPage->addChild(*lookPreset);

    // Tonescale Preset
    ChoiceParamDescriptor* tsPreset = p_Desc.defineChoiceParam(kParamTonescalePreset);
    tsPreset->setLabels("Tonescale Preset", "Tonescale Preset", "Tonescale Preset");
    tsPreset->setHint("Override the tonescale from the look preset");
    tsPreset->appendOption("Use Look Preset");
    tsPreset->appendOption("Low Contrast");
    tsPreset->appendOption("Medium Contrast");
    tsPreset->appendOption("High Contrast");
    tsPreset->appendOption("Arriba Tonescale");
    tsPreset->appendOption("Sylvan Tonescale");
    tsPreset->appendOption("Colorful Tonescale");
    tsPreset->appendOption("Aery Tonescale");
    tsPreset->appendOption("Dystopic Tonescale");
    tsPreset->appendOption("Umbra Tonescale");
    tsPreset->appendOption("ACES-1x");
    tsPreset->appendOption("ACES-20");
    tsPreset->appendOption("Marvelous Tonescape");
    tsPreset->appendOption("DaGrinchi Tonegroan");
    tsPreset->setDefault(0);
    renderPage->addChild(*tsPreset);

    // Creative Whitepoint
    ChoiceParamDescriptor* creativeWP = p_Desc.defineChoiceParam(kParamCreativeWP);
    creativeWP->setLabels("Creative Whitepoint", "Creative Whitepoint", "Creative Whitepoint");
    creativeWP->setHint("Creative whitepoint adaptation - overrides the look preset CWP");
    creativeWP->appendOption("Use Look Preset");
    creativeWP->appendOption("D93");
    creativeWP->appendOption("D75");
    creativeWP->appendOption("D65");
    creativeWP->appendOption("D60");
    creativeWP->appendOption("D55");
    creativeWP->appendOption("D50");
    creativeWP->setDefault(0);
    renderPage->addChild(*creativeWP);

    DoubleParamDescriptor* cwpRange = p_Desc.defineDoubleParam(kParamCWPRange);
    cwpRange->setLabels("CWP Range", "CWP Range", "CWP Range");
    cwpRange->setHint("Creative whitepoint blending range");
    cwpRange->setDefault(0.25);
    cwpRange->setRange(0.0, 1.0);
    cwpRange->setDisplayRange(0.0, 1.0);
    cwpRange->setIncrement(0.01);
    cwpRange->setDoubleType(eDoubleTypePlain);
    renderPage->addChild(*cwpRange);

    // Display Encoding Preset
    ChoiceParamDescriptor* dispEnc = p_Desc.defineChoiceParam(kParamDisplayEncoding);
    dispEnc->setLabels("Display Encoding", "Display Encoding", "Display Encoding");
    dispEnc->setHint("Combined display gamut and EOTF preset");
    dispEnc->appendOption("Rec 1886");
    dispEnc->appendOption("sRGB Display");
    dispEnc->appendOption("Display P3");
    dispEnc->appendOption("P3-D60 DCI");
    dispEnc->appendOption("P3-DCI");
    dispEnc->appendOption("DCI P3 XYZ");
    dispEnc->appendOption("Rec 2100 PQ");
    dispEnc->appendOption("Rec 2100 HLG");
    dispEnc->appendOption("Dolby PQ");
    dispEnc->setDefault(0);
    renderPage->addChild(*dispEnc);

    // --- Page: Diagnostics ---
    PageParamDescriptor* diagPage = p_Desc.definePageParam("Diagnostics");

    BooleanParamDescriptor* showChart = p_Desc.defineBooleanParam(kParamShowChart);
    showChart->setLabels("Show Chart", "Show Chart", "Show Chart");
    showChart->setHint("Display stop chart with TF name and mid-grey value");
    showChart->setDefault(false);
    diagPage->addChild(*showChart);

    BooleanParamDescriptor* showOverlay = p_Desc.defineBooleanParam(kParamShowOverlay);
    showOverlay->setLabels("Show Tonescale Overlay", "Show Tonescale Overlay", "Show Tonescale Overlay");
    showOverlay->setHint("Display tonescale curve overlay on the viewer");
    showOverlay->setDefault(false);
    diagPage->addChild(*showOverlay);

    // --- Page: Controls ---
    PageParamDescriptor* ctrlPage = p_Desc.definePageParam("Controls");

    BooleanParamDescriptor* bypass = p_Desc.defineBooleanParam(kParamBypass);
    bypass->setLabels("Bypass", "Bypass", "Bypass");
    bypass->setHint("Bypass all processing");
    bypass->setDefault(false);
    ctrlPage->addChild(*bypass);
}

ImageEffect* PrimeraPluginFactory::createInstance(OfxImageEffectHandle p_Handle,
                                                   ContextEnum /*p_Context*/)
{
    return new PrimeraPlugin(p_Handle);
}

void OFX::Plugin::getPluginIDs(PluginFactoryArray& p_FactoryArray)
{
    static PrimeraPluginFactory primeraPlugin;
    p_FactoryArray.push_back(&primeraPlugin);
}
