/*
 * fsm2_test.cpp
 *
 *  Created on: 30 okt. 2016
 *      Author: mikaelr
 */

#include "StateChart.h"

#include <gtest/gtest.h>

#include <string>

using std::cout;
using std::endl;

namespace
{ // Make sure no other names interfere with testing.

class UserFsm;

// A description class tying together the types needed for our FSM.
class UserFsmDesc
{
  public:
    // Each state is represented by a class and a an enum value.
    // This enum needs exactly one value for each state class.
    enum class StateId
    {
        state1,
        state2,
        state3,
        stateIdNo // Keep this last. Gives the number of states.
    };

    // Needed helper function. Convert each state class into a string
    // for e.g. logging.
    static std::string toString(StateId id);

    // Typename 'Event' indicate type name for events. Primitive types are ok.
    using Event = int;

    // Typename 'Fsm' Indicate the class that implement our FSM.
    // External code will deliver events to this class using postEvent fkn.
    using Fsm = UserFsm;

    // This function needs to exist. It will be called during setup phase
    // and should perform all the needed 'sc.addState<...>()' calls to
    // set up the state hierachy. Forward declared here.
    static void setupStates(FsmSetup<UserFsmDesc>& sc);
};

struct TD
{
    TD() = default;
    bool equal(int c, int d, int e) const
    {
        return construct == c && destruct == d && evCnt == e;
    }

    int construct = 0;
    int destruct = 0;
    int evCnt = 0;
};

// Our FSM class. It needs to inherit from 'FsmBase'.
// FsmBase has the 'postEvent function.
class UserFsm : public FsmBase<UserFsmDesc>
{
  public:
    UserFsm() = default;
    TD td;
};

// Helper 'using' to save some typing.
using StateId = UserFsmDesc::StateId;
// using EventId = TestEvent::TestEventId;

// First user state. Each state needs to inherit from StateBase,
// supplying the description class and state enum value
// for this particular state.
class State1 : public StateBase<UserFsmDesc, StateId::state1>
{
  public:
    // Constructor. Called when the state is entered.
    explicit State1(StateArgs& args) : StateBase(args)
    {
        fsm().td.construct++;
    }

    // Destructor, called when the state is left.
    ~State1()
    {
        fsm().td.destruct++;
    }

    // Event delivery function. Each state needs to implement this.
    // should return 'true' if the event was handled and no more
    // base state is to be called. Return false and the base states will
    // see the event.
    bool event(int ev)
    {
        fsm().td.evCnt++;
        return false;
    }

    const int state1Var = 1;
};

// Helper base class to add common functionality. (and reduce typing.)
// Useful for these tests but not needed i general.
// However, each state class needs to to inherit (transitively) from
// 'StateBase'. This base class helps to reduce the repetitions.
template <StateId id>
class UserBase : public StateBase<UserFsmDesc, id>
{
  public:
    // Introduce the Fsm type from base. Could use 'TestFSM' also.
    // auto solves this now so commented out.
    // using typename StateBase<TestFsmDescription, id>::Fsm;

    // Each state will receive an argument pack when the state is entered
    // (constructed)
    // This needs to be passed on to the base class.
    UserBase(StateArgs& args) : StateBase<UserFsmDesc, id>(args)
    {
    }
};

// Second user state class.
class State2 : public UserBase<StateId::state2>
{
  public:
    explicit State2(StateArgs args) : UserBase(args)
    {
        fsm().td.construct++;
    }

    ~State2()
    {
        fsm().td.destruct++;
    }

    bool event(int ev)
    {
        fsm().td.evCnt++;
        return false;
    }

    const int state2Var = 2;
};

class State3 : public UserBase<StateId::state3>
{
  public:
    explicit State3(StateArgs args) : UserBase(args)
    {
        fsm().td.construct++;
    }

    ~State3()
    {
        fsm().td.destruct++;
    }

    bool event(int ev)
    {
        fsm().td.evCnt++;
        if (ev == 1)
            return true;

        if (ev == 2)
            transition<State1>();

        return false;
    }
    const int state3Var = 3;
};

// Implementation of the state 'toString'.
// If not needed, supply a minimal dummy implementation.
std::string
UserFsmDesc::toString(StateId id)
{
    switch (id)
    {
    case UserFsmDesc::StateId::state1:
        return "state1";
    case UserFsmDesc::StateId::state2:
        return "state2";
    case UserFsmDesc::StateId::state3:
        return "state3";
    case UserFsmDesc::StateId::stateIdNo:
        return "max_num";
    }
    return "";
}

// Implementation of the state registration function.
// Should contain one 'sc.addState<...>() for each state.
// This is done with lazy evaluation and happens when the first
// fsm of a particular type is instantiated.
void
UserFsmDesc::setupStates(FsmSetup<UserFsmDesc>& sc)
{
    sc.addState<State1>();
    sc.addState<State2, State1>();
    sc.addState<State3, State2>();
}

TEST(StateChart, construction)
{
    UserFsm fsm;

    // default contructed fsm does not have a state yet.
    EXPECT_EQ(fsm.currentStateId(), UserFsm::nullStateId());
    EXPECT_TRUE(fsm.td.equal(0, 0, 0));

    // Do destruction of default initialized fsm.
}

TEST(StateChart, test_start_state_1)
{
    UserFsm fsm;

    // Set state1 as start state.
    fsm.setStartState(UserFsm::StateId::state1);

    // default contructed fsm does not have a state yet.
    EXPECT_EQ(fsm.currentStateId(), UserFsm::StateId::state1);
    EXPECT_TRUE(fsm.td.equal(1, 0, 0));

    // Access the current active state.
    const auto* p = fsm.currentState<State1>();
    EXPECT_TRUE(p);
    EXPECT_EQ(p->state1Var, 1);

    // Make sure wrong State gives a nullptr.
    const auto* p2 = fsm.currentState<State2>();
    EXPECT_FALSE(p2);

    // Make sure sub states are not valid here.
    EXPECT_FALSE(fsm.activeState<State3>());
    EXPECT_FALSE(fsm.activeState<State2>());
}

TEST(StateChart, test_start_state_2)
{
    UserFsm fsm;

    // Set state1 as start state.
    fsm.setStartState(UserFsm::StateId::state3);

    // default contructed fsm does not have a state yet.
    EXPECT_EQ(fsm.currentStateId(), UserFsm::StateId::state3);
    EXPECT_TRUE(fsm.td.equal(3, 0, 0));

    // Access the current active state.
    const auto* p = fsm.currentState<State3>();
    EXPECT_TRUE(p);
    EXPECT_EQ(p->state3Var, 3);

    // Make sure wrong State gives a nullptr.
    EXPECT_FALSE(fsm.currentState<State2>());
    EXPECT_FALSE(fsm.currentState<State1>());

    // However, the active state should be valid for all parent states.
    EXPECT_TRUE(fsm.activeState<State2>());
    EXPECT_TRUE(fsm.activeState<State1>());

    EXPECT_EQ(fsm.activeState<State2>()->state2Var, 2);
    EXPECT_EQ(fsm.activeState<State1>()->state1Var, 1);
}

TEST(StateChart, test_event_count1)
{
    UserFsm fsm;

    fsm.setStartState(UserFsm::StateId::state3);
    EXPECT_EQ(fsm.currentStateId(), UserFsm::StateId::state3);
    EXPECT_TRUE(fsm.td.equal(3, 0, 0));

    // Event zero doesn't do anythiung. Should be handled by all states.
    fsm.td = TD{};
    fsm.postEvent(0);
    EXPECT_TRUE(fsm.td.equal(0, 0, 3));

    // Event 1 will return true and not progress to parent states.
    fsm.td = TD{};
    fsm.postEvent(1);
    EXPECT_TRUE(fsm.td.equal(0, 0, 1));

    // Next, check transition. Event 2 will transition from state 3 to 1.
    fsm.td = TD{};
    EXPECT_EQ(fsm.currentStateId(), UserFsm::StateId::state3);
    fsm.postEvent(2);
    EXPECT_EQ(fsm.currentStateId(), UserFsm::StateId::state1);
    fsm.td.equal(0, 2, 3);
}
}
