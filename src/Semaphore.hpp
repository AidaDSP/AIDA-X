/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: ISC
 */

#ifndef DISTRHO_SEMAPHORE_HPP_INCLUDED
#define DISTRHO_SEMAPHORE_HPP_INCLUDED

#include "extra/String.hpp"

#if defined(DISTRHO_OS_MAC)
# include <mach/mach.h>
# include <mach/semaphore.h>
#elif defined(DISTRHO_OS_WINDOWS)
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <winsock2.h>
# include <windows.h>
#else
# include <semaphore.h>
# include <sys/time.h>
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class Semaphore
{
public:
    Semaphore(const int initialValue = 0)
    {
       #if defined(DISTRHO_OS_MAC)
        DISTRHO_SAFE_ASSERT_RETURN(semaphore_create(mach_task_self(),
                                                    &sem,
                                                    SYNC_POLICY_FIFO,
                                                    initialValue) == KERN_SUCCESS,);
       #elif defined(DISTRHO_OS_WINDOWS)
        handle = ::CreateSemaphoreA(nullptr, initialValue, std::max(initialValue, 1), nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(handle != INVALID_HANDLE_VALUE,);
       #else
        ::sem_init(&sem, 0, initialValue);
       #endif
    }

    ~Semaphore()
    {
       #if defined(DISTRHO_OS_MAC)
        ::semaphore_destroy(mach_task_self(), sem);
       #elif defined(DISTRHO_OS_WINDOWS)
        ::CloseHandle(handle);
       #else
        ::sem_destroy(&sem);
       #endif
    }

    void post()
    {
       #if defined(DISTRHO_OS_MAC)
        ::semaphore_signal(sem);
       #elif defined(DISTRHO_OS_WINDOWS)
        ::ReleaseSemaphore(handle, 1, nullptr);
       #else
        ::sem_post(&sem);
       #endif
    }

    bool wait()
    {
       #if defined(DISTRHO_OS_MAC)
        return ::semaphore_wait(sem) == KERN_SUCCESS;
       #elif defined(DISTRHO_OS_WINDOWS)
        return ::WaitForSingleObject(handle, INFINITE) == WAIT_OBJECT_0;
       #else
        return ::sem_wait(&sem) == 0;
       #endif
    }

    bool timedWait(const uint numSecs)
    {
       #if defined(DISTRHO_OS_MAC)
        const struct mach_timespec time = { numSecs, 0 };
        return ::semaphore_timedwait(sem, time) == KERN_SUCCESS;
       #elif defined(DISTRHO_OS_WINDOWS)
        return ::WaitForSingleObject(handle, numSecs * 1000) == WAIT_OBJECT_0;
       #else
        struct timespec timeout;
        ::clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += numSecs;
        return ::sem_timedwait(&sem, &timeout) == 0;
       #endif
    }

private:
   #if defined(DISTRHO_OS_MAC)
    ::semaphore_t sem;
   #elif defined(DISTRHO_OS_WINDOWS)
    ::HANDLE handle;
   #else
    ::sem_t sem;
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_SEMAPHORE_HPP_INCLUDED
