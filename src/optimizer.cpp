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

#include "optimizer.h"

#include <cassert>
#include <cmath>

Optimizer::Optimizer(StatusCallback &&callback) : callback_(callback)
{
  cashResult_.bid = 1;
  cashResult_.ask = 1;
  cashResult_.commission = 0;
}

bool Optimizer::Optimize(const Allocation &allocation, const RatesProvider &f)
{
  std::vector<double> bid(allocation.GetCount());
  std::vector<double> ask(allocation.GetCount());

  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    f(allocation.GetTicker(i), bid[i], ask[i]);

    assert(bid[i] >= 0);
    assert(ask[i] > 0);

    assert(ask[i] >= bid[i]);
  }


  result_.empty();
  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    const std::string &ticker = allocation.GetTicker(i);
    result_[ticker].ticker = ticker;

    result_[ticker].bid = bid[i];
    result_[ticker].ask = ask[i];

    result_[ticker].have   = allocation.GetExistingShares(i);
  }

  assert(cashResult_.ticker.empty());
  assert(cashResult_.bid == 1);
  assert(cashResult_.ask == 1);
  assert(cashResult_.commission == 0);

  cashResult_.have   = allocation.GetExistingCash();


  // Upper estimation
  double upperBound = allocation.GetExistingCash();
  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    upperBound += allocation.GetExistingShares(i) * bid[i];
  }


  auto callback = [&](int activeNodes, double progress) -> bool
  {
    assert(callback_);
    return callback_(iteration_, activeNodes, progress);
  };

  MIPSolver s(callback_ ? callback : MIPSolver::StatusCallback());

  std::vector<MIPSolver::Expression> count(allocation.GetCount());
  std::vector<MIPSolver::Expression> commission(allocation.GetCount());
  std::vector<MIPSolver::Expression> oneMore(allocation.GetCount());

  MIPSolver::Expression totalDeals;
  MIPSolver::Expression cash = allocation.GetExistingCash();

  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    double exists = allocation.GetExistingShares(i);

    count[i] = exists;

    MIPSolver::Expression allDeals;

    if (allocation.CanBuy(i))
    {
      // Upper estimation
      double maxBuyVol = floor((upperBound - allocation.GetExistingShares(i) * bid[i]) / ask[i]);

      if (maxBuyVol > 0)
      {
        auto buy = s.GetBinaryVariable();
        allDeals += buy;

        auto buyVol = s.GetIntegerVariable(maxBuyVol);
        s.Restrict(buyVol >=         1 * buy);
        s.Restrict(buyVol <= maxBuyVol * buy);

        count[i]   += buyVol;
        cash       -= buyVol * ask[i];
        oneMore[i] += buy    * ask[i];
      }
    }

    if (allocation.CanSell(i) && exists > 0)
    {
      auto sellAll = s.GetBinaryVariable();
      allDeals += sellAll;

      count[i]   -= sellAll * exists;
      cash       += sellAll * exists * bid[i];
      oneMore[i] += sellAll * (exists * bid[i] - allocation.GetCommission(i));

      double maxSellVol = floor(exists);
      if (maxSellVol != exists) maxSellVol--;

      if (maxSellVol > 1)
      {
        assert(maxSellVol >= 2);
        auto sell = s.GetBinaryVariable();
        allDeals += sell;

        auto sellVol = s.GetIntegerVariable(maxSellVol);
        s.Restrict(sellVol >= 1 * sell);
        s.Restrict(sellVol <= maxSellVol * sell);

        count[i]   -= sellVol;
        cash       += sellVol * bid[i];
        oneMore[i] += sell    * bid[i];
      }
    }

    totalDeals += allDeals;
    s.Restrict(allDeals <= 1);

    commission[i] = allocation.GetCommission(i) * allDeals;
    cash -= commission[i];

    if (allocation.CanBuy(i))
    {
      oneMore[i] += (1 - allDeals) * (ask[i] + allocation.GetCommission(i));
    }
    else
    {
      oneMore[i] += (1 - allDeals) * (upperBound + 0.01);
    }
  }

  if (allocation.GetMaxDeals() > 0)
  {
    s.Restrict(totalDeals <= static_cast<double>(allocation.GetMaxDeals()));
  }


  MIPSolver::Expression volume;
  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    if (allocation.IsTargetInPercents(i))
    {
      volume += count[i] * bid[i];
    }
  }

  if (allocation.IsTargetCashInPercents())
  {
    volume += cash;
  }


  Diffs diff(allocation.GetCount() + (allocation.HasTargetCash() ? 1 : 0));
  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    MIPSolver::Expression target;

    if (allocation.IsTargetInPercents(i))
    {
      target = volume * allocation.GetTargetShares(i) * 0.01;
    }
    else
    {
      target = allocation.GetTargetShares(i) * bid[i];
    }

    diff[i] = count[i] * bid[i] - target;
  }

  if (allocation.HasTargetCash())
  {
    MIPSolver::Expression cashTarget;
    if (allocation.IsTargetCashInPercents())
    {
      cashTarget = volume * allocation.GetTargetCash() * 0.01;
    }
    else
    {
      cashTarget = allocation.GetTargetCash();
    }

    assert(diff.size() == allocation.GetCount() + 1);
    diff[diff.size() - 1] = cash - cashTarget;
  }


  MIPSolver::Checkpoint cp = s.CreateCheckpoint();
  s.Restrict(totalDeals == 0);
  iteration_ = 0;
  MIPSolver::Solution source = s.Minimize(0);
  assert(source);
  assert(source(cash) == allocation.GetExistingCash());
  s.Rollback(cp);

  s.Restrict(cash >= 0);

  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    if (allocation.UseAllCash())
    {
      s.Restrict(cash <= oneMore[i] - 0.01);
    }
    else if (allocation.IsTargetInPercents(i))
    {
      // An artificial restriction to avoid trivial solutions
      s.Restrict(volume >= cash - oneMore[i] + 0.01);
    }
  }


  MIPSolver::Solution sol;
  if (allocation.UseLeastSquaresApproximation())
  {
    sol = RunLsOptimization(s, diff);
  }
  else
  {
    sol = RunLadOptimization(s, diff);
  }


  if (sol)
  {
    for (size_t i = 0; i < allocation.GetCount(); i++)
    {
      const std::string &ticker = allocation.GetTicker(i);
      result_[ticker].result     = sol(count[i]);
      result_[ticker].commission = sol(commission[i]);
    }
    cashResult_.result = sol(cash);
  }
  else
  {
    for (size_t i = 0; i < allocation.GetCount(); i++)
    {
      const std::string &ticker = allocation.GetTicker(i);
      result_[ticker].result     = result_[ticker].have;
      result_[ticker].commission = 0;
    }
    cashResult_.result = cashResult_.have;
  }

  for (auto it = result_.begin(); it != result_.end(); it++)
  {
    it->second.change = it->second.result - it->second.have;

  }
  cashResult_.change = cashResult_.result - cashResult_.have;


  double sourceVolume = source(volume);
  for (size_t i = 0; i < allocation.GetCount(); i++)
  {
    const std::string &ticker = allocation.GetTicker(i);
    Result &r = result_[ticker];
    r.percents = 0;
    r.sourcePercents = 0;

    r.inPercents = allocation.IsTargetInPercents(i);
    if (r.inPercents)
    {
      if (sourceVolume > 0)
      {
        r.sourcePercents = 100 * r.have * bid[i] / sourceVolume;
      }
      if (sol)
      {
        double volumeValue = sol(volume);
        if (volumeValue > 0)
        {
          r.percents = 100 * r.result * bid[i] / volumeValue;
        }
      }
    }
  }

  cashResult_.inPercents = allocation.IsTargetCashInPercents();
  cashResult_.percents = 0;
  cashResult_.sourcePercents = 0;
  if (cashResult_.inPercents)
  {
    if (sourceVolume > 0)
    {
      cashResult_.sourcePercents = 100 * cashResult_.have / sourceVolume;
    }
    if (sol)
    {
      double volumeValue = sol(volume);
      if (volumeValue > 0)
      {
        cashResult_.percents = 100 * cashResult_.result / volumeValue;
      }
    }
  }

  qsource_ = CalculateQuality(diff, source); // TODO: add tests
  if (sol)
  {
    qresult_ = CalculateQuality(diff, sol);
  }


  if (!sol)
  {
    for (size_t i = 0; i < allocation.GetCount(); i++)
    {
      const std::string &ticker = allocation.GetTicker(i);
      Result &r = result_[ticker];
      r.percents = r.sourcePercents;
    }
    cashResult_.percents = cashResult_.sourcePercents;
    qresult_ = qsource_;
  }

  return !!sol;
}

const Optimizer::Result &Optimizer::GetResult(const std::string &ticker) const
{
  auto it = result_.find(ticker);
  assert(it != result_.end());
  return it->second;
}

const Optimizer::Result &Optimizer::GetCashResult() const
{
  return cashResult_;
}

const Optimizer::Quality &Optimizer::GetSourceQuality() const
{
  return qsource_;
}

const Optimizer::Quality &Optimizer::GetResultQuality() const
{
  return qresult_;
}

MIPSolver::Solution Optimizer::RunLadOptimization(MIPSolver &s, const Diffs &diff)
{
  Diffs abs(diff.size());
  MIPSolver::Expression sum;
  for (size_t i = 0; i < diff.size(); i++)
  {
    abs[i] = s.GetAbsoluteValue(diff[i]);
    sum += abs[i];
  }

  iteration_ = 1;
  MIPSolver::Solution sol = s.Minimize(sum);
  if (sol)
  {
    s.Restrict(sum <= sol(sum));
    MIPSolver::Expression avg = sum / static_cast<double>(diff.size());

    MIPSolver::Expression var;
    for (size_t i = 0; i < diff.size(); i++)
    {
      var += s.GetAbsoluteValue(abs[i] - avg);
    }

    iteration_ = 2;
    sol = s.Minimize(var);
    assert(sol);
  }

  return std::move(sol);
}

MIPSolver::Solution Optimizer::RunLsOptimization(MIPSolver &s, const Diffs &diff)
{
  MIPSolver::Checkpoint cp = s.CreateCheckpoint();

  std::vector<MIPSolver::RefPoints> refpoints(diff.size());

  MIPSolver::Solution sol;
  for (iteration_ = 1;; iteration_++)
  {
    MIPSolver::Expression sum;
    for (size_t i = 0; i < diff.size(); i++)
    {
      sum += s.GetSquareApproximation(diff[i], refpoints[i]);
    }

    sol = s.Minimize(sum);
    assert(iteration_ == 1 || sol);
    if (!sol) break;

    bool done = true;
    for (size_t i = 0; i < diff.size(); i++)
    {
      if (refpoints[i].insert(sol(diff[i]))) done = false;
    }
    if (done) break;

    s.Rollback(cp);
  }

  return std::move(sol);
}

Optimizer::Quality Optimizer::CalculateQuality(const Diffs &diff, const MIPSolver::Solution &sol)
{
  Quality q;

  q.abserr = 0;
  double sumsqr = 0;

  for (size_t i = 0; i < diff.size(); i++)
  {
    double delta = sol(diff[i]);

    q.abserr += fabs(delta);
    sumsqr += delta * delta;
  }

  q.abserr /= diff.size();

  q.stddev = sumsqr / diff.size();
  assert(q.stddev >= 0);
  q.stddev = sqrt(q.stddev);

  return std::move(q);
}
