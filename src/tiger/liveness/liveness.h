#ifndef TIGER_LIVENESS_LIVENESS_H_
#define TIGER_LIVENESS_LIVENESS_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"
#include "tiger/liveness/flowgraph.h"
#include "tiger/util/graph.h"

namespace live {

class MoveList;

using INode = graph::Node<temp::Temp>;
using INodePtr = graph::Node<temp::Temp> *;
using INodeList = graph::NodeList<temp::Temp>;
using INodeListPtr = graph::NodeList<temp::Temp> *;
using IGraph = graph::Graph<temp::Temp>;
using IGraphPtr = graph::Graph<temp::Temp> *;
using MvList = graph::Table<temp::Temp, MoveList>;

class MoveList {
public:
  MoveList() = default;

  [[nodiscard]] const std::list<std::pair<INodePtr, INodePtr>> &
  GetList() const {
    return move_list_;
  }
  void Append(INodePtr src, INodePtr dst) { move_list_.emplace_back(src, dst); }
  bool Contain(INodePtr src, INodePtr dst);
  void Delete(INodePtr src, INodePtr dst);
  std::pair<INodePtr, INodePtr> Front() { return move_list_.front(); }
  std::pair<INodePtr, INodePtr> PopFront() {
    auto frt = move_list_.front();
    move_list_.pop_front();
    return frt;
  }
  void Prepend(INodePtr src, INodePtr dst) {
    move_list_.emplace_front(src, dst);
  }
  inline bool Empty() { return move_list_.empty(); }
  MoveList *Union(MoveList *list);
  MoveList *Intersect(MoveList *list);
  MoveList *Diff(MoveList *list);

private:
  std::list<std::pair<INodePtr, INodePtr>> move_list_;
};

struct LiveGraph {
  IGraphPtr interf_graph;
  MoveList *moves;
  MvList *moveList;

  LiveGraph(IGraphPtr interf_graph, MoveList *moves, MvList *moveList)
      : interf_graph(interf_graph), moves(moves), moveList(moveList) {}
};

class LiveGraphFactory {
public:
  explicit LiveGraphFactory(fg::FGraphPtr flowgraph)
      : flowgraph_(flowgraph),
        live_graph_(new IGraph(), new MoveList(), new MvList()),
        in_(std::make_unique<graph::Table<assem::Instr, temp::TempList>>()),
        out_(std::make_unique<graph::Table<assem::Instr, temp::TempList>>()),
        temp_node_map_(new tab::Table<temp::Temp, INode>()) {}
  void Liveness();
  LiveGraph GetLiveGraph() { return live_graph_; }
  tab::Table<temp::Temp, INode> *GetTempNodeMap() { return temp_node_map_; }

  MoveList *getMoveList(INodePtr node);

private:
  fg::FGraphPtr flowgraph_;
  LiveGraph live_graph_;

  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> in_;
  std::unique_ptr<graph::Table<assem::Instr, temp::TempList>> out_;
  tab::Table<temp::Temp, INode> *temp_node_map_;

  void init();
  void addMoveList(MvList *moveList, INodePtr node, INodePtr src, INodePtr dst);

  INodePtr getNode(temp::Temp *temp);
  void LiveMap();
  void InterfGraph();
};

} // namespace live

#endif