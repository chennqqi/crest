// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

/***
 * Author: Sudeep juvekar (sjuvekar@eecs.berkeley.edu)
 * 4/17/09
 */
#include <assert.h>

#include <yices_c.h>

#include "base/symbolic_expression.h"
#include "base/unary_expression.h"
#include "base/binary_expression.h"
#include "base/compare_expression.h"
#include "base/deref_expression.h"
#include "base/symbolic_object.h"
#include "base/basic_expression.h"

namespace crest {

typedef map<var_t,value_t>::iterator It;
typedef map<var_t,value_t>::const_iterator ConstIt;

SymbolicExpr::~SymbolicExpr() { }

SymbolicExpr* SymbolicExpr::Clone() const {
  return new SymbolicExpr(size_, value_);
}

void SymbolicExpr::AppendToString(string* s) const {
  assert(IsConcrete());

  char buff[32];
  sprintf(buff, "%lld", value());
  s->append(buff);
}

bool SymbolicExpr::Equals(const SymbolicExpr &e) const {
  return (e.IsConcrete()
          && (value() == e.value())
          && (size() == e.size()));
}

yices_expr SymbolicExpr::BitBlast(yices_context ctx) const {
  // TODO: Implement this method for size() > sizeof(unsigned long).
  assert(size() <= sizeof(unsigned long));
  return yices_mk_bv_constant(ctx, 8*size(), (unsigned long)value());
}

SymbolicExpr* SymbolicExpr::NewConcreteExpr(type_t ty, value_t val) {
  return new SymbolicExpr(kSizeOfType[ty], val);
}

SymbolicExpr* SymbolicExpr::NewConcreteExpr(size_t size, value_t val) {
  return new SymbolicExpr(size, val);
}

SymbolicExpr* SymbolicExpr::NewUnaryExpr(type_t ty, value_t val,
                                         ops::unary_op_t op, SymbolicExpr* e) {
  return new UnaryExpr(op, e, kSizeOfType[ty], val);
}

SymbolicExpr* SymbolicExpr::NewBinaryExpr(type_t ty, value_t val,
                                          ops::binary_op_t op,
                                          SymbolicExpr* e1, SymbolicExpr* e2) {
  return new BinaryExpr(op, e1, e2, kSizeOfType[ty], val);
}

SymbolicExpr* SymbolicExpr::NewBinaryExpr(type_t ty, value_t val,
                                          ops::binary_op_t op,
                                          SymbolicExpr* e1, value_t e2) {
  // TODO: Should special case for multiplying by a power of 2?
  return new BinaryExpr(op, e1, NewConcreteExpr(ty, e2), kSizeOfType[ty], val);
}


SymbolicExpr* SymbolicExpr::NewCompareExpr(type_t ty, value_t val,
                                           ops::compare_op_t op,
                                           SymbolicExpr* e1, SymbolicExpr* e2) {
  return new CompareExpr(op, e1, e2, kSizeOfType[ty], val);
}

SymbolicExpr* SymbolicExpr::NewConstDerefExpr(type_t ty, value_t val,
                                              const SymbolicObject& obj,
                                              addr_t addr) {
  // Copy the concrete bytes from the program.
  unsigned char* bytes = new unsigned char[obj.size()];
  memcpy((void*)bytes, (void*)addr, obj.size());

  return new DerefExpr(NewConcreteExpr(types::U_LONG, addr),
                       new SymbolicObject(obj), bytes,
                       kSizeOfType[ty], val);
}

SymbolicExpr* SymbolicExpr::NewDerefExpr(type_t ty, value_t val,
                                         const SymbolicObject& obj,
                                         SymbolicExpr* addr) {
  // Copy the concrete bytes from the program.
  unsigned char* bytes = new unsigned char[obj.size()];
  memcpy((void*)bytes, (void*)(addr->value()), obj.size());

  return new DerefExpr(addr, new SymbolicObject(obj), bytes,
                       kSizeOfType[ty], val);
}

SymbolicExpr* SymbolicExpr::Concatenate(SymbolicExpr *e1, SymbolicExpr *e2) {
  return new BinaryExpr(ops::CONCAT,
#ifdef CREST_BIG_ENDIAN
                        e1, e2,
#else
                        e2, e1,
#endif
                        e1->size() + e2->size(),
                        (e1->value() << (8 * e2->size())) + e2->value());
}


SymbolicExpr* SymbolicExpr::ExtractBytes(size_t size, value_t value,
                                         size_t i, size_t n) {
  // Assumption: i is n-aligned.
  assert(i % n == 0);

  // Little-Endian Example: Extract(0xABCDEF12, 4, 2) => 0xCD
  // Big-Endian Example: Extract(0xABCDEF12, 4, 2) => 0xEF
#ifdef CREST_BIG_ENDIAN
  i = size - i - n;
#endif

  // Extracting i-th, i+1-th, ..., i+n-1-th least significant bytes.
  return new SymbolicExpr(n, (value >> (8*i)) & ((1 << (8*n)) - 1));
}


SymbolicExpr* SymbolicExpr::ExtractBytes(SymbolicExpr* e, size_t i, size_t n) {
  // Assumption: i is n-aligned.
  assert(i % n == 0);

  // Little-Endian Example: Extract(0xABCDEF12, 4, 2) => 0xCD
  // Big-Endian Example: Extract(0xABCDEF12, 4, 2) => 0xEF
#ifdef CREST_BIG_ENDIAN
  i = e->size() - i - n;
#endif

  // Extracting i-th, i+1-th, ..., i+n-1-th least significant bytes.
  value_t val = (e->value() >> (8*i)) & ((1 << (8*n)) - 1);
  SymbolicExpr* i_e = NewConcreteExpr(types::U_LONG, i);
  return new BinaryExpr(ops::EXTRACT, e, i_e,  n, val);
}


SymbolicExpr* SymbolicExpr::Parse(istream& s) {
  value_t val;
  size_t size;
  var_t var;

  SymbolicExpr *left, *right, *child;
  compare_op_t cmp_op_;
  binary_op_t bin_op_;
  unary_op_t un_op_;

  SymbolicObject *obj;
  SymbolicExpr *addr;
  unsigned char* bytes;

  s.read((char*)&val, sizeof(value_t));
  if (s.fail()) return NULL;
  s.read((char*)&size, sizeof(size_t));
  if (s.fail()) return NULL;

  char type_ = s.get();
  switch(type_) {

  case kBasicNodeTag:
    s.read((char*)&var, sizeof(var_t));
    if(s.fail()) return NULL;
    return new BasicExpr(size, val, var);

  case kCompareNodeTag:
    cmp_op_ = (compare_op_t)s.get();
    if (s.fail()) return NULL;
    left = Parse(s);
    right = Parse(s);
    if (!left || !right) {
      // TODO: Leaks memory.
      return NULL;
    }
    return new CompareExpr(cmp_op_, left, right, size, val);

  case kBinaryNodeTag:
    bin_op_ = (binary_op_t)s.get();
    if (s.fail()) return NULL;
    left = Parse(s);
    right = Parse(s);
    if (!left || !right) {
      // TODO: Leaks memory.
      return NULL;
    }
    return new BinaryExpr(bin_op_, left, right, size, val);

  case kUnaryNodeTag:
    un_op_ = (unary_op_t)s.get();
    if (s.fail()) return NULL;
    child = Parse(s);
    if (child == NULL) return NULL;
    return new UnaryExpr(un_op_, child, size, val);

  case kDerefNodeTag:
    obj = SymbolicObject::Parse(s);
    if (obj == NULL) { // That means read has failed in object::Parse
      return NULL;
    }
    addr = SymbolicExpr::Parse(s);
    if (addr == NULL) { // Read has failed in expr::Parse
      delete obj;
      return NULL;
    }
    bytes = new unsigned char[obj->size()];
    s.read((char*)bytes, obj->size());
    if (s.fail()) {
      delete obj;
      delete addr;
      delete bytes;
      return NULL;
    }
    return new DerefExpr(addr, obj, bytes, size, val);

  case kConstNodeTag:
    return new SymbolicExpr(size, val);

  default:
    fprintf(stderr, "Unknown type of node: '%c'....exiting\n", type_);
    exit(1);
  }
}

void SymbolicExpr::Serialize(string *s) const {
	SymbolicExpr::Serialize(s, kConstNodeTag);
}

void SymbolicExpr::Serialize(string *s, char c) const {
  s->append((char*)&value_, sizeof(value_t));
  s->append((char*)&size_, sizeof(size_t));
  s->push_back(c);
}

}  // namespace crest