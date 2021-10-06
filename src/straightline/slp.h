#ifndef STRAIGHTLINE_SLP_H_
#define STRAIGHTLINE_SLP_H_

#include <algorithm>
#include <cassert>
#include <string>
#include <list>

namespace A {

class Stm;
class Exp;
class ExpList;

enum BinOp { PLUS = 0, MINUS, TIMES, DIV };

// some data structures used by interp
class Table;
class IntAndTable;

class Stm {
 public:
  virtual int MaxArgs() const = 0;
  virtual Table *Interp(Table *) const = 0;
};

class CompoundStm : public Stm {
 public:
  CompoundStm(Stm *stm1, Stm *stm2) : stm1(stm1), stm2(stm2) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  Stm *stm1, *stm2;
};

class AssignStm : public Stm {
 public:
  AssignStm(std::string id, Exp *exp) : id(std::move(id)), exp(exp) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  std::string id;
  Exp *exp;
};

class PrintStm : public Stm {
 public:
  explicit PrintStm(ExpList *exps) : exps(exps) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  ExpList *exps;
};

class Exp {
  // TODO: you'll have to add some definitions here (lab1).
  // Hints: You may add interfaces like `int MaxArgs()`,
  //        and ` IntAndTable *Interp(Table *)`
  public:
    virtual int MaxArgs() const = 0;
    virtual IntAndTable *Interp(Table *) const = 0;
};

class IdExp : public Exp {
 public:
  explicit IdExp(std::string id) : id(std::move(id)) {}
  // TODO: you'll have to add some definitions here (lab1).
  int MaxArgs() const override {
      return 0;
  }
  IntAndTable *Interp(Table *t) const override {
      return new IntAndTable(t->LookUp(id), t);
  }
 private:
  std::string id;
};

class NumExp : public Exp {
 public:
  explicit NumExp(int num) : num(num) {}
  // TODO: you'll have to add some definitions here.
  int MaxArgs() const override {
      return 0;
  }
  IntAndTable *Interp(Table *t) const override {
      return new IntAndTable(num, t);
  }
 private:
  int num;
};

class OpExp : public Exp {
 public:
  OpExp(Exp *left, BinOp oper, Exp *right)
      : left(left), oper(oper), right(right) {}
  int MaxArgs() const override {
      return 0;
  }
  IntAndTable *Interp(Table *t) const override {
      IntAndTable *lt = left->Interp(t);
      IntAndTable *rt = right->Interp(t);
      switch (oper) {
          case PLUS:
              return new IntAndTable(lt->i + rt->i, t);
          case MINUS:
              return new IntAndTable(lt->i - rt->i, t);
          case TIMES:
              return new IntAndTable(lt->i * rt->i, t);
          case DIV:
              return new IntAndTable(lt->i / rt->i, t);
      }
  }
 private:
  Exp *left;
  BinOp oper;
  Exp *right;
};

class EseqExp : public Exp {
 public:
  EseqExp(Stm *stm, Exp *exp) : stm(stm), exp(exp) {}
  int MaxArgs() const override {
      return stm->MaxArgs();
  }
  IntAndTable *Interp(Table *t) const override {
      Table *stmT = stm->Interp(t);
      return exp->Interp(stmT);
  }
 private:
  Stm *stm;
  Exp *exp;
};

class ExpList {
 public:
    virtual int MaxArgs() const = 0;
    virtual int NumExps() const = 0;
    virtual IntAndTable *Interp(Table *) const = 0;
  // TODO: you'll have to add some definitions here (lab1).
  // Hints: You may add interfaces like `int MaxArgs()`, `int NumExps()`,
  //        and ` IntAndTable *Interp(Table *)`
};

class PairExpList : public ExpList {
 public:
  PairExpList(Exp *exp, ExpList *tail) : exp(exp), tail(tail) {}
  // TODO: you'll have to add some definitions here (lab1).
  int MaxArgs() const override {
      int maxArgs = exp->MaxArgs();
      if (tail != nullptr) {
        maxArgs = std::max(maxArgs, tail->MaxArgs());
      }
      return maxArgs;
  }
  int NumExps() const override {
      return 1 + tail->NumExps();
  }
  IntAndTable *Interp(Table *t) const override {
      return tail->Interp(t);
  }
 private:
  Exp *exp;
  ExpList *tail;
};

class LastExpList : public ExpList {
 public:
  LastExpList(Exp *exp) : exp(exp) {}
  // TODO: you'll have to add some definitions here (lab1).
  int MaxArgs() const override {
    return exp->MaxArgs();
  }
  int NumExps() const override {
      return 1;
  }
  IntAndTable * Interp(Table *t) const override {
      return exp->Interp(t);
  }
 private:
  Exp *exp;
};

class Table {
 public:
  Table(std::string id, int value, const Table *tail)
      : id(std::move(id)), value(value), tail(tail) {}
  int Lookup(const std::string &key) const;
  Table *Update(const std::string &key, int val) const;

 private:
  std::string id;
  int value;
  const Table *tail;
};

struct IntAndTable {
  int i;
  Table *t;

  IntAndTable(int i, Table *t) : i(i), t(t) {}
};

}  // namespace A

#endif  // STRAIGHTLINE_SLP_H_
