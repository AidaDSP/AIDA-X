/*
 * DISTRHO OneKnob Series
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_LINEAR_SMOOTHER_HPP_INCLUDED
#define DISTRHO_LINEAR_SMOOTHER_HPP_INCLUDED

#include <cmath>

/**
 * @brief A linear smoother for control values
 *
 * This continually smooths a value towards a defined target,
 * using linear segments.
 *
 * The duration of the smoothing segment is defined by the given
 * time constant. Every time the target changes, a new segment
 * restarts for the whole duration of the time constant.
 *
 * Note that this smoother, unlike an exponential smoother,
 * eventually should converge to its target value.
 */
class LinearSmoother {
public:
    void setSampleRate(float newSampleRate) noexcept
    {
        if (sampleRate != newSampleRate)
        {
            sampleRate = newSampleRate;
            updateStep();
        }
    }

    void setTimeConstant(float newTau) noexcept
    {
        if (tau != newTau)
        {
            tau = newTau;
            updateStep();
        }
    }

    float getCurrentValue() const noexcept
    {
        return mem;
    }

    float getTarget() const noexcept
    {
        return target;
    }

    void setTarget(float newTarget) noexcept
    {
        if (target != newTarget)
        {
            target = newTarget;
            updateStep();
        }
    }

    void clear() noexcept
    {
        mem = 0.0f;
    }

    void clearToTarget() noexcept
    {
        mem = target;
    }

    inline float next() noexcept
    {
        float y0 = mem;
        float dy = target - y0;
        return (mem = y0 + std::copysign(std::fmin(std::abs(dy), std::abs(step)), dy));
    }

private:
    void updateStep() noexcept
    {
        step = (target - mem) / (tau * sampleRate);
    }

private:
    float step = 0.0f;
    float target = 0.0f;
    float mem = 0.0f;
    float tau = 0.0f;
    float sampleRate = 0.0f;
};

#endif // DISTRHO_LINEAR_SMOOTHER_HPP_INCLUDED
