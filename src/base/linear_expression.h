// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef BASE_LINEAR_EXPRESSION_H__
#define BASE_LINEAR_EXPRESSION_H__

#include <istream>
#include <map>
#include <ostream>
#include <set>
#include <string>

#include "base/basic_types.h"

using std::istream;
using std::map;
using std::ostream;
using std::set;
using std::string;

namespace crest {

class LinearExpr {
 public:
  // Constructs a symbolic expression for the constant 0.
  LinearExpr();

  // Constructs a symbolic expression for the given constant 'c'.
  explicit LinearExpr(value_t c);

  // Constructs a symbolic expression for the singleton 'c' * 'v'.
  LinearExpr(value_t c, var_t v);

  // Copy constructor.
  LinearExpr(const LinearExpr& e);

  // Desctructor.
  ~LinearExpr();

  void Negate();
  bool IsConcrete() const { return coeff_.empty(); }
  size_t Size() const { return (1 + coeff_.size()); }
  void AppendVars(set<var_t>* vars) const;
  bool DependsOn(const map<var_t,type_t>& vars) const;

  void AppendToString(string* s) const;

  void Serialize(string* s) const;
  bool Parse(istream& s);

  // Arithmetic operators.
  const LinearExpr& operator+=(const LinearExpr& e);
  const LinearExpr& operator-=(const LinearExpr& e);
  const LinearExpr& operator+=(value_t c);
  const LinearExpr& operator-=(value_t c);
  const LinearExpr& operator*=(value_t c);
  bool operator==(const LinearExpr& e) const;

  // Accessors.
  value_t const_term() const { return const_; }
  const map<var_t,value_t>& terms() const { return coeff_; }
  typedef map<var_t,value_t>::const_iterator TermIt;

 private:
  value_t const_;
  map<var_t,value_t> coeff_;

};

}  // namespace crest

#endif  // BASE_LINEAR_EXPRESSION_H__

