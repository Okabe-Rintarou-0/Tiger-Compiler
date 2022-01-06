#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

#include <iostream>

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) { root_->Traverse(env, 0); }

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  auto esc_entry = env->Look(sym_);
  if (esc_entry && esc_entry->depth_ < depth) {
    *(esc_entry->escape_) = true;
  }
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  var_->Traverse(env, depth);
  subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) { return; }

void IntExp::Traverse(esc::EscEnvPtr env, int depth) { return; }

void StringExp::Traverse(esc::EscEnvPtr env, int depth) { return; }

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  auto argList = args_->GetList();
  for (auto argExp : argList) {
    argExp->Traverse(env, depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  left_->Traverse(env, depth);
  right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  auto efieldList = fields_->GetList();
  for (auto efield : efieldList) {
    efield->exp_->Traverse(env, depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  auto expList = seq_->GetList();
  for (auto exp : expList) {
    exp->Traverse(env, depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  var_->Traverse(env, depth);
  exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  test_->Traverse(env, depth);
  then_->Traverse(env, depth);
  if (elsee_)
    elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  test_->Traverse(env, depth);
  body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  escape_ = false;
  lo_->Traverse(env, depth);
  hi_->Traverse(env, depth);
  body_->Traverse(env, depth);
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) { return; }

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  auto decList = decs_->GetList();
  for (auto dec : decList) {
    if (typeid(*dec) == typeid(FunctionDec))
      dec->Traverse(env, depth + 1);
    else
      dec->Traverse(env, depth);
  }
  body_->Traverse(env, depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  size_->Traverse(env, depth);
  init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) { return; }

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  auto funcList = functions_->GetList();
  for (auto func : funcList) {
    auto params = func->params_->GetList();
    for (auto param : params) {
      param->escape_ = false;
      env->Enter(param->name_, new esc::EscapeEntry(depth, &(param->escape_)));
    }
    func->body_->Traverse(env, depth);
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  escape_ = false;
  env->Enter(var_, new esc::EscapeEntry(depth, &escape_));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {}

} // namespace absyn
