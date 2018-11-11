/*
 * StateChart.h
 *
 *  Created on: 12 nov. 2016
 *      Author: mikaelr
 */

#ifndef SRC_STATECHART_STATECHART_H_
#define SRC_STATECHART_STATECHART_H_

#include <array>

// Collector class of a State type and the identifier.
template<typename StateType_, auto id_>
class State
{
public:
	using StateId = decltype(id_);
	using StateType = StateType_;
	static constexpr StateId id = id_;
	static constexpr size_t area = 1;
	enum {
	    subStateNo = 0
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

// A node in the area calculation tree.
template<typename State, typename ...Nodes>
class FsmArea;

template<typename State, typename ...Nodes>
class FsmArea
{
public:
  enum {
    // Note: fold expression (C++17)
    subStateNo = sizeof...(Nodes)
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

template<typename Node, typename ...Nodes>
struct TraverseNodes;

template<typename ...N>
struct SubNodes_
{};

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
	template<typename Array>
	static constexpr size_t writeLevelIndex(Array& levelArray, StateIndex baseOffset, size_t level)
	{
		auto t1 = TraverseNodes<Node>::writeLevelIndex(levelArray, baseOffset, level);
		auto t2 = TraverseNodes<Nodes...>::writeLevelIndex(levelArray, baseOffset + Node::area, level);
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

template<typename StateType_, auto id_>
struct TraverseNodes<State<StateType_, id_>>
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
	template<typename Array>
	static constexpr size_t writeLevelIndex(Array& levelArray, StateIndex baseOffset, size_t level)
	{
		levelArray[ baseOffset.get() ] = level;
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
		// Note: This temporary seems to be needed to avoid miscalculating the indexes.
		// It sure looks like a compiler bug...
		// At the same time, same behavior in clang-6/gcc-8
		const StateIndex bo = baseOffset;
		// TraverseNodes<Nodes...>::writeParentIndex(parentArray, baseOffset + 1ul, baseOffset);
		TraverseNodes<Nodes...>::writeParentIndex(parentArray, bo + 1ul, bo);
	}

	template<typename Array>
	static constexpr size_t writeLevelIndex(Array& levelArray, StateIndex baseOffset, size_t level)
	{
		levelArray[ baseOffset.get() ] = level;
		return TraverseNodes<Nodes...>::writeLevelIndex(levelArray, baseOffset + 1ul, level + 1);
	}
};

template<typename State_, typename ...Nodes>
class FsmNode
{
public:
  using State = State_;
  using Node = FsmNode<State, Nodes...>;
  using SubNodes = SubNodes_<Nodes...>;

  enum {
    area = FsmArea<State, Nodes...>::area,
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

	using BuilderFkn = void (*)(char*, int);
	using StateId = typename Root::StateId;

	static auto constexpr initIndex2Id()-> std::array<StateId, stateNo>
	{
		std::array<StateId, stateNo> res = {};
		TraverseNodes<Root>::writeIndex2Id(res, StateIndex(0));
		return res;
	}
	static auto constexpr initParentIndex()-> std::array<StateIndex, stateNo>
	{
		std::array<StateIndex, stateNo> res;
		for(auto& i : res) i = StateIndex(0xff); // For debug.
		TraverseNodes<Root>::writeParentIndex(res, StateIndex(0), StateIndex(0));
		return res;
	}

	struct Level {
		size_t maxLevel = 0;
		std::array<size_t, stateNo> levelIndex = {};
	};

	static auto constexpr initLevels()-> Level
	{
		Level res;
		// for(auto& i : res.levelIndex) i = 0xff; // For debug.
		res.maxLevel = TraverseNodes<Root>::writeLevelIndex(res.levelIndex, StateIndex(0), 0);
		return res;
	}

	static auto constexpr initId2Builder()-> std::array<BuilderFkn, stateNo>
	{
		return {};
	}

	static constexpr const Level levels = initLevels();

	//
	static constexpr const std::array<BuilderFkn, stateNo> id2Builder = initId2Builder();

	// Translation from index to stateId.
	static constexpr const std::array<StateId, stateNo> index2Id = initIndex2Id();

	// Given a state index return the index to the parent.
	static constexpr const std::array<StateIndex, stateNo> parentIndex = initParentIndex();


	// Given a state index return the index to the parent.
	static constexpr const std::array<size_t, stateNo>& levelIndex = levels.levelIndex;

	// Maximum depth of the FSM.
	static constexpr const size_t maxLevel = levels.maxLevel;

	// Storage depth for a state stack. (includes the root state).
	static constexpr const size_t maxStackSize = levels.maxLevel + 1;
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
