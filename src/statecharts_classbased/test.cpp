#include <iostream>

/**
 * General description:
 * Want to construct a tree of FsmNodes<State, SubState1, SubState2> where
 * State is the current State in the FSM, and SubState1, 2 etc are sub states of
 * State.
 * The FsmNode contain information such as the index of each state and the 'area'
 * needed for itself and its substates.
 * 
 * The tree is external to the Statetypes. All information is kept in the FsmNode class.
 * 
 */


using namespace std;
class Root;
class S1;
class S2;
class S3;
class S4;


template<typename State, typename ...Nodes>
class FsmNode;

// Calculate the area for each node.
template<typename State, typename ...Nodes>
class FsmArea;

// class Warp is used to get given state names and FsmNode:s a common
// inteface. Type of static subclassing with the 'virtual function' area.
// 
template<typename T>
class WrapNode;


template<typename State, typename ...Nodes>
class WrapNode<FsmNode<State, Nodes...>>
{
public:
  enum {
    area = FsmNode<State, Nodes...>::area
  };
};

template<typename T>
class WrapNode
{
public:
  enum {
    area = 1
  };
};

template<typename State, typename ...Nodes>
class FsmArea
{
public:
  enum {
    // Note: fold expression (C++17)
    area = 1 + (... + WrapNode<Nodes>::area),
    subStateNo = sizeof...(Nodes)
  };
};


template<size_t index, typename Node, typename ...Nodes>
size_t getN()
{
  if constexpr (index < 1) {
      return 0;
    } else {
    return getN<index-1, Nodes...>() + Node::area;
  }
}

template<size_t index, typename Node, typename ...Nodes>
struct SubType;

template<typename Node, typename ...Nodes>
struct SubType<0, Node, Nodes...>
{
  using Result = Node; 
};

template<size_t index, typename Node, typename ...Nodes>
struct SubType
{
  using Result = typename SubType<index-1, Nodes...>::Result; 
};


template<typename State, typename ...Nodes>
class FsmNode
{
public:
  enum {
    area = FsmArea<State, Nodes...>::area,
    subStateNo = FsmArea<State, Nodes...>::subStateNo
  };

  // Return the offset into storage for a particular subnode.
  // Position 0 is reserved for the current node. 1 for first
  // subnode etc. Will include areas of subnodes.
  template<size_t index>
  static const constexpr size_t get()
  {
    if constexpr (index < 2) {
      return index;
    } else {
      return 1 + getN<index-1, WrapNode<Nodes>...>(); }
  }

  // Get the type of for a particular storage at some index.
  template<size_t index>
  using SubType = typename ::SubType<index, WrapNode<Nodes>...>::Result; 
};

template<int I>
struct Ord
{
  enum {
    value = I
  };
};

int main()
{
  using Base1 = FsmNode<S1, S2, S3, S4>;
  cout << Base1::area << " " << Base1::subStateNo << endl;

  using RootNode = FsmNode<Root, Base1, S4>;  
  cout << RootNode::area << " " << RootNode::subStateNo << endl;

  cout << Base1::get<3>() << endl;
  cout << RootNode::get<2>() << endl;

  cout << Base1::SubType<2>::area << endl;
}
