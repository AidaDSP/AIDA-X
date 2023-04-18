# AIDA-X

![screenshot](docs/Screenshot.png "Screenshot")

AIDA-X is an Amp Model Player, allowing it to load models of AI trained music gear, which you can then play through! 🎸

Its main intended use is to provide high fidelity simulations of amplifiers.  
However, it is also possible to run entire signal chains consisting of any combination of amp, cab, dist, drive, fuzz, boost and eq.

This repository contains the source code for the [DPF-based](https://github.com/DISTRHO/DPF) plugin variant, see [aidadsp-lv2](https://github.com/AidaDSP/aidadsp-lv2) for the LV2 headless version of the same engine optimized to run on embedded systems such as [MOD Dwarf](https://mod.audio/dwarf/), RPi, Portenta-X8, Aida DSP OS and so on.

For ease of use, this plugin also contains a cabinet simulator via impulse response files, which runs after the Amp Model.

CLAP, LV2, VST2 and VST3 plugin formats are supported, plus a standalone.

### Installation ###

[Click here to access the latest stable release](https://github.com/AidaDSP/AIDA-X/releases/latest).

Installers are provided for both macOS and Windows, plus regular binary tarballs for Linux.

Do note **these builds are not signed**, so expect warnings saying they are from an "untrusted developer".  
This is particularly striking on macOS, where the plugin installer can only be opened with mouse right-click and the standalone app must be opened via right-click twice.

So-called "nightly builds" are available through [GitHub actions](https://github.com/AidaDSP/AIDA-X/actions/workflows/build.yml).

Alternatively an experimental online version is available at [mod.audio/aida-x-online](https://mod.audio/aida-x-online/).  
There is no audio input, instead a pre-selected list of guitar audio loops is used.  
This online version is otherwise fully functional and identical to the desktop version you can install and run on your machine.

### User Manual ###

#### Loading files ####

AIDA-X comes built-in with a single Amp Model and Cabinet IR.  
Click on the related filename to open a file browser and load a different file.  
The little icon on the left side allows to turn on/off the Amp Model and Cabinet IR.  
Both wav and flac audio formats are supported for IR files.

<img height="91" alt="image" src="https://raw.githubusercontent.com/AidaDSP/AIDA-X/main/docs/Screenshot-files.png">

A quick model pack can be downloaded from [our Google Drive folder](https://drive.google.com/drive/folders/18MwNhuo9fjK8hlne6SAdhpGtL4bWsVz-).

Check out the [MOD Forum's Neural Modelling section](https://forum.mod.audio/c/neural/62) for an online place for discussion, sharing and all things related to Amp Models.

#### Meters ####

The AIDA-X UI contains input and output meters, for ease of monitoring the sound.  
These are peak meters calculated at a maximum of 60 FPS.

Both meters will change colors to indicate when sound is clipping.  
When -3dB is reached the meters turn yellow, and on 0dB they turn red.  
The meters will change back to the previous color once the audio signal falls below -3dB of their tripping point (so -6dB for yellow, -3dB for red).

#### Controls ####

In AIDA-X knobs will move slowly when holding down the Ctrl key.  
Holding down the Shift key or double-clicking a knob will reset it to its default value.  
Please note not all hosts and formats support sending these modifier keys to plugins.

From left to right:

1. **Bypass and Pre-Gain**

<img height="112" alt="image" src="https://raw.githubusercontent.com/AidaDSP/AIDA-X/main/docs/Screenshot-params-1.png">

On/Off bypass switch, lets the audio pass-through when off.  
Integrates with the host provided bypass controls where possible.

Pre-Gain attenuates / emphasizes the input signal before sending it to the Amp Model.

2. **EQ tone controls (part 1)**

<img height="112" alt="image" src="https://raw.githubusercontent.com/AidaDSP/AIDA-X/main/docs/Screenshot-params-2.png">

Pre/Post switch changes the position of the tone controls, either running before or after the Amp Model.

Bandpass/Peak switch changes the MID control to Bandpass so that BASS and TREBLE will be excluded. Default setting for MID control is Peaking or Parametric EQ with adjustable Frequency and Q.

3. **EQ tone controls (part 2)**

<img height="112" alt="image" src="https://raw.githubusercontent.com/AidaDSP/AIDA-X/main/docs/Screenshot-params-3.png">

Extra EQ related controls.  
Depth is a Peaking or Parametric EQ to boost the low end.  
Presence is an High Shelf EQ to boost the high end.

4. **Master Gain**

<img height="112" alt="image" src="https://raw.githubusercontent.com/AidaDSP/AIDA-X/main/docs/Screenshot-params-4.png">

For a final gain compensation, does not apply to the bypassed signal.

#### Using the Standalone ####

When first run the AIDA-X standalone will not make any sound, requiring you to click "Enable Input" (which can trigger OS-level permission requests).  
This is to avoid unintended audio feedback loops when someone simply wants to check out the GUI or AIDA-X was started by accident.

After enabling audio input, an option for setting the audio buffer-size will be shown.  
Reducing buffer-size will reduce latency but at cost of more CPU.  
The optimal value depends on the running system, slower machines cannot do small buffer-sizes without audio crackles.

NOTE: The AIDA-X standalone will connect to your system-defined default audio device, for now there is no option to change to another input/output audio device.

### Technical Details ###

Behind the scenes AIDA-X uses [RTNeural](https://github.com/jatinchowdhury18/RTNeural), which does the heavy lifting for us.

The plugin format support together with the custom GUI is made with [DPF](https://github.com/DISTRHO/DPF), which allows a single codebase to export for many audio plugins at once (amongst other nice features).

Impulse Response handling is done with the help of a custom fork of [FFTConvolver](https://github.com/falkTX/FFTConvolver.git), together with [r8brain-free-src](https://github.com/avaneev/r8brain-free-src.git) for runtime audio file resampling.

#### Generate json models ####

This implies neural network training. Please follow:

- [AIDA-X Model Trainer.ipynb](https://colab.research.google.com/drive/1n3EOnroDSGoj_8PVRP6UEwUXUzPewv8p)
- [Automated_GuitarAmpModelling.ipynb](https://github.com/AidaDSP/Automated-GuitarAmpModelling/tree/aidadsp_devel)

### Building ###

Requires cmake and OpenGL related developer packages.  
For building just do the usual cmake steps, like:

```sh
git clone --recursive https://github.com/AidaDSP/AIDA-X.git && cd AIDA-X
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Binaries will be placed in `./build/bin`

### License ###

AIDA-X is licensed under `GPL-3.0-or-later`, see [LICENSE](LICENSE) for more details.
