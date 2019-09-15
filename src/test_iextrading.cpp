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

#include <catch.hpp>

namespace
{
  class TestProvider : public InternetProvider
  {
  private:
    std::string HttpGet(const std::string &url) const override
    {
      const char prefix[] = "https://cloud.iexapis.com/v1/stock/";
      const char suffix[] = "/quote?format=csv&token=TOKEN";
      REQUIRE(url.find(prefix) == 0);

      size_t pos = url.find(suffix);
      REQUIRE(pos == url.length() - sizeof(suffix) + 1);

      std::string request = url.substr(sizeof(prefix) - 1);
      std::string ticker = request.substr(0, request.length() - sizeof(suffix) + 1);

      if (ticker == "TLT")
      {
        return "companyName,latestPrice,iexBidPrice,iexAskPrice\n"
          "\"iShares 20+ Year Treasury Bond\",121.31,121.11,121.78";
      }
      if (ticker == "VTI")
      {
        return "companyName,latestPrice,iexBidPrice,iexAskPrice\n"
          "\"Vanguard Total Stock Market ETF\",117.22,,";
      }
      if (ticker == "GOOG")
      {
        return "companyName,latestPrice,iexBidPrice,iexAskPrice\n"
          "\"Alphabet Inc.\",807.88,807.88,808.25";
      }
      if (ticker == "O")
      {
        return "companyName,latestPrice,iexBidPrice,iexAskPrice\n"
          "\"Realty Income Corporation \",59.07,,";
      }
      if (ticker == "NA1" || ticker == "NA2")
      {
        return "companyName,latestPrice,iexBidPrice,iexAskPrice\n"
          "N/A,,,";
      }

      if (ticker == "TSLA")
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

      REQUIRE(false);
      return std::string();
    }
  };
}

TEST_CASE("IexTradingSimpleTest", "[iextrading]")
{
  IexTrading iet("TOKEN");
  iet.RetrieveAssetsInfo({ "TLT" }, TestProvider());

  std::string name;
  bool ok = iet.GetAssetName("VTI", name);
  REQUIRE(!ok);

  ok = iet.GetAssetName("VTI", name);
  REQUIRE(!ok);

  ok = iet.GetAssetName("TLT", name);
  REQUIRE(ok);
  REQUIRE(name == "iShares 20+ Year Treasury Bond");

  double price;
  ok = iet.GetAssetPrice("VTI", IexTrading::last, price);
  REQUIRE(!ok);

  ok = iet.GetAssetPrice("VTI", IexTrading::last, price);
  REQUIRE(!ok);

  ok = iet.GetAssetPrice("TLT", IexTrading::last, price);
  REQUIRE(ok);
  REQUIRE(price == 121.31);

  ok = iet.GetAssetPrice("TLT", IexTrading::bid, price);
  REQUIRE(ok);
  REQUIRE(price == 121.11);

  ok = iet.GetAssetPrice("TLT", IexTrading::ask, price);
  REQUIRE(ok);
  REQUIRE(price == 121.78);

  ok = iet.GetAssetPrice("TLT", IexTrading::iopv, price);
  REQUIRE_FALSE(ok);
}

TEST_CASE("IexTradingSecondTest", "[iextrading]")
{
  IexTrading iet("TOKEN");
  iet.RetrieveAssetsInfo({ "TLT", "VTI", "GOOG", "O" }, TestProvider());

  bool ok;
  double price;
  std::string name;

  ok = iet.GetAssetName("TLT", name);
  REQUIRE(ok);
  REQUIRE(name == "iShares 20+ Year Treasury Bond");
  ok = iet.GetAssetPrice("TLT", IexTrading::last, price);
  REQUIRE(ok);
  REQUIRE(price == 121.31);
  ok = iet.GetAssetPrice("TLT", IexTrading::bid, price);
  REQUIRE(ok);
  REQUIRE(price == 121.11);
  ok = iet.GetAssetPrice("TLT", IexTrading::ask, price);
  REQUIRE(ok);
  REQUIRE(price == 121.78);
  ok = iet.GetAssetPrice("TLT", IexTrading::iopv, price);
  REQUIRE_FALSE(ok);

  ok = iet.GetAssetName("VTI", name);
  REQUIRE(ok);
  REQUIRE(name == "Vanguard Total Stock Market ETF");
  ok = iet.GetAssetPrice("VTI", IexTrading::last, price);
  REQUIRE(ok);
  REQUIRE(price == 117.22);
  ok = iet.GetAssetPrice("VTI", IexTrading::bid, price);
  REQUIRE(!ok);
  ok = iet.GetAssetPrice("VTI", IexTrading::ask, price);
  REQUIRE(!ok);
  ok = iet.GetAssetPrice("VTI", IexTrading::iopv, price);
  REQUIRE_FALSE(ok);

  ok = iet.GetAssetName("GOOG", name);
  REQUIRE(ok);
  REQUIRE(name == "Alphabet Inc.");
  ok = iet.GetAssetPrice("GOOG", IexTrading::last, price);
  REQUIRE(ok);
  REQUIRE(price == 807.88);
  ok = iet.GetAssetPrice("GOOG", IexTrading::bid, price);
  REQUIRE(ok);
  REQUIRE(price == 807.88);
  ok = iet.GetAssetPrice("GOOG", IexTrading::ask, price);
  REQUIRE(ok);
  REQUIRE(price == 808.25);
  ok = iet.GetAssetPrice("GOOG", IexTrading::iopv, price);
  REQUIRE_FALSE(ok);

  ok = iet.GetAssetName("O", name);
  REQUIRE(ok);
  REQUIRE(name == "Realty Income Corporation ");
  ok = iet.GetAssetPrice("O", IexTrading::last, price);
  REQUIRE(ok);
  REQUIRE(price == 59.07);
  ok = iet.GetAssetPrice("O", IexTrading::bid, price);
  REQUIRE(!ok);
  ok = iet.GetAssetPrice("O", IexTrading::ask, price);
  REQUIRE(!ok);
  ok = iet.GetAssetPrice("O", IexTrading::iopv, price);
  REQUIRE_FALSE(ok);
}

TEST_CASE("IexTradingInvalidAnswerTest", "[iextrading]")
{
  IexTrading iet("TOKEN");
  iet.RetrieveAssetsInfo({ "TSLA" }, TestProvider());

  bool ok;
  double price;
  std::string name;

  ok = iet.GetAssetName("TSLA", name);
  REQUIRE(ok);
  ok = iet.GetAssetPrice("TSLA", IexTrading::last, price);
  REQUIRE_FALSE(ok);
  ok = iet.GetAssetPrice("TSLA", IexTrading::bid, price);
  REQUIRE_FALSE(ok);
  ok = iet.GetAssetPrice("TSLA", IexTrading::ask, price);
  REQUIRE_FALSE(ok);
}

TEST_CASE("IexTradingNATest", "[iextrading]")
{
  IexTrading iet("TOKEN");
  iet.RetrieveAssetsInfo({ "NA1", "NA2" }, TestProvider());

  std::string name;
  bool ok = iet.GetAssetName("NA1", name);
  REQUIRE(ok);
  REQUIRE(name == "N/A");

  ok = iet.GetAssetName("NA2", name);
  REQUIRE(ok);
  REQUIRE(name == "N/A");
}

TEST_CASE("IexTradingietCurlTest", "[iextrading]")
{
  IexTrading iet("pk_1651a96086e74d8a9083edf498f09647");
  iet.RetrieveAssetsInfo({ "GLD" }, Curl());

  std::string name;
  bool ok = iet.GetAssetName("GLD", name);
  REQUIRE(ok);
  REQUIRE(name == "SPDR Gold Trust");

  double price;
  ok = iet.GetAssetPrice("GLD", IexTrading::last, price);
  REQUIRE(ok);
  REQUIRE(price > 50);
  REQUIRE(price < 200);

  double iopv;
  ok = iet.GetAssetPrice("GLD", IexTrading::iopv, iopv);
  REQUIRE_FALSE(ok);
}
