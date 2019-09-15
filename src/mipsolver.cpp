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

#include "mipsolver.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glpk.h>

#ifdef _DEBUG
  #include <iostream>
#endif

namespace
{
  int Unused = glp_term_out(GLP_OFF);
}

template<class T>
static void GlpkCallbackHelper(T *tree, void *param)
{
  assert(param);
  reinterpret_cast<MIPSolver *>(param)->GlpkCallback(tree);
}

static void GlpkCallbackHelper(glp_tree *tree, void *param)
{
  GlpkCallbackHelper<glp_tree>(tree, param);
}

MIPSolver::MIPSolver(StatusCallback &&callback) : callback_(callback)
{
  // GLPK does not allow optimization without variables
  Variable var = CreateVariable(GLP_CV, 0, 0);

  // GLPK does not allow optimization without conditions
  AddCondition(var == 0);
}

MIPSolver::Variable MIPSolver::GetBinaryVariable()
{
  return std::move(CreateBinaryVariable());
}

MIPSolver::Variable MIPSolver::GetIntegerVariable(double minValue, double maxValue)
{
  return std::move(CreateIntegerVariable(minValue, maxValue));
}

MIPSolver::Variable MIPSolver::GetIntegerVariable(double maxValue)
{
  return std::move(CreateIntegerVariable(0, maxValue));
}

void MIPSolver::Restrict(const Condition &cond)
{
  AddCondition(cond);
}

MIPSolver::Checkpoint MIPSolver::CreateCheckpoint() const
{
  Checkpoint cp;
  cp.vars = vars_.size();
  cp.conds = conds_.size();
  return cp;
}

void MIPSolver::Rollback(const Checkpoint &cp)
{
  assert(cp.vars <= vars_.size());
  assert(cp.conds <= conds_.size());

  vars_.resize(cp.vars);
  conds_.resize(cp.conds, Expression() == 0);
}

MIPSolver::Expression MIPSolver::GetAbsoluteValue(const Expression &expr)
{
  double minValue;
  double maxValue;
  GetExpressionBounds(expr, minValue, maxValue);

  if (minValue >= 0)
  {
    return std::move(expr);
  }
  else if (maxValue <= 0)
  {
    return std::move(-expr);
  }
  else
  {
    assert(maxValue > 0);
    assert(minValue < 0);

    Variable isPositive = CreateBinaryVariable();
    Variable pos = CreateContinuousVariable(0, maxValue);
    Variable neg = CreateContinuousVariable(minValue, 0);

    AddCondition(pos + neg == expr);

    AddCondition(pos <= maxValue * isPositive);
    AddCondition(neg >= minValue * (1 - isPositive));

    return std::move(pos - neg);
  }
}

MIPSolver::Expression MIPSolver::GetSquareApproximation(const Expression &expr, RefPoints &points)
{
  double minValue, maxValue;
  GetExpressionBounds(expr, minValue, maxValue);

  if (minValue == maxValue)
  {
    return minValue * maxValue;
  }

  if (points.empty())
  {
    points.insert(std::min(std::max(0., minValue), maxValue));
  }

  auto p1 = points.begin();
  double x1 = minValue;
  double y1 = p1 * (2 * x1 - p1);

  Expression parts, source, result;
  for (; p1 != points.end(); p1++)
  {
    auto p2 = std::next(p1);

    double x2, y2;
    if (p2 != points.end())
    {
      x2 = (p1 + p2) / 2;
      y2 = p1 * p2;
    }
    else
    {
      x2 = maxValue;
      y2 = p1 * (2 * x2 - p1);
    }
    assert(x2 > x1);

    Variable enable = CreateBinaryVariable();
    parts += enable;

    Variable x = CreateContinuousVariable(0, x2 - x1);
    AddCondition(x <= enable * (x2 - x1));

    source += x + x1 * enable;
    result += (y2 - y1) / (x2 - x1) * x + y1 * enable;

    x1 = x2;
    y1 = y2;
  }

  AddCondition(parts == 1);
  AddCondition(expr == source);

  return std::move(result);
}

MIPSolver::Solution MIPSolver::Minimize(const Expression &expr)
{
  return std::move(Optimize(expr));
}

MIPSolver::Solution MIPSolver::Maximize(const Expression &expr)
{
  return std::move(Optimize(-expr));
}

double MIPSolver::SignedFloor(double x)
{
  double res;
  modf(x, &res);
  if (x >= 0)
  {
    assert(res >= 0 && res <= x);
  }
  else
  {
    assert(res < 0 && res >= x);
  }
  return res;
}

void MIPSolver::GetExpressionBounds(const Expression &expr, double &minValue, double &maxValue)
{
  minValue = maxValue = expr.GetC();

  const Expression::FactorMap &f = expr.GetFactors();
  for (auto it = f.begin(); it != f.end(); it++)
  {
    assert(it->first < vars_.size());
    const VariableInfo &vi = vars_[it->first];

    double k = it->second;
    if (k > 0)
    {
      maxValue += vi.max * k;
      minValue += vi.min * k;
    }
    else
    {
      maxValue += vi.min * k;
      minValue += vi.max * k;
    }
  }

  assert(minValue <= maxValue);
}

MIPSolver::Variable MIPSolver::CreateBinaryVariable()
{
  return std::move(CreateVariable(GLP_BV, 0, 1));
}

MIPSolver::Variable MIPSolver::CreateIntegerVariable(double minValue, double maxValue)
{
  assert(minValue <= maxValue);
  assert(SignedFloor(minValue) <= SignedFloor(maxValue));
  return std::move(CreateVariable(GLP_IV, SignedFloor(minValue), SignedFloor(maxValue)));
}

MIPSolver::Variable MIPSolver::CreateContinuousVariable(double minValue, double maxValue)
{
  assert(minValue <= maxValue);
  return std::move(CreateVariable(GLP_CV, minValue, maxValue));
}

MIPSolver::Variable MIPSolver::CreateVariable(int type, double minValue, double maxValue)
{
  MIPSolver::Variable var(*this, static_cast<int>(vars_.size()));
  vars_.push_back({ type, minValue, maxValue });
  return std::move(var);
}

void MIPSolver::AddCondition(const Condition &cond)
{
  conds_.push_back(cond);
}

MIPSolver::Solution MIPSolver::Optimize(const Expression &expr)
{
  glp_prob *lp = glp_create_prob();

  glp_add_cols(lp, static_cast<int>(vars_.size()));
  glp_add_rows(lp, static_cast<int>(conds_.size()));

  std::vector<int> idx(vars_.size() + 1);
  for (int i = 0; i < vars_.size(); i++)
  {
    idx[i + 1] = i + 1;

    const VariableInfo &vi = vars_[i];
    assert(vi.type == GLP_IV || vi.type == GLP_CV || vi.type == GLP_BV);
    glp_set_col_kind(lp, i + 1, vi.type);

    if (vi.min == vi.max)
    {
      glp_set_col_bnds(lp, i + 1, GLP_FX, vi.min, vi.max);
    }
    else
    {
      glp_set_col_bnds(lp, i + 1, GLP_DB, vi.min, vi.max);
    }
  }

  for (int i = 0; i < conds_.size(); i++)
  {
    std::vector<double> row(vars_.size() + 1);

    const Condition &cond = conds_[i];
    const Expression &expr = cond.GetExpression();
    const Expression::FactorMap &f = expr.GetFactors();
    for (auto it = f.begin(); it != f.end(); it++)
    {
      row[it->first + 1] = it->second;
    }
    glp_set_mat_row(lp, i + 1, static_cast<int>(vars_.size()), &idx.front(), &row.front());

    int rel = cond.GetRelation();
    assert(rel == GLP_FX || rel == GLP_LO || rel == GLP_UP);
    glp_set_row_bnds(lp, i + 1, rel, -expr.GetC(), -expr.GetC());
  }

  const Expression::FactorMap &f = expr.GetFactors();
  for (auto it = f.begin(); it != f.end(); it++)
  {
    glp_set_obj_coef(lp, static_cast<int>(it->first + 1), it->second);
  }

  Solution res;

  glp_iocp iocp;
  glp_init_iocp(&iocp);
  iocp.msg_lev  = GLP_MSG_OFF;
  iocp.br_tech  = GLP_BR_DTH;
  iocp.bt_tech  = GLP_BT_BLB;
  iocp.pp_tech  = GLP_PP_ROOT;
  iocp.mir_cuts = GLP_OFF;
  iocp.gmi_cuts = GLP_OFF;
  iocp.cov_cuts = GLP_OFF;
  iocp.clq_cuts = GLP_OFF;
  iocp.presolve = GLP_ON;

  if (callback_)
  {
    callback_(0, 0);

    iocp.cb_func = &GlpkCallbackHelper;
    iocp.cb_info = this;
  }

  if (glp_intopt(lp, &iocp) == 0)
  {
    int status = glp_mip_status(lp);
    if (status == GLP_OPT)
    {
      Solution::Vector v(vars_.size());
      for (int i = 0; i < vars_.size(); i++)
      {
        v[i] = glp_mip_col_val(lp, i + 1);
      }
      res = Solution(v);
    }
  }

  glp_delete_prob(lp);

  if (callback_)
  {
    callback_(0, 1);
  }

  return res;
}

template<class T>
void MIPSolver::GlpkCallback(T *tree) const
{
  if (glp_ios_reason(tree) == GLP_ISELECT) // Do not do this too often
  {
    int a, n, t;
    glp_ios_tree_size(tree, &a, &n, &t);

    double gap = glp_ios_mip_gap(tree);
    if (gap < 0) gap = 0;
    if (gap > 1) gap = 1;

    assert(callback_);
    if (!callback_(a, 1 - gap))
    {
      glp_ios_terminate(tree);
    }
  }
}

#ifdef _DEBUG
void MIPSolver::Dump() const
{
  for (size_t i = 0; i < conds_.size(); i++)
  {
    const Condition &cond = conds_[i];
    const Expression &expr = cond.GetExpression();
    const Expression::FactorMap &f = expr.GetFactors();
    for (auto it = f.begin(); it != f.end(); it++)
    {
      if (it->second == 1)
        std::cout << "+ x" << it->first << " ";
      else if (it->second == -1)
        std::cout << "- x" << it->first << " ";
      else if (it->second > 0)
        std::cout << "+ " << it->second << " * x" << it->first << " ";
      else if (it->second < 0)
        std::cout << it->second << " * x" << it->first << " ";
    }

    int rel = cond.GetRelation();
    if (rel == GLP_FX) std::cout << "==";
    else if (rel == GLP_UP) std::cout << "<=";
    else if (rel == GLP_LO) std::cout << ">=";

    std::cout << " " << -expr.GetC() << std::endl;
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MIPSolver::Expression MIPSolver::Expression::operator -() const
{
  return std::move(*this * -1);
}

MIPSolver::Expression MIPSolver::Expression::operator +(const Expression &expr) const
{
  return std::move(Expression(*this) += expr);
}

MIPSolver::Expression MIPSolver::Expression::operator -(const Expression &expr) const
{
  return std::move(Expression(*this) -= expr);
}

MIPSolver::Expression MIPSolver::Expression::operator +(double n) const
{
  return std::move(Expression(*this) += n);
}

MIPSolver::Expression MIPSolver::Expression::operator -(double n) const
{
  return std::move(Expression(*this) -= n);
}

MIPSolver::Expression MIPSolver::Expression::operator *(double n) const
{
  return std::move(Expression(*this) *= n);
}

MIPSolver::Expression MIPSolver::Expression::operator /(double n) const
{
  return std::move(Expression(*this) /= n);
}

const MIPSolver::Expression &MIPSolver::Expression::operator +=(const Expression &expr)
{
  for (auto it = expr.f_.begin(); it != expr.f_.end(); it++)
  {
    f_[it->first] += it->second;
  }
  c_ += expr.c_;
  return *this;
}

const MIPSolver::Expression &MIPSolver::Expression::operator -=(const Expression &expr)
{
  return *this += -expr;
}

const MIPSolver::Expression &MIPSolver::Expression::operator +=(double n)
{
  c_ += n;
  return *this;
}

const MIPSolver::Expression &MIPSolver::Expression::operator -=(double n)
{
  return *this += -n;
}

const MIPSolver::Expression &MIPSolver::Expression::operator *=(double n)
{
  for (auto it = f_.begin(); it != f_.end(); it++)
  {
    f_[it->first] *= n;
  }
  c_ *= n;
  return *this;
}

const MIPSolver::Expression &MIPSolver::Expression::operator /=(double n)
{
  for (auto it = f_.begin(); it != f_.end(); it++)
  {
    f_[it->first] /= n;
  }
  c_ /= n;
  return *this;
}

MIPSolver::Condition MIPSolver::Expression::operator <=(const Expression &expr) const
{
  return std::move(Condition(*this - expr, GLP_UP));
}

MIPSolver::Condition MIPSolver::Expression::operator >=(const Expression &expr) const
{
  return std::move(Condition(*this - expr, GLP_LO));
}

MIPSolver::Condition MIPSolver::Expression::operator ==(const Expression &expr) const
{
  return std::move(Condition(*this - expr, GLP_FX));
}

MIPSolver::Expression operator +(double n, const MIPSolver::Expression &expr)
{
  return expr + n;
}

MIPSolver::Expression operator -(double n, const MIPSolver::Expression &expr)
{
  return -expr + n;
}

MIPSolver::Expression operator *(double n, const MIPSolver::Expression &expr)
{
  return expr * n;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double MIPSolver::Solution::operator ()(const Expression &expr) const
{
  double res = expr.GetC();

  const Expression::FactorMap &f = expr.GetFactors();
  for (auto it = f.begin(); it != f.end(); it++)
  {
    assert(it->first < x_.size());
    res += x_[it->first] * it->second;
  }

  return res;
}

#ifdef _DEBUG
void MIPSolver::Solution::Dump() const
{
  for (size_t i = 0; i < x_.size(); i++)
  {
    std::cout << "x[" << i << "] = " << x_[i] << std::endl;
  }
  std::cout << std::endl;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool MIPSolver::RefPoints::insert(double x)
{
  const double precision = 1; // TODO: configure?

  Key key = static_cast<Key>(round(x * precision));

  return points_.insert(std::make_pair(key, x)).second;
}

size_t MIPSolver::RefPoints::size()
{
  return points_.size();
}

bool MIPSolver::RefPoints::empty()
{
  return points_.empty();
}

MIPSolver::RefPoints::Iterator MIPSolver::RefPoints::begin()
{
  return std::move(Iterator(points_.begin()));
}

MIPSolver::RefPoints::Iterator MIPSolver::RefPoints::end()
{
  return std::move(Iterator(points_.end()));
}
