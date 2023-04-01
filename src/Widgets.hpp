/*
 * Aida-X Widgets
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "EventHandlers.hpp"
#include "ImageWidgets.hpp"
#include "NanoVG.hpp"

#include "DistrhoPluginInfo.h"

START_NAMESPACE_DISTRHO

static constexpr const uint kSubWidgetsFontSize = 14;
static constexpr const uint kSubWidgetsFullHeight = 90;

// --------------------------------------------------------------------------------------------------------------------
// good old knob with tick markers, rotating image and on-actived value display, follows modgui style

class AidaKnob : public NanoSubWidget,
                 public KnobEventHandler
{
    NanoTopLevelWidget* const parent;
    const NanoImage& knobImage;
    const NanoImage& scaleImage;
    const char* label;
    const char* unit;

public:
    static constexpr const uint kScaleSize = 80;
    static constexpr const uint kKnobSize = 55;
    static constexpr const uint kKnobMargin = (kScaleSize - kKnobSize) / 2;

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
        unit = param.unit;
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
        const double wfontSize = kSubWidgetsFontSize * scaleFactor;

        beginPath();
        rect(0, 0, scaleSize, scaleSize);
        fillPaint(imagePattern(0, 0, scaleSize, scaleSize, 0.f, scaleImage, 1.f));
        fill();

        fillColor(Color(1.f, 1.f, 1.f));
        fontSize(wfontSize);
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

        if (getState() & kKnobStateDragging)
        {
            const double padding = 4 * scaleFactor;
            beginPath();
            roundedRect(padding, 0,
                        scaleSize - padding,
                        wfontSize + padding * 2,
                        2 * scaleFactor);
            fillColor(Color(0,0,0,0.5f));
            fill();

            char textBuf[24];
            std::snprintf(textBuf, sizeof(textBuf)-1, "%.2f %s", getValue(), unit);
            textBuf[sizeof(textBuf)-1] = '\0';

            fillColor(Color(1.f, 1.f, 1.f));
            textAlign(ALIGN_CENTER | ALIGN_TOP);
            text(width/2, padding, textBuf, nullptr);
        }
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
// simple vertical splitter

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
// simple push button

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
          parent(p)
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
            fillColor(Color(0x15,0x14,0x13));
            strokeColor(Color(0x15,0x14,0x13,0.8f));
            break;
        case kButtonStateHover:
            fillColor(Color(0x15,0x14,0x13,0.8f));
            strokeColor(Color(0x15,0x14,0x13,0.8f));
            break;
        case kButtonStateActive:
        case kButtonStateActiveHover:
            fillColor(Color(0x15,0x14,0x13,0.4f));
            strokeColor(Color(0x15,0x14,0x13,0.4f));
            break;
        }

        fill();
        stroke();

        if (label.isEmpty())
            return;

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
// filelist selector

class AidaFileList : public NanoSubWidget,
                     public ButtonEventHandler::Callback
{
    static constexpr const uint kCommonRadius = 10;
    static constexpr const uint kSingleHeight = 24;
    static constexpr const uint kSingleMargin = 8;
    static constexpr const uint kInitialWidth = 100;

    struct AidaFileButton : NanoSubWidget,
                            ButtonEventHandler
    {
        NanoTopLevelWidget* const parent;
        String label;

        enum Position {
            Top,
            Middle,
            Bottom
        } position;

        AidaFileButton(NanoTopLevelWidget* const p, const Position pos)
            : NanoSubWidget(p),
              ButtonEventHandler(this),
              parent(p),
              position(pos)
        {
            const double scaleFactor = p->getScaleFactor();
            setSize(kInitialWidth * scaleFactor, kSingleHeight * scaleFactor);
        }

        void onNanoDisplay() override
        {
            if (label.isEmpty())
                return;

            const uint width = getWidth();
            const uint height = getHeight();

            const double scaleFactor = parent->getScaleFactor();
            const double wfontSize = kSubWidgetsFontSize * scaleFactor;
            const double margin = kSingleMargin * scaleFactor;
            const double radius = kCommonRadius * scaleFactor;

            uint state = getState();
            if (isChecked())
                state |= kButtonStateActive;

            switch (state)
            {
            case kButtonStateDefault:
                fillColor(Color(0x74,0x92,0x32));
                break;
            case kButtonStateHover:
                fillColor(Color(0x56,0x73,0x28));
                break;
            case kButtonStateActive:
                fillColor(Color(0x15,0x2c,0x0d));
                break;
            case kButtonStateActiveHover:
                Color c(0x56,0x73,0x28);
                c.interpolate(Color(0x15,0x2c,0x0d), 0.5f);
                fillColor(c);
                break;
            }

            switch (position)
            {
            case Middle:
                beginPath();
                rect(0, 0, width, height);
                fill();
                break;
            case Top:
            case Bottom:
                beginPath();
                roundedRect(0, 0, width, height, radius);
                fill();
                beginPath();
                if (position == Top)
                    rect(0, height/2, width, height);
                else
                    rect(0, 0, width, height/2);
                fill();
                break;
            }

            fillColor(Color(1.f, 1.f, 1.f));
            fontSize(wfontSize);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            text(margin, height / 2, label, nullptr);
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

public:
    NanoTopLevelWidget* const parent;
    ScopedPointer<AidaFileButton> buttons[3];
    ButtonEventHandler::Callback* const callback;

    AidaFileList(NanoTopLevelWidget* const p, ButtonEventHandler::Callback* const cb, const uint buttonStartId)
        : NanoSubWidget(p),
          parent(p),
          callback(cb)
    {
        buttons[0] = new AidaFileButton(p, AidaFileButton::Top);
        buttons[0]->setCallback(this);
        buttons[0]->setId(buttonStartId);

        buttons[1] = new AidaFileButton(p, AidaFileButton::Middle);
        buttons[1]->setCallback(this);
        buttons[1]->setId(buttonStartId + 1);

        buttons[2] = new AidaFileButton(p, AidaFileButton::Bottom);
        buttons[2]->setCallback(this);
        buttons[2]->setId(buttonStartId + 2);

        const double scaleFactor = p->getScaleFactor();
        setSize(kInitialWidth * scaleFactor, kSingleHeight * 3 * scaleFactor);
    }

    void setFilename(const int id, const char* const filename, const bool setAsActive = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(id >= 0 && id < 3,);

        if (const char* const bname = std::strrchr(filename, DISTRHO_OS_SEP))
            buttons[id]->label = bname + 1;
        else
            buttons[id]->label = filename;

        if (setAsActive)
            buttons[id]->setChecked(true, false);

        buttons[id]->repaint();
    }

protected:
    void onNanoDisplay() override
    {
        beginPath();
        roundedRect(0, 0, getWidth(), getHeight(), kCommonRadius * parent->getScaleFactor());
        fillColor(Color(0.f, 0.f, 0.f));
        fill();
    }

    void buttonClicked(SubWidget* const widget, const int button) override
    {
        if (button != kMouseButtonLeft)
            return;

        /**/ if (buttons[0] == widget)
        {
            buttons[1]->setChecked(false, false);
            buttons[2]->setChecked(false, false);
            buttons[0]->setChecked(true, true);
        }
        else if (buttons[1] == widget)
        {
            buttons[0]->setChecked(false, false);
            buttons[2]->setChecked(false, false);
            buttons[1]->setChecked(true, true);
        }
        else if (buttons[2] == widget)
        {
            buttons[0]->setChecked(false, false);
            buttons[1]->setChecked(false, false);
            buttons[2]->setChecked(true, true);
        }

        callback->buttonClicked(widget, button);
    }

    void onPositionChanged(const PositionChangedEvent& event) override
    {
        NanoSubWidget::onPositionChanged(event);

        const double scaleFactor = parent->getScaleFactor();
        const double widgetX = event.pos.getX();
        const double widgetY = event.pos.getY();
        const double singleHeight = kSingleHeight * scaleFactor;

        buttons[0]->setAbsolutePos(widgetX, widgetY);
        buttons[1]->setAbsolutePos(widgetX, widgetY + singleHeight);
        buttons[2]->setAbsolutePos(widgetX, widgetY + singleHeight * 2);
    }

    void onResize(const ResizeEvent& event) override
    {
        NanoSubWidget::onResize(event);

        const uint width = event.size.getWidth();
        buttons[0]->setWidth(width);
        buttons[1]->setWidth(width);
        buttons[2]->setWidth(width);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// switch and filename as button combo

class AidaFilenameButton : public NanoSubWidget
{
    static constexpr const uint kCommonHeight = 32;
    static constexpr const uint kCommonRadius = 10;
    static constexpr const uint kButtonMargin = 8;
    static constexpr const uint kIconSize     = 20;
    static constexpr const uint kInitialWidth = 100;

    struct AidaFileSwitch : NanoSubWidget,
                            ButtonEventHandler
    {
        NanoTopLevelWidget* const parent;
        const NanoImage& imageOff;
        const NanoImage& imageOn;

        AidaFileSwitch(NanoTopLevelWidget* const p, const NanoImage& imgOff, const NanoImage& imgOn)
            : NanoSubWidget(p),
              ButtonEventHandler(this),
              parent(p),
              imageOff(imgOff),
              imageOn(imgOn)
        {
            const double scaleFactor = p->getScaleFactor();
            setSize((kCommonHeight + kButtonMargin) * scaleFactor, kCommonHeight * scaleFactor);

            setCheckable(true);
            setChecked(true, false);
        }

        bool isHover() const noexcept
        {
            return getState() & kButtonStateHover;
        }

        void onNanoDisplay() override
        {
            const uint width = getWidth();
            const uint height = getHeight();

            const double scaleFactor = parent->getScaleFactor();
            const double iconSize = kIconSize * scaleFactor;
            const double switchRadius = kCommonRadius * scaleFactor;

            uint state = getState();
            if (isChecked())
                state |= kButtonStateActive;

            switch (state)
            {
            case kButtonStateDefault:
                fillColor(Color(0x74,0x92,0x32));
                break;
            case kButtonStateHover:
                fillColor(Color(0x74,0x92,0x32));
                break;
            case kButtonStateActive:
                fillColor(Color(0x15,0x2c,0x0d));
                break;
            case kButtonStateActiveHover:
                fillColor(Color(0x15,0x2c,0x0d));
                break;
            }

            beginPath();
            roundedRect(0, 0, width, height, switchRadius);
            fill();

            beginPath();
            rect(height/2, 0, width-height/2, height);
            fill();

            save();
            translate(width/2 - iconSize/2, height/2 - iconSize/2);
            beginPath();
            rect(0, 0, iconSize, iconSize);
            fillPaint(imagePattern(0, 0, iconSize, iconSize, 0.f, isChecked() ? imageOn : imageOff, 1.f));
            fill();
            restore();
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

    struct AidaFileButton : NanoSubWidget,
                            ButtonEventHandler
    {
        NanoTopLevelWidget* const parent;

        struct {
            String enable;
            String disable;
            String load;
        } labels;

        String filename;
        AidaFileSwitch* hoverButton = nullptr;

        AidaFileButton(NanoTopLevelWidget* const p, const String& label)
            : NanoSubWidget(p),
              ButtonEventHandler(this),
              parent(p)
        {
            const double scaleFactor = p->getScaleFactor();
            setSize(kInitialWidth * scaleFactor, kCommonHeight * scaleFactor);

            labels.enable = "Click to enable " + label;
            labels.disable = "Click to disable " + label;
            labels.load = "Load " + label + "...";
        }

        void onNanoDisplay() override
        {
            const uint width = getWidth();
            const uint height = getHeight();

            const double scaleFactor = parent->getScaleFactor();
            const double buttonMargin = (kCommonHeight + kButtonMargin * 2) * scaleFactor;
            const double buttonRadius = kCommonRadius * scaleFactor;

            beginPath();
            roundedRect(0, 0, width, height, buttonRadius);

            switch (getState())
            {
            case kButtonStateDefault:
                fillColor(Color(0x56,0x73,0x28));
                break;
            case kButtonStateHover:
                fillColor(Color(0x74,0x92,0x32));
                break;
            case kButtonStateActive:
                fillColor(Color(0x15,0x2c,0x0d));
                break;
            case kButtonStateActiveHover:
                fillColor(Color(0x15,0x2c,0x0d));
                break;
            }

            fill();

            fillColor(Color(1.f, 1.f, 1.f));
            fontSize(16 * scaleFactor);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            save();
            scissor(buttonMargin, 0, width - buttonMargin, height/2 + 16 * scaleFactor / 2);
            textBox(buttonMargin, height/2, width - buttonMargin,
                    hoverButton->isHover() ? hoverButton->isChecked() ? labels.disable : labels.enable
                                           : getState() & kButtonStateHover ? labels.load : filename,
                    nullptr);
            restore();
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

    ScopedPointer<AidaFileButton> button;
    ScopedPointer<AidaFileSwitch> toggle;

public:
    AidaFilenameButton(NanoTopLevelWidget* const p, ButtonEventHandler::Callback* const cb,
                       const uint switchId,
                       const uint buttonId,
                       const char* const buttonLabel,
                       const NanoImage& imgOff,
                       const NanoImage& imgOn)
        : NanoSubWidget(p)
    {
        button = new AidaFileButton(p, String(buttonLabel));
        button->setCallback(cb);
        button->setId(buttonId);

        toggle = new AidaFileSwitch(p, imgOff, imgOn);
        toggle->setCallback(cb);
        toggle->setId(switchId);

        button->hoverButton = toggle;

        const double scaleFactor = p->getScaleFactor();
        setSize(kInitialWidth * scaleFactor, kCommonHeight * scaleFactor);
    }

    void setChecked(const bool checked)
    {
        toggle->setChecked(checked, false);
    }

    void setFilename(const char* const fname)
    {
        if (const char* const bname = std::strrchr(fname, DISTRHO_OS_SEP))
            button->filename = bname + 1;
        else
            button->filename = fname;
        button->repaint();
    }

protected:
    void onNanoDisplay() override {}

    void onPositionChanged(const PositionChangedEvent& event) override
    {
        NanoSubWidget::onPositionChanged(event);

        const double widgetX = event.pos.getX();
        const double widgetY = event.pos.getY();
        toggle->setAbsolutePos(widgetX, widgetY);
        button->setAbsolutePos(widgetX, widgetY);
    }

    void onResize(const ResizeEvent& event) override
    {
        NanoSubWidget::onResize(event);

        button->setWidth(event.size.getWidth());
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
