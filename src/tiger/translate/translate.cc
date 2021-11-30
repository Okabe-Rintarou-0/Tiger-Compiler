#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
}

using PatchList = std::list<temp::Label **>;
void DoPatch(PatchList t_list, temp::Label *label) {
  for (auto t : t_list)
    *t = label;
}

PatchList JoinPatch(PatchList first, PatchList second) {
  first.insert(first.end(), second.begin(), second.end());
  return first;
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_ = nullptr;

  Cx() = default;

  Cx(const PatchList &trues, const PatchList &falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return Cx();
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return Cx();
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList &trues, PatchList &falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    tr::DoPatch(cx_.trues_, t);
    tr::DoPatch(cx_.falses_, f);
    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
                new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                                                    new tree::ConstExp(0)),
                                  new tree::EseqExp(new tree::LabelStm(t),
                                                    new tree::TempExp(r))))));
  }

  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() { /* TODO: Put your lab5 code here */
  absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), nullptr,
                         errormsg_.get());
}

/**
 * MEM( + ( CONST k, MEM( +(CONST 2*wordsize, ( …MEM(+(CONST 2*wordsize, TEMP
 * FP) …))))
 */
tree::Exp *FramePtr(Level *level, Level *acc_level,
                    frame::RegManager *reg_manager) {
  tree::Exp *framePtr = new tree::TempExp(reg_manager->FramePointer());
  while (level != acc_level) {
    int wordSize = reg_manager->WordSize();
    tree::Exp *offset = new tree::ConstExp(2 * wordSize);
    tree::Exp *stat_link = new tree::BinopExp(tree::PLUS_OP, offset, framePtr);
    framePtr = new tree::MemExp(stat_link);
    level = level->parent_;
  }
  return framePtr;
}
} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto varEntry = dynamic_cast<env::VarEntry *>(venv->Look(sym_));
  auto access = varEntry->access_;
  tree::Exp *framePtr = FramePtr(level, access->level_, reg_manager);
  tr::Exp *varExp = new tr::ExExp(access->access_->ToExp(framePtr));
  return new tr::ExpAndTy(varExp, varEntry->ty_);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // by call var->translate, we can get the address stored by the record var.
  // notice that the record var is actually a pointer, which points to the real
  // data stored in the heap.
  auto expAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  auto varExp = expAndTy->exp_->UnEx();
  auto varTy = expAndTy->ty_;

  auto recordTy = dynamic_cast<type::RecordTy *>(varTy);
  auto fieldList = recordTy->fields_->GetList();
  int idx = 0;
  int wordSize = reg_manager->WordSize();

  // get the type of the field and count the offset
  type::Ty *fieldTy;
  for (auto field : fieldList) {
    if (field->name_ == sym_) {
      fieldTy = field->ty_;
      break;
    }
    ++idx;
  }

  auto offset = new tree::ConstExp(idx * wordSize);
  auto fieldAddr = new tree::BinopExp(tree::PLUS_OP, varExp, offset);
  auto fieldExp = new tr::ExExp(new tree::MemExp(fieldAddr));
  return new tr::ExpAndTy(fieldExp, fieldTy);
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto expAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  auto varExp = expAndTy->exp_->UnEx();
  auto ty = expAndTy->ty_;

  int wordSize = reg_manager->WordSize();
  auto index =
      subscript_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
  auto w = new tree::ConstExp(wordSize);
  auto offset = new tree::BinopExp(tree::MUL_OP, index, w);
  auto subVarAddr = new tree::BinopExp(tree::PLUS_OP, varExp, offset);
  auto subVarExp = new tr::ExExp(new tree::MemExp(subVarAddr));
  return new tr::ExpAndTy(subVarExp, ty);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto constExp = new tr::ExExp(new tree::ConstExp(val_));
  return new tr::ExpAndTy(constExp, type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto lab = temp::LabelFactory::NewLabel();
  auto labExp = new tree::NameExp(lab);
  return new tr::ExpAndTy(new tr::ExExp(labExp), type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto funExp =
      new tree::NameExp(temp::LabelFactory::NamedLabel(func_->Name()));
  auto argExpList = new tree::ExpList;
  auto argList = args_->GetList();
  for (auto arg : argList) {
    auto argExp =
        arg->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
    argExpList->Append(argExp);
  }
  auto callExp = new tree::CallExp(funExp, argExpList);
  auto funEntry = dynamic_cast<env::FunEntry *>(venv->Look(func_));
  return new tr::ExpAndTy(new tr::ExExp(callExp), funEntry->result_);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto leftExp =
      left_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
  auto rightExp =
      right_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
  auto oper = (tree::BinOp)oper_;
  auto opExp = new tree::BinopExp(oper, leftExp, rightExp);
  /// TODO: fix this naive method
  return new tr::ExpAndTy(new tr::ExExp(opExp), type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto recordTy = tenv->Look(typ_);
  auto efieldList = fields_->GetList();
  auto fieldLen = efieldList.size();
  int wordSize = reg_manager->WordSize();

  // alloc space
  auto r = temp::TempFactory::NewTemp();
  auto tempExp = new tree::TempExp(r);
  auto mallocExp = new tree::NameExp(temp::LabelFactory::NamedLabel("malloc"));
  auto allocSize = fieldLen * wordSize;
  auto sizeExp = new tree::ConstExp(allocSize);
  auto argList = new tree::ExpList;
  argList->Append(sizeExp);
  auto callMallocExp = new tree::CallExp(mallocExp, argList);

  auto allocStm = new tree::MoveStm(tempExp, callMallocExp);
  auto rootSeq = new tree::SeqStm(allocStm, nullptr);

  // assignment value
  auto curSeq = rootSeq;
  int idx = 0;
  for (auto efield : efieldList) {
    auto efdExp = efield->exp_->Translate(venv, tenv, level, label, errormsg)
                      ->exp_->UnEx();
    auto offset = new tree::ConstExp(idx * wordSize);
    auto tempExp = new tree::TempExp(r);
    auto fieldExp =
        new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, tempExp, offset));
    auto assignStm = new tree::MoveStm(fieldExp, efdExp);
    auto seq = new tree::SeqStm(assignStm, nullptr);
    curSeq->right_ = seq;
    curSeq = seq;
    ++idx;
  }

  auto eseq = new tree::EseqExp(rootSeq, new tree::TempExp(r));
  return new tr::ExpAndTy(new tr::ExExp(eseq), recordTy);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  /// TODO: seqExp optimize
  auto expList = seq_->GetList();
  tree::EseqExp *rootExp = nullptr;
  tree::EseqExp *curExp = nullptr;
  for (auto exp : expList) {
    auto trExp =
        exp->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
    auto expStm = new tree::ExpStm(trExp);
    auto seqExp = new tree::EseqExp(expStm, nullptr);
    if (!curExp) {
      rootExp = curExp = seqExp;
    } else {
      curExp->exp_ = seqExp;
      curExp = seqExp;
    }
  }
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto varExp =
      var_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
  auto valueExpAndTy = exp_->Translate(venv, tenv, level, label, errormsg);
  auto valueExp = valueExpAndTy->exp_->UnEx();
  auto valueTy = valueExpAndTy->ty_;
  auto assignExp = new tree::MoveStm(varExp, valueExp);
  return new tr::ExpAndTy(new tr::NxExp(assignExp), valueTy);
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto t = temp::LabelFactory::NewLabel();
  auto f = temp::LabelFactory::NewLabel();

  auto testExp = test_->Translate(venv, tenv, level, label, errormsg)->exp_;
  tr::Cx testCx = testExp->UnCx(errormsg);
  auto thenExpAndTy = then_->Translate(venv, tenv, level, label, errormsg);
  auto thenExp = thenExpAndTy->exp_;
  auto thenTy = thenExpAndTy->ty_;

  tr::DoPatch(testCx.trues_, t);
  tr::DoPatch(testCx.falses_, f);
  auto testStm = testCx.stm_;

  // if-then exp
  if (elsee_ == nullptr) {
    auto thenStm = (dynamic_cast<tr::NxExp *>(thenExp))->stm_;
    auto trueBranchStm = new tree::SeqStm(new tree::LabelStm(t), thenStm);
    auto ifThenStm = new tree::SeqStm(testStm, trueBranchStm);
    return new tr::ExpAndTy(new tr::NxExp(ifThenStm), type::VoidTy::Instance());
  }
  auto elseExp = elsee_->Translate(venv, tenv, level, label, errormsg)->exp_;
  // if-then-else exp
  if (typeid(*thenExp) == typeid(tr::NxExp)) {
    auto thenStm = thenExp->UnNx();
    auto elseStm = elseExp->UnNx();
    auto trueBranchStm = new tree::SeqStm(new tree::LabelStm(t), thenStm);
    auto falseBranchStm = new tree::SeqStm(new tree::LabelStm(f), elseStm);
    auto ifThenElseStm = new tree::SeqStm(
        testStm, new tree::SeqStm(trueBranchStm, falseBranchStm));
    return new tr::ExpAndTy(new tr::NxExp(ifThenElseStm),
                            type::VoidTy::Instance());
  }

  tree::Stm *assignStm;
  tree::Stm *thenStm;
  tree::Stm *elseStm;
  /// TODO: finish this, Cx or Ex
  // should not use naive UnEx, judge each statements, but need example here.
  //  if (typeid(*thenExp) == typeid(tr::ExExp)) {
  //    auto thenTreeExp = thenExp->UnEx();
  //    auto elseTreeExp = elseExp->UnEx();
  //    auto r = temp::TempFactory::NewTemp();
  //    assignStm =
  //        new tree::SeqStm(new tree::LabelStm(t),
  //                         new tree::MoveStm(new tree::TempExp(r),
  //                         thenTreeExp));
  //  } else {
  //    auto thenCx = thenExp->UnCx(errormsg);
  //    thenStm = new tree::SeqStm(new tree::LabelStm(t), thenCx.stm_);
  //    auto then_t = temp::TempFactory::NewTemp();
  //    auto then_f = temp::TempFactory::NewTemp();
  //    assignStm = new tree::SeqStm(
  //        new tree::SeqStm(
  //            new tree::LabelStm(then_t),
  //            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1))),
  //        new tree::SeqStm(
  //            new tree::LabelStm(then_f),
  //            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0))))
  //  }
  //  if (typeid(*elseExp) == typeid(tr::ExExp)) {
  //    assignStm = new SeqStm(
  //        assignStm,
  //        new tree::SeqStm(new tree::LabelStm(f),
  //                         new tree::MoveStm(new tree::TempExp(r),
  //                         elseTreeExp)));
  //  } else {
  //    auto elseCx = elseExp->UnCx(errormsg);
  //    elseStm = new tree::SeqStm(new tree::LabelStm(f), elseCx.stm_);
  //    auto else_t = temp::TempFactory::NewTemp();
  //    auto else_f = temp::TempFactory::NewTemp();
  //    assignStm = new tree::SeqStm(
  //        new tree::SeqStm(
  //            new tree::LabelStm(else_t),
  //            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1))),
  //        new tree::SeqStm(
  //            new tree::LabelStm(else_f),
  //            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0))))
  //  }
  //  auto seqStm = new tree::SeqStm(
  //      testStm, new tree::SeqStm(trueBranchStm, falseBranchStm));
  //  auto ifThenElseExp = new tree::EseqExp(seqStm, new tree::TempExp(r));
  //  return new tr::ExpAndTy(new tr::ExExp(ifThenElseExp), thenTy);
  //  else {
  //  }
  return nullptr;
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto passed = temp::LabelFactory::NewLabel();
  auto done = temp::LabelFactory::NewLabel();
  auto testExp = test_->Translate(venv, tenv, level, label, errormsg)->exp_;
  auto testCx = testExp->UnCx(errormsg);

  auto bodyStm =
      body_->Translate(venv, tenv, level, done, errormsg)->exp_->UnNx();

  tr::DoPatch(testCx.trues_, passed);
  tr::DoPatch(testCx.falses_, done);

  auto testStm = testExp->UnCx(errormsg).stm_;
  auto seqStm = new tree::SeqStm(
      testStm, new tree::SeqStm(new tree::LabelStm(passed), bodyStm));
  return new tr::ExpAndTy(new tr::NxExp(seqStm), type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto iDec = new absyn::VarDec(0, var_, nullptr, lo_);
  auto limitDec =
      new absyn::VarDec(0, sym::Symbol::UniqueSymbol("limit"), nullptr, hi_);
  auto decList = new absyn::DecList(limitDec);
  decList->Prepend(iDec);
  auto iVar = new absyn::SimpleVar(0, var_);
  auto limitVar = new absyn::SimpleVar(0, sym::Symbol::UniqueSymbol("limit"));
  auto testExp = new absyn::OpExp(0, absyn::LE_OP, new absyn::VarExp(0, iVar),
                                  new absyn::VarExp(0, limitVar));
  auto whileExp = new absyn::WhileExp(0, testExp, body_);
  auto expList = new absyn::ExpList(whileExp);
  auto bodyExp = new absyn::SeqExp(0, expList);
  auto letExp = new absyn::LetExp(0, decList, bodyExp);
  return letExp->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto doneStm = new tree::LabelStm(label);
  auto jumpStm = new tree::JumpStm(new tree::NameExp(label), nullptr);
  return new tr::ExpAndTy(new tr::NxExp(jumpStm), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto decList = decs_->GetList();
  auto it = decList.begin();
  auto seqStm = new tree::SeqStm(
      ((*it)->Translate(venv, tenv, level, label, errormsg))->UnNx(), nullptr);
  auto curSeqStm = seqStm;
  for (; it != decList.end(); ++it) {
    auto decStm =
        ((*it)->Translate(venv, tenv, level, label, errormsg))->UnNx();
    curSeqStm->right_ = new tree::SeqStm(decStm, nullptr);
  }
  //  curSeqStm->right_ = seqbody_->Translate();
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto nameAndTyList = types_->GetList();
  for (auto nameAndTy : nameAndTyList) {
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }

  for (auto nameAndTy : nameAndTyList) {
    auto nameTy = static_cast<type::NameTy *>(tenv->Look(nameAndTy->name_));
    auto rightTy = nameAndTy->ty_->SemAnalyze(tenv, errormsg);
    nameTy->ty_ = rightTy;
    tenv->Set(nameAndTy->name_, nameTy);
  }
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

} // namespace absyn
