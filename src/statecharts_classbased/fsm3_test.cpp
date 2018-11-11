/*
 * PosixFileIfReal_test.cpp
 *
 *  Created on: 30 okt. 2016
 *      Author: mikaelr
 */

#include "StateChart3.h"

#include <gtest/gtest.h>

#include <string>

using std::cout;
using std::endl;

struct RootState
{};

// General State structs. Template to save typing.
template<int i>
struct S;

enum class SId
{
	root = 10,
	state1,
	state2,
	state3,
	state4
};

using namespace std;

TEST(StateChart, testStateChart)
{
    cout << "start" << endl;

    // Vanilla FSM. Can declare a root node.
    using RootNode = FsmNode<State<RootState, SId::root>,
    		State<S<1>, SId::state1>,
			State<S<2>, SId::state2>,
			State<S<3>, SId::state3>
    >;

    EXPECT_EQ(RootNode::area, 4);
    EXPECT_EQ(RootNode::subStateNo, 3);
    EXPECT_EQ(RootNode::id, SId::root);
    EXPECT_EQ(RootNode::childOffset(0), 1);
    EXPECT_EQ(RootNode::childOffset(1), 2);
    EXPECT_EQ(RootNode::childOffset(2), 3);

    using SubNode0 = RootNode::SubType<0>;
    EXPECT_EQ(SubNode0::area, 1);
    EXPECT_EQ(SubNode0::subStateNo, 0);
    EXPECT_EQ(SubNode0::id, SId::state1);

    using SubNode1 = RootNode::SubType<1>;
    EXPECT_EQ(SubNode1::area, 1);
    EXPECT_EQ(SubNode1::subStateNo, 0);
    EXPECT_EQ(SubNode1::id, SId::state2);

    using SubNode2 = RootNode::SubType<2>;
    EXPECT_EQ(SubNode2::area, 1);
    EXPECT_EQ(SubNode2::subStateNo, 0);
    EXPECT_EQ(SubNode2::id, SId::state3);

    FsmStatic<RootNode> fsms;

    EXPECT_EQ(fsms.stateNo, 4);
    EXPECT_EQ(fsms.index2Id[0], SId::root);
    EXPECT_EQ(fsms.index2Id[1], SId::state1);
    EXPECT_EQ(fsms.index2Id[2], SId::state2);
    EXPECT_EQ(fsms.index2Id[3], SId::state3);

}

int
main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
