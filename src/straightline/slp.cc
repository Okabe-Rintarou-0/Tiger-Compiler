#include "straightline/slp.h"
#include <algorithm>
#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  return std::max(stm1->MaxArgs(), stm2->MaxArgs());
}

Table *A::CompoundStm::Interp(Table *t) const {
  Table* t1 = stm1->Interp(t);
  return stm2->Interp(t1);
}

int A::AssignStm::MaxArgs() const {
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  IntAndTable *expT = exp->Interp(t);
  return expT->t->Update(id, expT->i);
}

int A::PrintStm::MaxArgs() const {
  return std::max(exps->MaxArgs(), exps->NumExps());
}

Table *A::PrintStm::Interp(Table *t) const {
  return exps->Interp(t)->t;
}


int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
