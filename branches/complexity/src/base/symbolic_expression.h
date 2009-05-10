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
 * Authors: Jacob Burnim (jburnim@cs.berkeley.edu)
 *          Sudeep Juvekar (Sjuvekar@eecs.berkeley.edu)
 */

// TODO:
// (1) Implement Parse
// Serialization is done using ( ) parantheses and prefix notation.

#ifndef BASE_SYMBOLIC_EXPRESSION_H__
#define BASE_SYMBOLIC_EXPRESSION_H__

#include <istream>
#include <map>
#include <set>
#include <string>

#include "base/basic_types.h"

using std::istream;
using std::map;
using std::set;
using std::string;

typedef void* yices_expr;
typedef void* yices_context;

namespace crest {

class SymbolicObject;
class UnaryExpr;
class BinaryExpr;
class DerefExpr;
class CompareExpr;
class BasicExpr;

class SymbolicExpr {
 public:
  virtual ~SymbolicExpr();

  virtual SymbolicExpr* Clone() const;

  virtual void AppendVars(set<var_t>* vars) const { }
  virtual bool DependsOn(const map<var_t,type_t>& vars) const { return false; }
  virtual void AppendToString(string* s) const;

  virtual bool IsConcrete() const { return true; }

  // Convert to Yices.
  virtual yices_expr BitBlast(yices_context ctx) const;

  // Parsing
  static SymbolicExpr* Parse(istream& s);

  //Serialization: Format
  // Value | size | Node type | operator/var | children
  virtual void Serialize(string* s) const;

  // Factory methods for constructing symbolic expressions.
  static SymbolicExpr* NewConcreteExpr(type_t ty, value_t val);
  static SymbolicExpr* NewConcreteExpr(size_t size, value_t val);

  static SymbolicExpr* NewUnaryExpr(type_t ty, value_t val,
                                    ops::unary_op_t op, SymbolicExpr* e);

  static SymbolicExpr* NewBinaryExpr(type_t ty, value_t val,
                                     ops::binary_op_t op,
                                     SymbolicExpr* e1, SymbolicExpr* e2);

  static SymbolicExpr* NewBinaryExpr(type_t ty, value_t val,
                                     ops::binary_op_t op,
                                     SymbolicExpr* e1, value_t e2);

  static SymbolicExpr* NewCompareExpr(type_t ty, value_t val,
                                      ops::compare_op_t op,
                                      SymbolicExpr* e1, SymbolicExpr* e2);

  static SymbolicExpr* NewConstDerefExpr(type_t ty, value_t val,
                                         const SymbolicObject& obj,
                                         addr_t addr);

  static SymbolicExpr* NewDerefExpr(type_t ty, value_t val,
                                    const SymbolicObject& obj,
                                    SymbolicExpr* addr);

  static SymbolicExpr* Concatenate(SymbolicExpr* e1, SymbolicExpr* e2);

  // Extract n bytes from e, starting at the i-th leftmost byte
  // (and then the i+1-th most significant, etc.).
  //
  // NOTE: This function (should) respect endian-ness, returning
  // *most* significant bytes when configured as big-endian and
  // *least* significant bytes when confugred as little-endian.
  static SymbolicExpr* ExtractBytes(SymbolicExpr* e, size_t i, size_t n);
  static SymbolicExpr* ExtractBytes(size_t size, value_t value,
                                    size_t i, size_t n);

  // Virtual methods for dynamic casting.
  virtual const UnaryExpr* CastUnaryExpr() const { return NULL; }
  virtual const BinaryExpr* CastBinaryExpr() const { return NULL; }
  virtual const DerefExpr* CastDerefExpr() const { return NULL; }
  virtual const CompareExpr* CastCompareExpr() const { return NULL; }
  virtual const BasicExpr* CastBasicExpr() const { return NULL; }

  // Equals.
  virtual bool Equals(const SymbolicExpr &e) const;

  // Accessors.
  value_t value() const { return value_; }
  size_t size() const { return size_; }

 protected:
  // Constructor for sub-classes.
  SymbolicExpr(size_t size, value_t value)
    : value_(value), size_(size) { }

  //Serializing with a Tag
  void Serialize(string* s, char c) const;

  enum kNodeTags {
	  kBasicNodeTag = 0,
	  kCompareNodeTag = 1,
	  kBinaryNodeTag = 2,
	  kUnaryNodeTag = 3,
	  kDerefNodeTag = 4,
	  kConstNodeTag = 5

  };

 private:
  const value_t value_;
  const size_t size_;
};

}  // namespace crest

#endif  // BASE_SYMBOLIC_EXPRESSION_H__
