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

#include "curl.h"
#include "iextrading.h"
#include "optimizer.h"
#include "tableformatter.h"
#include "yahoofinance.h"

#include <algorithm>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#define VERSION "2.01"

int main(int argc, char* argv[])
{
  std::string config;
  std::string proxy;

  // Parse command line
  for (int i = 1; i < argc; i++)
  {
    std::string arg(argv[i]);

    bool v = arg == "-v" || arg == "--version";
    bool h = arg == "-h" || arg == "--help";

    if (v || h)
    {
      std::cout << "Allocator version " VERSION << std::endl;
    }

    if (h)
    {
      std::cout << std::endl;
      std::cout << "Usage:" << std::endl;
      std::cout << "  " << argv[0] << " <config> [<proxy>]" << std::endl;
    }

    if (v || h) return 0;

    if (config.empty())
    {
      config = arg;
      std::cout << "Config: " << config << std::endl;
    }
    else if (proxy.empty())
    {
      proxy = arg;
      std::cout << "Proxy: " << proxy << std::endl;
    }
    else
    {
      std::cout << "Error: Unexpected argument: '" << arg << "'" << std::endl;
      return 1;
    }
  }

  if (config.empty())
  {
    std::cout << "Error: Config file was not specified" << std::endl;
    return 1;
  }

  Allocation a;
  if (!a.Load(config))
  {
    std::cout << "Error: Failed to load config '" << config << "'" << std::endl;
    return 1;
  }

  std::cout << "Model: Least "
    << (a.UseLeastSquaresApproximation() ? "Squares Approximation" : "Absolute Deviations") << std::endl;

  MarketInfoProvider::Tickers tickers(a.GetCount());
  for (size_t i = 0; i < a.GetCount(); i++)
  {
    tickers[i] = a.GetTicker(i);
  }

  const std::string &pname = a.GetProviderName();
  std::unique_ptr<MarketInfoProvider> provider;
  if (pname == "YAHOO FINANCE")
  {
    provider = std::make_unique<YahooFinance>();
  }
  else if (pname == "IEX TRADING")
  {
    if (a.GetProviderToken().empty())
    {
      std::cout << "Error: API Token was not specified (required for IEX TRADING)" << std::endl;
      return 1;
    }
    provider = std::make_unique<IexTrading>(a.GetProviderToken());
  }
  else
  {
    std::cout << "Error: Unknown provider: " << pname << std::endl;
    return 1;
  }

  std::cout << "Provider: " << pname << std::endl;
  provider->RetrieveAssetsInfo(tickers, Curl(proxy));

  for (size_t i = 0; i < a.GetCount(); i++)
  {
    double dummy;
    const std::string &ticker = a.GetTicker(i);
    if (!provider->GetAssetPrice(ticker, MarketInfoProvider::last, dummy))
    {
      std::cout << "Error: Failed to retrieve information about: " << ticker.c_str() << std::endl;
      return 1;
    }
  }

  std::cout << "Assets info:" << std::endl;
  for (size_t i = 0; i < a.GetCount(); i++)
  {
    const std::string &ticker = a.GetTicker(i);
    std::string name;
    provider->GetAssetName(ticker, name);
    std::cout << "  " << ticker << "\t" << name << std::endl;
  }

  double avgRelativeSpread = 0;
  size_t count = 0;
  for (size_t i = 0; i < a.GetCount(); i++)
  {
    const std::string &ticker = a.GetTicker(i);

    double bid, ask;
    if (provider->GetAssetPrice(ticker, MarketInfoProvider::bid, bid) &&
      provider->GetAssetPrice(ticker, MarketInfoProvider::ask, ask) &&
      bid > 0 && ask > bid)
    {
      avgRelativeSpread += (ask - bid) / bid;
      count++;
    }
  }

  if (count > 0)
  {
    avgRelativeSpread /= count;
  }
  else
  {
    avgRelativeSpread = 0.05 / 100; // TODO: configure?
  }

  bool haveAllAsks = true;
  auto ratesProvider = [&provider, avgRelativeSpread, &haveAllAsks](const std::string &ticker, double &bid, double &ask)
  {
    if (!provider->GetAssetPrice(ticker, MarketInfoProvider::bid, bid))
    {
      bool ok = provider->GetAssetPrice(ticker, MarketInfoProvider::last, bid);
      assert(ok); // Each ticker should have last price
    }

    bool ok = provider->GetAssetPrice(ticker, MarketInfoProvider::ask, ask);
    if (!ok || ask <= bid)
    {
      ask = bid + std::max(bid * avgRelativeSpread, 0.01);
      haveAllAsks = false;
    }
  };

  double lastProgress = 0;
  clock_t lastStatusClock = clock();
  size_t maxStatusLength = 0;
  Optimizer o
  (
    [&lastProgress, &lastStatusClock, &maxStatusLength](size_t iter, int nodes, double progress) -> bool
    {
      clock_t clk = clock();
      if (clk - lastStatusClock > CLOCKS_PER_SEC / 10 || (progress - lastProgress) > .5)
      {
        std::stringstream ss;

        ss << "Iteration: " << iter << "      ";
        ss << "Nodes: "     << std::setw(4) << nodes << "      Iteration progress:";

        ss << " [";
        const int barSize = 20;
        for (int i = 0; i < barSize; i++)
        {
          ss << (i < barSize * progress ? '#' : '.');
        }
        ss << "] ";

        ss << static_cast<int>(progress * 100) << "%";

        std::string s(ss.str());
        while (s.length() < maxStatusLength) s += " ";
        maxStatusLength = s.length();

        std::cout << s << "\r" << std::flush;

        lastProgress = progress;
        lastStatusClock = clk;
      }
      return true;
    }
  );

  o.Optimize(a, ratesProvider);
  std::cout << std::string(maxStatusLength, ' ') << std::endl;

  struct Result : public Optimizer::Result
  {
    bool isCash;
    bool askIsValid = false;

    double iopv;
    bool iopvIsValid = false;

    double target;
    bool targetInPercents;
    bool targetIsValid = true;

    bool canBuy;
    bool canSell;
  };

  bool haveValidIopvs = false;
  std::vector<Result> results(a.GetCount() + 1);
  for (size_t i = 0; i < results.size(); i++)
  {
    assert(i <= a.GetCount());

    Result &r = results[i];
    r.isCash = i == a.GetCount();

    dynamic_cast<Optimizer::Result &>(r) = r.isCash ? o.GetCashResult() : o.GetResult(a.GetTicker(i));

    if (r.isCash)
    {
      r.ticker = "Cash";

      if ((r.targetIsValid = a.HasTargetCash()))
      {
        r.targetInPercents = a.IsTargetCashInPercents();
        r.target = a.GetTargetCash();
      }
    }
    else
    {
      double ask;
      r.askIsValid =
        provider->GetAssetPrice(r.ticker, MarketInfoProvider::ask, ask) && ask == r.ask;
      assert(r.askIsValid || !haveAllAsks);

      r.iopvIsValid = provider->GetAssetPrice(r.ticker, MarketInfoProvider::iopv, r.iopv);
      if (r.iopvIsValid)
      {
        haveValidIopvs = true;

        double last;
        bool ok = provider->GetAssetPrice(r.ticker, MarketInfoProvider::last, last);
        assert(ok);

        r.iopv -= last;
      }

      r.targetInPercents = a.IsTargetInPercents(i);
      r.target = a.GetTargetShares(i);

      r.canBuy = a.CanBuy(i);
      r.canSell = a.CanSell(i);
    }
  }

  // Format results table
  TableFormatter tf;
  tf[0][0]  = "Asset";
  tf[0][1]  = "Bid";
  tf[0][2]  = "Ask";
  if (haveValidIopvs) tf[0][3]  = "IOPV";
  tf[0][4]  = "Source";
  tf[0][7]  = "Change";
  tf[0][8]  = "Result";
  tf[0][11] = "Target";
  tf[0][12] = "Buy";
  tf[0][13] = "Sell";
  tf[0][14] = "Commission";

  tf[0][4].Merge(0, 2);
  tf[0][8].Merge(0, 2);
  tf[0][14].Merge(0, 1);

  struct
  {
    double have    = 0;
    double result  = 0;
    double commiss = 0;
    double sum     = 0;
  } total;

  for (size_t i = 0; i < results.size(); i++)
  {
    const Result &r = results[i];
    auto row = tf[i + 1];

    // Asset
    row[0] = r.ticker;

    // Bid
    row[1] = r.bid;

    // Ask
    row[2] = r.ask;
    row[2].SetSuffix(r.askIsValid ? haveAllAsks ? "" : " " : "*");

    // IOPV
    row[3] = r.iopv;
    if (!r.iopvIsValid)
    {
      row[3] = "";
    }
    else if (r.iopv > 0)
    {
      row[3].SetPrefix("+$");
    }
    else
    {
      row[3].SetPrefix("$");
    }

    // Source
    double have = r.have * r.bid;
    row[4] = have;
    total.have += have;

    row[5] = r.have;
    if (r.isCash) row[5] = "";

    row[6] = r.sourcePercents;
    if (!r.inPercents) row[6] = "";

    // Change
    row[7] = r.change;
    if (r.change == 0)
    {
      row[7] = "";
    }
    else if (r.isCash)
    {
      row[7].SetPrefix(r.change > 0 ? "+$" : "$");
      row[7].SetDigits(2);
    }
    else if (r.change > 0)
    {
      row[7].SetPrefix("+");
    }

    // Result
    double result = r.result * r.bid;
    row[8] = result;
    total.result += result;

    row[9] = r.result;
    if (r.isCash) row[9] = "";

    row[10] = r.percents;
    if (!r.inPercents) row[10] = "";

    // Target
    row[11] = r.target;
    row[11].SetSuffix(r.targetInPercents ? "%" : " ");
    if (r.targetInPercents) row[11].SetDigits(1);
    if (!r.targetIsValid) row[11] = "";

    // Buy
    row[12] = r.canBuy ? "Yes" : "No";
    if (r.isCash) row[12] = "";

    // Sell
    row[13] = r.canSell ? "Yes" : "No";
    if (r.isCash) row[13] = "";

    // Commission
    double commission = r.commission;
    row[14] = commission;
    if (commission == 0) row[14] = "";
    total.commiss += commission;

    double sum = r.change * (r.change > 0 ? r.ask : -r.bid);
    row[15] = r.commission * 100 / (r.commission + sum);
    row[15].SetDigits(1).SetSuffix("%)").SetPrefix("(").SetSpace("");
    if (r.commission == 0) row[15] = "";
    total.sum += sum;

    if (r.isCash)
    {
      row[0].Merge(0, 3);
      tf.GetRow(i + 1).AddFrame(TableFormatter::topbottom);
    }
  }

  tf.GetRow(0)
    .AddFrame(TableFormatter::topbottom)
    .SetAlign(TableFormatter::acenter);

  tf.GetCols({ 0, 7, 11 })
    .AddFrame(TableFormatter::leftright);

  tf.GetCols({4, 14})
    .AddFrame(TableFormatter::left);
  (tf.GetCol(15) & tf[0][14])
    .AddFrame(TableFormatter::right);

  auto row = tf[results.size() + 1];
  row[0].Merge(0, 3) = "Total (average deviation)";
  row[4] = total.have;
  row[8] = total.result;

  if (total.commiss > 0)
  {
    row[14] = total.commiss;
    if (total.commiss + total.sum > 0)
    {
      row[15] = total.commiss * 100 / (total.commiss + total.sum);
      row[15].SetDigits(1).SetSuffix("%)").SetPrefix("(").SetSpace("");
    }
  }

  double spread = total.result + total.commiss - total.have;
  row[7] = spread;
  row[7].SetPrefix("$");
  row[7].SetDigits(2);
  if (spread >= 0) row[7] = "";

  row[5] = o.GetSourceQuality().stddev;
  row[9] = o.GetResultQuality().stddev;

  (row[5] & row[9])
    .Merge(0, 1)
    .SetDigits(1)
    .SetPrefix("(")
    .SetSuffix(")");

  (tf.GetCols({ 1, 2, 4, 8, 14 }) ^ tf.GetRow(0))
    .SetDigits(2)
    .SetPrefix("$")
    .SetAlign(TableFormatter::aright);

  (tf.GetCol(3) ^ tf.GetRow(0))
    .SetDigits(2)
    .SetAlign(TableFormatter::aright);

  tf.GetCols({ 5, 9, 11 })
    .SetAlign(TableFormatter::aright);

  tf.GetCols({ 6, 10 })
    .SetAlign(TableFormatter::aright).SetDigits(1).SetSuffix("%");

  tf.GetCol(7)
    .SetAlign(TableFormatter::acenter);
  (tf[results.size()][7] & tf[results.size() + 1][7])
    .SetAlign(TableFormatter::aright);

  tf.Render(std::cout);


  results.pop_back();
  std::sort(results.begin(), results.end(),
    [](const Result &r1, const Result &r2)
    {
      int sign1 = r1.change < 0 ? -1 : r1.change > 0 ? +1 : 0;
      int sign2 = r2.change < 0 ? -1 : r2.change > 0 ? +1 : 0;

      if (sign1 == -1 && sign2 == -1)
      {
        if (r1.iopvIsValid && r2.iopvIsValid) return r1.iopv > r2.iopv;

        if (r1.iopvIsValid && r1.iopv > 0) return true;
        if (r2.iopvIsValid && r2.iopv > 0) return false;

        if (r1.iopvIsValid && r1.iopv < 0) return false;
        if (r2.iopvIsValid && r2.iopv < 0) return true;

        return r1.change * r1.bid < r2.change * r2.bid;
      }

      if (sign1 == 1 && sign2 == 1)
      {
        return r1.ask > r2.ask;
      }

      if (sign1 == 0) return false;
      if (sign2 == 0) return true;

      return sign1 < sign2;
    }
  );

  if (!haveAllAsks)
  {
    std::cout << std::endl;
    std::cout << "(*) Approximating value (not from the Market)" << std::endl;
  }

  for (size_t i = 0; i < results.size(); i++)
  {
    const Result &r = results[i];
    if (r.change == 0) break;

    if (i == 0)
    {
      std::cout << std::endl;
      std::cout << "Rebalancing strategy:" << std::endl;
    }

    std::cout << "  " << i + 1 << ". ";

    int count;
    double price;
    if (r.change > 0)
    {
      std::cout << "Buy ";
      count = static_cast<int>(r.change);
      price = r.ask;
    }
    else
    {
      std::cout << "Sell ";
      count = static_cast<int>(-r.change);
      price = r.bid;
    }

    std::cout << count << (count > 1 ? " shares of " : " share of ") << r.ticker;
    std::cout << ", market price is $" << price;
    std::cout << ", total deal sum is $" << price * count;
    std::cout << std::endl;
  }

  return 0;
}
