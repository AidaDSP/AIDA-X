/*
 * Aida-X Widgets
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "EventHandlers.hpp"
#include "ImageWidgets.hpp"

#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO

static constexpr const uint kSubWidgetsFontSize = 14;
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
        fontSize(kSubWidgetsFontSize * scaleFactor);
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
// vertical switch with labels on top and bottom, follows modgui style

class AidaPluginSwitch : public NanoSubWidget,
                         public ButtonEventHandler
{
    NanoTopLevelWidget* const parent;
    const ParameterEnumerationValues& enumValues;
    const bool isInverted;

public:
    static constexpr const uint kSwitchWidth = 25;
    static constexpr const uint kSwitchHeight = 60;
    static constexpr const uint kSwitchPadding = 8;
    static constexpr const uint kSwitchRadius = kSwitchWidth / 2;
    static constexpr const uint kFullWidth = kSwitchWidth + kSwitchPadding * 2;

    AidaPluginSwitch(NanoTopLevelWidget* const p, ButtonEventHandler::Callback* const cb, const Parameters paramId)
        : NanoSubWidget(p),
          ButtonEventHandler(this),
          parent(p),
          enumValues(kParameters[paramId].enumValues),
          isInverted(paramId == kParameterGLOBALBYPASS)
    {
        const double scaleFactor = p->getScaleFactor();
        setSize(kFullWidth * scaleFactor, kSubWidgetsFullHeight * scaleFactor);

        const Parameter& param(kParameters[paramId]);
        setId(paramId);
        setCheckable(true);
        setChecked(isInverted ? param.ranges.def < 0.5f : param.ranges.def > 0.5f, false);
        setCallback(cb);
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = parent->getScaleFactor();
        const double switchWidth = kSwitchWidth * scaleFactor;
        const double switchHeight = kSwitchHeight * scaleFactor;
        const double switchPadding = kSwitchPadding * scaleFactor;
        const double switchRadius = kSwitchRadius * scaleFactor;

        const bool checked = isInverted ? !isChecked() : isChecked();

        beginPath();
        rect(0, 0, width, height);

        fontSize(kSubWidgetsFontSize * scaleFactor);
        fillColor(Color(1.f, 1.f, 1.f));

        textAlign(ALIGN_CENTER | ALIGN_TOP);
        text(width/2, 0, enumValues.values[0].label, nullptr);

        textAlign(ALIGN_CENTER | ALIGN_BASELINE);
        text(width/2, height, enumValues.values[1].label, nullptr);

        beginPath();
        roundedRect(switchPadding, height/2 - switchHeight/2, switchWidth, switchHeight, switchRadius);
        fillColor(checked ? Color(84, 84, 84) : Color(129, 247, 0));
        fill();

        beginPath();
        circle(width / 2,
               checked ? height/2 + switchHeight/2 - switchWidth/2
                       : height/2 - switchHeight/2 + switchWidth/2,
               10 * scaleFactor);
        fillColor(checked ? Color(218, 214, 203) : Color(24, 112, 4));
        fill();
    }

    bool onMouse(const MouseEvent& event) override
    {
        return ButtonEventHandler::mouseEvent(event);
    }

    bool onMotion(const MotionEvent& event) override
    {
        return ButtonEventHandler::motionEvent(event);
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
// horizontal switch with label on its right side

class AidaHorizontalSwitch : public NanoSubWidget,
                             public ButtonEventHandler
{
    NanoTopLevelWidget* const parent;
    const String label;

public:
    static constexpr const uint kSwitchWidth = 60;
    static constexpr const uint kSwitchHeight = 25;
    static constexpr const uint kSwitchPadding = 2;
    static constexpr const uint kSwitchRadius = kSwitchHeight / 2;
    static constexpr const uint kFullWidth = kSwitchWidth + kSwitchPadding * 2;

    AidaHorizontalSwitch(NanoTopLevelWidget* const p, const char* const lbl)
        : NanoSubWidget(p),
          ButtonEventHandler(this),
          parent(p),
          label(lbl)
    {
        const double scaleFactor = p->getScaleFactor();
        setSize((kSwitchWidth + kSwitchPadding) * scaleFactor, kSwitchHeight * scaleFactor);

        setCheckable(true);
        setChecked(false, false);
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();

        const double scaleFactor = parent->getScaleFactor();
        const double switchWidth = kSwitchWidth * scaleFactor;
        const double switchHeight = kSwitchHeight * scaleFactor;
        const double switchPadding = kSwitchPadding * scaleFactor;
        const double switchRadius = kSwitchRadius * scaleFactor;

        const bool checked = isChecked();

        beginPath();
        roundedRect(0, 0, switchWidth, switchHeight, switchRadius);
        fillColor(checked ? Color(129, 247, 0) : Color(84, 84, 84));
        fill();

        beginPath();
        circle(checked ? switchWidth/2 - switchHeight/2 - switchPadding
                       : switchWidth/2 + switchHeight/2 + switchPadding,
               switchHeight / 2,
               10 * scaleFactor);
        fillColor(checked ? Color(24, 112, 4) : Color(218, 214, 203));
        fill();

        fillColor(Color(0.f, 0.f, 0.f));
        fontSize((kSubWidgetsFontSize + 2) * scaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        text(switchWidth + switchPadding * 4, switchHeight / 2, label, nullptr);
    }

    bool onMouse(const MouseEvent& event) override
    {
        return ButtonEventHandler::mouseEvent(event);
    }

    bool onMotion(const MotionEvent& event) override
    {
        return ButtonEventHandler::motionEvent(event);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class AidaPushButton : public NanoSubWidget,
                       public ButtonEventHandler
{
    NanoTopLevelWidget* const parent;
    String label;

public:
    static constexpr const uint kButtonWidth = 140;
    static constexpr const uint kButtonHeight = 25;
    static constexpr const uint kButtonRadius = 4;

    AidaPushButton(NanoTopLevelWidget* const p)
        : NanoSubWidget(p),
          ButtonEventHandler(this),
          parent(p),
          label("R")
    {
        const double scaleFactor = p->getScaleFactor();
        setSize(kButtonWidth * scaleFactor, kButtonHeight * scaleFactor);
    }

    void setLabel(const char* const lbl)
    {
        label = lbl;
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = parent->getScaleFactor();
        const double buttonRadius = kButtonRadius * scaleFactor;

        beginPath();
        roundedRect(0, 0, width, height, buttonRadius);

        switch (getState())
        {
        case kButtonStateDefault:
            fillColor(Color(0, 0, 0));
            strokeColor(Color(90, 90, 90));
            break;
        case kButtonStateHover:
            fillColor(Color(50, 50, 50));
            strokeColor(Color(90, 90, 90));
            break;
        case kButtonStateActive:
            fillColor(Color(120, 120, 120));
            strokeColor(Color(90, 90, 90));
            break;
        case kButtonStateActiveHover:
            fillColor(Color(120, 120, 120));
            strokeColor(Color(120, 120, 120));
            break;
        }

        fill();
        stroke();

        fillColor(Color(1.f, 1.f, 1.f));
        fontSize(16 * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        text(width/2, height/2, label, nullptr);
    }

    bool onMouse(const MouseEvent& event) override
    {
        return ButtonEventHandler::mouseEvent(event);
    }

    bool onMotion(const MotionEvent& event) override
    {
        return ButtonEventHandler::motionEvent(event);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class AidaFileButton : public NanoSubWidget,
                       public ButtonEventHandler
{
    NanoTopLevelWidget* const parent;
    const String label;
    String filename;

public:
    static constexpr const uint kButtonWidth = 140;
    static constexpr const uint kButtonHeight = 25;
    static constexpr const uint kButtonRadius = 4;

    AidaFileButton(NanoTopLevelWidget* const p, const char* const lbl)
        : NanoSubWidget(p),
          ButtonEventHandler(this),
          parent(p),
          label(lbl),
          filename("file.name")
    {
        const double scaleFactor = p->getScaleFactor();
        setSize(kButtonWidth * scaleFactor, kButtonHeight * scaleFactor);
    }

    void setFilename(const char* const fname)
    {
        if (const char* const bname = std::strrchr(fname, DISTRHO_OS_SEP))
            filename = bname + 1;
        else
            filename = fname;
        repaint();
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = parent->getScaleFactor();
        const double buttonRadius = kButtonRadius * scaleFactor;

        beginPath();
        roundedRect(0, 0, width, height, buttonRadius);

//         switch (getState())
//         {
//         case kButtonStateDefault:
//             fillColor(Color(0, 0, 0));
//             strokeColor(Color(90, 90, 90));
//             break;
//         case kButtonStateHover:
//             fillColor(Color(50, 50, 50));
//             strokeColor(Color(90, 90, 90));
//             break;
//         case kButtonStateActive:
//             fillColor(Color(120, 120, 120));
//             strokeColor(Color(90, 90, 90));
//             break;
//         case kButtonStateActiveHover:
            fillColor(Color(0x59, 0x75, 0x29));
            strokeColor(Color(0x59, 0x75, 0x29).minus(0x10));
//             break;
//         }

        fill();
        stroke();

        fillColor(Color(1.f, 1.f, 1.f));
        fontSize(16 * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        text(width/2, height/2, getState() & kButtonStateHover ? label : filename, nullptr);
    }

    bool onMouse(const MouseEvent& event) override
    {
        return ButtonEventHandler::mouseEvent(event);
    }

    bool onMotion(const MotionEvent& event) override
    {
        return ButtonEventHandler::motionEvent(event);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class AidaFileGroup : public NanoSubWidget
{
    NanoTopLevelWidget* const parent;

    ScopedPointer<AidaHorizontalSwitch> toggle;
    ScopedPointer<AidaFileButton> button;

public:
    static constexpr const uint kMargin = 6;
    static constexpr const uint kInitialWidth = 200;
    static constexpr const uint kInitialHeight = 80;

    AidaFileGroup(NanoTopLevelWidget* const p, ButtonEventHandler::Callback* const cb,
                  const uint switchId,
                  const char* const switchLabel,
                  const uint buttonId,
                  const char* const buttonLabel)
        : NanoSubWidget(p),
          parent(p)
    {
        toggle = new AidaHorizontalSwitch(p, switchLabel);
        toggle->setCallback(cb);
        toggle->setId(switchId);
        // note: checked by default
        toggle->setChecked(true, false);

        button = new AidaFileButton(p, buttonLabel);
        button->setCallback(cb);
        button->setId(buttonId);

        const double scaleFactor = p->getScaleFactor();
        const double margin = kMargin * scaleFactor;
        setSize(kInitialWidth * scaleFactor, toggle->getHeight() + button->getHeight() + margin * 4);
    }

    void setChecked(const bool checked)
    {
        toggle->setChecked(checked, false);
    }

    void setFilename(const char* const filename)
    {
        button->setFilename(filename);
    }

protected:
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = parent->getScaleFactor();

        beginPath();
        roundedRect(0, 0, width, height, 8 * scaleFactor);
        fillColor(Color(0,0,0,0.1f));
        fill();
    }

    void onPositionChanged(const PositionChangedEvent& event) override
    {
        NanoSubWidget::onPositionChanged(event);

        const double scaleFactor = parent->getScaleFactor();
        const double margin = kMargin * scaleFactor;
        const double widgetX = event.pos.getX() + margin;
        const double widgetY1 = event.pos.getY() + margin;

        toggle->setAbsolutePos(widgetX, widgetY1);
        button->setAbsolutePos(widgetX, widgetY1 + toggle->getHeight() + margin);
    }

    void onResize(const ResizeEvent& event) override
    {
        NanoSubWidget::onResize(event);

        const double scaleFactor = parent->getScaleFactor();
        const double widgetWidth = event.size.getWidth() - (kMargin * 2) * scaleFactor;

        toggle->setWidth(widgetWidth);
        button->setWidth(widgetWidth);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
