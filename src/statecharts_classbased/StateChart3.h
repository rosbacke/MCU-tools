/*
 * StateChart.h
 *
 *  Created on: 12 nov. 2016
 *      Author: mikaelr
 */

#ifndef SRC_STATECHART_STATECHART_H_
#define SRC_STATECHART_STATECHART_H_

#include <array>
#include <tuple>

template<typename State, typename Event, typename ...Args>
class Maker;

template<class Event>
class ModelIface
{
public:
	virtual bool event(const Event& e)=0;
	virtual ~ModelIface() {};
};

template<typename State_, typename Event_>
class Model : public ModelIface<Event_>
{
public:
	using Event = Event_;
	using State = State_;

	template<typename ...Args>
	explicit Model(Args... args) : state(args...)
	{}

	bool event(const Event& e) override
	{
		return state.event(e);
	}

	// TODO: Better type for fsm.
	void setFsm(void* fsm)
	{
		state.setFsm(fsm);
	}
	State state;
};

// Collector class of a State type and the identifier.
template<auto id_, typename StateType_, typename Maker_ = Maker<StateType_, int>>
class State
{
public:
	using StateId = decltype(id_);
	using StateType = StateType_;
	using Maker = Maker_;
	static constexpr StateId id = id_;
	static constexpr size_t area = 1;
	enum {
	    subStateNo = 0,
		maxheight = 0
	};
};

// Explicit type for indexing the Fsm array. Help the compiler keep track
// of used indexes.
struct StateIndex
{
	constexpr StateIndex() = default;
	explicit constexpr StateIndex(std::size_t i) : index(i) {}

	constexpr size_t get() const
	{ return index; }
	constexpr StateIndex operator+(size_t i) const
	{ StateIndex t(index + i); return t; }
	constexpr bool operator==(int i) const
	{ return index == i; }

	std::size_t index = 0;
};

// A Node in the FSM tree.
template<typename State, typename ...Nodes>
class FsmNode;

struct MaxOp
{
	explicit MaxOp(size_t val_) : val(val_) {}
	MaxOp operator^(MaxOp mb)
	{
		return MaxOp(std::max(val, mb.val));
	}
	size_t val = 0;
};

template<typename State, typename ...Nodes>
class FsmArea
{
public:
  enum {
	// Number of sub states for this particular node.
    // Note: fold expression (C++17)
    subStateNo = sizeof...(Nodes),

	// Reciprocal value of the level
	maxheight = 1 + (... ^ Nodes::maxheight)
  };
  static constexpr size_t area = 1 + (... + Nodes::area);
};

// Helper struct to get access to subtypes of a node.
// index : downcount to keep track of how far into the type to go.
// Advances into the parameter pack.
template<size_t index, typename Node, typename ...Nodes>
struct SubTypeImpl;

template<typename Node, typename ...Nodes>
struct SubTypeImpl<0, Node, Nodes...>
{
  using Result = Node;
};

template<size_t index, typename Node, typename ...Nodes>
struct SubTypeImpl
{
  using Result = typename SubTypeImpl<index-1, Nodes...>::Result;
};

// Helper struct to implement recursive functions on the FsmNode graph.
template<typename Node, typename ...Nodes>
struct TraverseNodes
{
	template<typename Array>
	static constexpr void writeIndex2Id(Array& array, StateIndex baseOffset)
	{
		TraverseNodes<Node>::writeIndex2Id(array, baseOffset);
		TraverseNodes<Nodes...>::writeIndex2Id(array, baseOffset + Node::area);
	}

	template<typename Array>
	static constexpr void writeParentIndex(Array& parentArray, StateIndex baseOffset, StateIndex parent)
	{
		TraverseNodes<Node>::writeParentIndex(parentArray, baseOffset, parent);
		TraverseNodes<Nodes...>::writeParentIndex(parentArray, baseOffset + Node::area, parent);
	}
	template<typename Array, typename StoreArray>
	static constexpr size_t writeLevelIndex(Array& levelArray, StoreArray& sArray, StateIndex baseOffset, size_t level)
	{
		auto t1 = TraverseNodes<Node>::writeLevelIndex(levelArray, sArray, baseOffset, level);
		auto t2 = TraverseNodes<Nodes...>::writeLevelIndex(levelArray, sArray, baseOffset + Node::area, level);
		return t1 < t2 ? t2 : t1;
	}

	static constexpr size_t childOffset(size_t childIndex)
	{
		if (childIndex == 0)
			return 0;
		return Node::area + TraverseNodes<Nodes...>::childOffset(childIndex + -1);
	}
	template<size_t childIndex>
	static constexpr size_t childOffset()
	{
		if constexpr (childIndex == 0)
			return 0;
		else
			return Node::area + TraverseNodes<Nodes...>::template childOffset<childIndex - 1>();
	}
};

template<auto id_, typename StateType_>
struct TraverseNodes<State<id_, StateType_>>
{
	template<typename Array>
	static constexpr void writeIndex2Id(Array& array, StateIndex baseOffset)
	{
		array[ baseOffset.get() ] = id_;
	}

	template<typename Array>
	static constexpr void writeParentIndex(Array& parentArray, StateIndex baseOffset, StateIndex parent)
	{
		parentArray[ baseOffset.get() ] = parent;
	}
	template<typename Array, typename StoreArray>
	static constexpr size_t writeLevelIndex(Array& levelArray, StoreArray& sArray, StateIndex baseOffset, size_t level)
	{
		levelArray[ baseOffset.get() ] = level;
		auto t = sArray[ level ];
		sArray[ level ] = t; //std::max(t, sizeof );
		return level;
	}

	static constexpr size_t childOffset(size_t childIndex)
	{
		return 0;
	}
	template<size_t childIndex>
	static constexpr size_t childOffset()
	{
		static_assert(childIndex == 0, "Child index out of bounds");
		return 0;
	}
};

template<typename State, typename ...Nodes>
struct TraverseNodes<FsmNode<State, Nodes...>>
{
	template<typename Array>
	static constexpr void writeIndex2Id(Array& array, StateIndex baseOffset)
	{
		array[ baseOffset.get() ] = State::id;
		TraverseNodes<Nodes...>::writeIndex2Id(array, baseOffset + 1ul);
	}

	template<typename Array>
	static constexpr void writeParentIndex(Array& parentArray, StateIndex baseOffset, StateIndex parent)
	{
		parentArray[ baseOffset.get() ] = parent;
		TraverseNodes<Nodes...>::writeParentIndex(parentArray, baseOffset + 1ul, baseOffset);
	}

	template<typename Array, typename StoreArray>
	static constexpr size_t writeLevelIndex(Array& levelArray, StoreArray& sArray, StateIndex baseOffset, size_t level)
	{
		levelArray[ baseOffset.get() ] = level;
		auto t = sArray[  baseOffset.get() ];
		sArray[ level ] = t; //std::max(t, sizeof );
		return TraverseNodes<Nodes...>::writeLevelIndex(levelArray, sArray, baseOffset + 1ul, level + 1);
	}
};

template<typename State_, typename ...Nodes>
class FsmNode
{
public:
  using State = State_;
  using Node = FsmNode<State, Nodes...>;

  enum {
    area = FsmArea<State, Nodes...>::area,
	maxheight = FsmArea<State, Nodes...>::maxheight,
    subStateNo = FsmArea<State, Nodes...>::subStateNo,
  };
  using StateId = typename State::StateId;
  static constexpr StateId id = State::id;

  using StateType = typename State::StateType;

  // Offset for a child into the linear storage.
  static constexpr StateIndex childOffset(size_t childIndex)
  {
	  return StateIndex(TraverseNodes<Nodes...>::childOffset(childIndex) + 1);
  }
  template<size_t childIndex>
  static constexpr StateIndex childOffset()
  {
	  return StateIndex(TraverseNodes<Nodes...>::template childOffset<childIndex>() + 1);
  }

  // Get the type of for a particular storage at some index.
  template<size_t index>
  using SubType = typename SubTypeImpl<index, Nodes...>::Result;
};

// Static setup for the FSM stateschart
template<typename Root>
class FsmStatic
{
public:

	// Total number of states.
	static const constexpr std::size_t stateNo = Root::area;
	static const constexpr std::size_t maxLevels = Root::maxheight + 1;

	using BuilderFkn = void (*)(char*, int);
	using StateId = typename Root::StateId;

	struct Level {
		std::array<size_t, stateNo> levelIndex = {};
		std::array<StateIndex, stateNo> parentIndex = {};
		std::array<StateId, stateNo> index2Id = {};
		std::array<size_t, maxLevels> maxStorageSize = {};
	};

	static auto constexpr initLevels()-> Level
	{
		Level res;
		TraverseNodes<Root>::writeLevelIndex(res.levelIndex, res.maxStorageSize, StateIndex(0), 0);
		TraverseNodes<Root>::writeParentIndex(res.parentIndex, StateIndex(0), StateIndex(0));
		TraverseNodes<Root>::writeIndex2Id(res.index2Id, StateIndex(0));
		return res;
	}

	static constexpr const Level levels = initLevels();

	//
	// static constexpr const std::array<BuilderFkn, stateNo> id2Builder = initId2Builder();

	// Translation from index to stateId.
	static constexpr const std::array<StateId, stateNo>& index2Id = levels.index2Id;

	// Given a state index return the index to the parent.
	static constexpr const std::array<StateIndex, stateNo>& parentIndex = levels.parentIndex;

	// Given a state index return the index to the parent.
	static constexpr const std::array<size_t, stateNo>& levelIndex = levels.levelIndex;

	// Maximum depth of the FSM.
	static constexpr const size_t maxLevel = levels.maxLevel;

	// Storage depth for a state stack. (includes the root state).
	// static constexpr const size_t maxStackSize = levels.maxLevel + 1;
};

namespace detail {
template <class T, class Storage, class Tuple, std::size_t... I>
constexpr T* place_new_from_tuple_impl( Storage mem, Tuple&& t, std::index_sequence<I...> )
{
  return new (mem) T(std::get<I>(std::forward<Tuple>(t))...);
}
} // namespace detail

template <class T, class Storage, class Tuple>
constexpr T* place_new_from_tuple( Storage mem, Tuple&& t )
{
    return detail::place_new_from_tuple_impl<T>(mem, std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

// Create a functor to construct state instances using placement new.
template<typename State, typename Event, typename ...Args>
class Maker
{
public:
	Maker(Args ... args) : arg(args...) {};

	template<typename Storage>
	ModelIface<Event>* operator()(Storage mem)
	{
		return static_cast<ModelIface<Event>*>(place_new_from_tuple<Model<State, Event>>( mem, arg ));
	}
private:
	std::tuple<Args...> arg;
};



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
 * The statechart allocates memory for each level and this is reused for each9999
 * state change by using placement new/delete. This should ensure deterministic
 * timing for all state changes.
 */


#endif /* SRC_STATECHART_STATECHART_H_ */
