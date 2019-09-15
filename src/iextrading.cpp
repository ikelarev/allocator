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

#include "iextrading.h"

#include <algorithm>
#include <cassert>
#include <sstream>

IexTrading::IexTrading(const std::string &token) : token_(token)
{
}

void IexTrading::RetrieveAssetsInfo(const Tickers &t, const InternetProvider &prov)
{
  for (size_t i = 0; i < t.size(); i++)
  {
    StringMap quote = Download("https://cloud.iexapis.com/v1/stock/" +
      t[i] + "/quote?format=csv&token=" + token_, prov);

    AssetInfo &a = assets_[t[i]];
    a.name = quote["companyName"];
    a.hasPrice = ParsePrice(quote["latestPrice"], a.price);
    a.hasBid   = ParsePrice(quote["iexBidPrice"], a.bid);
    a.hasAsk   = ParsePrice(quote["iexAskPrice"], a.ask);
  }
}

bool IexTrading::GetAssetName(const std::string &ticker, std::string &name) const
{
  auto it = assets_.find(ticker);
  if (it == assets_.end()) return false;

  const AssetInfo &a = it->second;
  name = a.name;
  return true;
}

bool IexTrading::GetAssetPrice(const std::string &ticker, PriceType type, double &price) const
{
  auto it = assets_.find(ticker);
  if (it == assets_.end()) return false;

  const AssetInfo &a = it->second;
  bool res = false;
  switch (type)
  {
  case last:
    if ((res = a.hasPrice)) price = a.price;
    break;
  case bid:
    if ((res = a.hasBid)) price = a.bid;
    break;
  case ask:
    if ((res = a.hasAsk)) price = a.ask;
    break;
  case iopv:
    break;
  default:
    assert(false);
  }

  return res;
}

IexTrading::StringMap IexTrading::Download(const std::string &url, const InternetProvider &prov)
{
  StringMap res;

  bool haveHeaders = false;
  std::vector<std::string> headers;

  std::stringstream ss(prov.HttpGet(url));
  while (!ss.eof())
  {
    std::string line;
    std::getline(ss, line);

    if (!haveHeaders)
    {
      headers = Split(line);
      haveHeaders = true;
      continue;
    }

    std::vector<std::string> parts = Split(line);
    for (size_t i = 0; i < std::min(parts.size(), headers.size()); i++)
    {
      res[headers[i]] = parts[i];
    }
  }

  return std::move(res);
}

bool IexTrading::ParsePrice(const std::string &s, double &price)
{
  char *end;
  price = strtod(s.c_str(), &end);
  return !*end && price > 0;
}

std::vector<std::string> IexTrading::Split(const std::string &str)
{
  std::vector<std::string> parts;

  std::istringstream ss(str);
  std::string s;
  while (!ss.eof())
  {
    if (ss.peek() == '"')
    {
      char ch = ss.get();
      assert(ch == '"');
      std::getline(ss, s, '"');
      parts.push_back(s);

      if (ss.get() != ',') break;
    }
    else
    {
      std::getline(ss, s, ',');
      parts.push_back(s);
    }
  }

  return std::move(parts);
}
