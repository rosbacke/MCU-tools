/*
 * PosixFileIfReal_test.cpp
 *
 *  Created on: 30 okt. 2016
 *      Author: mikaelr
 */

#include "StateChart3.h"

#include <gtest/gtest.h>

#include <string>

//using std::cout;
//using std::endl;

using namespace std;

struct RootState
{};

// General State structs. Template to save typing.
template<int i>
struct S
{
	// A few different sizes.
	int32_t data[ i ];
};

enum class SId
{
	root = 10,
	state1,
	state2,
	state3,
	state4
};

struct TestEvent
{
	int32_t val;
};

using namespace std;

// Base case with a root node and 2 normal states.
TEST(StateChart, testModel)
{
	EXPECT_EQ(sizeof(S<1>), 4);
	EXPECT_EQ(sizeof(S<2>), 8);
	EXPECT_EQ(sizeof(S<3>), 12);
	EXPECT_EQ(sizeof(S<4>), 16);

	// Model contain vptr + align to 8 bytes.
	EXPECT_EQ(sizeof(Model<S<1>, TestEvent>), 16);
	EXPECT_EQ(sizeof(Model<S<2>, TestEvent>), 16);
	EXPECT_EQ(sizeof(Model<S<3>, TestEvent>), 24);
	EXPECT_EQ(sizeof(Model<S<4>, TestEvent>), 24);
}


// Base case with a root node and 2 normal states.
TEST(StateChart, testStateChart)
{
    // Vanilla FSM. Can declare a root node.
    using RootNode = FsmNode<State<SId::root, RootState>,
    		State<SId::state1, S<1>>,
			State<SId::state2, S<2>>
    >;

    EXPECT_EQ(RootNode::area, 3);
    EXPECT_EQ(RootNode::subStateNo, 2);
    EXPECT_EQ(RootNode::id, SId::root);
    EXPECT_EQ(RootNode::maxheight, 1);

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
    EXPECT_EQ(SubNode0::maxheight, 0);

    using SubNode1 = RootNode::SubType<1>;
    EXPECT_EQ(SubNode1::area, 1);
    EXPECT_EQ(SubNode1::subStateNo, 0);
    EXPECT_EQ(SubNode1::id, SId::state2);

    FsmStatic<RootNode, TestEvent> fsms;

    EXPECT_EQ(fsms.maxLevels, 2);
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

    EXPECT_EQ(fsms.maxLevels, 2);

    // Rootnode empty class. But will require alignment 8 + vptr 8.
    EXPECT_EQ(fsms.levels.maxStorageSize[0], 16);

    // Both state 1 & 2 fit in 8 bytes. Vptr 8.
    EXPECT_EQ(fsms.levels.maxStorageSize[1], 16);
}


// Base case with a root node and 2 normal states.
TEST(StateChart, testStateChart2)
{

    // Vanilla FSM. Can declare a root node.
    using SubNode = FsmNode<State<SId::state1, S<1>>,
    		State<SId::state2, S<2>>,
			State<SId::state3, S<3>>
    >;

    using RootNode = FsmNode<State<SId::root, RootState>,
    		SubNode,
			State<SId::state4, S<4>>
    >;

    EXPECT_EQ(RootNode::area, 5);
    EXPECT_EQ(RootNode::subStateNo, 2);
    EXPECT_EQ(RootNode::id, SId::root);
    EXPECT_EQ(RootNode::maxheight, 2);

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
    EXPECT_EQ(SubNode0::maxheight, 1);

    using SubNode1 = RootNode::SubType<1>;
    EXPECT_EQ(SubNode1::area, 1);
    EXPECT_EQ(SubNode1::subStateNo, 0);
    EXPECT_EQ(SubNode1::id, SId::state4);
    EXPECT_EQ(SubNode1::maxheight, 0);

    FsmStatic<RootNode, TestEvent> fsms;

    EXPECT_EQ(fsms.stateNo, 5);
    EXPECT_EQ(fsms.index2Id[0], SId::root);
    EXPECT_EQ(fsms.index2Id[1], SId::state1);
    EXPECT_EQ(fsms.index2Id[2], SId::state2);
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

    EXPECT_EQ(fsms.maxLevels, 3);

    // Rootnode empty class. But will require alignment 8 + vptr 8.
    EXPECT_EQ(fsms.levels.maxStorageSize[0], 16);

    // State 3 req. 16 bytes + vptr.
    EXPECT_EQ(fsms.levels.maxStorageSize[1], 24);

    // State 3 req. 16 bytes + vptr.
    EXPECT_EQ(fsms.levels.maxStorageSize[2], 24);

    EXPECT_EQ(fsms.levels.storageOffset[0], 0);
    EXPECT_EQ(fsms.levels.storageOffset[1], 16);
    EXPECT_EQ(fsms.levels.storageOffset[2], 40);
    EXPECT_EQ(fsms.levels.storageOffset[3], 64);
}

// Test maker function. Make sure we can build states using the Maker class.
TEST(StateChart, testMaker)
{
	struct TEvent
	{
		int e;
	};

	struct STest
	{
		STest(int i_, double j_, std::string s_) : i(i_), j(j_), s(s_) {}
		int i;
		double j;
		std::string s;

		bool event(const TEvent& e)
		{
			return i == e.e;
		}
		~STest()
		{
			i = 10;
		}
	};


	Maker<STest, TEvent, int, double, string> maker(4, 5.0, "rewq");

	aligned_storage_t<sizeof (Model<STest, TEvent>)> storage[1];

	auto i = maker(storage);

	// The maker does an placement new on the storage. Hence there should
	// be a proper Model object in our storage.
	EXPECT_EQ(static_cast<void*>(i), static_cast<void*>(storage));
	auto* t = static_cast<Model<STest, TEvent>*>(static_cast<void*>(storage));

	EXPECT_EQ(t->state.i, 4);
	EXPECT_EQ(t->state.j, 5.0);
	EXPECT_EQ(t->state.s, "rewq");
	t->~Model<STest, TEvent>();

	//Note: suspicious. Prob. UB.
	EXPECT_EQ(t->state.i, 10);
}

TEST(StateChart, testAlignment)
{
	struct TEvent
	{
		int e;
	};
    // Vanilla FSM. Can declare a root node.
    using RootNode = FsmNode<State<SId::root, RootState>,
    		State<SId::state1, S<1>>,
			State<SId::state2, S<3>>
    >;

    using FsmS = FsmStatic<RootNode, TEvent>;
    // Assuming 8 byte alignment for natural alignment.
    EXPECT_EQ(FsmS::addAlignment(0), 0);
    EXPECT_EQ(FsmS::addAlignment(1), 8);
    EXPECT_EQ(FsmS::addAlignment(7), 8);
    EXPECT_EQ(FsmS::addAlignment(8), 8);
    EXPECT_EQ(FsmS::addAlignment(9), 16);
    EXPECT_EQ(FsmS::addAlignment(16), 16);

    FsmStorage<FsmS> storage;
    // Root req 8 + 8 = 16 bytes, S<3> req 12 -> 16 + 8 = 24 bytes.
    EXPECT_EQ(storage.size(), 16 + 24); // Root and State require 16 bytes each.

    // Should be able to do placement new into the storage.
    int *val_p = new (storage.begin()) int(123);
    EXPECT_EQ(*val_p, 123);
}

TEST(StateChart, testFsm)
{
	struct TEvent
	{
		int e;
	};
    // Vanilla FSM. Can declare a root node.
    using RootNode = FsmNode<State<SId::root, RootState>,
    		State<SId::state1, S<1>>,
			State<SId::state2, S<2>>
    >;

    Fsm<RootNode, TEvent> fsm;
    fsm.post(TEvent{0});

}
int
main(int ac, char* av[])
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
