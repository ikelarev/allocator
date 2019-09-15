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

#include "allocation.h"
#include "mipsolver.h"

#include <functional>
#include <map>
#include <vector>

class Optimizer
{
public:
  using StatusCallback = std::function<bool (size_t iteration, int nodes, double progress)>;

  Optimizer(StatusCallback &&callback = nullptr);

  using RatesProvider = std::function<void (const std::string &ticker, double &bid, double &ask)>;
  bool Optimize(const Allocation &allocation, const RatesProvider &f);

  struct Result
  {
    std::string ticker;

    double bid;
    double ask;

    double have;
    double result;
    double change;
    double commission;

    bool inPercents;
    double percents;
    double sourcePercents;
  };

  const Result &GetResult(const std::string &ticker) const;
  const Result &GetCashResult() const;

  struct Quality
  {
    double abserr;
    double stddev;
  };

  const Quality &GetSourceQuality() const;
  const Quality &GetResultQuality() const;

private:
  using Diffs = std::vector<MIPSolver::Expression>;
  MIPSolver::Solution RunLadOptimization(MIPSolver &s, const Diffs &diff);
  MIPSolver::Solution RunLsOptimization(MIPSolver &s, const Diffs &diff);

  Quality CalculateQuality(const Diffs &diff, const MIPSolver::Solution &sol);

private:
  std::map<std::string, Result> result_;
  Result cashResult_;

  Quality qsource_;
  Quality qresult_;

  size_t iteration_;
  StatusCallback callback_;
};
