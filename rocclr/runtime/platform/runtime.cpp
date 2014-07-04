//
// Copyright (c) 2008 Advanced Micro Devices, Inc. All rights reserved.
//

#include "platform/runtime.hpp"
#include "thread/atomic.hpp"
#include "os/os.hpp"
#include "thread/thread.hpp"
#include "device/device.hpp"
#include "utils/flags.hpp"
#include "utils/options.hpp"
#include "platform/context.hpp"
#include "platform/agent.hpp"

#include "amdocl/cl_gl_amd.hpp"

#ifdef _WIN32
#include <d3d10_1.h>
#include <dxgi.h>
#include "CL/cl_d3d10.h"
#endif //_WIN32

#if defined(_MSC_VER) //both Win32 and Win64
#include <intrin.h>
#endif

#include <cstdlib>
#include <iostream>

#ifdef TIMEBOMB
# include <cstdio>
# include <time.h>
#endif // TIMEBOMB

namespace amd {

#ifdef __linux__

static void __runtime_exit() __attribute__((destructor(102)));
static void __runtime_exit()
{
    if (ENABLE_CAL_SHUTDOWN) {
        Runtime::tearDown();
    }
}

#endif

volatile bool
Runtime::initialized_ = false;

bool
Runtime::init()
{
    if (initialized_) {
        return true;
    }

    // Enter a very basic critical region. We want to prevent 2 threads
    // from concurrently executing the init() routines. We can't use a
    // Monitor since the system is not yet initialized.

    static Atomic<int> lock = 0;
    struct CriticalRegion
    {
        Atomic<int>& lock_;
        CriticalRegion(Atomic<int>& lock) : lock_(lock)
        {
            while (true) {
                if (lock == 0 && lock.swap(1) == 0) {
                    break;
                }
                Os::yield();
            }
        }
        ~CriticalRegion()
        {
            lock_.storeRelease(0);
        }
    } region(lock);

    if (initialized_) {
        return true;
    }

#ifdef TIMEBOMB
    time_t current = time(NULL);
    time_t expiration = TIMEBOMB;

    if (current > expiration) {
        fprintf(stderr, "Expired on %s", asctime(gmtime(&expiration)));
        return false;
    }
    else {
        fprintf(stderr, "For test only: Expires on %s",
            asctime(gmtime(&expiration)));
    }
#endif // TIMEBOMB

    if (   !Flag::init()
        || !option::init()
        || !Device::init()
        // Agent initializes last
        || !Agent::init()) {
        return false;
    }

    initialized_ = true;
    return true;
}

void
Runtime::tearDown()
{
    if (!initialized_) {
        return;
    }

    Agent::tearDown();
    Device::tearDown();
    option::teardown();
    Flag::tearDown();
}

uint
ReferenceCountedObject::retain()
{
    return ++make_atomic(referenceCount_);
}

uint
ReferenceCountedObject::release()
{
    uint newCount = --make_atomic(referenceCount_);
    if (newCount == 0) {
        if (terminate()) {
            delete this;
        }
    }
    return newCount;
}

#ifdef _WIN32
#ifdef DEBUG
static int
reportHook(int reportType, char *message, int *returnValue)
{
        std::cerr << message;
        ::exit(3);
        return TRUE;
}
#endif // DEBUG

extern "C" BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
#   ifdef DEBUG
        if (AMD_OCL_SUPPRESS_MESSAGE_BOX) {
            _CrtSetReportHook(reportHook);
            _set_error_mode(_OUT_TO_STDERR);
        }
#   endif // DEBUG
        break;
    case DLL_PROCESS_DETACH:
        if (!reserved || ENABLE_CAL_SHUTDOWN) {
            Runtime::tearDown();
        }
        break;
    case DLL_THREAD_DETACH: {
            amd::Thread* thread = amd::Thread::current();
            delete thread;
        }
        break;
    default:
        break;
    }
    return true;
}
#endif

} // namespace amd
