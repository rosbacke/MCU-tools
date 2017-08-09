#include "Callback2.h"

#include <cassert>

void
test_construction()
{
    Callback2<void()> cb;

    // Make sure default constructed tests as false. (similar to normal
    // pointers.)
    assert(cb == false);

    // Make sure default constructed tests as equal to nullptr;
    assert(cb == nullptr);

    // Ensure copy construction.
    auto cb2 = cb;

    // Ensure assignment.
    cb2 = cb;
}

static int
testAdd(int x, int y)
{
    return x + y;
}

static int
testDiff(int x, int y)
{
    return x - y;
}

void
testFreeFunction()
{
    // Create simple callback to a normal free function.
    auto cb = Callback2<int(int, int)>::makeSimpleFreeCB<testAdd>();

    // Try making a call. Check result.
    int res = cb(2, 3);
    assert(res == 5);

    // Test copy constructor.
    auto cb2(cb);
    int res2 = cb2(3, 4);
    assert(res2 == 7);

    // Test assign.
    cb = Callback2<int(int, int)>::makeSimpleFreeCB<testDiff>();
    res = cb(5, 2);
    assert(res == 3);
}

struct TestObj
{
    int m_val = 3;

    int add(int x)
    {
        return m_val + x;
    }
};

int
adder(TestObj* o, int val)
{
    return o->m_val + val;
}

void
testFreeFunctionWithPtr()
{
    TestObj o;

    // Create simple callback to a normal free function.
    auto cb = Callback2<int(int)>::makeFreeCB<TestObj*, adder>(&o);

    o.m_val = 6;
    int res = cb(3);
    assert(res == 9);

    o.m_val = 3;
    res = cb(9);
    assert(res == 12);
}

void
testMemberFunction()
{
    TestObj o;

    // Create member function callback.
    auto cb = Callback2<int(int)>::makeMemberCB<TestObj, &TestObj::add>(o);

    o.m_val = 6;
    int res = cb(3);
    assert(res == 9);

    o.m_val = 3;
    res = cb(9);
    assert(res == 12);
}

int
main()
{
    test_construction();
    testFreeFunction();
    testFreeFunctionWithPtr();
    testMemberFunction();
}
