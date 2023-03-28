/*
 * 2-Stage Threaded Convolver
 * Copyright (C) 2022-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#pragma once

#include "Semaphore.hpp"
#include "extra/Thread.hpp"

#include "TwoStageFFTConvolver.h"

START_NAMESPACE_DISTRHO

static constexpr const size_t headBlockSize = 128;
static constexpr const size_t tailBlockSize = 1024;

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
