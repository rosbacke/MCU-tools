/*
 * isr.h
 *
 *  Created on: Jul 16, 2018
 *      Author: mikaelr
 */

#ifndef SRC_ISR_ISR_H_
#define SRC_ISR_ISR_H_

#include <atomic>

/**
 * The C++ specification is very silent on the subject of
 * synchronization between threads and interrupt service routines.
 *
 * This is a vendor specific area but to reduce friction between platforms
 * this is an attempt to get primitives that could work when compiled
 * on a C++11 standard platform.
 * Specifically I want to move old style volatile based handling to something
 * that uses std::atomic_* family of functions. These have been analyzed and
 * are much better understood by the community.
 *
 * An interrupt service routine can be viewed as a thread with a few caveats:
 * - It must never block, waiting on some other interrupt or thread that it has
 *   already blocked.
 * - We need to convince the compiler that the top level ISR function can be
 *   called at any time. It is not to be optimized away due to not being called.
 * - We lack the start and end synchronization points that a thread start / join
 *   normally provides.
 *
 * So assuming that we have lock-free atomics we could set up communication
 * that respect the standard data-race free requirement for C++.
 * We still assume we have the conventional disable / enable interrupt for
 * expressing critical sections in thread code. Those are patform defined and
 * outside the scope here.
 *
 *
 * Then we can either:
 * - At ISR entry / exit do an atomic read / write and when the thread does
 *   disable / enable interrupt it will also do an atomic read / write.
 *   It will allow operating on normal variables in both cases.
 * or:
 * - Always use atomics on our ISR / thread. This is suitable for e.g.
 *   short ISR that only updates a counter etc.
 *
 * The second use case is easy. Just implement lock free acquire / release
 * atomics and use those.
 *
 * The first could use some help. An ISR protection class with an atomic
 * that both the ISR and thread call. Then A lock like class that unlocks on
 * exit. It would need external functions to do the actual disable / enable
 * interrupt.
 */

namespace isr
{

/**
 * An analog to a mutex but for high/low priority tasks/interrupts
 * which possibly share a stack. The low level task creates a critical
 * section by calling lock / unlock. This will do 2 things:
 * - Disable the ability for the high level task to run and later reenable it.
 * - At lock, do a an acquire operation and at unlock do a release operation.
 *
 * The high level task calls acquire at the beginning and release at the end.
 * It will generate synchronization points to allow communication between tasks.
 *
 * To do its work the cover object forward all calls to system dependent parts.
 */
template <typename SystemCover>
class cover : private SystemCover
{
  public:
    // Called in thread context to start a critical section and sync with the
    // last interrupt.
    void protect()
    {
        SystemCover::protect();
    }
    // Called in thread context to end the critical section.
    void unprotect()
    {
        SystemCover::unprotect();
    }
    // Called in isr context to start the isr and sync with the critical
    // section.
    void sync()
    {
        SystemCover::sync();
    }

    // Called in isr context to end the isr and set up sync with the critical
    // section.
    void unsync()
    {
        SystemCover::unsync();
    }

    // Get access to the underlying system dependent part.
    SystemCover& systemCover()
    {
        return static_cast<SystemCover&>(*this);
    }
};

/**
 * lock object for RAII style disable interrupt from a thread. will also
 * generate the acquire / release to have the synchronize-with event with the
 * interrupt lock.
 */
template <typename Cover>
class protect_lock
{
  public:
    protect_lock(Cover& is) : m_is(is)
    {
        m_is.protect();
    }
    ~protect_lock()
    {
        m_is.unprotect();
    }
    Cover& m_is;
};

// constructor function, allows deducing template argumet from function
// argument. Relies on return value copy elision. (Mandatory from C++17, works
// on most compilers)
template <typename Cover>
protect_lock<Cover>
make_protectlock(Cover& c)
{
    return protect_lock<Cover>(c);
}

/**
 * lock object for RAII style acquire / release in the interrupt function.
 * Intended to create synchronizes-with with the thread lock operation.
 */
template <typename Cover>
class sync_lock
{
  public:
    sync_lock(Cover& is) : m_is(is)
    {
        m_is.sync();
    }
    ~sync_lock()
    {
        m_is.unsync();
    }
    Cover& m_is;
};

// constructor function, allows deducing template argumet from function
// argument. Relies on return value copy elision. (Mandatory from C++17, works
// on most compilers)
template <typename Cover>
sync_lock<Cover>
make_synclock(Cover& c)
{
    return sync_lock<Cover>(c);
}
}

/**
 * System dependent part. This is the parts that needs to be implemented for
 * each system.
 */

#if defined(__linux__)
// Linux do not have interrupt, but we can simulate them with a thread. Hence a
// mutex should be fine.
#include <mutex>

namespace isr
{
namespace arch_linux
{

class SystemCover
{
    std::mutex m;

  public:
    // Called in thread context to start a critical section and sync with the
    // last interrupt.
    void protect()
    {
        m.lock();
    }
    // Called in thread context to end the critical section.
    void unprotect()
    {
        m.unlock();
    }
    // Called in isr context to start the isr and sync with the critical
    // section.
    void sync()
    {
        m.lock();
    }

    // Called in isr context to end the isr and set up sync with the critical
    // section.
    void unsync()
    {
        m.unlock();
    }
};
}
}

#endif

// Mainly Cortex M0 microcontroller.
#if defined(__ARM_ARCH_6M__)

namespace isr
{
namespace arch_armv6_m
{

class SystemCover
{
  public:
    // Called in thread context to start a critical section and sync with the
    // last interrupt.
    void protect()
    {
        __asm__ __volatile__(" cpsid i\n");
        __asm__ __volatile__("" : : : "memory");
    }
    // Called in thread context to end the critical section.
    void unprotect()
    {
        __asm__ __volatile__("" : : : "memory");
        __asm__ __volatile__(" cpsie i\n");
    }
    // Called in isr context to start the isr and sync with the critical
    // section.
    void sync()
    {
        __asm__ __volatile__("" : : : "memory");
    }

    // Called in isr context to end the isr and set up sync with the critical
    // section.
    void unsync()
    {
        __asm__ __volatile__("" : : : "memory");
    }
};
}
}

#endif

// Mainly Cortex M3-7 microcontrollers.
#if defined(__ARM_ARCH_7M__)

namespace isr
{
namespace arch_armv7_m
{

class SystemCover
{
  public:
    void protect()
    {
        __asm__ __volatile__(" cpsid i\n");
        __asm__ __volatile__("" : : : "memory");
    }
    void unprotect()
    {
        __asm__ __volatile__("" : : : "memory");
        __asm__ __volatile__(" cpsie i\n");
    }
    void sync()
    {
        __asm__ __volatile__("" : : : "memory");
    }
    void unsync()
    {
        __asm__ __volatile__("" : : : "memory");
    }
};
}
}

#endif

#endif /* SRC_ISR_ISR_H_ */
