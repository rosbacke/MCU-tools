/*
 * Callback.cpp
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#include <utility/Callback.h>



class UserClass {
public:
	void testFkn(int i);
};


void testFkn(UserClass* uc, int val)
{

}

void testFkn2()
{

	UserClass uc;

	Callback st;
	st = Callback::makeMemberCB<decltype(uc), &UserClass::testFkn>(uc);


	st = MAKE_MEMBER_CB(UserClass::testFkn, uc);


	st(0);

	st = Callback::makeFreeCB<UserClass*, testFkn>(&uc);

	st = MAKE_FREE_CB(testFkn, &uc);
	st(1);
}

