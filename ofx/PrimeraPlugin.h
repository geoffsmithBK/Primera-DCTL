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

#include "ofxsImageEffect.h"
#include "ofxsInteract.h"

class PrimeraPluginFactory : public OFX::PluginFactoryHelper<PrimeraPluginFactory> {
public:
    PrimeraPluginFactory();
    virtual void load() {}
    virtual void unload() {}
    virtual void describe(OFX::ImageEffectDescriptor& p_Desc);
    virtual void describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum p_Context);
    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle p_Handle, OFX::ContextEnum p_Context);
};

// Overlay interact for tonescale curve visualization
class PrimeraOverlayInteract : public OFX::OverlayInteract {
public:
    PrimeraOverlayInteract(OfxInteractHandle handle, OFX::ImageEffect* effect);
    virtual bool draw(const OFX::DrawArgs& args);
private:
    OFX::ImageEffect* _effect;
};

class PrimeraOverlayDescriptor
    : public OFX::DefaultEffectOverlayDescriptor<PrimeraOverlayDescriptor, PrimeraOverlayInteract>
{
};
