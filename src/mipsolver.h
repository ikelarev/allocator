// MIT License
//
// Copyright (c) 2019 Ivan Kelarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <functional>
#include <map>
#include <set>
#include <vector>

class MIPSolver
{
public:
  using StatusCallback = std::function<bool (int activeNodes, double progress)>;

  MIPSolver(StatusCallback &&callback = nullptr);

  class Variable;
  Variable GetBinaryVariable();
  Variable GetIntegerVariable(double minValue, double maxValue);
  Variable GetIntegerVariable(double maxValue);

  class Condition;
  void Restrict(const Condition &cond);

  struct Checkpoint;
  Checkpoint CreateCheckpoint() const;
  void Rollback(const Checkpoint &cp);

  class Expression;
  Expression GetAbsoluteValue(const Expression &expr);

  class RefPoints;
  Expression GetSquareApproximation(const Expression &expr, RefPoints &refpoints);

  class Solution;
  Solution Minimize(const Expression &expr);
  Solution Maximize(const Expression &expr);

#ifdef _DEBUG
  void Dump() const;
#endif

private:
  static double SignedFloor(double x);

  void GetExpressionBounds(const Expression &expr, double &minValue, double &maxValue);

  Variable CreateBinaryVariable();
  Variable CreateIntegerVariable(double minValue, double maxValue);
  Variable CreateContinuousVariable(double minValue, double maxValue);
  Variable CreateVariable(int type, double minValue, double maxValue);

  void AddCondition(const Condition &cond);

  Solution Optimize(const Expression &expr);

  template<class T>
  void GlpkCallback(T *tree) const;

private:
  struct VariableInfo
  {
    int type;
    double min;
    double max;
  };

  std::vector<VariableInfo> vars_;
  std::vector<Condition> conds_;

  StatusCallback callback_;

  template<class T>
  friend void GlpkCallbackHelper(T *tree, void *param);
};

class MIPSolver::Expression
{
public:
  using Condition = MIPSolver::Condition;
  using FactorMap = std::map<size_t, double>;

  Expression(double c = 0) : c_(c) { }

  Expression operator -() const;

  Expression operator +(const Expression &expr) const;
  Expression operator -(const Expression &expr) const;

  Expression operator +(double n) const;
  Expression operator -(double n) const;
  Expression operator *(double n) const;
  Expression operator /(double n) const;

  const Expression &operator +=(const Expression &expr);
  const Expression &operator -=(const Expression &expr);

  const Expression &operator +=(double n);
  const Expression &operator -=(double n);
  const Expression &operator *=(double n);
  const Expression &operator /=(double n);

  Condition operator <=(const Expression &expr) const;
  Condition operator >=(const Expression &expr) const;
  Condition operator ==(const Expression &expr) const;

  const FactorMap &GetFactors() const { return f_; }
  double GetC() const { return c_; }

protected:
  Expression(const MIPSolver &, size_t index)
  {
    f_[index] = 1;
    c_ = 0;
  }

private:
  FactorMap f_;
  double c_;
};

MIPSolver::Expression operator +(double n, const MIPSolver::Expression &expr);
MIPSolver::Expression operator -(double n, const MIPSolver::Expression &expr);
MIPSolver::Expression operator *(double n, const MIPSolver::Expression &expr);

class MIPSolver::Variable : public MIPSolver::Expression
{
public:
  Variable() = default;

private:
  Variable(const MIPSolver &s, size_t index) : Expression(s, index) { }

  friend class MIPSolver;
};

class MIPSolver::Condition
{
public:
  using Expression = MIPSolver::Expression;

  const Expression &GetExpression() const { return expr_; }
  int GetRelation() const { return relation_; }

private:
  Condition(const Expression &expr, int relation) : expr_(expr), relation_(relation) { }

private:
  Expression expr_;
  int relation_;

  friend class MIPSolver::Expression;
};

struct MIPSolver::Checkpoint
{
private:
  size_t vars;
  size_t conds;

  friend class MIPSolver;
};

class MIPSolver::Solution
{
public:
  using Expression = MIPSolver::Expression;

  Solution() = default;

  explicit operator bool() const { return !x_.empty(); }
  double operator ()(const Expression &expr) const;

#ifdef _DEBUG
  void Dump() const;
#endif

private:
  using Vector = std::vector<double>;

  Solution(const Vector &x) : x_(x) { }

private:
  Vector x_;

  friend class MIPSolver;
};

class MIPSolver::RefPoints
{
public:
  bool insert(double x);
  size_t size();
  bool empty();

  class Iterator;
  Iterator begin();
  Iterator end();

private:
  using Key = int64_t;
  using Map = std::map<Key, double>;

  Map points_;
};

class MIPSolver::RefPoints::Iterator : public Map::iterator
{
public:
  Iterator(const Map::iterator &it) : Map::iterator(it) { }

  operator double() const
  {
    return (*this)->second;
  }
};
