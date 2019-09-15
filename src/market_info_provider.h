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

#include "internet_provider.h"

#include <string>
#include <vector>

class MarketInfoProvider
{
public:
  virtual ~MarketInfoProvider() { }

  enum PriceType
  {
    last,
    bid,
    ask,
    iopv, // indicative optimized portfolio value
  };

  using Tickers = std::vector<std::string>;

  virtual void RetrieveAssetsInfo(const Tickers &t, const InternetProvider &prov) = 0; // TODO: return bool
  virtual bool GetAssetName(const std::string &ticker, std::string &name) const = 0;
  virtual bool GetAssetPrice(const std::string &ticker, PriceType type, double &price) const = 0;
};
