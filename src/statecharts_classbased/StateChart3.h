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
	enum {
		area = 1,
	    subStateNo = 0
	};
};

// Class containing per state static data.
struct StateData
{};

// Explicit type for indexing the Fsm array.
struct FsmIndex
{
	explicit FsmIndex(std::size_t i) : index(i) {}
	std::size_t index;
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
    area = 1 + (... + Nodes::area),
    subStateNo = sizeof...(Nodes)
  };
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

// A node in the area calculation tree.
template<typename State, typename ...Nodes>
struct FsmIndex2Area;

template<typename State, typename Node, typename ...Nodes>
struct FsmIndex2Area<State, Node, Nodes...>
{
	template<typename Array>
	void constexpr write(Array& arr, size_t offset)
	{
		arr[offset] = State::stateId;
		FsmIndex2Area<State, Nodes...>::write(arr, offset + Node::area);
	}
};

template<typename State, typename Node>
struct FsmIndex2Area<State, Node>
{
	template<typename Array>
	void constexpr write(Array& arr, size_t offset)
	{
		arr[offset] = State::stateId;
	}
};

template<typename State>
struct FsmIndex2Area<State>
{
	template<typename Array>
	void constexpr write(Array& arr, size_t offset)
	{
		arr[offset] = State::stateId;
	}
};

template<typename Node, typename ...Nodes>
struct NodeSibTraverse;

template<typename ...N>
struct SubNodes_
{};

// Helper struct to implement recursive functions on the FsmNode graph.
template<typename Node, typename ...Nodes>
struct NodeSibTraverse
{
	static constexpr size_t getN(size_t index)
	{
	  if (index < 1) {
	      return 0;
	    } else {
	    return NodeSibTraverse<Nodes...>::getN(index-1) + Node::area;
	  }
	}

	template<typename Array>
	static constexpr void writeIndex2Id(Array& array, size_t baseOffset)
	{
		NodeSibTraverse<Node>::writeIndex2Id(array, baseOffset);
		NodeSibTraverse<Nodes...>::writeIndex2Id(array, baseOffset+1);
	}

	static constexpr size_t childOffset(size_t childIndex)
	{
		if (childIndex == 0)
			return 0;
		return Node::area + NodeSibTraverse<Nodes...>::childOffset(childIndex - 1);
	}
};

template<typename StateType_, auto id_>
struct NodeSibTraverse<State<StateType_, id_>>
{
	template<typename Array>
	static constexpr void writeIndex2Id(Array& array, size_t baseOffset)
	{
		array[ baseOffset ] = id_;
	}

	static constexpr size_t childOffset(size_t childIndex)
	{
		// static_assert(childIndex == 0, "");
		return 0;
	}
};

template<typename State, typename ...Nodes>
struct NodeSibTraverse<FsmNode<State, Nodes...>>
{
	template<typename Array>
	static constexpr void writeIndex2Id(Array& array, size_t baseOffset)
	{
		array[ baseOffset ] = State::id;
		NodeSibTraverse<Nodes...>::writeIndex2Id(array, baseOffset + 1);
	}

	static constexpr size_t childOffset(size_t childIndex)
	{
		// static_assert(childIndex == 0, "");
		return 0;
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


  // Get the offset where a particular subnodes starts.
  // index 0 : this node, 1 : first subnode.
  static const constexpr size_t get(size_t index)
  {
    if (index < 2) {
      return index;
    } else {
      return 1 + NodeSibTraverse<Nodes...>::getN(index-1); }
  }
  static constexpr size_t childOffset(size_t childIndex)
  {
	  return 1 + NodeSibTraverse<Nodes...>::childOffset(childIndex);
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
		NodeSibTraverse<Root>::writeIndex2Id(res, 0);
		return res;
	}
	static auto constexpr initId2Index()-> std::array<std::size_t, stateNo>
	{
		std::array<std::size_t, stateNo> res = {};
		//WriteIndex2Array<Root>::fkn(res, 0);
		return res;
	}

	static auto constexpr initId2Builder()-> std::array<BuilderFkn, stateNo>
	{
		return {};
	}

	//
	static constexpr std::array<BuilderFkn, stateNo> id2Builder = initId2Builder();

	// Translation from stateId into index into an array.
	static constexpr std::array<std::size_t, stateNo> id2Index = initId2Index();

	// Translation from stateId into index into an array.
	static constexpr std::array<StateId, stateNo> index2Id = initIndex2Id();
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
