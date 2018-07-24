/*
 * isr_test.cpp
 *
 *  Created on: Jul 17, 2018
 *      Author: mikaelr
 */
#include "isr.h"

#include <assert.h>
#include <atomic>

static inline void
compilerBarrier()
{
    asm volatile("" : : : "memory");
}

struct CoverTest
{
  public:
    void protect()
    {
        m_protect = true;
    }
    void unprotect()
    {
        m_unprotect = true;
    }
    void sync()
    {
        m_sync = true;
    }
    void unsync()
    {
        m_unsync = true;
    }

    bool m_protect = false;
    bool m_unprotect = false;
    bool m_sync = false;
    bool m_unsync = false;
};

void
test_CoverTest()
{
    isr::cover<CoverTest> cov;
    auto& ct = cov.systemCover();

    assert(ct.m_protect == false);
    assert(ct.m_unprotect == false);
    assert(ct.m_sync == false);
    assert(ct.m_unsync == false);
    {
        auto lk = isr::make_protectlock(cov);
        //		isr::protect_lock<isr::cover<CoverTest>> lk(cov);
        assert(ct.m_protect == true);
        assert(ct.m_unprotect == false);
        assert(ct.m_sync == false);
        assert(ct.m_unsync == false);
    }
    assert(ct.m_protect == true);
    assert(ct.m_unprotect == true);
    assert(ct.m_sync == false);
    assert(ct.m_unsync == false);
}

void
test_CoverTest2()
{
    isr::cover<CoverTest> cov;
    auto& ct = cov.systemCover();

    assert(ct.m_protect == false);
    assert(ct.m_unprotect == false);
    assert(ct.m_sync == false);
    assert(ct.m_unsync == false);
    {
        auto lk = isr::make_synclock(cov);
        // isr::sync_lock<isr::cover<CoverTest>> lk(cov);
        assert(ct.m_protect == false);
        assert(ct.m_unprotect == false);
        assert(ct.m_sync == true);
        assert(ct.m_unsync == false);
    }
    assert(ct.m_protect == false);
    assert(ct.m_unprotect == false);
    assert(ct.m_sync == true);
    assert(ct.m_unsync == true);
}

void
test_CoverTestLinux()
{
    isr::cover<isr::linux::SystemCover> cov;
    auto& ct = cov.systemCover();
    {
        auto lk = isr::make_protectlock(cov);
    }
    {
        auto lk = isr::make_synclock(cov);
    }
}

int
main()
{
    test_CoverTest();
    test_CoverTest2();
}
