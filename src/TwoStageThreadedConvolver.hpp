/*
 * 2-Stage Threaded Convolver
 * Copyright (C) 2022-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#pragma once

#ifndef DISTRHO_OS_WASM
# include "Semaphore.hpp"
# include "extra/Thread.hpp"
# include "TwoStageFFTConvolver.h"
#else
# include "TwoStageFFTConvolver.h"
#endif

START_NAMESPACE_DISTRHO

static constexpr const size_t headBlockSize = 128;
static constexpr const size_t tailBlockSize = 1024;

// --------------------------------------------------------------------------------------------------------------------

#ifndef DISTRHO_OS_WASM
class TwoStageThreadedConvolver : public fftconvolver::TwoStageFFTConvolver,
                                  private Thread
{
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
        stop();
    }

    void start()
    {
        startThread(true);
    }

    void stop()
    {
        signalThreadShouldExit();
        semBgProcStart.post();
        stopThread(5000);
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
public:
    TwoStageThreadedConvolver()
        : fftconvolver::FFTConvolver() {}

    bool init(const size_t headBlockSize, size_t, const fftconvolver::Sample* const ir, const size_t irLen)
    {
        return fftconvolver::FFTConvolver::init(headBlockSize, ir, irLen);
    }

    void start()
    {
    }

    void stop()
    {
    }
};
#endif

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
