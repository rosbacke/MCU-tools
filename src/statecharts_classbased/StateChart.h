/*
 * StateChart.h
 *
 *  Created on: 12 nov. 2016
 *      Author: mikaelr
 */

#ifndef SRC_STATECHART_STATECHART_H_
#define SRC_STATECHART_STATECHART_H_

/**
 * The statechart module implement a hierarchical state machine. (Fsm)
 * Basic idea is to use classes as states. The constructor/destructor
 * implement entry/exit functions for the states.
 * It is hierarchical so that a state can be a sub state to a base state.
 * Note, this is _not_ inheritance. The base state have a much larger
 * lifetime than the sub state.
 *
 * The unit tests in fsm_test.cpp is fairly documented for an implementation
 * example.
 *
 * An Fsm is created by creating a class inheriting from FsmBase<...>
 * This requires the following support types:
 * - A description class tying all relevant types together. (Desc)
 * - An Event class which is posted to the FSM.
 * - An enum class enumerating all the states.
 * - A function setting up the state hierarchy.
 *
 * In addition we have one class for the action FSM and
 * one class for each implemented state.
 *
 * In the state setup function you need to call 'addState' for each
 * state that belongs to the state machine. Here you specify the State
 * class and the class of a possible parent state.
 *
 * Each state inherits from the class BaseState<Desc, StateId>.
 * All events are delivered through the function 'event' that needs to be
 * implemented.
 *
 * Each state has a particular level given by the number of transitive parents.
 * For each level there is at most 1 active state at any time.
 * The statechart allocates memory for each level and this is reused for each
 * state change by using placement new/delete. This should ensure deterministic
 * timing for all state changes.
 */

#include "VecQueue.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include <cassert>
#include <iostream>

//#include "utility/Log.h"

class FsmBaseBase;

/**
 * Bundle of arguments passed from the FSM down to StateBase when constructing
 * a state.
 */
struct StateArgs
{
    StateArgs(FsmBaseBase* fsmBase) : m_fsmBase(fsmBase)
    {
    }

    FsmBaseBase* m_fsmBase;
};

/**
 * Base class for all state classes. Templated on the FsmDesc allow easy
 * access to additional functions on the user FSM object with correct types.
 *
 * Supplies a number of helper function to be available in state context
 * to ease user interaction. Constrain move/copy due to using placement
 * construction upon state entry.
 *
 * Note that 'event' function is not handled here as a virtual function.
 * Rather it is done internally via the Model class and direct call
 * on the member. Hence the state classes do not need a vtable.
 */
template <typename FsmDesc, typename FsmDesc::StateId stId>
class StateBase
{
    // No moving these around due to placement new.
    StateBase() = delete;
    StateBase(const StateBase& s) = delete;
    StateBase(StateBase&& s) = delete;
    StateBase& operator=(const StateBase& s) = delete;
    StateBase& operator=(StateBase&& s) = delete;

  public:
    using FsmDescription = FsmDesc;
    using StateId = typename FsmDesc::StateId;
    using Fsm = typename FsmDesc::Fsm;

    static constexpr const typename FsmDesc::StateId stateId = stId;

    explicit StateBase(const StateArgs& args)
        : m_fsm(static_cast<Fsm*>(args.m_fsmBase))
    {
    }

    /**
     * Perform a transition to a new state.
     * @param id state id to transition to.
     */
    void transition(StateId id)
    {
        m_fsm->m_base.transition(static_cast<int>(id));
    }

    /**
     * Transition, given target state type.
     */
    template <typename TargetState>
    void transition();

    /// Reference to the custom state machine object.
    Fsm& fsm()
    {
        return *m_fsm;
    }

    /// Reference to the custom state machine object.
    const Fsm& fsm() const
    {
        return *m_fsm;
    }

    // Get a reference to the parent object.
    // Semantics of the statechart guarantee that the reference
    // is valid as long as this state is valid.
    template <class ParentState>
    ParentState& parent();

  private:
    Fsm* m_fsm;
};

/**
 * Base for 'StateModel' class. StateModel keeps a State as a member and
 * introduce inheritance for event passing. Purpose of ModelBase is to get a
 * uniform handling of destruction.
 */
class ModelBase
{
  public:
    virtual ~ModelBase()
    {
    }
};

/**
 * Keep track of the state hierarchy. One object of this class
 * exist for each type of FSM that is created.
 */
class FsmStaticData
{
  public:
    explicit FsmStaticData(int stateNo) : m_states(stateNo)
    {
    }

    static const constexpr int nullStateId = -1;

    /**
     * Signature for the creator function for a particular state.
     * Called when entering a new state to construct the state object.
     * @param store  A memory array large enough to create the object on.
     * @param fsm Pointer to the current fsm.
     * @return Pointer to the newly created Model object.
     */
    using CreateFkn = ModelBase* (*)(char* store, FsmBaseBase* fsm);

    // Collection of meta data for one state.
    struct StateInfo
    {
        StateInfo() : m_maker(nullptr)
        {
        }
        template <class StateId>
        StateInfo(StateId parentId, int level, const CreateFkn& fkn)
            : m_parentId(static_cast<int>(parentId)), m_level(level),
              m_maker(fkn)
        {
        }
        int m_parentId;
        int m_level;
        CreateFkn m_maker;
    };

    const StateInfo* findState(int id) const
    {
        const auto& el = m_states[id];
        return el.m_maker == nullptr ? nullptr : &el;
    }

    void addStateBase(int stateId, int parentId, size_t size, CreateFkn fkn);

    const std::vector<size_t>& sizes() const
    {
        return m_objectSizes;
    }

  private:
    // Store of the information structure for all the states.
    std::vector<StateInfo> m_states;

    // Store the maximum size needed for each level to construct the objects.
    std::vector<size_t> m_objectSizes;
};

class FsmBaseMember
{
  public:
    using StateInfo = FsmStaticData::StateInfo;
    FsmBaseMember(const FsmStaticData& setup) : m_setup(setup)
    {
    }

    ~FsmBaseMember()
    {
        cleanup();
    }

    void transition(int id);

    void setStartState(int id, FsmBaseBase* hsm);

    const StateInfo* activeInfo() const
    {
        return m_currentInfo;
    }

    // Return the working area for a particular level.
    ModelBase* getModelBase(int level)
    {
        return m_stackFrames[level].m_activeState.get();
    }

    void possiblyDoTransition(FsmBaseBase* fbb);

  private:
    // Implement placement destruction for the smart pointer.
    struct PlacementDestroyer
    {
        void operator()(ModelBase* p)
        {
            p->~ModelBase();
        }
    };

    // Smart pointer type implementing the placement delete.
    using UniquePtr = std::unique_ptr<ModelBase, PlacementDestroyer>;

    // Structure for one level of the state stack.
    struct LevelData
    {
        explicit LevelData(size_t size)
            : m_stateInfo(nullptr), m_activeState(nullptr),
              m_stateStorage(std::make_unique<char[]>(size))
        {
        }

        // Active meta information pointer.
        const StateInfo* m_stateInfo;

        // Current active state for this level.
        UniquePtr m_activeState;

        // Storage for the current active State object.
        std::unique_ptr<char[]> m_stateStorage;
    };

    // Do final exit handlers prior to destructing the fsm.
    void cleanup();

    // Do initial entry calls when starting the fsm.
    void setupTransition(const StateInfo* nextInfo, FsmBaseBase* fsm);

    // Do a normal state 2 state transition.
    void doTransition(const StateInfo* nextInfo, FsmBaseBase* fsm);

    void doEntry(const StateInfo* newState, FsmBaseBase* fsm);

    void doExit(const StateInfo* currState);

    const StateInfo*& stateInfo(int level)
    {
        return m_stackFrames[level].m_stateInfo;
    }
    std::vector<LevelData> m_stackFrames;

    const StateInfo* m_currentInfo = nullptr;

    const FsmStaticData& m_setup;

    int m_nextState = FsmStaticData::nullStateId;
};

class FsmBaseBase
{
  protected:
    FsmBaseBase(const FsmStaticData& setup) : m_base(setup)
    {
    }

    ~FsmBaseBase()
    {
    }

    FsmBaseMember m_base;
};

template <class Event>
class EventInterface : public ModelBase
{
  public:
    ~EventInterface() override
    {
    }
    virtual bool event(const Event& ev) = 0;
};

template <class FsmDesc, class St>
class StateModel : public EventInterface<typename FsmDesc::Event>
{
  public:
    StateModel(StateArgs args) : m_state(args)
    {
    }
    bool event(const typename FsmDesc::Event& event) override
    {
        return m_state.event(event);
    }
    ~StateModel() override
    {
    }

    St m_state;
};

/**
 * Helper class for setting up the FSM state description table at
 * startup. Capture type information and forward it to the state table
 * at construction.
 */
template <class FsmDesc>
class FsmSetup
{
  public:
    FsmSetup() : m_data(static_cast<int>(FsmDesc::StateId::stateIdNo))
    {
        FsmDesc::setupStates(*this);
    }

    /**
     * Add a bottom level state to the FSM. State will haVE 'level' == 0.
     * @param State    Type name for the class that implement the state.
     *                 Must inherit StateBase<...>.
     */
    template <class State>
    void addState()
    {
        addState<State, State>();
    }

    /**
     * Add a state to the FSM. State will have 'level one above the parent
     * state.
     * @param State    Type name for the class that implement the state.
     *                 Must inherit StateBase<...>.
     * @param ParentState State type for the parent state.
     */
    template <class State, class ParentState>
    void addState()
    {
        // StateId parentId = ParentState::stateId;
        static_assert(static_cast<int>(State::stateId) !=
                          FsmStaticData::nullStateId,
                      "state id is reserved.");
        auto makerFkn = [](char* store, FsmBaseBase* fsm) -> ModelBase* {
            auto p = new (store) StateModel<FsmDesc, State>(StateArgs(fsm));
            return static_cast<ModelBase*>(p);
        };
        auto size = sizeof(StateModel<FsmDesc, State>);
        m_data.addStateBase(static_cast<int>(State::stateId),
                            static_cast<int>(ParentState::stateId), size,
                            makerFkn);
    }

    const FsmStaticData& data()
    {
        return m_data;
    }

  private:
    FsmStaticData m_data;
};

template <class Event>
class FsmBaseEvent : public FsmBaseBase
{
  public:
    FsmBaseEvent(const FsmStaticData& setup) : FsmBaseBase(setup)
    {
    }
    void postEvent(const Event& ev)
    {
        // LOG_DEBUG << "Post event:" << int(ev.m_id);
        bool empty = m_eventQueue.empty();
        m_eventQueue.push(ev);
        if (empty)
        { // Nobody else is currently processing events.
            while (!m_eventQueue.empty())
            {
                // Keep a local copy in case the vector reallocate during the
                // event processing. (due to internal event posting.)
                Event ev = m_eventQueue.front();
                processEvent(ev);
                m_eventQueue.pop();
            }
        }
    }

    void processEvent(const Event& ev)
    {
        auto activeInfo = m_base.activeInfo();
        if (!activeInfo)
            return;

        bool eventHandled = false;
        int level = activeInfo->m_level;
        while (!eventHandled && level >= 0)
        {
            auto activeState = m_base.getModelBase(level);
            eventHandled = emitEvent(activeState, ev);
            level--;
        }
        m_base.possiblyDoTransition(this);
    }

    static bool emitEvent(ModelBase* sbb, const Event& ev)
    {
        return static_cast<EventInterface<Event>*>(sbb)->event(ev);
    }

  private:
    VecQueue<Event> m_eventQueue;
};

/**
 * Base class for the custom FSM.
 */
template <class FsmDesc>
class FsmBase : public FsmBaseEvent<typename FsmDesc::Event>
{
  public:
    using StateId = typename FsmDesc::StateId;
    using Event = typename FsmDesc::Event;
    using FsmDescription = FsmDesc;
    static const constexpr int stateNo = static_cast<int>(StateId::stateIdNo);

    template <class FD, typename FD::StateId id>
    friend class StateBase;

    FsmBase() : FsmBaseEvent<Event>(instance())
    {
    }

    ~FsmBase() = default;

    /**
     * Set start state and perform initial jump to that state.
     * After this, it is legal to send events into the HSM.
     */
    void setStartState(StateId id)
    {
        FsmBaseBase::m_base.setStartState(static_cast<int>(id), this);
    }

  private:
    static const FsmStaticData& instance()
    {
        static FsmSetup<FsmDesc> base;
        return base.data();
    }
};

template <typename FsmDesc, typename FsmDesc::StateId stId>
template <class ParentState>
ParentState&
StateBase<FsmDesc, stId>::parent()
{
    using StateInfo = FsmStaticData::StateInfo;
    const StateInfo* myInfo = fsm().m_base.activeInfo();
    if (myInfo->m_level == 0)
        throw std::runtime_error("No parent available for root states.");

    StateId parentId = ParentState::stateId;
    if (parentId != static_cast<StateId>(myInfo->m_parentId))
        throw std::runtime_error("Type mismatch for parent state.");

    ModelBase* mb = fsm().m_base.getModelBase(myInfo->m_level - 1);
    auto* sm = static_cast<StateModel<FsmDesc, ParentState>*>(mb);
    return sm->m_state;
}

template <typename FsmDesc, typename FsmDesc::StateId stId>
template <typename TargetState>
void
StateBase<FsmDesc, stId>::transition()
{
    static constexpr const StateId targetId = TargetState::stateId;
    m_fsm->m_base.transition(static_cast<int>(targetId));
}

#endif /* SRC_STATECHART_STATECHART_H_ */
