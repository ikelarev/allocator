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
#include "yahoofinance.h"

#include <catch.hpp>

class TestProvider : public InternetProvider
{
private:
  std::string HttpGet(
    const std::string &url, const std::vector<std::pair<std::string, std::string>> &headers) const override
  {
    const char prefix[] =
      "https://apidojo-yahoo-finance-v1.p.rapidapi.com/market/v2/get-quotes?region=US&symbols=";
    REQUIRE(url.find(prefix) == 0);

    std::string request = url.substr(sizeof(prefix) - 1);

    const std::string jsonPrefix = R"( {"quoteResponse":{"result":[ )";
    const std::string jsonSuffix = R"( ]}} )";

    const std::string TLT = R"(
      {
        "ask": 121.78,
        "bid": 121.11,
        "regularMarketPrice": 121.31,
        "shortName": "iShares 20+ Year Treasury Bond",
        "symbol": "TLT"
      },
      {
        "regularMarketPrice": 121.22,
        "shortName": "iShares 20+ Year Treasury Bond",
        "symbol": "^TLT-IV"
      }
    )";

    const std::string VTI = R"(
      {
        "regularMarketPrice": 117.22,
        "shortName": "Vanguard Total Stock Market ETF",
        "symbol": "VTI"
      },
      {
        "regularMarketPrice": 117.1881,
        "shortName": "Vanguard Total Stock Market ETF",
        "symbol": "^VTI-IV"
      }
    )";

    const std::string GOOG = R"(
      {
        "ask": 808.25,
        "bid": 807.88,
        "regularMarketPrice": 807.88,
        "shortName": "Alphabet Inc.",
        "symbol": "GOOG"
      },
      {
        "shortName": "Alphabet Inc.",
        "symbol": "^GOOG-IV"
      }
    )";

    const std::string O = R"(
      {
        "regularMarketPrice": 59.07,
        "shortName": "Realty Income Corporation ",
        "symbol": "O"
      },
      {
        "shortName": "Realty Income Corporation ",
        "symbol": "^O-IV"
      }
    )";

    const std::string BND = R"(
      {
        "regularMarketPrice": 81.0545,
        "shortName": "Vanguard Total Bond Mkt ETF (In",
        "symbol": "^BND-IV"
      },
      {
        "regularMarketPrice": 81.06,
        "shortName": "Vanguard Total Bond Market ETF",
        "symbol": "BND"
      }
    )";

    const std::string SPY = R"(
      {
        "regularMarketPrice": 227.0126,
        "shortName": "SPDR Trust Series 1 (Intraday V",
        "symbol": "^SPY-IV"
      }
    )";

    const std::string NA = R"(
      {
        "shortName": "N/A",
        "symbol": "NA1"
      },
      {
        "shortName": "N/A",
        "symbol": "NA2"
      }
    )";

    if (request == "TLT,^TLT-IV")
    {
      return jsonPrefix + TLT + jsonSuffix;
    }

    if (request == "TLT,^TLT-IV,VTI,^VTI-IV,GOOG,^GOOG-IV,O,^O-IV")
    {
      return jsonPrefix + TLT + "," + VTI + "," + GOOG + "," + O + jsonSuffix;
    }

    if (request == "SPY,^SPY-IV,BND,^BND-IV")
    {
      // Shuffled incomplete response
      return jsonPrefix + BND + "," + SPY + jsonSuffix;
    }

    if (request == "TSLA,^TSLA-IV")
    {
      // Invalid response
      return
        "\n\n\n"
        "TSLA\n"
        "\"TSLA\"\n"
        "TSLA,111\n"
        "\"\n"
        "wrong line\n"
        "TSLA  ,  237.75  ,  \" 237.41\"  ,  \"237.75 \"  ,   \"Tesla Motors, Inc.\"\n"
        "TS LA\n"
        ",,,\"A\",\"B\",\"C\",D,E,,,,F,\"G\",\"H\",\"I\",,,,"
        "\n\n\n";
    }

    if (request == "NA1,^NA1-IV,NA2,^NA2-IV")
    {
      return jsonPrefix + NA + jsonSuffix;
    }

    REQUIRE(false);
    return std::string();
  }
};

TEST_CASE("SimpleTest", "[yahoofinance]")
{
  YahooFinance yf("APIKEY");
  yf.RetrieveAssetsInfo({ "TLT" }, TestProvider());

  std::string name;
  bool ok = yf.GetAssetName("VTI", name);
  REQUIRE(!ok);

  ok = yf.GetAssetName("VTI", name);
  REQUIRE(!ok);

  ok = yf.GetAssetName("TLT", name);
  REQUIRE(ok);
  REQUIRE(name == "iShares 20+ Year Treasury Bond");

  double price;
  ok = yf.GetAssetPrice("VTI", YahooFinance::last, price);
  REQUIRE(!ok);

  ok = yf.GetAssetPrice("VTI", YahooFinance::last, price);
  REQUIRE(!ok);

  ok = yf.GetAssetPrice("TLT", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price == 121.31);

  ok = yf.GetAssetPrice("TLT", YahooFinance::bid, price);
  REQUIRE(ok);
  REQUIRE(price == 121.11);

  ok = yf.GetAssetPrice("TLT", YahooFinance::ask, price);
  REQUIRE(ok);
  REQUIRE(price == 121.78);

  ok = yf.GetAssetPrice("TLT", YahooFinance::iopv, price);
  REQUIRE(ok);
  REQUIRE(price == 121.22);
}

TEST_CASE("SecondTest", "[yahoofinance]")
{
  YahooFinance yf("APIKEY");
  yf.RetrieveAssetsInfo({ "TLT", "VTI", "GOOG", "O" }, TestProvider());

  bool ok;
  double price;
  std::string name;

  ok = yf.GetAssetName("TLT", name);
  REQUIRE(ok);
  REQUIRE(name == "iShares 20+ Year Treasury Bond");
  ok = yf.GetAssetPrice("TLT", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price == 121.31);
  ok = yf.GetAssetPrice("TLT", YahooFinance::bid, price);
  REQUIRE(ok);
  REQUIRE(price == 121.11);
  ok = yf.GetAssetPrice("TLT", YahooFinance::ask, price);
  REQUIRE(ok);
  REQUIRE(price == 121.78);
  ok = yf.GetAssetPrice("TLT", YahooFinance::iopv, price);
  REQUIRE(ok);
  REQUIRE(price == 121.22);

  ok = yf.GetAssetName("VTI", name);
  REQUIRE(ok);
  REQUIRE(name == "Vanguard Total Stock Market ETF");
  ok = yf.GetAssetPrice("VTI", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price == 117.22);
  ok = yf.GetAssetPrice("VTI", YahooFinance::bid, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("VTI", YahooFinance::ask, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("VTI", YahooFinance::iopv, price);
  REQUIRE(ok);
  REQUIRE(price == 117.1881);

  ok = yf.GetAssetName("GOOG", name);
  REQUIRE(ok);
  REQUIRE(name == "Alphabet Inc.");
  ok = yf.GetAssetPrice("GOOG", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price == 807.88);
  ok = yf.GetAssetPrice("GOOG", YahooFinance::bid, price);
  REQUIRE(ok);
  REQUIRE(price == 807.88);
  ok = yf.GetAssetPrice("GOOG", YahooFinance::ask, price);
  REQUIRE(ok);
  REQUIRE(price == 808.25);
  ok = yf.GetAssetPrice("GOOG", YahooFinance::iopv, price);
  REQUIRE(!ok);

  ok = yf.GetAssetName("O", name);
  REQUIRE(ok);
  REQUIRE(name == "Realty Income Corporation ");
  ok = yf.GetAssetPrice("O", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price == 59.07);
  ok = yf.GetAssetPrice("O", YahooFinance::bid, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("O", YahooFinance::ask, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("O", YahooFinance::iopv, price);
  REQUIRE(!ok);
}

TEST_CASE("IncompleteTest", "[yahoofinance]")
{
  YahooFinance yf("APIKEY");
  yf.RetrieveAssetsInfo({ "SPY", "BND" }, TestProvider());

  bool ok;
  double price;
  std::string name;

  ok = yf.GetAssetName("SPY", name);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("SPY", YahooFinance::last, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("SPY", YahooFinance::bid, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("SPY", YahooFinance::ask, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("SPY", YahooFinance::iopv, price);
  REQUIRE(ok);

  ok = yf.GetAssetName("BND", name);
  REQUIRE(ok);
  REQUIRE(name == "Vanguard Total Bond Market ETF");
  ok = yf.GetAssetPrice("BND", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price == 81.06);
  ok = yf.GetAssetPrice("BND", YahooFinance::iopv, price);
  REQUIRE(ok);
  REQUIRE(price == 81.0545);
}

TEST_CASE("InvalidResponseTest", "[yahoofinance]")
{
  YahooFinance yf("APIKEY");
  yf.RetrieveAssetsInfo({ "TSLA" }, TestProvider());

  bool ok;
  double price;
  std::string name;

  ok = yf.GetAssetName("TSLA", name);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("TSLA", YahooFinance::last, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("TSLA", YahooFinance::bid, price);
  REQUIRE(!ok);
  ok = yf.GetAssetPrice("TSLA", YahooFinance::ask, price);
  REQUIRE(!ok);
}

TEST_CASE("NATest", "[yahoofinance]")
{
  YahooFinance yf("APIKEY");
  yf.RetrieveAssetsInfo({ "NA1", "NA2" }, TestProvider());

  std::string name;
  bool ok = yf.GetAssetName("NA1", name);
  REQUIRE(ok);
  REQUIRE(name == "N/A");

  ok = yf.GetAssetName("NA2", name);
  REQUIRE(ok);
  REQUIRE(name == "N/A");
}

TEST_CASE("YFCurlTest", "[yahoofinance]")
{
  YahooFinance yf("12fd58682fmsh893aa1c5a80b513p12eadajsn4712484e61f3");
  yf.RetrieveAssetsInfo({ "GLD" }, Curl());

  std::string name;
  bool ok = yf.GetAssetName("GLD", name);
  REQUIRE(ok);
  REQUIRE(name == "SPDR Gold Trust");

  double price;
  ok = yf.GetAssetPrice("GLD", YahooFinance::last, price);
  REQUIRE(ok);
  REQUIRE(price > 50);
  REQUIRE(price < 200);

  double iopv;
  ok = yf.GetAssetPrice("GLD", YahooFinance::iopv, iopv);
  REQUIRE(ok);
  REQUIRE(fabs((price - iopv) / price) < 0.02);
}
