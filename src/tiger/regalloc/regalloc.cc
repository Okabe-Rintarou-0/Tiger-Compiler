#include "tiger/regalloc/regalloc.h"
#include "tiger/output/logger.h"
#include <regex>
#include <set>
#define LOG(format, args...)                                                   \
  do {                                                                         \
    FILE *debug_log = fopen("debug.log", "a+");                                \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)

extern frame::RegManager *reg_manager;

namespace ra {

void printTempNode(live::INodePtr node) {
  std::cout << *temp::Map::Name()->Look(node->NodeInfo()) << "(addr = " << node
            << ")";
}

/* TODO: Put your lab6 code here */
void RegAllocator::RegAlloc() {
  Color();
  AssignRegisters();
}

void RegAllocator::Color() {
  bool done = true;
  int x = 0;
  std::cout << "Called" << std::endl;
  do {
    LivenessAnalysis();
    Build();
    MakeWorkList();
    do {
      if (!simplifyWorkList->Empty()) {
        //        std::cout << "Simplifying..." << std::endl;
        Simplify();
      } else if (!workListMoves->Empty()) {
        //        std::cout << "Coalescing..." << std::endl;
        Coalesce();
      } else if (!freezeWorkList->Empty()) {
        //        std::cout << "Freezing..." << std::endl;
        Freeze();
      } else if (!spillWorkList->Empty()) {
        //        std::cout << "Selecting..." << std::endl;
        SelectSpill();
      }
    } while (!(simplifyWorkList->Empty() && workListMoves->Empty() &&
               freezeWorkList->Empty() && spillWorkList->Empty()));
    AssignColors();
    if (!spilledNodes->Empty()) {
      RewriteProgram();
      done = false;
    }
    ++x;
    std::cout << "Finished " << x << std::endl;
    //  } while (0);
  } while (x < 1);

  std::cout << "Finished All\n\n";
}

void RegAllocator::Build() {
  initial->Clear();
  precolored->Clear();
  alias.clear();
  colors.clear();
  auto &nodes = interf_graph->Nodes()->GetList();
  for (auto node : nodes) {
    //    std::cout << "initing... " << node << ", "
    //              << *(temp::Map::Name()->Look(node->NodeInfo())) <<
    //              std::endl;
    if (initial->Contain(node) || precolored->Contain(node))
      continue;
    alias[node] = node;
    std::cout << node << " "
              << "alias[" << *temp::Map::Name()->Look(node->NodeInfo())
              << "] = " << node << std::endl;
    auto temp = node->NodeInfo();
    if (!temp->isBuiltin()) {
      initial->Append(node);
      //      std::cout << "Init add node: ";
      //      printTempNode(node);
      //      std::cout << std::endl;
    } else {
      precolored->Append(node);
      colors[node->NodeInfo()] = node->NodeInfo()->Int();
      //      std::cout << "init colors["
      //                << *(temp::Map::Name()->Look(node->NodeInfo()))
      //                << "] = " << node->NodeInfo()->Int() << std::endl;
    }
  }
}

live::MoveList *RegAllocator::NodeMoves(live::INodePtr node) {
  live::MoveList *moveList_n = moveList->Look(node);
  moveList_n = moveList_n ? moveList_n : new live::MoveList();
  return moveList_n->Intersect(activeMoves->Union(workListMoves));
}

void AddEdge(live::INodePtr u, live::INodePtr v){

}

bool RegAllocator::MoveRelated(live::INodePtr node) {
  return !NodeMoves(node)->Empty();
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  if (freezeWorkList->Contain(v)) {
    freezeWorkList->DeleteNode(v);
  } else {
    spillWorkList->DeleteNode(v);
  }
  coalescedNodes->Append(v);
  // alias[v] = u;
  // now u will stand for v, and there will be nothing to do with v;
  alias[v] = u;
  std::cout << v << " "
            << "alias[" << *temp::Map::Name()->Look(v->NodeInfo())
            << "] = " << u << std::endl;
  // moveList[u] = moveList[u] U moveList[v]
  moveList->Set(u, moveList->Look(u)->Union(moveList->Look(v)));
  auto singleton = new graph::NodeList<temp::Temp>;
  singleton->Append(v);
  EnableMoves(singleton);

  for (auto t : v->Adj()->GetList()) {
    interf_graph->AddEdge(t, u);
    interf_graph->AddEdge(u, t);
    t->Succ()->DeleteNode(v);
    t->Pred()->DeleteNode(v);
    CheckDegree(t);
  }

  if (u->Degree() >= K && freezeWorkList->Contain(u)) {
    freezeWorkList->DeleteNode(u);
    spillWorkList->Append(u);
  }
}

void RegAllocator::AssignColors() {
  std::cout << "assign colors\n";
  //  LOG("assign colors\n");
  while (!selectStack.empty()) {
    auto n = selectStack.top();
    selectStack.pop();

    if (precolored->Contain(n)) {
      continue;
    }

    std::set<int> okColors;
    for (int i = 0; i < K; ++i)
      okColors.insert(i);

    for (auto w : n->Adj()->GetList()) {
      auto wAlias = GetAlias(w);

      auto allColored = coloredNodes->Union(precolored);
      if (allColored->Contain(wAlias)) {
        okColors.erase(colors[wAlias->NodeInfo()]);
      }
    }

    if (okColors.empty()) {
      spilledNodes->Append(n);
    } else {
      coloredNodes->Append(n);
      int c = *okColors.begin();
      std::cout << "set " << *temp::Map::Name()->Look(n->NodeInfo())
                << "'s color to: " << std::endl;
      colors[n->NodeInfo()] = c;
    }
  }
  std::cout << "HERE" << std::endl;
  for (auto n : coalescedNodes->GetList()) {
    std::cout
        << "set " << *temp::Map::Name()->Look(n->NodeInfo()) << " "
        << GetAlias(n) << " while "
        << alias[n]
        //              << "'s color to: " << colors[GetAlias(n)->NodeInfo()]
        << std::endl;
    colors[n->NodeInfo()] = colors[GetAlias(n)->NodeInfo()];
  }
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
  for (auto m : NodeMoves(u)->GetList()) {
    live::INodePtr v;
    auto x = m.first;
    auto y = m.second;
    if (GetAlias(y) == GetAlias(u)) {
      v = GetAlias(x);
    } else {
      v = GetAlias(y);
    }
    activeMoves->Delete(x, y);
    frozenMoves->Append(x, y);
    if (NodeMoves(v)->Empty() && v->Degree() < K) {
      freezeWorkList->DeleteNode(v);
      //      std::cout << "FreezeMoves add node: ";
      //      printTempNode(v);
      //      std::cout << std::endl;
      simplifyWorkList->Append(v);
    }
  }
}

void RegAllocator::Freeze() {
  auto u = freezeWorkList->PopFront();
  //  std::cout << "Freeze add node: ";
  //  printTempNode(u);
  //  std::cout << std::endl;
  simplifyWorkList->Append(u);
  FreezeMoves(u);
}

void RegAllocator::MakeWorkList() {
  //  std::cout << "Making worklist..." << std::endl;
  auto &nodes = initial->GetList();
  for (auto node : nodes) {
    if (node->Degree() >= K) {
      spillWorkList->Append(node);
    } else if (MoveRelated(node)) {
      freezeWorkList->Append(node);
    } else {
      //      std::cout << "makeworklist add node: ";
      //      printTempNode(node);
      //      std::cout << std::endl;
      simplifyWorkList->Append(node);
    }
  }
}

void RegAllocator::Simplify() {
  if (simplifyWorkList->Empty())
    return;

  auto node = simplifyWorkList->PopFront();
  //  std::cout << "simplify: ";
  //  printTempNode(node);
  //  std::cout << std::endl;
  selectStack.push(node);

  interf_graph->DeleteNode(node);

  for (auto adj : node->Adj()->GetList()) {
    CheckDegree(adj);
  }
}

void RegAllocator::CheckDegree(live::INodePtr node) {
  //  std::cout << "Checking node ";
  //  printTempNode(node);
  //  std::cout << std::endl;
  int degree = node->Degree();
  if (degree == K - 1) {
    auto singleton = new graph::NodeList<temp::Temp>;
    singleton->Append(node); // namely {m}
    EnableMoves(node->Adj()->Union(singleton));
    spillWorkList->DeleteNode(node);
    if (MoveRelated(node)) {
      freezeWorkList->Append(node);
    } else {
      //      std::cout << "check degree add node: ";
      //      printTempNode(node);
      //      std::cout << std::endl;
      simplifyWorkList->Append(node);
    }
  }
}

void RegAllocator::EnableMoves(graph::NodeList<temp::Temp> *nodes) {
  for (auto n : nodes->GetList()) {
    auto nodeMvs = NodeMoves(n);
    for (auto m : nodeMvs->GetList()) {
      activeMoves->Delete(m.first, m.second);
      workListMoves->Append(m.first, m.second);
    }
  }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr node) {
  //  assert(node);
  return coalescedNodes->Contain(node) ? GetAlias(alias[node]) : node;
}

void RegAllocator::AddWorkList(live::INodePtr node) {
  if (!precolored->Contain(node) && !MoveRelated(node) && node->Degree() < K) {
    freezeWorkList->DeleteNode(node);
    simplifyWorkList->Append(node);
    //    std::cout << "AddWorkList add node: ";
    //    printTempNode(node);
    //    std::cout << std::endl;
  }
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return t->Degree() < K || precolored->Contain(t) || t->Adj()->Contain(r);
}

bool RegAllocator::CoalesceOK(live::INodePtr u, live::INodePtr v) {
  for (auto t : v->Adj()->GetList()) {
    if (!OK(t, u))
      return false;
  }
  return true;
}

bool RegAllocator::Conservative(graph::NodeList<temp::Temp> *nodes) {
  int k = 0;
  for (auto n : nodes->GetList()) {
    if (n->Degree() >= k)
      ++k;
  }
  return k < K;
}

void RegAllocator::SelectSpill() {
  auto m = spillWorkList->PopFront();
  //  std::cout << "selectspill add node: ";
  //  printTempNode(m);
  //  std::cout << std::endl;
  simplifyWorkList->Append(m);
  FreezeMoves(m);
}

void RegAllocator::Coalesce() {
  auto m = workListMoves->PopFront();
  auto x = m.first;
  auto y = m.second;
  //  std::cout << "Try to coalesce: ";
  //  printTempNode(x);
  //  std::cout << ", ";
  //  printTempNode(y);
  //  std::cout << std::endl;
  live::INodePtr u, v;
  if (precolored->Contain(y)) {
    u = y;
    v = x;
    //    (u, v) = (y, x);
  } else {
    u = x;
    v = y;
    //    (u, v) = (x, y);
  }

  //  std::cout << "u = " << u << "and v = " << v << std::endl;

  if (u == v) {
    coalescedMoves->Append(x, y);
    AddWorkList(u);
  } else if (precolored->Contain(v) || u->Adj()->Contain(v)) {
    constrainedMoves->Append(x, y);
    AddWorkList(u);
    AddWorkList(v);
  } else if ((precolored->Contain(u) && CoalesceOK(u, v)) ||
             (!precolored->Contain(u)) &&
                 Conservative(u->Adj()->Union(v->Adj()))) {
    coalescedMoves->Append(x, y);
    Combine(u, v);
    AddWorkList(u);
  } else {
    activeMoves->Append(x, y);
  }
}

void RegAllocator::LivenessAnalysis() {
  auto il = assemInstr.get()->GetInstrList();
  fg::FlowGraphFactory flowGraphFactory(il);
  flowGraphFactory.AssemFlowGraph();
  auto flowGraph = flowGraphFactory.GetFlowGraph();
  liveGraphFactory = new live::LiveGraphFactory(flowGraph);
  liveGraphFactory->Liveness();
  auto liveGraph = liveGraphFactory->GetLiveGraph();
  interf_graph = liveGraph.interf_graph;
  workListMoves = liveGraph.moves;
  moveList = liveGraph.moveList;
}

void RegAllocator::RewriteProgram() {
  std::cout << "rewriting prog\n";
  //  LOG("rewriting prog\n");
  graph::NodeList<temp::Temp> *newTemps = new graph::NodeList<temp::Temp>;

  auto il = assemInstr->GetInstrList();
  char buf[256];
  const int wordSize = reg_manager->WordSize();
  for (auto v : spilledNodes->GetList()) {
    auto newTemp = temp::TempFactory::NewTemp(); // namely vi
    auto oldTemp = v->NodeInfo();
    std::cout << "For spilled temp: " << *temp::Map::Name()->Look(oldTemp)
              << std::endl
              << "We create a new temp: " << *temp::Map::Name()->Look(newTemp)
              << std::endl;
    ++frame->localNumber;
    temp::TempList *src = nullptr, *dst = nullptr;
    auto &instrList = il->GetList();
    auto iter = instrList.begin();
    for (; iter != instrList.end(); ++iter) {
      auto instr = *iter;
      std::cout << "For instr: ";
      instr->Print(stdout, temp::Map::Name());
      if (typeid(*instr) == typeid(assem::MoveInstr)) {
        auto moveInstr = dynamic_cast<assem::MoveInstr *>(instr);
        src = moveInstr->src_;
        dst = moveInstr->dst_;
      } else if (typeid(*instr) == typeid(assem::OperInstr)) {
        auto operInstr = dynamic_cast<assem::OperInstr *>(instr);
        src = operInstr->src_;
        dst = operInstr->dst_;
      }

      if (src && src->Contain(oldTemp)) {
        src->Replace(oldTemp, newTemp);
        sprintf(buf, "# Ops!Spilled before\nmovq (%s_framesize-%d)(`s0), `d0",
                frame->func_->Name().c_str(), frame->localNumber * wordSize);
        auto newInstr = new assem::OperInstr(
            buf, new temp::TempList({newTemp}),
            new temp::TempList({reg_manager->StackPointer()}), nullptr);
        iter = instrList.insert(iter, newInstr);
        ++iter;
      }

      if (dst && dst->Contain(oldTemp)) {
        dst->Replace(oldTemp, newTemp);
        sprintf(buf, "# Ops!Spilled after\nmovq `s0, (%s_framesize-%d)(`d0)",
                frame->func_->Name().c_str(), frame->localNumber * wordSize);
        auto newInstr = new assem::OperInstr(
            buf, new temp::TempList({reg_manager->StackPointer()}),
            new temp::TempList({newTemp}), nullptr);
        iter = instrList.insert(std::next(iter), newInstr);
      }
    }
  }

  spilledNodes->Clear();
  //  initial = coloredNodes->Union(coalescedNodes)->Union(newTemps);
  coloredNodes->Clear();
  coalescedNodes->Clear();
}

void RegAllocator::AssignTemps(temp::TempList *temps) {
  if (!temps)
    return;
  static auto regs = reg_manager->Registers();
  auto map = temp::Map::Name();
  for (auto temp : temps->GetList()) {
    //    std::cout << "color is: " << colors[temp] << std::endl;
    auto tgt = map->Look(regs->NthTemp(colors[temp]));
    //    std::cout << "assign: " << *map->Look(temp) << " to "
    //              << *map->Look(regs->NthTemp(colors[temp])) << std::endl;
    map->Set(temp, tgt);
  }
}

bool RegAllocator::MeaninglessMove(temp::TempList *src, temp::TempList *dst) {
  static auto map = temp::Map::Name();
  if (src && dst) {
    auto &srcTemp = src->GetList().front();
    auto &dstTemp = dst->GetList().front();
    int srcColor = colors[srcTemp];
    int dstColor = colors[dstTemp];
    return srcColor == dstColor;
  }
  return false;
}

void RegAllocator::AssignRegisters() {
  //  std::cout << "assign regs\n";
  //  LOG("assign regs\n");
  auto il = assemInstr.get()->GetInstrList();
  /// TODO: something wrong with it.
  auto instrList = il->GetList();
  auto iter = instrList.begin();
  char framesize_buf[256];
  sprintf(framesize_buf, "%s_framesize", frame->func_->Name().c_str());
  std::string framesize(framesize_buf);
  for (; iter != instrList.end();) {
    auto instr = *iter;
    instr->assem_ = std::regex_replace(instr->assem_, std::regex(framesize),
                                       std::to_string(frame->frameSize()));
    if (typeid(*instr) == typeid(assem::MoveInstr)) {
      auto moveInstr = dynamic_cast<assem::MoveInstr *>(instr);
      auto src = moveInstr->src_;
      auto dst = moveInstr->dst_;
      if (MeaninglessMove(src, dst)) {
        std::cout << "Meaningless: ";
        instr->Print(stdout, temp::Map::Name());
      }
      //      if (!MeaninglessMove(src, dst)) {
      AssignTemps(src);
      AssignTemps(dst);
      //      } else {
      //        iter = instrList.erase(iter);
      //        continue;
      //      }
    } else if (typeid(*instr) == typeid(assem::OperInstr)) {
      auto operInstr = dynamic_cast<assem::OperInstr *>(instr);
      AssignTemps(operInstr->src_);
      AssignTemps(operInstr->dst_);
    }
    ++iter;
  }
  result->il_ = il;
}

} // namespace ra