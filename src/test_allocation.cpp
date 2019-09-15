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

#include "allocation.h"

#include <catch.hpp>
#include <sstream>

TEST_CASE("EmptyAllocationTest", "[allocation]")
{
  Allocation a;

  REQUIRE(a.GetCount() == 0);
  REQUIRE(a.GetExistingCash() == 0);
  REQUIRE(!a.HasTargetCash());
  REQUIRE(a.UseAllCash() == false);
  REQUIRE(a.GetMaxDeals() == 0);

  std::string s = "[have]\n\n\nspy=0\n\n";
  std::stringstream ss(s);

  bool b = a.Load(ss);
  REQUIRE(b);

#ifdef _DEBUG
  a.Dump();
#endif

  REQUIRE(a.GetCount() == 1);
  REQUIRE(a.GetTicker(0) == "SPY");
  REQUIRE(a.GetExistingShares(0) == 0);
  REQUIRE(a.GetTargetShares(0) == 0);
  REQUIRE(!a.IsTargetInPercents(0));
  REQUIRE(a.GetCommission(0) == 0);
  REQUIRE(a.CanBuy(0) == true);
  REQUIRE(a.CanSell(0) == true);
}

TEST_CASE("FullAllocationTest", "[allocation]")
{
  std::string s =
    "[have]\n"
    "vti = 1\n"
    "ief = 3.7\n"
    "vti = 2\n"
    "vnq = 6\n"
    "\n"
    "[want]\n"
    "vti = 4\n"
    "vnq = 15%\n"
    "\n"
    "[trade]\n"
    "vnq=keep\n"
    "vti=buy\n"
    "iau=sell\n"
    "\n"
    "[cash]\n"
    "have=17.3\n"
    "withdraw=27.1\n"
    "want=3.3%\n"
    "\n"
    "[commission]\n"
    "vnq = 5.1\n"
    "\n"
    "[options]\n"
    "commission = 2\n"
    "no more deals = true\n"
    "max deals = 5\n"
    "\n"
    "[have]\n"
    "vti = 3\n";

  Allocation a;
  std::stringstream ss(s);
  bool b;
  b = a.Load(ss);
  REQUIRE(b);
#ifdef _DEBUG
  a.Dump();
#endif

  REQUIRE(a.GetCount() == 4);
  REQUIRE(a.GetTicker(0) == "VTI");
  REQUIRE(a.GetExistingShares(0) == 3);
  REQUIRE(a.GetTargetShares(0) == 4);
  REQUIRE(!a.IsTargetInPercents(0));
  REQUIRE(a.GetCommission(0) == 2);
  REQUIRE(a.CanBuy(0) == true);
  REQUIRE(a.CanSell(0) == false);

  REQUIRE(a.GetTicker(1) == "IEF");
  REQUIRE(a.GetExistingShares(1) == 3.7);
  REQUIRE(a.GetTargetShares(1) == 0);
  REQUIRE(!a.IsTargetInPercents(1));
  REQUIRE(a.GetCommission(1) == 2);
  REQUIRE(a.CanBuy(1) == true);
  REQUIRE(a.CanSell(1) == true);

  REQUIRE(a.GetTicker(2) == "VNQ");
  REQUIRE(a.GetExistingShares(2) == 6);
  REQUIRE(a.GetTargetShares(2) == 15);
  REQUIRE(a.IsTargetInPercents(2));
  REQUIRE(a.GetCommission(2) == 5.1);
  REQUIRE(a.CanBuy(2) == false);
  REQUIRE(a.CanSell(2) == false);

  REQUIRE(a.GetTicker(3) == "IAU");
  REQUIRE(a.GetExistingShares(3) == 0);
  REQUIRE(a.GetTargetShares(3) == 0);
  REQUIRE(!a.IsTargetInPercents(3));
  REQUIRE(a.GetCommission(3) == 2);
  REQUIRE(a.CanBuy(3) == false);
  REQUIRE(a.CanSell(3) == true);

  REQUIRE(a.GetExistingCash() == -9.8);
  REQUIRE(a.HasTargetCash());
  REQUIRE(a.GetTargetCash() == 3.3);
  REQUIRE(a.IsTargetCashInPercents());
  REQUIRE(a.UseAllCash() == true);
  REQUIRE(a.GetMaxDeals() == 5);
}

TEST_CASE("ModelTest", "[allocation]")
{
  Allocation a;
  REQUIRE(a.UseLeastSquaresApproximation() == true);

  std::stringstream ss("[options]\nmodel=lad");

  bool b = a.Load(ss);
  REQUIRE(b);

  REQUIRE(a.UseLeastSquaresApproximation() == false);

  ss.clear();
  ss.str("[options]\nmodel=lsapprox");
  b = a.Load(ss);
  REQUIRE(b);

  REQUIRE(a.UseLeastSquaresApproximation() == true);

  ss.clear();
  ss.str("[options]\nmodel=lad");
  b = a.Load(ss);
  REQUIRE(b);

  REQUIRE(a.UseLeastSquaresApproximation() == false);
}
