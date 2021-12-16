#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() { /* TODO: Put your lab6 code here */
  std::list<FNodePtr> jumpNodes;

  auto instrList = instr_list_->GetList();
  FNodePtr curNode = nullptr;
  FNodePtr lastNode = nullptr;
  for (auto instr : instrList) {
    bool is_jmp = false;
    curNode = flowgraph_->NewNode(instr);

    // for the case that instr is label
    if (typeid(*instr) == typeid(assem::LabelInstr)) {
      auto labelInstr = dynamic_cast<assem::LabelInstr *>(instr);
      auto label = labelInstr->label_;
      label_map_->Enter(label, curNode);
    } else if (typeid(*instr) == typeid(assem::OperInstr)) {
      auto operInstr = dynamic_cast<assem::OperInstr *>(instr);
      // if its a jump instr, then handle it later
      if (operInstr->jumps_) {
        jumpNodes.push_back(curNode);
        is_jmp = operInstr->assem_.find("jmp") != std::string::npos;
      }
    }

    // if there exists the last node, then connect lastNode to curNode.
    if (lastNode) {
      flowgraph_->AddEdge(lastNode, curNode);
    }

    // update lastNode; If current instr is jump, then lastNode should be null.
    lastNode = !is_jmp ? curNode : nullptr;
  }

  // Then let's deal with those jumpNodes
  for (auto jumpNode : jumpNodes) {
    auto instr = jumpNode->NodeInfo();
    auto jmpInstr = dynamic_cast<assem::OperInstr *>(instr);
    auto label = (*jmpInstr->jumps_->labels_)[0];
    auto labelNode = label_map_->Look(label);
    // just link two.
    // since it will jump from jumpNode to the label Node.
    flowgraph_->AddEdge(jumpNode, labelNode);
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList({});
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  auto ret = dst_ != nullptr ? dst_ : new temp::TempList({});
  return ret;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList({});
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  auto ret = src_ ? src_ : new temp::TempList({});
  return ret;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  auto ret = new temp::TempList({});
  if (dst_) {
    auto list = dst_->GetList();
    for (auto temp : list) {
      ret->Append(temp);
    }
  }
  if (src_) {
    auto list = src_->GetList();
    for (auto temp : list) {
      ret->Append(temp);
    }
  }
  return ret;
}
} // namespace assem
