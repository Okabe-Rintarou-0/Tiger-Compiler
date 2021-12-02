#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    auto offsetExp = new tree::ConstExp(offset);
    auto addrExp = new tree::BinopExp(tree::PLUS_OP, framePtr, offsetExp);
    return new tree::MemExp(addrExp);
  }
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put you
   * r lab5 code here */
  int localNumber = 0;

public:
  Access *AllocLocal(bool escape) override;

  tree::Stm *Sp2Fp() override;
  /// TODO: fix here
};

Access *X64Frame::AllocLocal(bool escape) {
  Access *access = escape ? dynamic_cast<Access *>(new InFrameAccess(
                                (-(++localNumber)) * reg_manager->WordSize()))
                          : dynamic_cast<Access *>(
                                new InRegAccess(temp::TempFactory::NewTemp()));
  return access;
}

tree::Stm *X64Frame::Sp2Fp() {
  return new tree::MoveStm(new tree::TempExp(reg_manager->FramePointer()),
                           new tree::TempExp(reg_manager->StackPointer()));
}

/* TODO: Put your lab5 code here */
Frame *NewFrame(temp::Label *fun, const std::list<bool> formals) {
  Frame *frame = new X64Frame;
  int idx = 1;
  for (bool escape : formals) {
    Access *access =
        escape ? dynamic_cast<Access *>(
                     new InFrameAccess((idx++) * reg_manager->WordSize()))
               : dynamic_cast<Access *>(
                     new InRegAccess(temp::TempFactory::NewTemp()));
    frame->Append(access);
  }
  return frame;
}

tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  auto argRegs = reg_manager->ArgRegs();
  auto formals = frame->Formals();
  int i = 0;
  tree::Stm *viewShift = new tree::ExpStm(new tree::ConstExp(0));
  auto fp = reg_manager->FramePointer();
  for (auto access : formals) {
    if (i >= 6)
      break;
    auto reg = argRegs->NthTemp(i++);
    viewShift = new tree::SeqStm(
        viewShift, new tree::MoveStm(access->ToExp(new tree::TempExp(fp)),
                                     new tree::TempExp(reg)));
  }
  return new tree::SeqStm(viewShift, stm);
}

} // namespace frame