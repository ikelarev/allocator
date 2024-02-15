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

#include <catch.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>

#define HAVE(...) "[have]",       __VA_ARGS__
#define WANT(...) "[want]",       __VA_ARGS__
#define CASH(...) "[cash]",       __VA_ARGS__
#define TRAD(...) "[trade]",      __VA_ARGS__
#define COMM(...) "[commission]", __VA_ARGS__
#define OPTS(...) "[options]",    __VA_ARGS__

#define LS(x)  +(isLsTest<TestType>() ? (x) : (0))
#define LAD(x) +(isLsTest<TestType>() ? (0) : (x))

using LadTestType = char;
using LsTestType  = long;
static_assert(sizeof(LsTestType) != sizeof(LadTestType), "");

template<class TestType>
bool isLsTest()
{
  if (sizeof(TestType) == sizeof(LsTestType))
  {
    return true;
  }

  REQUIRE(sizeof(TestType) == sizeof(LadTestType));
  return false;
}

template<class TestType>
Allocation CreateAllocation(const std::vector<std::string> &lines)
{
  std::stringstream ss;
  ss << "[options]\ncommission = 1\n";
  if (isLsTest<TestType>())
  {
    ss << "model = lsapprox\n";
  }
  else
  {
    ss << "model = lad\n";
  }

  for (size_t i = 0; i < lines.size(); i++)
  {
    ss << lines[i] << std::endl;
  }

  Allocation a;
  bool ok = a.Load(ss);
  REQUIRE(ok);

  return std::move(a);
}

template<class TestType, class ...Args>
Allocation CreateAllocation(const Args &...args)
{
  return std::move(CreateAllocation<TestType>({ args... }));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Optimizer::RatesProvider GetRatesProvider()
{
  std::map<std::string, std::pair<double, double>> prices;

  prices["ANY"] = std::make_pair(1.23, 4.56);
  prices["ONE"] = std::make_pair(1, 2);
  prices["TWO"] = std::make_pair(2, 3);
  prices["TEN"] = std::make_pair(10, 12);

  prices["VTI"] = std::make_pair(116.71, 116.71);
  prices["IEF"] = std::make_pair(103.81, 103.81);
  prices["SPY"] = std::make_pair(226.27, 226.27);
  prices["BND"] = std::make_pair(80.20,  80.20);
  prices["VNQ"] = std::make_pair(80.99, 80.99);
  prices["VWO"] = std::make_pair(35.20, 35.20);
  prices["TLT"] = std::make_pair(117.48, 118.08);
  prices["IAU"] = std::make_pair(10.97, 10.97);
  prices["GOOG"] = std::make_pair(790, 798);
  prices["TSLA"] = std::make_pair(219.90, 220.50);
  prices["O"] = std::make_pair(56.43, 56.43);

  prices["BNO"] = std::make_pair(15.46, 15.50);
  prices["DBO"] = std::make_pair(9.51, 9.55);
  prices["XOP"] = std::make_pair(40.70, 40.74);
  prices["AAPL"] = std::make_pair(119.14, 119.18);

  prices["VTI*"] = std::make_pair(117.22, 117.22);
  prices["VNQ*"] = std::make_pair(82.61, 82.61);
  prices["VWO*"] = std::make_pair(37.24, 37.24);
  prices["TLT*"] = std::make_pair(121.11, 121.11);
  prices["IEF*"] = std::make_pair(105.39, 105.39);
  prices["IAU*"] = std::make_pair(11.55, 11.55);

  return std::move(
    [prices](const std::string &ticker, double &bid, double &ask)
    {
      auto it = prices.find(ticker);
      CHECK(it != prices.end());

      bid = it->second.first;
      ask = it->second.second;
    }
  );
};

Optimizer Optimize(const Allocation &a, bool expected = true, bool checkq = true)
{
#ifdef _DEBUG
  a.Dump();
#endif

  Optimizer o
  (
    [](size_t iter, int nodes, double progress) -> bool
    {
#ifdef _DEBUG
      std::cout << iter << ", " << nodes << ", " << int(progress * 100) << "%  ";
      if (progress == 1)
        std::cout << std::endl;
      else
        std::cout << "\r";
#endif
      return true;
    }
  );

  bool ok = o.Optimize(a, GetRatesProvider());
  REQUIRE(ok == expected);

  if (checkq)
  {
    REQUIRE(o.GetResultQuality().abserr <= o.GetSourceQuality().abserr);
    REQUIRE(o.GetResultQuality().stddev <= o.GetSourceQuality().stddev);
  }

  return std::move(o);
}

template<class TestType, class ...Args>
Optimizer Optimize(const Args &...args)
{
  return std::move(Optimize(CreateAllocation<TestType>(args...)));
}

template<class TestType, class ...Args>
Optimizer Optimize(bool expected, const Args &...args)
{
  return std::move(Optimize(CreateAllocation<TestType>(args...), expected));
}

template<class TestType, class ...Args>
Optimizer Optimize2(const Args &...args)
{
  return std::move(Optimize(CreateAllocation<TestType>(args...), true, false));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEMPLATE_TEST_CASE("SimpleTest", "[optimizer]", LadTestType, LsTestType)
{
  Optimizer o1 = Optimize<TestType>(HAVE("ONE = 1"), WANT("ONE = 5"));
  auto res = o1.GetResult("ONE");
  REQUIRE(res.ticker == "ONE");
  REQUIRE(res.bid == 1);
  REQUIRE(res.ask == 2);
  REQUIRE(res.have == 1);
  REQUIRE(res.result == 1);
  REQUIRE(res.change == 0);
  REQUIRE(res.commission == 0);
  auto qr = o1.GetResultQuality();
  REQUIRE(qr.abserr == 4);
  REQUIRE(qr.stddev == 4);

  Optimizer o2 = Optimize<TestType>(HAVE("TWO = 1"), WANT("TWO = 5"), CASH("have = 11"));
  res = o2.GetResult("TWO");
  REQUIRE(res.have == 1);
  REQUIRE(res.result == 4);
  REQUIRE(res.change == 3);
  REQUIRE(res.commission == 1);

  res = o2.GetCashResult();
  REQUIRE(res.have == 11);
  REQUIRE(res.result == 1);
  REQUIRE(res.change == -10);

  qr = o2.GetResultQuality();
  REQUIRE(qr.abserr == 2);
  REQUIRE(qr.stddev == 2);

  Optimizer o3 = Optimize<TestType>(HAVE("TEN = 10"), WANT("TEN = 8"), CASH("withdraw = 20"), COMM("TEN = 3"));
  res = o3.GetResult("TEN");
  REQUIRE(res.result == 7);
  REQUIRE(res.change == -3);
  REQUIRE(res.commission == 3);

  res = o3.GetCashResult();
  REQUIRE(res.have == -20);
  REQUIRE(res.result == 7);

  qr = o3.GetResultQuality();
  REQUIRE(qr.abserr == 10);
  REQUIRE(qr.stddev == 10);

  Optimizer o3a = Optimize<TestType>(
    HAVE("TEN = 10"), WANT("TEN = 8"), CASH("withdraw = 20"), COMM("TEN = 3"), TRAD("TEN = sell"));
  res = o3a.GetResult("TEN");
  REQUIRE(res.result == 7);
  REQUIRE(res.change == -3);
  REQUIRE(res.commission == 3);

  res = o3a.GetCashResult();
  REQUIRE(res.have == -20);
  REQUIRE(res.result == 7);

  qr = o3a.GetResultQuality();
  REQUIRE(qr.abserr == 10);
  REQUIRE(qr.stddev == 10);

  Optimizer o4 = Optimize<TestType>
  (
    false,
    HAVE("ANY = 10"),
    WANT("ANY = 20"),
    TRAD("ANY = keep"),
    CASH("have = 10", "withdraw = 100")
  );

  res = o4.GetResult("ANY");
  REQUIRE(res.change == 0);
  REQUIRE(res.commission == 0);

  res = o4.GetCashResult();
  REQUIRE(res.have == -90);
  REQUIRE(res.change == 0);

  qr = o4.GetResultQuality();
  REQUIRE(qr.abserr == 12.3);
  REQUIRE(qr.stddev == 12.3);

  Optimizer o5 = Optimize<TestType>
  (
    HAVE("ANY = 10", "TEN = 10"),
    WANT("ANY = 20"), // TODO: if there's no TEN = 0 -- do not include TEN into diff (?)
    TRAD("ANY = keep"),
    CASH("have = 10", "withdraw = 100"),
    OPTS("commission = 2")
  );

  res = o5.GetResult("ANY");
  REQUIRE(res.change == 0);
  REQUIRE_FALSE(res.inPercents);

  res = o5.GetResult("TEN");
  REQUIRE(res.change == -10);
  REQUIRE_FALSE(res.inPercents);

  res = o5.GetCashResult();
  REQUIRE(res.have == -90);
  REQUIRE(res.result == 8);
  REQUIRE_FALSE(res.inPercents);

  qr = o5.GetResultQuality();
  REQUIRE(qr.abserr == 6.15);
  REQUIRE(qr.stddev == sqrt(75.645));
  REQUIRE_FALSE(res.inPercents);
}

TEMPLATE_TEST_CASE("FracTest", "[optimizer]", LadTestType, LsTestType)
{
  Optimizer o1 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6"), CASH("withdraw = 1"));
  auto res = o1.GetResult("ONE");
  REQUIRE(res.have == 3.4);
  REQUIRE(res.result == 1.4);
  REQUIRE(res.change == -2);
  REQUIRE(res.commission == 1);

  res = o1.GetCashResult();
  REQUIRE(res.result == 0);

  auto qr = o1.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.2) < 1e-6);
  REQUIRE(fabs(qr.stddev - 0.2) < 1e-6);

  Optimizer o2 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6"), CASH("want = 0"));
  res = o2.GetResult("ONE");
  REQUIRE(res.change == -1);
  REQUIRE(res.commission == 1);

  res = o2.GetCashResult();
  REQUIRE(res.change == 0);

  qr = o2.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.4) < 1e-6);
  REQUIRE(fabs(qr.stddev - sqrt(0.32)) < 1e-6);

  Optimizer o2a = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6"));
  res = o2a.GetResult("ONE");
  REQUIRE(res.change == -2);
  REQUIRE(res.commission == 1);

  res = o2a.GetCashResult();
  REQUIRE(res.change == 1);

  qr = o2a.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.2) < 1e-6);
  REQUIRE(fabs(qr.stddev - 0.2) < 1e-6);

  Optimizer o3 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.2"));
  res = o3.GetResult("ONE");
  REQUIRE(res.change == -2);
  REQUIRE(res.commission == 1);

  res = o3.GetCashResult();
  REQUIRE(res.change == 1);

  qr = o3.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.2) < 1e-6);
  REQUIRE(fabs(qr.stddev - 0.2) < 1e-6);

  Optimizer o4 = Optimize<TestType>(HAVE("ONE = 1.9"), WANT("ONE = 0.9"), CASH("want = 0"));
  res = o4.GetResult("ONE");
  REQUIRE(res.change == 0);
  REQUIRE(res.commission == 0);

  res = o4.GetCashResult();
  REQUIRE(res.result == 0);

  qr = o4.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.5) < 1e-6);
  REQUIRE(fabs(qr.stddev - sqrt(0.5)) < 1e-6);

  Optimizer o4a = Optimize<TestType>(HAVE("ONE = 1.9"), WANT("ONE = 0.9"));
  res = o4a.GetResult("ONE");
  REQUIRE(res.result == 0);
  REQUIRE(res.commission == 1);

  res = o4a.GetCashResult();
  REQUIRE(fabs(res.result - 0.9) < 1e-6);

  qr = o4a.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.9) < 1e-6);
  REQUIRE(fabs(qr.stddev - 0.9) < 1e-6);

  Optimizer o5 = Optimize<TestType>(HAVE("ONE = 1.4"), WANT("ONE = 0.4"));
  res = o5.GetResult("ONE");
  REQUIRE(res.result == 0);
  REQUIRE(res.commission == 1);

  res = o5.GetCashResult();
  REQUIRE(fabs(res.result - 0.4) < 1e-6);

  qr = o5.GetResultQuality();
  REQUIRE(fabs(qr.abserr - 0.4) < 1e-6);
  REQUIRE(fabs(qr.stddev - 0.4) < 1e-6);

  Optimizer o6 = Optimize<TestType>(HAVE("TWO = 6.9"), WANT("TWO = 100%"), CASH("withdraw = 11"), COMM("TWO = 0"));
  res = o6.GetResult("TWO");
  REQUIRE(res.result == 0);
  REQUIRE(res.commission == 0);

  res = o6.GetCashResult();
  REQUIRE(res.have == -11);
  REQUIRE(fabs(res.result - 2.8) < 1e-6);

  qr = o6.GetResultQuality();
  REQUIRE(fabs(qr.abserr) < 1e-6);
  REQUIRE(fabs(qr.stddev) < 1e-6);

  Optimizer o7 = Optimize<TestType>(
    HAVE("TWO = 6.9"),
    WANT("TWO = 26%", "ONE = 74%"),
    CASH("want = 0"),
    OPTS("commission = 0"));
  res = o7.GetResult("TWO");
  REQUIRE(fabs(res.result - 1.9) < 1e-6);
  res = o7.GetResult("ONE");
  REQUIRE(res.result == 5);

  res = o7.GetCashResult();
  REQUIRE(fabs(res.result) < 1e-6);

  Optimizer o8 = Optimize<TestType>(
    HAVE("TWO = 6.9"),
    WANT("TWO = 19%", "ONE = 81%"),
    CASH("want = 0"),
    OPTS("commission = 0"));
  res = o8.GetResult("TWO");
  REQUIRE(res.result == 0);
  res = o8.GetResult("ONE");
  REQUIRE(res.result == 6);

  res = o8.GetCashResult();
  REQUIRE(fabs(res.result - 1.8) < 1e-6);

  Optimizer o9 = Optimize<TestType>(
    HAVE("TWO = 6.9", "ONE = 1"),
    WANT("TWO = 17%", "ONE = 83%"),
    CASH("want = 0"),
    OPTS("commission = 0"));
  res = o9.GetResult("TWO");
  REQUIRE(res.result == 0);
  res = o9.GetResult("ONE");
  REQUIRE(res.result == 7);

  res = o9.GetCashResult();
  REQUIRE(fabs(res.result - 1.8) < 1e-6);

  Optimizer o10 = Optimize<TestType>(
    HAVE("TWO = 6.9", "ONE = 1"),
    WANT("TWO = 18%", "ONE = 82%"),
    CASH("want = 0"),
    OPTS("commission = 0"));
  res = o10.GetResult("TWO");
  REQUIRE(fabs(res.result - LAD(1.9) LS(0)) < 1e-6);
  REQUIRE(res.inPercents);
  REQUIRE(fabs(res.percents - LAD(38.7755) - LS(0)) < 1e-3);
  REQUIRE(fabs(res.sourcePercents - 93.2432) < 1e-3);
  res = o10.GetResult("ONE");
  REQUIRE(res.change == LAD(5) LS(6));
  REQUIRE(res.inPercents);
  REQUIRE(fabs(res.percents - LAD(61.2244) - LS(100)) < 1e-3);
  REQUIRE(fabs(res.sourcePercents - 6.7567) < 1e-3);

  res = o10.GetCashResult();
  REQUIRE(fabs(res.result - LS(1.8) LAD(0)) < 1e-6);

  Optimizer o11 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6"), CASH("have = 100", "want = 0"));
  res = o11.GetResult("ONE");
  REQUIRE(res.change == LAD(49)   LS(39));
  REQUIRE(res.result == LAD(52.4) LS(42.4));
  res = o11.GetCashResult();
  REQUIRE(res.result == LAD(1)    LS(21));

  Optimizer o11a = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6"), CASH("have = 100"));
  res = o11a.GetResult("ONE");
  REQUIRE(res.change == -2);
  REQUIRE(res.result == 1.4);
  res = o11a.GetCashResult();
  REQUIRE(res.result == 101);

  Optimizer o12 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6"), CASH("have = 100", "want = 0%"));
  res = o12.GetResult("ONE");
  REQUIRE(res.change == LAD(49)   LS(39));
  REQUIRE(res.result == LAD(52.4) LS(42.4));
  res = o12.GetCashResult();
  REQUIRE(res.result == LAD(1)    LS(21));

  Optimizer o13 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6", "TWO = 30"), CASH("have = 100", "want = 0"));
  res = o13.GetResult("ONE");
  REQUIRE(fabs(res.change - LAD(4) - LS(2)) < 1e-6);
  res = o13.GetResult("TWO");
  REQUIRE(res.result == LAD(30) LS(31));
  res = o13.GetCashResult();
  REQUIRE(res.result == LAD(0)  LS(1));

  Optimizer o13a = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 1.6", "TWO = 30"), CASH("have = 100"));
  res = o13a.GetResult("ONE");
  REQUIRE(res.change == -2);
  res = o13a.GetResult("TWO");
  REQUIRE(res.result == 30);
  res = o13a.GetCashResult();
  REQUIRE(res.result == 10);

  Optimizer o14 = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 20", "TWO = 20"), CASH("have = 100", "want = 0"));
  res = o14.GetResult("ONE");
  REQUIRE(res.change == LAD(19) LS(17));
  res = o14.GetResult("TWO");
  REQUIRE(res.result == LAD(20) LS(21));
  res = o14.GetCashResult();
  REQUIRE(res.result == LAD(0)  LS(1));

  Optimizer o14a = Optimize<TestType>(HAVE("ONE = 3.4"), WANT("ONE = 20", "TWO = 20"), CASH("have = 100"));
  res = o14a.GetResult("ONE");
  REQUIRE(res.change == 17);
  REQUIRE_FALSE(res.inPercents);
  res = o14a.GetResult("TWO");
  REQUIRE(res.result == 20);
  REQUIRE_FALSE(res.inPercents);
  res = o14a.GetCashResult();
  REQUIRE(res.result == 4);
  REQUIRE_FALSE(res.inPercents);
}

TEMPLATE_TEST_CASE("StocksBondsTest", "[optimizer]", LadTestType, LsTestType)
{
  auto o = Optimize<TestType>(WANT("VTI = 50%", "IEF = 50%"), CASH("have = 1000", "want = 0"), OPTS("commission = 2"));
  auto res = o.GetResult("VTI");
  REQUIRE(res.result == 4);
  res = o.GetResult("IEF");
  REQUIRE(res.result == 5);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 10.11) < 1e-6);

  o = Optimize<TestType>(WANT("VTI = 70%", "IEF = 30%"), CASH("have = 1000", "want = 0"), OPTS("commission = 2"));
  res = o.GetResult("VTI");
  REQUIRE(res.result == 5);
  res = o.GetResult("IEF");
  REQUIRE(res.result == 3);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 101.02) < 1e-6);

  o = Optimize<TestType>(WANT("SPY = 80%", "BND = 20%"), CASH("have = 1000", "want = 0"), OPTS("commission = 15"));
  res = o.GetResult("SPY");
  REQUIRE(res.result == 3);
  res = o.GetResult("BND");
  REQUIRE(res.result == LAD(2) LS(3));
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - LAD(130.79) - LS(50.59)) < 1e-6);

  o = Optimize<TestType>(
    WANT("SPY = 80%", "BND = 20%"),
    CASH("have = 1000", "want = 0"),
    OPTS("commission = 15", "no more deals = true"));
  res = o.GetResult("SPY");
  REQUIRE(res.result == 3);
  res = o.GetResult("BND");
  REQUIRE(res.result == 3);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 50.59) < 1e-6);

  o = Optimize<TestType>(
    WANT("SPY = 80%", "BND = 20%"),
    CASH("have = 949.41", "want = 0"),
    OPTS("commission = 15", "no more deals = true"));
  res = o.GetResult("SPY");
  REQUIRE(res.result == 3);
  res = o.GetResult("BND");
  REQUIRE(res.result == 3);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result) < 1e-6);

  o = Optimize<TestType>(
    WANT("SPY = 80%", "BND = 20%"),
    CASH("have = 949.40", "want = 0"),
    OPTS("commission = 15", "no more deals = true"));
  res = o.GetResult("SPY");
  REQUIRE(res.result == 3);
  REQUIRE(res.inPercents);
  REQUIRE(fabs(res.percents - 80.8867) < 1e-3);
  REQUIRE(res.sourcePercents == 0);
  res = o.GetResult("BND");
  REQUIRE(res.result == 2);
  REQUIRE(res.inPercents);
  REQUIRE(fabs(res.percents - 19.1132) < 1e-3);
  REQUIRE(res.sourcePercents == 0);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 80.19) < 1e-6);
  REQUIRE_FALSE(res.inPercents);
}

TEMPLATE_TEST_CASE("UseAllCashTest", "[optimizer]", LadTestType, LsTestType)
{
  auto o = Optimize<TestType>(HAVE("ONE = 4"), WANT("ONE = 0"), CASH("want = 4"), OPTS("commission = 0"));
  auto res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(res.result == 4);

  o = Optimize<TestType>(HAVE("ONE = 4"), WANT("ONE = 0"), OPTS("commission = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(res.result == 4);

  o = Optimize<TestType>(HAVE("ONE = 4"), WANT("ONE = 0"), CASH("want = 0"), OPTS("commission = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 2);
  res = o.GetCashResult();
  REQUIRE(res.result == 2);

  o = Optimize<TestType>(HAVE("ONE = 4"), WANT("ONE = 0"), OPTS("commission = 0", "no more deals = true"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 4);
  res = o.GetCashResult();
  REQUIRE(res.result == 0);

  o = Optimize<TestType>(HAVE("ONE = 5"), WANT("ONE = 0"), CASH("want = 4"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(res.result == 4);

  o = Optimize<TestType>(HAVE("ONE = 5"), WANT("ONE = 0"), CASH("want = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 2);
  res = o.GetCashResult();
  REQUIRE(res.result == 2);

  o = Optimize<TestType>(HAVE("ONE = 5"), WANT("ONE = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(res.result == 4);

  o = Optimize<TestType>(HAVE("ONE = 5"), WANT("ONE = 0"), OPTS("no more deals = true"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 4);
  res = o.GetCashResult();
  REQUIRE(res.result == 0);

  o = Optimize<TestType>(HAVE("ONE = 1.3"), WANT("ONE = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 0.3) < 1e-6);

  o = Optimize<TestType>(HAVE("ONE = 1.3"), WANT("ONE = 0"), OPTS("no more deals = true"));
  res = o.GetResult("ONE");
  REQUIRE(res.change == 0);
  res = o.GetCashResult();
  REQUIRE(res.change == 0);

  o = Optimize<TestType>(HAVE("ONE = 2.3"), WANT("ONE = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 1.3) < 1e-6);

  o = Optimize<TestType>(HAVE("ONE = 2.3"), WANT("ONE = 0"), OPTS("no more deals = true"));
  res = o.GetResult("ONE");
  REQUIRE(res.change == 0);
  res = o.GetCashResult();
  REQUIRE(res.change == 0);

  o = Optimize<TestType>(HAVE("ONE = 3.3"), WANT("ONE = 0"), CASH("want = 0"));
  res = o.GetResult("ONE");
  REQUIRE(fabs(res.result - 1.3) < 1e-6);
  res = o.GetCashResult();
  REQUIRE(res.result == 1);

  o = Optimize<TestType>(HAVE("ONE = 3.3"), WANT("ONE = 0"));
  res = o.GetResult("ONE");
  REQUIRE(res.result == 0);
  res = o.GetCashResult();
  REQUIRE(fabs(res.result - 2.3) < 1e-6);

  o = Optimize<TestType>(HAVE("ONE = 3.3"), WANT("ONE = 0"), OPTS("no more deals = true"));
  res = o.GetResult("ONE");
  REQUIRE(res.change == -1);
  REQUIRE_FALSE(res.inPercents);
  res = o.GetCashResult();
  REQUIRE(res.change == 0);
  REQUIRE_FALSE(res.inPercents);
}

TEMPLATE_TEST_CASE("MyFirstPortfolioTest", "[optimizer]", LadTestType, LsTestType)
{
  //
  // LAD/LS  want=0  want=0%  nomoredeals  nmd&=0     nmd&=0%  default
  // VTI         5        5       5          5          5         5
  // VNQ         7        7       7          7          7         7
  // VWO        17       17      18        17/18      17/18      17
  // TLT         5        5       5          5          5         5
  // IEF         3        3       3          3          3         3
  // IAU     27/29       29      27        31/27      31/27      27
  // CASH    45/23    23.23    9.97      1.29/9.97  1.29/9.97   45.17

  // want = 0
  auto o = Optimize<TestType>(
    WANT(
      "VTI = 20%",
      "VNQ = 20%",
      "VWO = 20%",
      "TLT = 20%",
      "IEF = 10%",
      "IAU = 10%",
      "GOOG = 1",
      "TSLA = 1",
      "O = 1"),
    CASH("have = 4085", "want = 0"),
    OPTS("commission = 2"));

  REQUIRE(o.GetResult("VTI").result == 5);
  REQUIRE(o.GetResult("VNQ").result == 7);
  REQUIRE(o.GetResult("VWO").result == 17);
  REQUIRE(o.GetResult("TLT").result == 5);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(o.GetResult("IAU").result == LAD(27) LS(29));
  REQUIRE(o.GetResult("GOOG").result == 1);
  REQUIRE(o.GetResult("TSLA").result == 1);
  REQUIRE(o.GetResult("O").result == 1);
  REQUIRE(fabs(o.GetCashResult().result - LAD(45.17) - LS(23.23)) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - LAD(17.1265) - LS(14.3730)) < 1e-3);

  // want = 0%
  o = Optimize<TestType>(
    WANT(
      "VTI = 20%",
      "VNQ = 20%",
      "VWO = 20%",
      "TLT = 20%",
      "IEF = 10%",
      "IAU = 10%",
      "GOOG = 1",
      "TSLA = 1",
      "O = 1"),
    CASH("have = 4085", "want = 0%"),
    OPTS("commission = 2"));

  REQUIRE(o.GetResult("VTI").result == 5);
  REQUIRE(o.GetResult("VNQ").result == 7);
  REQUIRE(o.GetResult("VWO").result == 17);
  REQUIRE(o.GetResult("TLT").result == 5);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(o.GetResult("IAU").result == LAD(30) LS(29));
  REQUIRE(o.GetResult("GOOG").result == 1);
  REQUIRE(o.GetResult("TSLA").result == 1);
  REQUIRE(o.GetResult("O").result == 1);
  REQUIRE(fabs(o.GetCashResult().result - LAD(12.26) - LS(23.23)) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - LAD(15.7633) - LS(15.2711)) < 1e-3);

  REQUIRE(o.GetCashResult().inPercents);
  REQUIRE(fabs(o.GetCashResult().percents - LAD(0.4101) - LS(0.7771)) < 1e-3);
  REQUIRE(o.GetCashResult().sourcePercents == 100);

  // no more deals
  o = Optimize<TestType>(
    WANT(
      "VTI = 20%",
      "VNQ = 20%",
      "VWO = 20%",
      "TLT = 20%",
      "IEF = 10%",
      "IAU = 10%",
      "GOOG = 1",
      "TSLA = 1",
      "O = 1"),
    CASH("have = 4085"),
    OPTS("commission = 2", "no more deals = true"));

  REQUIRE(o.GetResult("VTI").result == 5);
  REQUIRE(o.GetResult("VNQ").result == 7);
  REQUIRE(o.GetResult("VWO").result == 18);
  REQUIRE(o.GetResult("TLT").result == 5);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(o.GetResult("IAU").result == 27);
  REQUIRE(o.GetResult("GOOG").result == 1);
  REQUIRE(o.GetResult("TSLA").result == 1);
  REQUIRE(o.GetResult("O").result == 1);
  REQUIRE(fabs(o.GetCashResult().result - 9.97) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - 17.2213) < 1e-3);

  // want = 0 & no more deals
  o = Optimize<TestType>(
    WANT(
      "VTI = 20%",
      "VNQ = 20%",
      "VWO = 20%",
      "TLT = 20%",
      "IEF = 10%",
      "IAU = 10%",
      "GOOG = 1",
      "TSLA = 1",
      "O = 1"),
    CASH("have = 4085", "want = 0"),
    OPTS("commission = 2", "no more deals = true"));

  REQUIRE(o.GetResult("VTI").result == 5);
  REQUIRE(o.GetResult("VNQ").result == 7);
  REQUIRE(o.GetResult("VWO").result == LAD(17) LS(18));
  REQUIRE(o.GetResult("TLT").result == 5);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(o.GetResult("IAU").result == LAD(31) LS(27));
  REQUIRE(o.GetResult("GOOG").result == 1);
  REQUIRE(o.GetResult("TSLA").result == 1);
  REQUIRE(o.GetResult("O").result == 1);
  REQUIRE(fabs(o.GetCashResult().result - LAD(1.29) - LS(9.97)) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - LAD(17.6203) - LS(16.639)) < 1e-3);

  // want = 0% & no more deals
  o = Optimize<TestType>(
    WANT(
      "VTI = 20%",
      "VNQ = 20%",
      "VWO = 20%",
      "TLT = 20%",
      "IEF = 10%",
      "IAU = 10%",
      "GOOG = 1",
      "TSLA = 1",
      "O = 1"),
    CASH("have = 4085", "want = 0%"),
    OPTS("commission = 2", "no more deals = true"));

  REQUIRE(o.GetResult("VTI").result == 5);
  REQUIRE(o.GetResult("VNQ").result == 7);
  REQUIRE(o.GetResult("VWO").result == LAD(17) LS(18));
  REQUIRE(o.GetResult("TLT").result == 5);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(o.GetResult("IAU").result == LAD(31) LS(27));
  REQUIRE(o.GetResult("GOOG").result == 1);
  REQUIRE(o.GetResult("TSLA").result == 1);
  REQUIRE(o.GetResult("O").result == 1);
  REQUIRE(fabs(o.GetCashResult().result - LAD(1.29) - LS(9.97)) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - LAD(17.6606) - LS(16.7629)) < 1e-3);

  REQUIRE(o.GetResult("VTI").inPercents);
  REQUIRE(o.GetResult("VNQ").inPercents);
  REQUIRE(o.GetResult("VWO").inPercents);
  REQUIRE(o.GetResult("TLT").inPercents);
  REQUIRE(o.GetResult("IEF").inPercents);
  REQUIRE(o.GetResult("IAU").inPercents);
  REQUIRE_FALSE(o.GetResult("GOOG").inPercents);
  REQUIRE_FALSE(o.GetResult("TSLA").inPercents);
  REQUIRE_FALSE(o.GetResult("O").inPercents);
  REQUIRE(o.GetCashResult().inPercents);

  REQUIRE(fabs(o.GetResult("VTI").percents - 19.5227) < 1e-3);
  REQUIRE(fabs(o.GetResult("VNQ").percents - 18.9667) < 1e-3);
  REQUIRE(fabs(o.GetResult("VWO").percents - LAD(20.0196) - LS(21.1972)) < 1e-3);
  REQUIRE(fabs(o.GetResult("TLT").percents - 19.6515) < 1e-3);
  REQUIRE(fabs(o.GetResult("IEF").percents - 10.4189) < 1e-3);
  REQUIRE(fabs(o.GetResult("IAU").percents - LAD(11.3771) - LS(9.90910)) < 1e-3);
  REQUIRE(fabs(o.GetCashResult().percents - LAD(0.0431) - LS(0.3335)) < 1e-3);

  REQUIRE(o.GetResult("VTI").sourcePercents == 0);
  REQUIRE(o.GetResult("VNQ").sourcePercents == 0);
  REQUIRE(o.GetResult("VWO").sourcePercents == 0);
  REQUIRE(o.GetResult("TLT").sourcePercents == 0);
  REQUIRE(o.GetResult("IEF").sourcePercents == 0);
  REQUIRE(o.GetResult("IAU").sourcePercents == 0);
  REQUIRE(o.GetCashResult().sourcePercents == 100);

  // default
  o = Optimize<TestType>(
    WANT(
      "VTI = 20%",
      "VNQ = 20%",
      "VWO = 20%",
      "TLT = 20%",
      "IEF = 10%",
      "IAU = 10%",
      "GOOG = 1",
      "TSLA = 1",
      "O = 1"),
    CASH("have = 4085"),
    OPTS("commission = 2"));

  REQUIRE(o.GetResult("VTI").result == 5);
  REQUIRE(o.GetResult("VNQ").result == 7);
  REQUIRE(o.GetResult("VWO").result == 17);
  REQUIRE(o.GetResult("TLT").result == 5);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(o.GetResult("IAU").result == 27);
  REQUIRE(o.GetResult("GOOG").result == 1);
  REQUIRE(o.GetResult("TSLA").result == 1);
  REQUIRE(o.GetResult("O").result == 1);
  REQUIRE(fabs(o.GetCashResult().result - 45.17) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - 9.96) < 1e-3);

  REQUIRE(o.GetResult("VTI").inPercents);
  REQUIRE(o.GetResult("VNQ").inPercents);
  REQUIRE(o.GetResult("VWO").inPercents);
  REQUIRE(o.GetResult("TLT").inPercents);
  REQUIRE(o.GetResult("IEF").inPercents);
  REQUIRE(o.GetResult("IAU").inPercents);
  REQUIRE_FALSE(o.GetResult("GOOG").inPercents);
  REQUIRE_FALSE(o.GetResult("TSLA").inPercents);
  REQUIRE_FALSE(o.GetResult("O").inPercents);
  REQUIRE_FALSE(o.GetCashResult().inPercents);

  REQUIRE(fabs(o.GetResult("VTI").percents - 19.8223) < 1e-3);
  REQUIRE(fabs(o.GetResult("VNQ").percents - 19.2577) < 1e-3);
  REQUIRE(fabs(o.GetResult("VWO").percents - 20.3267) < 1e-3);
  REQUIRE(fabs(o.GetResult("TLT").percents - 19.9531) < 1e-3);
  REQUIRE(fabs(o.GetResult("IEF").percents - 10.5788) < 1e-3);
  REQUIRE(fabs(o.GetResult("IAU").percents - 10.0611) < 1e-3);
}

TEMPLATE_TEST_CASE("RebalanceMyPortfolioTest", "[optimizer][.heavy]", LadTestType, LsTestType)
{
  #define MY_PORTFOLIO      \
    HAVE(                  \
      "VTI = 6",         \
      "VNQ = 7",         \
      "VWO = 17",        \
      "TLT = 4",         \
      "IEF = 3",         \
      "IAU = 25",        \
      "GOOG = 1",        \
      "TSLA = 1",        \
      "O = 1"),          \
    WANT(                  \
      "VTI = 20%",       \
      "VNQ = 20%",       \
      "VWO = 20%",       \
      "TLT = 20%",       \
      "IEF = 10%",       \
      "IAU = 10%"),      \
    CASH("have = 65.01"),  \
    OPTS("commission = 2")

  SECTION("want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, CASH("want = 0"));
    REQUIRE(o.GetResult("VTI").change == 1);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == 5);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 1);
    REQUIRE(o.GetResult("IAU").change == 10);
    REQUIRE(o.GetResult("GOOG").result == 0);
    REQUIRE(o.GetResult("TSLA").result == 0);
    REQUIRE(o.GetResult("O").result == 0);
    REQUIRE(fabs(o.GetCashResult().result - 9.91) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 14.1814) < 1e-3);
  }

  SECTION("want = 0%")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, CASH("want = 0%"));
    REQUIRE(o.GetResult("VTI").change == 1);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == 5);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 1);
    REQUIRE(o.GetResult("IAU").change == 10);
    REQUIRE(o.GetResult("GOOG").result == 0);
    REQUIRE(o.GetResult("TSLA").result == 0);
    REQUIRE(o.GetResult("O").result == 0);
    REQUIRE(fabs(o.GetCashResult().result - 9.91) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 14.2062) < 1e-3);
  }

  SECTION("GOOG = 0, TSLA = 0, O = 0, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, HAVE("GOOG = 0", "TSLA = 0", "O = 0"), CASH("want = 0"));
    REQUIRE(o.GetResult("VTI").change == -1);
    REQUIRE(o.GetResult("VNQ").change == 0);
    REQUIRE(o.GetResult("VWO").change == 0);
    REQUIRE(o.GetResult("TLT").change == 1);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == LAD(2) LS(3));
    REQUIRE(o.GetResult("GOOG").change == 0);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - LAD(35.7) - LS(24.73)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(14.7218) - LS(13.0505)) < 1e-3);
  }

  SECTION("GOOG = keep, TSLA = keep, O = keep, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, TRAD("GOOG = keep", "TSLA = keep", "O = keep"), CASH("want = 0"));
    REQUIRE(o.GetResult("VTI").change == -1);
    REQUIRE(o.GetResult("VNQ").change == 0);
    REQUIRE(o.GetResult("VWO").change == 0);
    REQUIRE(o.GetResult("TLT").change == 1);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == LAD(2) LS(3));
    REQUIRE(o.GetResult("GOOG").change == 0);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - LAD(35.7) - LS(24.73)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(260.3473) - LS(260.2582)) < 1e-3);
  }

  SECTION("GOOG = keep, TSLA = keep, O = keep, want = 0%")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, TRAD("GOOG = keep", "TSLA = keep", "O = keep"), CASH("want = 0%"));
    REQUIRE(o.GetResult("VTI").change == -1);
    REQUIRE(o.GetResult("VNQ").change == 0);
    REQUIRE(o.GetResult("VWO").change == 0);
    REQUIRE(o.GetResult("TLT").change == 1);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == LAD(3) LS(4));
    REQUIRE(o.GetResult("GOOG").change == 0);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - LAD(24.73) - LS(13.76)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(260.3056) - LS(260.2864)) < 1e-3);
  }

  SECTION("GOOG = keep, TSLA = keep, O = keep, no more deals = true")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, TRAD("GOOG = keep", "TSLA = keep", "O = keep"), OPTS("no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == -1);
    REQUIRE(o.GetResult("VNQ").change == 0);
    REQUIRE(o.GetResult("VWO").change == 0);
    REQUIRE(o.GetResult("TLT").change == 1);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == 5);
    REQUIRE(o.GetResult("GOOG").change == 0);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - 2.79) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 274.4362) < 1e-3);
  }

  SECTION("GOOG = keep, TSLA = keep, O = keep, want = 0, no more deals = true")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO,
      TRAD("GOOG = keep", "TSLA = keep", "O = keep"),
      CASH("want = 0"),
      OPTS("no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == -1);
    REQUIRE(o.GetResult("VNQ").change == 0);
    REQUIRE(o.GetResult("VWO").change == 0);
    REQUIRE(o.GetResult("TLT").change == 1);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == 5);
    REQUIRE(o.GetResult("GOOG").change == 0);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - 2.79) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 260.3545) < 1e-3);
  }

  SECTION("GOOG = keep, TSLA = keep, O = keep, want = 0%, no more deals = true")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO,
      TRAD("GOOG = keep", "TSLA = keep", "O = keep"),
      CASH("want = 0%"),
      OPTS("no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == -1);
    REQUIRE(o.GetResult("VNQ").change == 0);
    REQUIRE(o.GetResult("VWO").change == 0);
    REQUIRE(o.GetResult("TLT").change == 1);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == 5);
    REQUIRE(o.GetResult("GOOG").change == 0);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - 2.79) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 260.3596) < 1e-3);
  }

  // use all cash
  SECTION("use all cash")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == 1);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == 5);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 1);
    REQUIRE(o.GetResult("IAU").change == 10);
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == -1);
    REQUIRE(o.GetResult("O").change == -1);
    REQUIRE(fabs(o.GetCashResult().result - 9.91) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 14.5789) < 1e-3);
  }

  // 8 deals
  SECTION("8 deals, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 8", "no more deals = true"), CASH("want = 0"));
    REQUIRE(o.GetResult("VTI").change == 1);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == LAD(7) LS(4));
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == LS(1));
    REQUIRE(o.GetResult("IAU").change == LAD(14) LS(9));
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == -1);
    REQUIRE(o.GetResult("O").change == LAD(-1));
    REQUIRE(fabs(o.GetCashResult().result - LAD(1.44) - LS(1.65)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(32.9636) - LS(29.4398)) < 1e-3);
  }

  SECTION("8 deals")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 8", "no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == 1);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == LAD(6)  LS(4));
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == LS(1));
    REQUIRE(o.GetResult("IAU").change == LAD(17) LS(9));
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == -1);
    REQUIRE(o.GetResult("O").change == LAD(-1));
    REQUIRE(fabs(o.GetCashResult().result - LAD(3.73) - LS(1.65)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(36.7092) - LS(31.0274)) < 1e-3);
  }

  // 7 deals
  SECTION("7 deals")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 7", "no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == 1);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == 6);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == 12);
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == -1);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - 4.15) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 36.9850) < 1e-3);
  }

  // 6 deals
  SECTION("6 deals, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 6", "no more deals = true"), CASH("want = 0"));
    REQUIRE(o.GetResult("VTI").change == 0);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == LAD(6) LS(7));
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == LAD(23) LS(19));
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == -1);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - LAD(2.19) - LS(10.87)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(61.1974) - LS(55.1273)) < 1e-3);
  }

  SECTION("6 deals")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 6", "no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == 0);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == 7);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == 19);
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == -1);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - 10.87) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 57.9963) < 1e-3);
  }

  // 5 deals
  SECTION("5 deals")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 5", "no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == 0);
    REQUIRE(o.GetResult("VNQ").change == LAD(3) LS(2));
    REQUIRE(o.GetResult("VWO").change == LAD(4) LS(5));
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == LAD(9) LS(13));
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - LAD(8.27) - LS(10.18)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(86.2242) - LS(85.6749)) < 1e-3);
  }

  // 4 deals
  SECTION("4 deals")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 4", "no more deals = true"));
    REQUIRE(o.GetResult("VTI").change == 0);
    REQUIRE(o.GetResult("VNQ").change == 3);
    REQUIRE(o.GetResult("VWO").change == 7);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("IEF").change == 0);
    REQUIRE(o.GetResult("IAU").change == 0);
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(o.GetResult("TSLA").change == 0);
    REQUIRE(o.GetResult("O").change == 0);
    REQUIRE(fabs(o.GetCashResult().result - 3.4) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 97.4979) < 1e-3);
  }

  // 3 deals
  SECTION("3 deals, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 3"), CASH("want = 0"));
    REQUIRE(o.GetResult("VNQ").change == LAD(3) LS(4));
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(fabs(o.GetCashResult().result - LAD(251.8) - LS(170.81)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(126.1355) - LS(120.5608)) < 1e-3);
  }

  SECTION("3 deals")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 3", "no more deals = true"));
    REQUIRE(o.GetResult("VNQ").change == 6);
    REQUIRE(o.GetResult("TLT").change == 3);
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(fabs(o.GetCashResult().result - 8.83) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 144.4607) < 1e-3);
  }

  // 2 deals
  SECTION("2 deals, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 2"), CASH("want = 0"));
    REQUIRE(o.GetResult("TLT").change == LAD(2) LS(5));
    REQUIRE(o.GetResult("GOOG").change == -1);
    REQUIRE(fabs(o.GetCashResult().result - LAD(614.85) - LS(260.61)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(211.3554) - LS(167.9817)) < 1e-3);
  }

  SECTION("2 deals")
  {
    auto o = Optimize2<TestType>(MY_PORTFOLIO, OPTS("max deals = 2", "no more deals = true"));
    REQUIRE(o.GetResult("TLT").change  == LAD(1));
    REQUIRE(o.GetResult("IAU").change  == LAD(-6));
    REQUIRE(o.GetResult("VWO").change  == LS(24));
    REQUIRE(o.GetResult("GOOG").change == LS(-1));
    REQUIRE(fabs(o.GetCashResult().result - LAD(8.75) - LS(6.21)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(278.0359) - LS(270.8238)) < 1e-3);
  }

  // 1 deal
  SECTION("1 deal, want = 0")
  {
    auto o = Optimize<TestType>(MY_PORTFOLIO, OPTS("max deals = 1"), CASH("want = 0"));
    REQUIRE(o.GetResult("IAU").change == LAD(5) LS(4));
    REQUIRE(fabs(o.GetCashResult().result - LAD(8.16) - LS(19.13)) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - LAD(265.4115) - LS(265.3412)) < 1e-3);
  }

  SECTION("1 deal")
  {
    auto o = Optimize2<TestType>(MY_PORTFOLIO, OPTS("max deals = 1", "no more deals = true"));
    REQUIRE(o.GetResult("IAU").change == 5);
    REQUIRE(fabs(o.GetCashResult().result - 8.16) < 1e-6);
    REQUIRE(fabs(o.GetResultQuality().stddev - 279.7551) < 1e-3);
  }
}

TEMPLATE_TEST_CASE("TrivialAllocationsTest", "[optimizer]", LadTestType, LsTestType)
{
  auto o = Optimize<TestType>(WANT("VTI = 60%", "IEF = 40%"), CASH("have = 10"));
  REQUIRE(o.GetResult("VTI").result == 0);
  REQUIRE(o.GetResult("IEF").result == 0);
  REQUIRE(o.GetCashResult().result == 10);

  o = Optimize2<TestType>(WANT("VTI = 60%", "IEF = 40%"), CASH("have = 1000"));
  REQUIRE(o.GetResult("VTI").result == 4);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(fabs(o.GetCashResult().result - 219.73) < 1e-6);

  o = Optimize<TestType>(HAVE("VTI = 1", "IEF = 1"), WANT("VTI = 60%", "IEF = 40%"));
  REQUIRE(o.GetResult("VTI").result == 1);
  REQUIRE(o.GetResult("IEF").result == 1);
  REQUIRE(o.GetCashResult().result == 0);

  o = Optimize<TestType>(HAVE("VTI = 1", "IEF = 5"), WANT("VTI = 60%", "IEF = 40%"));
  REQUIRE(o.GetResult("VTI").result == 3);
  REQUIRE(o.GetResult("IEF").result == 2);
  REQUIRE(fabs(o.GetCashResult().result - 76.01) < 1e-6);

  o = Optimize<TestType>(HAVE("VTI = 1", "IEF = 5"), WANT("VTI = 60%", "IEF = 40%"), CASH("have = 27.8"));
  REQUIRE(o.GetResult("VTI").result == 3);
  REQUIRE(o.GetResult("IEF").result == 2);
  REQUIRE(fabs(o.GetCashResult().result - 103.81) < 1e-6);

  o = Optimize<TestType>(HAVE("VTI = 1", "IEF = 5"), WANT("VTI = 60%", "IEF = 40%"), CASH("have = 27.8", "want = 0"));
  REQUIRE(o.GetResult("VTI").result == 3);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(fabs(o.GetCashResult().result) < 1e-6);

  o = Optimize<TestType>(
    HAVE("VTI = 1", "IEF = 5"),
    WANT("VTI = 60%", "IEF = 40%"),
    CASH("have = 27.8"),
    OPTS("no more deals = true"));
  REQUIRE(o.GetResult("VTI").result == 3);
  REQUIRE(o.GetResult("IEF").result == 3);
  REQUIRE(fabs(o.GetCashResult().result) < 1e-6);

  o = Optimize<TestType>(WANT("IAU = 2"), CASH("have = 100000"), TRAD("IAU = sell"), OPTS("no more deals = true"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(WANT("IAU = 10%"), CASH("have = 100000"), TRAD("IAU = sell"), OPTS("no more deals = true"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(
    HAVE("IAU = 1"), WANT("IAU = 2"), CASH("have = 100000"), TRAD("IAU = sell"), OPTS("no more deals=true"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(
    HAVE("IAU = 1"), WANT("IAU = 10%"), CASH("have = 100000"), TRAD("IAU = sell"), OPTS("no more deals=true"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(WANT("IAU = 2"), CASH("have = 100000"), TRAD("IAU = sell"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(WANT("IAU = 10%"), CASH("have = 100000"), TRAD("IAU = sell"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(HAVE("IAU = 1"), WANT("IAU = 2"), CASH("have = 100000"), TRAD("IAU = sell"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);

  o = Optimize<TestType>(HAVE("IAU = 1"), WANT("IAU = 10%"), CASH("have = 100000"), TRAD("IAU = sell"));
  REQUIRE(o.GetResult("IAU").change == 0);
  REQUIRE(o.GetCashResult().change == 0);
}

#if 0
TEMPLATE_TEST_CASE("BigTest", "[optimizer][.heavy]", LadTestType, LsTestType)
{
  auto o = Optimize<TestType>(
    HAVE(
      "vti=6000",
      "vnq=7000",
      "vwo=17000",
      "tlt=4000",
      "ief=3000",
      "iau=25000"),
    WANT(
      "VTI  = 20%",
      "VNQ  = 20%",
      "VWO  = 20%",
      "TLT  = 20%",
      "IEF  = 10%",
      "IAU  = 10%"),
    CASH("have=44790"),
    OPTS(
      "commission=15",
      "no more deals=true"));
  REQUIRE(o.GetResult("VTI").change == -919);
  REQUIRE(o.GetResult("VNQ").change == 323);
  REQUIRE(o.GetResult("VWO").change == LAD(-152) LS(-151));
  REQUIRE(o.GetResult("TLT").change == 1048);
  REQUIRE(o.GetResult("IEF").change == LAD(-143) LS(-144));
  REQUIRE(o.GetResult("IAU").change == LAD(2027) LS(2033));
  REQUIRE(fabs(o.GetCashResult().result - LAD(7.92) - LS(10.71)) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - LAD(38.7331) - LS(35.8386)) < 1e-3);

  auto o = Optimize<TestType>(
    HAVE(
      "vti*=600",
      "vnq*=700",
      "vwo*=1700",
      "tlt*=400",
      "ief*=300",
      "iau*=2500"),
    WANT(
      "VTI*  = 20%",
      "VNQ*  = 20%",
      "VWO*  = 20%",
      "TLT*  = 20%",
      "IEF*  = 10%",
      "IAU*  = 10%"),
    CASH("have=4479"),
    OPTS(
      "commission=2",
      "no more deals=true"));
  REQUIRE(o.GetResult("VTI*").change == -80);
  REQUIRE(o.GetResult("VNQ*").change == 38);
  REQUIRE(o.GetResult("VWO*").change == -63);
  REQUIRE(o.GetResult("TLT*").change == 104);
  REQUIRE(o.GetResult("IEF*").change == -11);
  REQUIRE(o.GetResult("IAU*").change == 139);
  REQUIRE(fabs(o.GetCashResult().result - 9.94) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - 31.1579) < 1e-3);

  if (isLsTest<TestType>()) return; // TODO: remove?

  auto o = Optimize<TestType>(
    HAVE(
      "vti=6",
      "vnq=7",
      "vwo=17",
      "tlt=4",
      "ief=3",
      "iau=25",
      "bno=16",
      "dbo=26",
      "bnd=6",
      "spy=3",
      "xop=3",
      "goog=1",
      "tsla=4",
      "o=2",
      "aapl=6"),
    WANT(
      "VTI  = 10%",
      "VNQ  = 20%",
      "VWO  = 20%",
      "TLT  = 20%",
      "IEF  = 5%",
      "IAU  = 5%",
      "BNO  = 5%",
      "DBO  = 5%",
      "BND  = 5%",
      "SPY  = 10%",
      "XOP  = 5%",
      "GOOG = 2%",
      "TSLA = 2%",
      "O    = 2%",
      "AAPL = 2%"), // = 120%
    CASH("have=65.01", "want = 2%"),
    OPTS(
      "commission=15",
      "no more deals=true"));
  // TODO: add checks

  o = Optimize<TestType>(
    HAVE(
      "vti=6000",
      "vnq=7000",
      "vwo=17000",
      "tlt=4000",
      "ief=3000",
      "iau=25000",
      "bno=16000",
      "dbo=26000",
      //"bnd=6000",
      //"spy=3000",
      "xop=3000",
      "goog=1000",
      "tsla=4000",
      "o=2000",
      "aapl=6000"),
    WANT(
      "VTI  = 10%",
      "VNQ  = 20%",
      "VWO  = 20%",
      "TLT  = 20%",
      "IEF  = 5%",
      "IAU  = 5%",
      "BNO  = 5%",
      "DBO  = 5%",
      //"BND  = 5%",
      //"SPY  = 10%",
      "XOP  = 5%",
      "GOOG = 2%",
      "TSLA = 2%",
      "O    = 2%",
      "AAPL = 2%"), // = 120%
    CASH("have=65010", "want = 2%"),
    OPTS(
      "commission=15",
      "no more deals=true"));
  // TODO: add checks
}
#endif

TEMPLATE_TEST_CASE("LadIsBad", "[optimizer]", LadTestType, LsTestType)
{
  #define ALLOCATION \
    HAVE("vti*=6", "vnq*=7", "vwo*=17", "tlt*=4", "ief*=3", "iau*=25"), \
    WANT("vti*=20%", "vnq*=20%", "vwo*=20%", "tlt*=20%", "ief*=10%", "iau*=10%"), \
    CASH("have=65.01"), OPTS("commission=2","no more deals=true")

  auto o = Optimize<TestType>(ALLOCATION);
  REQUIRE(o.GetResult("VTI*").change == LAD(-1) LS(-1));
  REQUIRE(o.GetResult("VNQ*").change == LAD(+1) LS( 0));
  REQUIRE(o.GetResult("VWO*").change == LAD(-1) LS( 0));
  REQUIRE(o.GetResult("TLT*").change == LAD(+1) LS(+1));
  REQUIRE(o.GetResult("IEF*").change == LAD( 0) LS( 0));
  REQUIRE(o.GetResult("IAU*").change == LAD( 0) LS(+4));
  REQUIRE(fabs(o.GetCashResult().result - LAD(7.75) - LS(8.92)) < 1e-6);
  REQUIRE(fabs(o.GetResultQuality().stddev - LAD(25.0446) - LS(23.0304)) < 1e-3);
}
