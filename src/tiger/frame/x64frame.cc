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
  /* TODO: Put your lab5 code here */
public:
  /// TODO: fix here
  Access *AllocLocal(bool escape) override {
    Access *access = escape ? dynamic_cast<Access *>(new InFrameAccess(0))
                            : dynamic_cast<Access *>(new InRegAccess(
                                  temp::TempFactory::NewTemp()));
    return access;
  }
};
/* TODO: Put your lab5 code here */
Frame *NewFrame(temp::Label *fun, const std::list<bool> formals) {
  Frame *frame = new X64Frame;
  for (bool escape : formals) {
    Access *access = escape ? dynamic_cast<Access *>(new InFrameAccess(0))
                            : dynamic_cast<Access *>(new InRegAccess(
                                  temp::TempFactory::NewTemp()));
    frame->Append(access);
  }
  return frame;
}
} // namespace frame