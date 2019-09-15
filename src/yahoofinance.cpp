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

#include "yahoofinance.h"

#include <json.hpp>

using json = nlohmann::json;

struct YahooFinance::Impl
{
  std::map<std::string, json> data;
};

YahooFinance::YahooFinance()
{
  impl_ = std::make_unique<Impl>();
}

YahooFinance::~YahooFinance()
{
}

// by https://github.com/pstadler/ticker.sh/blob/master/ticker.sh
void YahooFinance::RetrieveAssetsInfo(const Tickers &t, const InternetProvider &prov)
{
  impl_->data.clear();

  std::string url =
    "http://query1.finance.yahoo.com/v7/finance/quote?"
    "lang=en-US&region=US&corsDomain=finance.yahoo.com&symbols=";

  for (size_t i = 0; i < t.size(); i++)
  {
    if (i > 0) url += ',';
    url += t[i] + "," + GetIopvTicker(t[i]);
  }

  std::string resp = prov.HttpGet(url);

  try
  {
    json json = json::parse(resp);
    json = json["quoteResponse"]["result"];

    for (auto it = json.cbegin(); it != json.cend(); it++)
    {
      try
      {
        std::string symbol = (*it)["symbol"];
        impl_->data[symbol] = *it;
      }
      catch (json::exception &)
      {
      }
    }
  }
  catch (json::exception &ex)
  {
  }
}

bool YahooFinance::GetAssetName(const std::string &ticker, std::string &name) const
{
  try
  {
    auto it = impl_->data.find(ticker);
    if (it != impl_->data.end())
    {
      name = it->second["shortName"].get<std::string>();
      return true;
    }
  }
  catch (json::exception &)
  {
  }

  return false;
}

bool YahooFinance::GetAssetPrice(const std::string &ticker, PriceType type, double &price) const
{
  try
  {
    if (type == PriceType::iopv)
    {
      auto it = impl_->data.find(GetIopvTicker(ticker));
      if (it != impl_->data.end())
      {
        price = it->second["regularMarketPrice"].get<double>();
        return price > 0;
      }
      return false;
    }

    auto it = impl_->data.find(ticker);
    if (it != impl_->data.end())
    {
      if (type == PriceType::last)
      {
        price = it->second["regularMarketPrice"].get<double>();
        return price > 0;
      }
      if (type == PriceType::bid)
      {
        price = it->second["bid"].get<double>();
        return price > 0;
      }
      if (type == PriceType::ask)
      {
        price = it->second["ask"].get<double>();
        return price > 0;
      }
    }
  }
  catch (json::exception &)
  {
  }

  return false;
}

std::string YahooFinance::GetIopvTicker(const std::string &ticker) const
{
  return "^" + ticker + "-IV";
}
