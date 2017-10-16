#include "Callback.h"

#include <cassert>

void
test_construction()
{
    Callback cb;

    // Make sure default constructed tests as false. (similar to normal
    // pointers.)
    assert(cb == false);

    // Make sure default constructed tests as equal to nullptr;
    assert(cb == nullptr);

    // Ensure copy construction.
    Callback cb2 = cb;

    // Ensure assignment.
    cb2 = cb;
}

static int testVar = 0;

// Note: Need external linkage to use as template argument.
// use namespace  {} instead of static.

namespace
{
void
testAdd(int x)
{
    testVar += x;
}

void
testDiff(int x)
{
    testVar -= x;
}
}
void
testFreeFunction()
{
    // Create simple callback to a normal free function.
    Callback cb = Callback::make<testAdd>();

    // Try making a call. Check result.
    testVar = 2;
    cb(3);
    assert(testVar == 5);

    // Test copy constructor.
    Callback cb2(cb);
    testVar = 4;
    cb2(3);
    assert(testVar == 7);

    // Test assign.
    cb = Callback::make<testDiff>();
    testVar = 4;
    cb(3);
    assert(testVar == 1);
}

struct TestObj
{
    TestObj() : m_val(3)
    {
    }
    int m_val;

    void add(int x)
    {
        m_val += x;
    }
};

void
adder(TestObj* o, int val)
{
    o->m_val += val;
}

void
testFreeFunctionWithPtr()
{
    TestObj o;

    // Create simple callback to a normal free function.
    Callback cb = Callback::make<TestObj*, adder>(&o);

    o.m_val = 6;
    cb(3);
    assert(o.m_val == 9);

    o.m_val = 3;
    cb(9);
    assert(o.m_val == 12);
}

void
testMemberFunction()
{
    TestObj o;

    // Create member function callback.
    Callback cb = Callback::make<TestObj, &TestObj::add>(o);

    o.m_val = 6;
    cb(3);
    assert(o.m_val == 9);

    o.m_val = 3;
    cb(9);
    assert(o.m_val);

    // Try MACRO construction.
    Callback cb2 = MAKE_MEMBER_CB(TestObj, TestObj::add, o);

    o.m_val = 6;
    cb2(3);
    assert(o.m_val == 9);

    o.m_val = 3;
    cb2(9);
    assert(o.m_val == 12);
}

struct Functor
{
    void operator()(int x)
    {
        val += x;
    }
    int val;
};

void
testFunctorFunction()
{
    Functor fkn;

    // Create simple callback to a normal free function.
    Callback cb = Callback::make(fkn);

    fkn.val = 3;
    cb(4);

    assert(fkn.val == 7);
}

int
main()
{
    test_construction();
    testFreeFunction();
    testFreeFunctionWithPtr();
    testMemberFunction();
}
