/*
 * Aida-X Widgets
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "Artwork.hpp"
#include "EventHandlers.hpp"
#include "ImageWidgets.hpp"

#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO

static constexpr const uint kSubWidgetsFullHeight = 100;

// --------------------------------------------------------------------------------------------------------------------

class AidaKnob : public NanoSubWidget,
                 public KnobEventHandler
{
    NanoTopLevelWidget* const parent;
    const NanoImage& knobImage;
    const NanoImage& scaleImage;
    const char* label;

public:
    static constexpr const uint kKnobSize = 55;
    static constexpr const uint kKnobMargin = 17;
    static constexpr const uint kScaleSize = 90;

    AidaKnob(NanoTopLevelWidget* const p, KnobEventHandler::Callback* const cb,
             const NanoImage& knobImg, const NanoImage& scaleImg, const Parameters paramId)
        : NanoSubWidget(p),
          KnobEventHandler(this),
          parent(p),
          knobImage(knobImg),
          scaleImage(scaleImg)
    {
        const double scaleFactor = p->getScaleFactor();
        setSize(kScaleSize * scaleFactor, kSubWidgetsFullHeight * scaleFactor);

        const Parameter& param(kParameters[paramId]);
        setId(paramId);
        setRange(param.ranges.min, param.ranges.max);
        setDefault(param.ranges.def);
        setValue(param.ranges.def, false);
        setCallback(cb);
        label = param.name;
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = parent->getScaleFactor();
        const double scaleSize = kScaleSize * scaleFactor;
        const double knobSize = kKnobSize * scaleFactor;
        const double knobHalfSize = knobSize / 2;
        const double knobMargin = kKnobMargin * scaleFactor;

        beginPath();
        rect(0, 0, scaleSize, scaleSize);
        fillPaint(imagePattern(0, 0, scaleSize, scaleSize, 0.f, scaleImage, 1.f));
        fill();

        strokeColor(Color(1.f, 1.f, 1.f));
        fontSize(13.f * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_BASELINE);
        text(width/2, height, label, nullptr);

        const Paint knobImgPat = imagePattern(-knobHalfSize, -knobHalfSize, knobSize, knobSize, 0.f, knobImage, 1.f);

        save();
        translate(knobMargin + knobHalfSize, knobMargin + knobHalfSize);
        rotate(degToRad(270.f * (getNormalizedValue() - 0.5f)));

        beginPath();
        rect(-knobHalfSize, -knobHalfSize, knobSize, knobSize);
        fillPaint(knobImgPat);
        fill();

        restore();
    }

    bool onMouse(const MouseEvent& event) override
    {
        return KnobEventHandler::mouseEvent(event);
    }

    bool onMotion(const MotionEvent& event) override
    {
        return KnobEventHandler::motionEvent(event);
    }

    bool onScroll(const ScrollEvent& event) override
    {
        return KnobEventHandler::scrollEvent(event);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class AidaSplitter : public NanoSubWidget
{
    NanoTopLevelWidget* const parent;

public:
    static constexpr const uint kLineWidth = 4;
    static constexpr const uint kLineHeight = 80;

    AidaSplitter(NanoTopLevelWidget* const p)
        : NanoSubWidget(p),
          parent(p)
    {
        const double scaleFactor = p->getScaleFactor();
        setSize(kLineWidth * scaleFactor, kSubWidgetsFullHeight * scaleFactor);
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = parent->getScaleFactor();
        const double lineHeight = kLineHeight * scaleFactor;

        beginPath();
        moveTo(width/2, height/2 - lineHeight/2);
        lineTo(width/2, height/2 + lineHeight/2);
        lineCap(ROUND);
        strokeColor(Color(97, 97, 97, 0.484f));
        strokeWidth(width);
        stroke();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
