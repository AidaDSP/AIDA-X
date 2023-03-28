/*
 * Aida-X DPF plugin
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "DistrhoUI.hpp"

#include "Layout.hpp"
#include "Widgets.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class AidaDSPLoaderUI : public UI,
                        public ButtonEventHandler::Callback,
                        public KnobEventHandler::Callback
{
    float parameters[kNumParameters];

    struct {
        NanoImage aida;
        NanoImage ax;
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
        ScopedPointer<AidaSwitch> bypass;
        ScopedPointer<AidaSwitch> eqpos;
        ScopedPointer<AidaSwitch> midtype;
    } switches;

    struct {
        ScopedPointer<AidaSplitter> s1, s2, s3;
    } splitters;
    
    struct {
        ScopedPointer<AidaFileButton> model;
        ScopedPointer<AidaFileButton> ir;
    } filebuttons;

    HorizontalLayout subwidgetsLayout;

    enum {
        kFileLoaderNull,
        kFileLoaderModel,
        kFileLoaderImpulse,
    } fileLoaderMode = kFileLoaderNull;

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
        images.aida = createImageFromMemory(aidaData, aidaDataSize, IMAGE_GENERATE_MIPMAPS);
        images.ax = createImageFromMemory(axData, axDataSize, IMAGE_GENERATE_MIPMAPS);
        images.background = createImageFromMemory(backgroundData, backgroundDataSize, IMAGE_REPEAT_X|IMAGE_REPEAT_Y);
        images.knob = createImageFromMemory(knobData, knobDataSize, IMAGE_GENERATE_MIPMAPS);
        images.scale = createImageFromMemory(scaleData, scaleDataSize, IMAGE_GENERATE_MIPMAPS);
        // fontFaceId(createFontFromMemory("conthrax", conthrax_sbData, conthrax_sbDataSize, false));
        loadSharedResources();

        // Create subwidgets
        knobs.pregain = new AidaKnob(this, this, images.knob, images.scale, kParameterPREGAIN);
        knobs.bass = new AidaKnob(this, this, images.knob, images.scale, kParameterBASSGAIN);
        knobs.middle = new AidaKnob(this, this, images.knob, images.scale, kParameterMIDGAIN);
        knobs.treble = new AidaKnob(this, this, images.knob, images.scale, kParameterTREBLEGAIN);
        knobs.depth = new AidaKnob(this, this, images.knob, images.scale, kParameterDEPTH);
        knobs.presence = new AidaKnob(this, this, images.knob, images.scale, kParameterPRESENCE);
        knobs.master = new AidaKnob(this, this, images.knob, images.scale, kParameterMASTER);

        switches.bypass = new AidaSwitch(this, this, kParameterGLOBALBYPASS);
        switches.eqpos = new AidaSwitch(this, this, kParameterEQPOS);
        switches.midtype = new AidaSwitch(this, this, kParameterMTYPE);

        splitters.s1 = new AidaSplitter(this);
        splitters.s2 = new AidaSplitter(this);
        splitters.s3 = new AidaSplitter(this);

        filebuttons.model = new AidaFileButton(this, this, kParameterNETBYPASS, "Load model...");
        filebuttons.ir = new AidaFileButton(this, this, kParameterCONVOLVERENABLE, "Load IR...");

        // Setup subwidgets layout
        subwidgetsLayout.widgets.push_back({ switches.bypass, Fixed });
        subwidgetsLayout.widgets.push_back({ knobs.pregain, Fixed });
        subwidgetsLayout.widgets.push_back({ splitters.s1, Fixed });
        subwidgetsLayout.widgets.push_back({ switches.eqpos, Fixed });
        subwidgetsLayout.widgets.push_back({ switches.midtype, Fixed });
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
            switches.eqpos->setChecked(value > 0.5f, false);
            break;
        case kParameterBASSGAIN:
            knobs.bass->setValue(value, false);
            break;
        case kParameterMIDGAIN:
            knobs.middle->setValue(value, false);
            break;
        case kParameterMTYPE:
            switches.midtype->setChecked(value > 0.5f, false);
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
        case kParameterCONVOLVERENABLE:
            // TODO
            break;
        case kParameterGLOBALBYPASS:
            switches.bypass->setChecked(value < 0.5f, false);
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

        // outer bounds gradient
        beginPath();
        rect(0, 0, width, height);
        fillPaint(linearGradient(0, 0, 0, height,
                                 Color(0xcd, 0xff, 0x05).plus(50),
                                 Color(0x8b, 0xf7, 0x00).plus(50)));
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
        const Size<uint> headBgSize(images.background.getSize() / 2 * scaleFactor);

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
                                 Color(0x8b, 0xf7, 0x00),
                                 Color(0xcd, 0xff, 0x05)));
        fill();

        fillPaint(imagePattern(marginHorizontal + marginHead,
                               marginVertical + marginHead,
                               headBgSize.getWidth(),
                               headBgSize.getHeight(),
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
        const Size<uint> aidaLogoSize(111 * scaleFactor, 25 * scaleFactor);

        save();
        translate(marginHorizontal + marginHead * 2, marginVertical + headBgSize.getHeight());
        beginPath();
        rect(0, 0, aidaLogoSize.getWidth(), aidaLogoSize.getHeight());
        fillPaint(imagePattern(0, 0, aidaLogoSize.getWidth(), aidaLogoSize.getHeight(), 0.f, images.aida, 1.f));
        fill();
        restore();

        // .rt-neural .plate
        const Size<uint> axLogoSize(250 * scaleFactor, 96 * scaleFactor);

        save();
        translate(width/2 - axLogoSize.getWidth()/2, marginVertical + marginHead + headBgSize.getHeight() / 6);
        beginPath();
        rect(0, 0, axLogoSize.getWidth(), axLogoSize.getHeight());
        fillPaint(imagePattern(0, 0, axLogoSize.getWidth(), axLogoSize.getHeight(), 0.f, images.ax, 1.f));
        fill();
        restore();

        fillColor(Color(0x0c, 0x2f, 0x03, 0.686f));
        fontSize(24 * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_BASELINE);
        text(width/2, marginVertical + heightHead - marginHead, "AI CRAFTED TONE", nullptr);
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
                               - (AidaSwitch::kFullWidth  * scaleFactor * 3)
                               - (AidaSplitter::kLineWidth * scaleFactor * 3);
        const uint padding = unusedSpace / 14;
        const uint maxHeight = subwidgetsLayout.setAbsolutePos(marginHorizontal + margin,
                                                               height - marginVertical - margin - 110 * scaleFactor,
                                                               padding);
        subwidgetsLayout.setSize(maxHeight, 0);

        filebuttons.model->setAbsolutePos(marginHorizontal + widthPedal * 2 / 3, marginVertical + margin + 20 * scaleFactor);
        filebuttons.ir->setAbsolutePos(marginHorizontal + widthPedal * 2 / 3, marginVertical + margin + 80 * scaleFactor);
    }

    void buttonClicked(SubWidget* const widget, int button) override
    {
        const uint id = widget->getId();

        switch (id)
        {
        case kParameterEQPOS:
        case kParameterMTYPE:
            editParameter(id, true);
            setParameterValue(id, static_cast<AidaSwitch*>(widget)->isChecked() ? 1.f : 0.f);
            editParameter(id, false);
            break;
        case kParameterGLOBALBYPASS:
            editParameter(id, true);
            setParameterValue(id, static_cast<AidaSwitch*>(widget)->isChecked() ? 0.f : 1.f);
            editParameter(id, false);
            break;
        case kParameterNETBYPASS:
            fileLoaderMode = kFileLoaderModel;
            requestStateFile("json", "Open AidaDSP model json");
            break;
        case kParameterCONVOLVERENABLE:
            fileLoaderMode = kFileLoaderImpulse;
            requestStateFile("ir", "Open Cabinet Simulator IR");
            break;
        }
    }

    void requestStateFile(const char* const stateKey, const char* const fallbackTitle)
    {
        if (UI::requestStateFile(stateKey))
            return;

        d_stdout("File through host failed, doing it manually");

        DISTRHO_NAMESPACE::FileBrowserOptions opts;
        opts.title = fallbackTitle;
        if (!openFileBrowser(opts))
        {
            d_stdout("Failed to open a file dialog!");
        }
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

   /**
      Window file selected function, called when a path is selected by the user, as triggered by openFileBrowser().
      This function is for plugin UIs to be able to override Window::onFileSelected(const char*).

      This action happens after the user confirms the action, so the file browser dialog will be closed at this point.
      The default implementation does nothing.
    */
    void uiFileBrowserSelected(const char* const filename) override
    {
        if (filename == nullptr)
            return;

        switch (fileLoaderMode)
        {
        case kFileLoaderNull:
            break;
        case kFileLoaderModel:
            setState("json", filename);
            break;
        case kFileLoaderImpulse:
            setState("ir", filename);
            break;
        }

        fileLoaderMode = kFileLoaderNull;
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
