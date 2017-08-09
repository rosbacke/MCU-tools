/*
 * Callback.cpp
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#include <utility/Callback2.h>

class UserClass2
{
  public:
    void testFkn(int i);
};

void
testFkn2(UserClass2* uc, int val)
{
}

void
testFkn3()
{

    UserClass2 uc;

    Callback2<void(int)> st;
    st = Callback2<void(int)>::makeMemberCB<decltype(uc), &UserClass2::testFkn>(
        uc);

    st = MAKE_MEMBER_CB2(void(int), UserClass2::testFkn, uc);

    st(0);

    st = Callback2<void(int)>::makeFreeCB<UserClass2*, testFkn2>(&uc);

    st = MAKE_FREE_CB2(void(int), testFkn2, &uc);
    st(1);
}
