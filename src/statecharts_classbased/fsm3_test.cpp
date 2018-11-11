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

// Base case with a root node and 2 normal states.
TEST(StateChart, testStateChart)
{
    cout << "start" << endl;

    // Vanilla FSM. Can declare a root node.
    using RootNode = FsmNode<State<RootState, SId::root>,
    		State<S<1>, SId::state1>,
			State<S<2>, SId::state2>
    >;

    EXPECT_EQ(RootNode::area, 3);
    EXPECT_EQ(RootNode::subStateNo, 2);
    EXPECT_EQ(RootNode::id, SId::root);

    // Run time function variants.
    EXPECT_EQ(RootNode::childOffset(0), 1);
    EXPECT_EQ(RootNode::childOffset(1), 2);

    // And compile time. Added bonus, compile time error when out of range.
    EXPECT_EQ(RootNode::childOffset<0>(), 1);
    EXPECT_EQ(RootNode::childOffset<1>(), 2);
    // EXPECT_EQ(RootNode::childOffset<2>(), 3); // Gives compile error, as expected.

    using SubNode0 = RootNode::SubType<0>;
    EXPECT_EQ(SubNode0::area, 1);
    EXPECT_EQ(SubNode0::subStateNo, 0);
    EXPECT_EQ(SubNode0::id, SId::state1);

    using SubNode1 = RootNode::SubType<1>;
    EXPECT_EQ(SubNode1::area, 1);
    EXPECT_EQ(SubNode1::subStateNo, 0);
    EXPECT_EQ(SubNode1::id, SId::state2);

    FsmStatic<RootNode> fsms;


    EXPECT_EQ(fsms.stateNo, 3);
    EXPECT_EQ(fsms.index2Id[0], SId::root);
    EXPECT_EQ(fsms.index2Id[1], SId::state1);
    EXPECT_EQ(fsms.index2Id[2], SId::state2);

    EXPECT_EQ(fsms.parentIndex[0], 0);
    EXPECT_EQ(fsms.parentIndex[1], 0);
    EXPECT_EQ(fsms.parentIndex[2], 0);

    EXPECT_EQ(fsms.levelIndex[0], 0);
    EXPECT_EQ(fsms.levelIndex[1], 1);
    EXPECT_EQ(fsms.levelIndex[2], 1);

    EXPECT_EQ(fsms.maxLevel, 1);
    EXPECT_EQ(fsms.maxStackSize, 2);
}


// Base case with a root node and 2 normal states.
TEST(StateChart, testStateChart2)
{

    // Vanilla FSM. Can declare a root node.
    using SubNode = FsmNode<State<S<1>, SId::state1>,
    		State<S<2>, SId::state2>,
			State<S<3>, SId::state3>
    >;

    using RootNode = FsmNode<State<RootState, SId::root>,
    		SubNode,
			State<S<4>, SId::state4>
    >;

    EXPECT_EQ(RootNode::area, 5);
    EXPECT_EQ(RootNode::subStateNo, 2);
    EXPECT_EQ(RootNode::id, SId::root);

    // Run time function variants.
    EXPECT_EQ(RootNode::childOffset(0), 1);
    EXPECT_EQ(RootNode::childOffset(1), 4);

    // And compile time. Added bonus, compile time error when out of range.
    EXPECT_EQ(RootNode::childOffset<0>(), 1);
    EXPECT_EQ(RootNode::childOffset<1>(), 4);
    // EXPECT_EQ(RootNode::childOffset<2>(), 3); // Gives compile error, as expected.

    using SubNode0 = RootNode::SubType<0>;
    EXPECT_EQ(SubNode0::area, 3);
    EXPECT_EQ(SubNode0::subStateNo, 2);
    EXPECT_EQ(SubNode0::id, SId::state1);

    using SubNode1 = RootNode::SubType<1>;
    EXPECT_EQ(SubNode1::area, 1);
    EXPECT_EQ(SubNode1::subStateNo, 0);
    EXPECT_EQ(SubNode1::id, SId::state4);

    FsmStatic<RootNode> fsms;

    EXPECT_EQ(fsms.stateNo, 5);
    EXPECT_EQ(fsms.index2Id[0], SId::root);
    EXPECT_EQ(fsms.index2Id[1], SId::state1);
    EXPECT_EQ(fsms.index2Id[2], SId::state2); // ??
    EXPECT_EQ(fsms.index2Id[3], SId::state3);
    EXPECT_EQ(fsms.index2Id[4], SId::state4);

    EXPECT_EQ(fsms.parentIndex[0], 0);
    EXPECT_EQ(fsms.parentIndex[1], 0);
    EXPECT_EQ(fsms.parentIndex[2], 1);
    EXPECT_EQ(fsms.parentIndex[3], 1);
    EXPECT_EQ(fsms.parentIndex[4], 0);

    EXPECT_EQ(fsms.levelIndex[0], 0);
    EXPECT_EQ(fsms.levelIndex[1], 1);
    EXPECT_EQ(fsms.levelIndex[2], 2);
    EXPECT_EQ(fsms.levelIndex[3], 2);
    EXPECT_EQ(fsms.levelIndex[4], 1);

    EXPECT_EQ(fsms.maxLevel, 2);
    EXPECT_EQ(fsms.maxStackSize, 3);
}

int
main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
