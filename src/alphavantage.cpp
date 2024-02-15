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

#include "alphavantage.h"

#include <json.hpp>

using json = nlohmann::json;

AlphaVantage::AlphaVantage(const std::string &apikey) : apikey_(apikey)
{
}

void AlphaVantage::RetrieveAssetsInfo(const Tickers &t, const InternetProvider &prov)
{
  for (size_t i = 0; i < t.size(); i++)
  {
    std::string resp1 = prov.HttpGet(
      "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=" + t[i] + "&apikey=" + apikey_);

    std::string resp2 = prov.HttpGet(
      "https://www.alphavantage.co/query?function=SYMBOL_SEARCH&keywords=" + t[i] + "&apikey=" + apikey_);

    try
    {
      json json = json::parse(resp1);
      json = json["Global Quote"];
      if (json["01. symbol"] != t[i])
        continue;

      assets_[t[i]].price = std::stod(json["05. price"].get<std::string>());
      assets_[t[i]].name = "?";

      json = json::parse(resp2);
      json = json["bestMatches"];

      for (int i = 0; i < json.size(); i++)
      {
        std::string symbol = json[i]["1. symbol"];
        if (symbol != t[i])
          continue;

        assets_[t[i]].name = json[i]["2. name"].get<std::string>();

        break;
      }
    }
    catch (json::exception &)
    {
    }
  }
}

bool AlphaVantage::GetAssetName(const std::string &ticker, std::string &name) const
{
  auto it = assets_.find(ticker);
  if (it == assets_.end())
    return false;

  name = it->second.name;

  return true;
}

bool AlphaVantage::GetAssetPrice(const std::string &ticker, PriceType type, double &price) const
{
  if (type != last)
    return false;

  auto it = assets_.find(ticker);
  if (it == assets_.end())
    return false;

  price = it->second.price;

  return true;
}
