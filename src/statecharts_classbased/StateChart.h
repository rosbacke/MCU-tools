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
 *
 * In addition we have one class for the action FSM and
 * one class for each implemented state.
 *
 * In the constructor of the FSM you need to call 'addState' for each
 * state that belongs to the state machine. Here you specify the the State
 * class and the id of a possible parent state.
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
 *
 */

#include "VecQueue.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include <cassert>

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
 * Base class for all state classes. Templated on the HSMto allow easy
 * access to additional functions on the user FSM object.
 *
 * Supplies a number of helper function to be available in state context
 * to ease user interaction.
 *
 * Note that 'event' function is not handled here as a virtual function.
 * Rather it is done internally via the Model class and direct call
 * on the member.
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

    	// m_fsm->transition(id);
    }

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
 * Helper class. Keeps track of all state information objects for at particular
 * FSM type.
 */
class FsmSetupBase
{
  public:
	static const constexpr int nullStateId = -1;
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

    using CreateFkn = std::function<ModelBase*(char* store, FsmBaseBase* fsm)>;

    // Structure for one level of the state stack.
    struct LevelData
    {
        // Current active state for this level.
        UniquePtr m_activeState;

        // Storage for the current active State object.
        std::vector<char> m_stateStorage;
    };

    // Collection of meta data for one state.
    struct StateInfo
    {
        template <class StateId>
        StateInfo(StateId id, StateId parentId, int level, size_t stateSize,
                  const CreateFkn& fkn)
            : m_id(static_cast<int>(id)),
              m_parentId(static_cast<int>(parentId)), m_level(level),
              m_stateSize(stateSize), m_maker(fkn)
        {
        }

        int m_id;
        int m_parentId;
        int m_level;
        size_t m_stateSize;
        CreateFkn m_maker;
    };

    const StateInfo* findState(int id) const
    {
        auto i = std::find_if(m_states.begin(), m_states.end(),
                              [&](const auto& el) -> bool {
                                  return el.m_id == static_cast<int>(id);
                              });
        return i != m_states.end() ? &(*i) : nullptr;
    }

    template <class StateId>
    const StateInfo* findState(StateId id) const
    {
        return findState(static_cast<int>(id));
    }

    void addStateBase(int stateId, int parentId, size_t size, CreateFkn fkn);

    // Store of the information structure for all the states.
    std::vector<StateInfo> m_states;

    // Maximum level represented in the statechart.
    int m_maxLevel = 0;
};

class FsmBaseSupport
{
  public:
    using StateInfo = FsmSetupBase::StateInfo;
    FsmBaseSupport(FsmSetupBase& setup) : m_setup(setup)
    {
    }

    virtual ~FsmBaseSupport()
    {
    }

    void cleanup();

    void transition(int id);

    void setStartState(int id, FsmBaseBase* hsm);

    void prepareTransition(FsmBaseBase* fbb);

  private:
    void doTransition(const StateInfo* nextInfo, FsmBaseBase* fsm);

    void populateNextInfos(const StateInfo* nextInfo);

    size_t findFirstThatDiffer();

    void doExit(size_t bl);

    void doEntry(size_t targetLevel, FsmBaseBase* fsm);

  public:
    std::vector<FsmSetupBase::LevelData> m_stackFrames;
    std::vector<const StateInfo*> m_currentInfos;

    FsmSetupBase& m_setup;

  private:
    std::vector<const StateInfo*> m_nextInfos;
    int m_nextState = FsmSetupBase::nullStateId;
};

class FsmBaseBase
{
  protected:
    FsmBaseBase(FsmSetupBase& setup) : m_base(setup)
    {
    }

    ~FsmBaseBase()
    {
        m_base.cleanup();
    }

    FsmBaseSupport m_base;
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

  private:
    St m_state;
};

template <class Event>
class FsmBaseEvent : public FsmBaseBase
{
  public:
    FsmBaseEvent(FsmSetupBase& setup) : FsmBaseBase(setup)
    {
    }
    void postEvent(const Event& ev)
    {
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

    static bool emitEvent(ModelBase* sbb, const Event& ev)
    {
        return static_cast<EventInterface<Event>*>(sbb)->event(ev);
    }

    void processEvent(const Event& ev)
    {
        bool eventHandled = false;
        int level = m_base.m_currentInfos.size();
        if (level == 0)
        {
            return;
        }
        do
        {
            level--;
            auto activeState = m_base.m_stackFrames[level].m_activeState.get();
            eventHandled = emitEvent(activeState, ev);

        } while (!eventHandled && level > 0);
        m_base.prepareTransition(this);
    }

  private:
    VecQueue<Event> m_eventQueue;
};

/**
 * Base class based on the state description type. This level
 * is suitable for declaring a static FsmSetupBase. (To be implemented.)
 */
template <class FsmDesc>
class FsmBase : public FsmBaseEvent<typename FsmDesc::Event>
{
    using MyFsm = typename FsmDesc::Fsm;

  public:
    using StateId = typename FsmDesc::StateId;
    using Event = typename FsmDesc::Event;
    using FsmDescription = FsmDesc;

    template<class FD, typename FD::StateId id>
    friend class StateBase;

    FsmBase() : FsmBaseEvent<Event>(instance())
    {
    }

    ~FsmBase()
    {
    }

    /**
     * Set start state and perform initial jump to that state.
     * After this, it is legal to send events into the HSM.
     */
    void setStartState(StateId id)
    {
        FsmBaseBase::m_base.setStartState(static_cast<int>(id), this);
    }

  protected:
    /**
     * Add a root state to the HSM. Needs to be called before starting
     * the HSM. (Suggest the HSM constructor.)
     * @param State    Type name for the class that implement the state.
     *                 Must inherit StateBase<'HSM'>.
     * @param stateId  State identity number from the range given by StateId.
     */
    template <class State>
    void addState()
    {
        addState<State, State::stateId>();
    }

    /**
     * Add a state to the HSM. Needs to be called before starting
     * the HSM. (Suggest the HSM constructor.)
     * @param State    Type name for the class that implement the state.
     *                 Must inherit StateBase<'HSM'>.
     * @param stateId  State identity number from the range given by StateId.
     * @param parentId State identity of the parent state. If == stateId,
     *                 then this is a root state.
     */
    template <class State, StateId parentId>
    void addState()
    {
		static_assert(static_cast<int>(State::stateId) != FsmSetupBase::nullStateId, "state id is reserved.");
        auto fkn = [&](char* store, FsmBaseBase* fsm) -> ModelBase* {
            auto p = new (store) StateModel<FsmDesc, State>(StateArgs(fsm));
            return static_cast<ModelBase*>(p);
        };
        auto size = sizeof(StateModel<FsmDesc, State>);
        instance().addStateBase(static_cast<int>(State::stateId),
                                static_cast<int>(parentId), size, fkn);
    }

    static FsmSetupBase& instance()
    {
        static FsmSetupBase base;
        return base;
    }
};

#endif /* SRC_STATECHART_STATECHART_H_ */
