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

#include "market_info_provider.h"

#include <map>
#include <string>
#include <vector>

class IexTrading : public MarketInfoProvider
{
public:
  IexTrading(const std::string &token);

  void RetrieveAssetsInfo(const Tickers &t, const InternetProvider &prov) override;

  bool GetAssetName(const std::string &ticker, std::string &name) const override;
  bool GetAssetPrice(const std::string &ticker, PriceType type, double &price) const override;

private:
  using StringMap = std::map<std::string, std::string>;
  StringMap Download(const std::string &url, const InternetProvider &prov);

  static bool ParsePrice(const std::string &s, double &price);
  static std::vector<std::string> Split(const std::string &str);

private:
  struct AssetInfo
  {
    std::string name;

    double price;
    double bid;
    double ask;

    bool hasPrice = false;
    bool hasBid   = false;
    bool hasAsk   = false;
  };

  using AssetInfoMap = std::map<std::string, AssetInfo>;
  AssetInfoMap assets_;

  std::string token_;
};
