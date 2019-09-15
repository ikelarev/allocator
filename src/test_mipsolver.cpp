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

#include <catch.hpp>
#include <cmath>

TEST_CASE("TestNoSolution", "[mipsolver]")
{
  MIPSolver s1;
  auto x = s1.GetIntegerVariable(100);
  s1.Restrict(x <= 1);
  s1.Restrict(x >= 2);
  auto sol = s1.Maximize(x);
  REQUIRE(!sol);

  MIPSolver s2;
  x = s2.GetIntegerVariable(100);
  s2.Restrict(x <= -1);
  sol = s2.Maximize(x);
  REQUIRE(!sol);

  MIPSolver s3;
  x = s3.GetIntegerVariable(100);
  auto y = s3.GetIntegerVariable(100);
  s3.Restrict(x + y >= 10);
  s3.Restrict(x <= 4.9);
  s3.Restrict(y <= 4.9);
  sol = s3.Minimize(x + y);
  REQUIRE(!sol);
}

TEST_CASE("TestNoIntegerSolution", "[mipsolver]")
{
  MIPSolver s1;
  auto x = s1.GetIntegerVariable(100);
  s1.Restrict(x <= 1.9);
  s1.Restrict(x >= 1.1);
  auto sol = s1.Maximize(x);
  REQUIRE(!sol);
  sol = s1.Minimize(x);
  REQUIRE(!sol);

  MIPSolver s3;
  x = s3.GetIntegerVariable(100);
  auto y = s3.GetIntegerVariable(100);
  s3.Restrict(x + y >= 10.1);
  s3.Restrict(x <= 5.9);
  s3.Restrict(y <= 5.9);
  sol = s3.Minimize(x + y);
  REQUIRE(!sol);
}

TEST_CASE("MIPSolverTest1", "[mipsolver]")
{
  MIPSolver s;

  auto x0 = s.GetIntegerVariable(100);
  auto x1 = s.GetIntegerVariable(100);
  auto x2 = s.GetIntegerVariable(100);

  s.Restrict(4 * x0 + 3 * x1 -     x2 <= 10);
  s.Restrict(        -2 * x1 + 5 * x2 >=  3);
  s.Restrict(    x0          + 2 * x2 ==  9);

  auto sol = s.Minimize(x0 + 2 * x1 - 2 * x2);

  REQUIRE(sol);
  if (sol) { } else { REQUIRE(0); }
  if (!sol) { REQUIRE(0); }

  REQUIRE(sol(x0) == 1);
  REQUIRE(sol(x1) == 0);
  REQUIRE(sol(x2) == 4);

  REQUIRE(sol(x0 + 2 * x1 - 2 * x2) == -7);
}

TEST_CASE("MIPSolverTest2", "[mipsolver]")
{
  MIPSolver s;

  auto x0 = s.GetIntegerVariable(100);
  auto x1 = s.GetIntegerVariable(100);

  s.Restrict(-2 * x0 + 5 * x1 >= 3);

  auto y = s.GetIntegerVariable(100);

  s.Restrict(    y          + 2 * x1 ==  9);
  s.Restrict(4 * y + 3 * x0 -     x1 <= 10);

  auto sol = s.Maximize(2 * x1 - 2 * x0 - y);
  REQUIRE(sol);

  REQUIRE(sol(y) == 1);
  REQUIRE(sol(x0) == 0);
  REQUIRE(sol(x1) == 4);

  REQUIRE(sol(y + 2 * x0 - 2 * x1) == -7);
}

template<bool harder>
void TestHomam()
{
  struct Being
  {
    const char *name;

    struct
    {
      int gold;
      int gems;
      int mercury;
    } price;

    int force;
    int available;

    struct
    {
      bool far;
      bool fly;
    } is;
  };

  Being beings[] =
  {
    { "Titan",    { 5000, 3, 1 }, 300,  10, {  true, false } },
    { "Naga",     { 1500, 2, 0 }, 120,  20, { false, false } },
    { "Genie",    {  750, 1, 1 },  60,  30, { false,  true } },
    { "Mage",     {  500, 1, 1 },  40,  55, {  true, false } },
    { "Golem",    {  400, 0, 1 },  35,  60, { false, false } },
    { "Gargoyle", {  200, 0, 0 },  20, 110, { false,  true } },
    { "Gremlin",  {   70, 0, 0 },   4, 500, {  true, false } },
  };

  const size_t beingsCount = sizeof(beings) / sizeof(*beings);

  const int minFarForce = 4000;
  const int minFlyForce = 2000;

  const int haveGold    = harder ? 100000 : 200000;
  const int haveGems    = 115;
  const int haveMercury = 80;

  const int gemsToGoldRate = 500;

  MIPSolver s;
  std::vector<MIPSolver::Variable> x(beingsCount);
  for (size_t i = 0; i < beingsCount; i++)
  {
    x[i] = s.GetIntegerVariable(beings[i].available);
  }
  auto y = s.GetIntegerVariable(haveGems);

  MIPSolver::Expression gold, gems, mercury;
  MIPSolver::Expression force, far, fly;
  for (size_t i = 0; i < beingsCount; i++)
  {
    gold    += x[i] * beings[i].price.gold;
    gems    += x[i] * beings[i].price.gems;
    mercury += x[i] * beings[i].price.mercury;

    auto f = x[i] * beings[i].force;
    force += f;
    if (beings[i].is.far) far += f;
    if (beings[i].is.fly) fly += f;
  }

  gold -= gemsToGoldRate * y;
  gems += y;

  s.Restrict(gold <= haveGold);
  s.Restrict(gems <= haveGems);
  s.Restrict(mercury <= haveMercury);

  s.Restrict(far >= minFarForce);
  s.Restrict(fly >= minFlyForce);

  auto sol = s.Maximize(force);
  REQUIRE(sol);

#ifdef _DEBUG
  sol.Dump();
#endif

  if (harder)
  {
    REQUIRE(sol(force) == 9884);
    REQUIRE(sol(x[0]) == 7);
    REQUIRE(sol(2 * x[1] + x[2]) == 25);
    REQUIRE(sol(x[3]) == 0);
    REQUIRE(sol(x[4]) == 60);
    REQUIRE(sol(x[5]) == 110);
    REQUIRE(sol(x[6]) == 496);
    REQUIRE(sol(y) == 69);
  }
  else
  {
    REQUIRE(sol(force) == 12875);
    REQUIRE(sol(x[0]) == 10);
    REQUIRE(sol(x[1]) == 20);
    REQUIRE(sol(x[2]) == 30);
    REQUIRE(sol(x[3]) == 15);
    REQUIRE(sol(x[4]) == 25);
    REQUIRE(sol(x[5]) == 110);
    REQUIRE(sol(x[6]) == 500);
    REQUIRE(sol(y) == 0);
  }
}

TEST_CASE("TestHomam", "[mipsolver]")
{
  SECTION("Easier")
  {
    TestHomam<false>();
  }

  SECTION("Harder")
  {
    TestHomam<true>();
  }
}

TEST_CASE("TestOnOff", "[mipsolver]")
{
  MIPSolver s;

  auto x = s.GetIntegerVariable(100);
  auto y = s.GetIntegerVariable(100);

  auto u = s.GetBinaryVariable(); // turns x on/off
  auto v = s.GetBinaryVariable(); // turns y on/off

  s.Restrict(3 * u - x         <= 0);
  s.Restrict(        x - 5 * u <= 0);

  s.Restrict(3 * v - y         <= 0);
  s.Restrict(        y - 5 * v <= 0);

  s.Restrict(u + v == 1);

  auto sol = s.Minimize(x - y);
  REQUIRE(sol);
  REQUIRE(sol(x) == 0);
  REQUIRE(sol(y) == 5);
  REQUIRE(sol(u) == 0);
  REQUIRE(sol(v) == 1);

  sol = s.Maximize(x - y);
  REQUIRE(sol);
  REQUIRE(sol(x) == 5);
  REQUIRE(sol(y) == 0);
  REQUIRE(sol(u) == 1);
  REQUIRE(sol(v) == 0);
}

TEST_CASE("TestAbs", "[mipsolver]")
{
  MIPSolver s;
  auto x = s.GetIntegerVariable(-10, 20);
  auto absx = s.GetAbsoluteValue(x);

  auto sol = s.Maximize(absx);
  REQUIRE(sol);
  REQUIRE(sol(x) == 20);

  sol = s.Minimize(absx);
  REQUIRE(sol);
  REQUIRE(sol(x) == 0);

  auto absx1 = s.GetAbsoluteValue(x - 1);

  sol = s.Maximize(absx1);
  REQUIRE(sol);
  REQUIRE(sol(x) == 20);

  sol = s.Minimize(absx1);
  REQUIRE(sol);
  REQUIRE(sol(x) == 1);

  auto y = s.GetIntegerVariable(-20, 10);
  auto absy = s.GetAbsoluteValue(y);

  sol = s.Maximize(absy);
  REQUIRE(sol);
  REQUIRE(sol(y) == -20);

  sol = s.Minimize(absy);
  REQUIRE(sol);
  REQUIRE(sol(y) == 0);

  auto absy2 = s.GetAbsoluteValue(y + 2);

  sol = s.Maximize(absy2);
  REQUIRE(sol);
  REQUIRE(sol(y) == -20);

  sol = s.Minimize(absy2);
  REQUIRE(sol);
  REQUIRE(sol(y) == -2);

  auto absxy = s.GetAbsoluteValue(x + y);

  sol = s.Maximize(absxy);
  REQUIRE(sol);
  REQUIRE(fabs(sol(x + y)) == 30);

  sol = s.Minimize(absxy);
  REQUIRE(sol);
  REQUIRE(sol(x + y) == 0);

  s = MIPSolver();
  auto z = s.GetIntegerVariable(20);
  auto absz = s.GetAbsoluteValue(z);

  sol = s.Maximize(absz);
  REQUIRE(sol);
  REQUIRE(sol(z) == 20);
  sol = s.Minimize(absz);
  REQUIRE(sol);
  REQUIRE(sol(z) == 0);

  z = s.GetIntegerVariable(-10, 0);
  absz = s.GetAbsoluteValue(z);
  sol = s.Maximize(absz);
  REQUIRE(sol);
  REQUIRE(sol(z) == -10);
  sol = s.Minimize(absz);
  REQUIRE(sol);
  REQUIRE(sol(z) == 0);
}

TEST_CASE("TestQQ", "[mipsolver]")
{
  // approximate
  //   (x - 1)^2 + (y - 2)^2 --> min
  //   x + y >= 5

  MIPSolver s;

  auto x = s.GetIntegerVariable(100);
  auto y = s.GetIntegerVariable(100);
  s.Restrict(x + y >= 5);

  auto dx = s.GetAbsoluteValue(x - 1);
  auto dy = s.GetAbsoluteValue(y - 2);
  auto sum = dx + dy;

  auto sol = s.Minimize(sum);
  REQUIRE(sol);

  s.Restrict(sum <= sol(sum));

  auto ddx = s.GetAbsoluteValue(2 * dx - sum);
  auto ddy = s.GetAbsoluteValue(2 * dy - sum);

#ifdef _DEBUG
  s.Dump();
#endif

  sol = s.Minimize(ddx + ddy);
  REQUIRE(sol);
#ifdef _DEBUG
  sol.Dump();
#endif

  REQUIRE(sol(x) == 2);
  REQUIRE(sol(y) == 3);
}

void TestRandomQQ()
{
  // approximate
  //   (x - a)^2 + (y - b)^2 --> min
  //   x + y >= c, c >= a + b

  int a = rand() % 100;
  int b = rand() % 100;
  int c = a + b + rand() % 100;

  int min = 0;
  std::vector<int> exp;
  for (int i = 0; i <= c; i++)
  {
    int x = i;
    int y = c - x;
    int f = (x - a) * (x - a) + (y - b) * (y - b);
    if (i == 0 || f < min)
    {
      min = f;
      exp.resize(1);
      exp[0] = x;
    }
    else if (f == min)
    {
      exp.push_back(x);
    }
  }

  MIPSolver s;

  auto x = s.GetIntegerVariable(1000);
  auto y = s.GetIntegerVariable(1000);
  s.Restrict(x + y >= c);

  auto dx = s.GetAbsoluteValue(x - a);
  auto dy = s.GetAbsoluteValue(y - b);
  auto sum = dx + dy;

  auto sol = s.Minimize(sum);
  REQUIRE(sol);

  s.Restrict(sum <= sol(sum));

  auto ddx = s.GetAbsoluteValue(2 * dx - sum);
  auto ddy = s.GetAbsoluteValue(2 * dy - sum);

  sol = s.Minimize(ddx + ddy);
  REQUIRE(sol);

  while (sol(x) != exp[0])
  {
    exp.erase(exp.begin());
    REQUIRE(!exp.empty());
  }
  REQUIRE(sol(x + y) == c);
}

TEST_CASE("TestRandomQQ", "[mipsolver]")
{
  for (int i = 0; i < 100; i++)
  {
    TestRandomQQ();
  }
}

TEST_CASE("TestQQQ", "[mipsolver]")
{
  // approximate
  //   (x - 1)^2 + (y - 2)^2 + (z - 3)^2 --> min
  //   x + y + z >= 11

  MIPSolver s;

  auto x = s.GetIntegerVariable(100);
  auto y = s.GetIntegerVariable(100);
  auto z = s.GetIntegerVariable(100);
  s.Restrict(x + y + z >= 11);

  auto dx = s.GetAbsoluteValue(x - 1);
  auto dy = s.GetAbsoluteValue(y - 2);
  auto dz = s.GetAbsoluteValue(z - 3);
  auto sum = dx + dy + dz;

  auto sol = s.Minimize(sum);
  REQUIRE(sol);

  s.Restrict(sum <= sol(sum));

  auto ddx = s.GetAbsoluteValue(3 * dx - sum);
  auto ddy = s.GetAbsoluteValue(3 * dy - sum);
  auto ddz = s.GetAbsoluteValue(3 * dz - sum);

#ifdef _DEBUG
  s.Dump();
#endif

  sol = s.Minimize(ddx + ddy + ddz);
  REQUIRE(sol);
#ifdef _DEBUG
  sol.Dump();
#endif

  REQUIRE(pow(sol(x) - 1, 2) + pow(sol(y) - 2, 2) + pow(sol(z) - 3, 2) == 9);
  REQUIRE(sol(x + y + z) == 11);
}

TEST_CASE("TestSquares", "[mipsolver]")
{
  // approximate
  //   (x - 1)^2 + (y - 2)^2 --> min
  //   x + y >= 5

  MIPSolver s;

  auto x = s.GetIntegerVariable(10);
  auto y = s.GetIntegerVariable(10);
  s.Restrict(x + y >= 5);

  MIPSolver::RefPoints refpoints;
  refpoints.insert(0);
  refpoints.insert(1);
  refpoints.insert(2);
  auto dx = s.GetSquareApproximation(x - 1, refpoints);
  auto dy = s.GetSquareApproximation(y - 2, refpoints);
  auto sum = dx + dy;

#ifdef _DEBUG
  s.Dump();
#endif

  auto sol = s.Minimize(sum);
  REQUIRE(sol);

  REQUIRE(sol(x) == 2);
  REQUIRE(sol(y) == 3);
}

TEST_CASE("SquareApproximationTest", "[mipsolver]")
{
  struct
  {
    double from, to, gran;
  }
  intervals[] =
  {
    { -3, 3, 2 },
    { -9, 10, 1 },
    { -10, 9, 0.000001 },
    { -9, 10, 0.000001 },
    { 0, 10, 1.5 },
    { -10, 0, 0.9 },
    { 10, 20, 2.3 },
    { -20, -10, 0.01 },
    { 0, 100000, 0.00001 },
    { -100000, -50, 0.01 },
  };

  for (size_t i = 0; i < sizeof(intervals) / sizeof(*intervals); i++)
  {
    size_t steps = 100 / (i + 1);
    double dx = ceil((intervals[i].to - intervals[i].from) * 1. / steps);

    MIPSolver::RefPoints refpoints;
    for (size_t j = 0; j < steps; j++)
    {
      double x = intervals[i].from + dx * j;
      if (x > intervals[i].to) break;
      refpoints.insert(x);
    }
    refpoints.insert(intervals[i].from);
    refpoints.insert(intervals[i].to);

    MIPSolver s;
    auto x = s.GetIntegerVariable(intervals[i].from, intervals[i].to);
    auto q = s.GetSquareApproximation(x, refpoints);

    // max
    auto sol = s.Maximize(q);
    REQUIRE(sol);
    if (fabs(intervals[i].from) > fabs(intervals[i].to))
    {
      REQUIRE(sol(x) == intervals[i].from);
    }
    else if (fabs(intervals[i].from) < fabs(intervals[i].to))
    {
      REQUIRE(sol(x) == intervals[i].to);
    }
    else
    {
      REQUIRE(fabs(intervals[i].from) == fabs(intervals[i].to));
      REQUIRE(fabs(sol(x)) == fabs(intervals[i].from));
      REQUIRE(fabs(sol(x)) == fabs(intervals[i].to));
    }
    REQUIRE(fabs(sol(q) - sol(x) * sol(x)) < 1e-6);

    // min
    sol = s.Minimize(q);
    REQUIRE(sol);
    if (intervals[i].from * intervals[i].to <= 0)
    {
      REQUIRE(sol(x) == 0);
    }
    else if (intervals[i].to < 0)
    {
      REQUIRE(sol(x) == intervals[i].to);
    }
    else
    {
      REQUIRE(sol(x) == intervals[i].from);
    }
    REQUIRE(fabs(sol(q) - sol(x) * sol(x)) < 1e-6);

    // internal points
    double xprev = 0, yprev = 0;

    for (size_t j = 0; j < steps; j++)
    {
      double x1 = intervals[i].from + dx * j;
      if (x1 > intervals[i].to) break;

      double x2 = x1 + dx;
      if (x2 > intervals[i].to) x2 = intervals[i].to;

      double xval = (x1 + x2) / 2;

      MIPSolver t = s;
      t.Restrict(x >= x1);
      t.Restrict(x <= x2);

      auto min = t.Minimize(q);
      REQUIRE(min);

      auto max = t.Maximize(q);
      REQUIRE(max);

      double x0 = (min(x) + max(x)) / 2;
      REQUIRE(x0 >= x1);
      REQUIRE(x0 <= x2);

      double y = (min(q) + max(q)) / 2;

      if (j > 0)
      {
        if (xval * xval > xprev * xprev)
        {
          REQUIRE(y >= yprev);
        }
        else if (xval * xval < xprev * xprev)
        {
          REQUIRE(y <= yprev);
        }
        else
        {
          REQUIRE(fabs(y - yprev) < 1e-6);
        }
      }

      xprev = xval;
      yprev = y;
    }
  }
}

TEST_CASE("CheckpointTest", "[mipsolver]")
{
  MIPSolver s;

  MIPSolver::Variable x = s.GetIntegerVariable(100);

  MIPSolver::Solution sol = s.Maximize(x);
  REQUIRE(sol(x) == 100);

  MIPSolver::Checkpoint cp1 = s.CreateCheckpoint();
  s.Restrict(x <= 50);

  sol = s.Maximize(x);
  REQUIRE(sol(x) == 50);

  MIPSolver::Checkpoint cp2 = s.CreateCheckpoint();
  s.Restrict(2 * x <= 20);

  sol = s.Maximize(x);
  REQUIRE(sol(x) == 10);

  s.Rollback(cp2);
  sol = s.Maximize(x);
  REQUIRE(sol(x) == 50);

  s.Rollback(cp1);
  sol = s.Maximize(x);
  REQUIRE(sol(x) == 100);
}
