/*
 * aidadsp-loader
 * Copyright (C) 2022-2023 Massimo Pennazio <maxipenna@libero.it>
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "DistrhoUI.hpp"

#include "Layout.hpp"
#include "Widgets.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class AidaDSPLoaderUI : public UI,
                        public KnobEventHandler::Callback
{
    float parameters[kNumParameters];

    struct {
        NanoImage background;
        NanoImage knob;
        NanoImage scale;
    } images;

    struct {
        ScopedPointer<AidaKnob> pregain;
        ScopedPointer<AidaKnob> bass;
        ScopedPointer<AidaKnob> middle;
        ScopedPointer<AidaKnob> treble;
        ScopedPointer<AidaKnob> depth;
        ScopedPointer<AidaKnob> presence;
        ScopedPointer<AidaKnob> master;
    } knobs;

    struct {
        ScopedPointer<AidaSplitter> s1, s2, s3;
    } splitters;

    HorizontalLayout subwidgetsLayout;

public:
    /* constructor */
    AidaDSPLoaderUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        // Initialize parameters to their defaults
        for (uint i=0; i<kNumParameters; ++i)
            parameters[i] = kParameters[i].ranges.def;

        // Load resources
        using namespace Artwork;
        images.background = createImageFromMemory(backgroundData, backgroundDataSize, IMAGE_REPEAT_X|IMAGE_REPEAT_Y);
        images.knob = createImageFromMemory(knobData, knobDataSize, IMAGE_GENERATE_MIPMAPS);
        images.scale = createImageFromMemory(scaleData, scaleDataSize, IMAGE_GENERATE_MIPMAPS);
        fontFaceId(createFontFromMemory("conthrax", conthrax_sbData, conthrax_sbDataSize, false));

        // Create subwidgets
        knobs.pregain = new AidaKnob(this, this, images.knob, images.scale, kParameterPREGAIN);
        knobs.bass = new AidaKnob(this, this, images.knob, images.scale, kParameterBASSGAIN);
        knobs.middle = new AidaKnob(this, this, images.knob, images.scale, kParameterMIDGAIN);
        knobs.treble = new AidaKnob(this, this, images.knob, images.scale, kParameterTREBLEGAIN);
        knobs.depth = new AidaKnob(this, this, images.knob, images.scale, kParameterDEPTH);
        knobs.presence = new AidaKnob(this, this, images.knob, images.scale, kParameterPRESENCE);
        knobs.master = new AidaKnob(this, this, images.knob, images.scale, kParameterMASTER);

        splitters.s1 = new AidaSplitter(this);
        splitters.s2 = new AidaSplitter(this);
        splitters.s3 = new AidaSplitter(this);

        // Setup subwidgets layout
        subwidgetsLayout.widgets.push_back({ knobs.pregain, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s1, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.bass, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.middle, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.treble, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s2, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.depth, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.presence, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s3, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.master, Fixed });
        repositionWidgets();

        // adjust size
        const double scaleFactor = getScaleFactor();
        setGeometryConstraints((kPedalWidth + kPedalMargin/2) * scaleFactor,
                               (kPedalHeight + kPedalMargin/2) * scaleFactor);

        if (scaleFactor != 1.0)
            setSize(DISTRHO_UI_DEFAULT_WIDTH*scaleFactor, DISTRHO_UI_DEFAULT_HEIGHT*scaleFactor);
    }

protected:
   /* -----------------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(const uint32_t index, const float value) override
    {
        parameters[index] = value;

        switch (static_cast<Parameters>(index))
        {
        case kParameterINLPF:
            // TODO
            break;
        case kParameterPREGAIN:
            knobs.pregain->setValue(value, false);
            break;
        case kParameterNETBYPASS:
            // TODO
            break;
        case kParameterEQBYPASS:
            // TODO
            break;
        case kParameterEQPOS:
            // TODO
            break;
        case kParameterBASSGAIN:
            knobs.bass->setValue(value, false);
            break;
        case kParameterMIDGAIN:
            knobs.middle->setValue(value, false);
            break;
        case kParameterMTYPE:
            // TODO
            break;
        case kParameterTREBLEGAIN:
            knobs.treble->setValue(value, false);
            break;
        case kParameterDEPTH:
            knobs.depth->setValue(value, false);
            break;
        case kParameterPRESENCE:
            knobs.presence->setValue(value, false);
            break;
        case kParameterMASTER:
            knobs.master->setValue(value, false);
            break;
        case kParameterBASSFREQ:
        case kParameterMIDFREQ:
        case kParameterMIDQ:
        case kParameterTREBLEFREQ:
        case kParameterCount:
            break;
        }
    }

    void stateChanged(const char* key, const char* value) override
    {
    }

   /* -----------------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The OpenGL drawing function.
      This UI will draw a 3x3 grid, with on/off states according to plugin parameters.
    */
    void onNanoDisplay() override
    {
        const GraphicsContext& ctx(getGraphicsContext());

        const uint width = getWidth();
        const uint height = getHeight();
        const double scaleFactor = getScaleFactor();

        const double cornerRadius = 12 * scaleFactor;
        const double marginHead = 12 * scaleFactor;
        const double widthPedal = kPedalWidth * scaleFactor;
        const double heightPedal = kPedalHeight * scaleFactor;
        const double widthHead = widthPedal - marginHead * 2;
        const double heightHead = 177 * scaleFactor;
        const double marginHorizontal = (width - widthPedal) / 2;
        const double marginVertical = (height - heightPedal) / 2;

        using namespace Artwork;

        // outer bounds gradient
        beginPath();
        rect(0, 0, width, height);
        fillPaint(linearGradient(0, 0, 0, height,
                                 Color::fromHTML("#cdff05").plus(50),
                                 Color::fromHTML("#8bf700").plus(50)));
        fill();

        // outer bounds inner shadow matching host color, if provided
        Color bgColor;
        if (const uint hostBgColor = getBackgroundColor())
        {
            const int red   = (hostBgColor >> 24) & 0xff;
            const int green = (hostBgColor >> 16) & 0xff;
            const int blue  = (hostBgColor >>  8) & 0xff;
            bgColor = Color(red, green, blue);
        }
        else
        {
            bgColor = Color(0,0,0);
        }

        fillPaint(boxGradient(0, 0, width, height, cornerRadius, cornerRadius, bgColor.withAlpha(0.f), bgColor));
        fill();

        // .rt-neural .grid
        beginPath();
        roundedRect(marginHorizontal, marginVertical, widthPedal, heightPedal, cornerRadius);
        fillPaint(linearGradient(marginHorizontal,
                                 marginVertical,
                                 marginHorizontal + widthPedal,
                                 marginVertical + heightPedal,
                                 Color(28, 23, 12),
                                 Color(19, 19, 19)));
        fill();

        // extra
        strokeColor(Color(50, 50, 50, 0.5f));
        stroke();

        // .rt-neural .background_head
        beginPath();
        roundedRect(marginHorizontal + marginHead,
                    marginVertical + marginHead,
                    widthHead,
                    heightHead,
                    cornerRadius);
        fillPaint(linearGradient(marginHorizontal + marginHead,
                                 marginVertical + marginHead,
                                 marginHorizontal + marginHead,
                                 marginVertical + heightHead,
                                 Color::fromHTML("#8bf700"),
                                 Color::fromHTML("#cdff05")));
        fill();

        fillPaint(imagePattern(marginHorizontal + marginHead,
                               marginVertical + marginHead,
                               images.background.getSize().getWidth() / 2 * scaleFactor,
                               images.background.getSize().getHeight() / 2 * scaleFactor,
                               0.f, images.background, 1.f));
        fill();

        fillPaint(boxGradient(marginHorizontal + marginHead,
                              marginVertical + marginHead,
                              widthHead,
                              heightHead,
                              cornerRadius,
                              12 * scaleFactor,
                              Color(0, 0, 0, 0.f), Color(0, 0, 0)));
        fill();

        // .rt-neural .brand
        // TODO

        // .rt-neural .plate
        // TODO
    }

    bool onMouse(const MouseEvent& event) override
    {
        if (UI::onMouse(event))
            return true;

        const uint width = getWidth();
        const uint height = getHeight();

        if (!event.press)
            return false;

        using namespace Artwork;

        const Rectangle<uint> area(kPedalMargin, kPedalMargin, kPedalWidth, kPedalHeight);

        if (area.contains(event.pos))
        {
            d_stdout("click!");

            if (!requestStateFile("json"))
            {
                d_stdout("File through host failed, doing it manually");

                DISTRHO_NAMESPACE::FileBrowserOptions opts;
                opts.title = "Open AidaDSP model json";
                if (!openFileBrowser(opts))
                {
                    d_stdout("Failed to open a file dialog!");
                }
            }

            return true;
        }

        return false;
    }

    void onResize(const ResizeEvent& event) override
    {
        UI::onResize(event);
        repositionWidgets();
    }

    void repositionWidgets()
    {
        const uint width = getWidth();
        const uint height = getHeight();

        const double scaleFactor = getScaleFactor();
        const double widthPedal = kPedalWidth * scaleFactor;
        const double heightPedal = kPedalHeight * scaleFactor;
        const double marginHorizontal = (width - widthPedal) / 2;
        const double marginVertical = (height - heightPedal) / 2;
        const double margin = 15 * scaleFactor;

        const uint unusedSpace = widthPedal
                               - (AidaKnob::kScaleSize * scaleFactor * 7)
                               - (AidaSplitter::kLineWidth * scaleFactor * 3);
        const uint padding = unusedSpace / 10;
        const uint maxHeight = subwidgetsLayout.setAbsolutePos(marginHorizontal + margin,
                                                               height - marginVertical - margin - 110 * scaleFactor,
                                                               padding);
        subwidgetsLayout.setSize(maxHeight, 0);
    }

    void knobDragStarted(SubWidget* const widget) override
    {
        editParameter(widget->getId(), true);
    }

    void knobDragFinished(SubWidget* const widget) override
    {
        editParameter(widget->getId(), false);
    }

    void knobValueChanged(SubWidget* const widget, float value) override
    {
        setParameterValue(widget->getId(), value);
    }

    // ----------------------------------------------------------------------------------------------------------------

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AidaDSPLoaderUI)
};

/* --------------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new AidaDSPLoaderUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
