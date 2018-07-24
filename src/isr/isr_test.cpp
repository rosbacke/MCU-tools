/*
 * isr_test.cpp
 *
 *  Created on: Jul 17, 2018
 *      Author: mikaelr
 */
#include "isr.h"

#include <assert.h>
#include <atomic>

struct CoverTest
{
  public:
    void protect()
    {
        m_protect = 1;
    }
    void unprotect()
    {
        m_unprotect = 1;
    }
    void sync()
    {
        m_sync = 1;
    }
    void unsync()
    {
        m_unsync = 1;
    }

    int m_protect = 0;
    int m_unprotect = 0;
    int m_sync = 0;
    int m_unsync = 0;
};

void
test_CoverTest()
{
    isr::cover<CoverTest> cov;
    auto& ct = cov.systemCover();

    assert(ct.m_protect == 0);
    assert(ct.m_unprotect == 0);
    assert(ct.m_sync == 0);
    assert(ct.m_unsync == 0);
    {
        auto lk = isr::make_protectlock(cov);
        //		isr::protect_lock<isr::cover<CoverTest>> lk(cov);
        assert(ct.m_protect == 1);
        assert(ct.m_unprotect == 0);
        assert(ct.m_sync == 0);
        assert(ct.m_unsync == 0);
    }
    assert(ct.m_protect == 1);
    assert(ct.m_unprotect == 1);
    assert(ct.m_sync == 0);
    assert(ct.m_unsync == 0);
}

void
test_CoverTest2()
{
    isr::cover<CoverTest> cov;
    auto& ct = cov.systemCover();

    assert(ct.m_protect == 0);
    assert(ct.m_unprotect == 0);
    assert(ct.m_sync == 0);
    assert(ct.m_unsync == 0);
    {
        auto lk = isr::make_synclock(cov);
        // isr::sync_lock<isr::cover<CoverTest>> lk(cov);
        assert(ct.m_protect == 0);
        assert(ct.m_unprotect == 0);
        assert(ct.m_sync == 1);
        assert(ct.m_unsync == 0);
    }
    assert(ct.m_protect == 0);
    assert(ct.m_unprotect == 0);
    assert(ct.m_sync == 1);
    assert(ct.m_unsync == 1);
}

void
test_CoverTestLinux()
{
    // Make sure the linux system builds.
    isr::cover<isr::arch_linux::SystemCover> cov;
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
    test_CoverTestLinux();
}
