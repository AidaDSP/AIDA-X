/*
 * 2-Stage Threaded Convolver
 * Copyright (C) 2022-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#pragma once

#ifndef DISTRHO_OS_WASM
# include "Semaphore.hpp"
# include "extra/ScopedPointer.hpp"
# include "extra/Thread.hpp"
#endif

#include "TwoStageFFTConvolver.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#ifndef DISTRHO_OS_WASM
class TwoStageThreadedConvolver : public fftconvolver::TwoStageFFTConvolver,
                                  private Thread
{
    static constexpr const size_t kHeadBlockSize = 128;
    static constexpr const size_t kTailBlockSize = 1024;

    ScopedPointer<fftconvolver::FFTConvolver> nonThreadedConvolver;
    Semaphore semBgProcStart;
    Semaphore semBgProcFinished;

public:
    TwoStageThreadedConvolver()
        : fftconvolver::TwoStageFFTConvolver(),
          Thread("TwoStageThreadedConvolver"),
          semBgProcStart(1),
          semBgProcFinished(0)
    {
    }

    ~TwoStageThreadedConvolver() override
    {
        if (nonThreadedConvolver != nullptr)
        {
            nonThreadedConvolver = nullptr;
            return;
        }

        signalThreadShouldExit();
        semBgProcStart.post();
        stopThread(5000);
    }

    bool init(const fftconvolver::Sample* const ir, const size_t irLen)
    {
        if (irLen > kTailBlockSize * 2)
        {
            if (! fftconvolver::TwoStageFFTConvolver::init(kHeadBlockSize, kTailBlockSize, ir, irLen))
                return false;

            startThread(true);
            return true;
        }

        nonThreadedConvolver = new fftconvolver::FFTConvolver();
        return nonThreadedConvolver->init(kHeadBlockSize, ir, irLen);
    }

    void process(const fftconvolver::Sample* const input, fftconvolver::Sample* const output, const size_t len)
    {
        if (nonThreadedConvolver != nullptr)
            nonThreadedConvolver->process(input, output, len);
        else
            fftconvolver::TwoStageFFTConvolver::process(input, output, len);
    }

protected:
    void startBackgroundProcessing() override
    {
        semBgProcStart.post();
    }

    void waitForBackgroundProcessing() override
    {
        if (isThreadRunning() && !shouldThreadExit())
            semBgProcFinished.wait();
    }

    void run() override
    {
        while (!shouldThreadExit())
        {
            semBgProcStart.wait();

            if (shouldThreadExit())
                break;

            doBackgroundProcessing();
            semBgProcFinished.post();
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TwoStageThreadedConvolver)
};
#else
class TwoStageThreadedConvolver : public fftconvolver::FFTConvolver
{
    static constexpr const size_t kHeadBlockSize = 128;

public:
    TwoStageThreadedConvolver()
        : fftconvolver::FFTConvolver() {}

    bool init(const fftconvolver::Sample* const ir, const size_t irLen)
    {
        return fftconvolver::FFTConvolver::init(kHeadBlockSize, ir, irLen);
    }
};
#endif

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
