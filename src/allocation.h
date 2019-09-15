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

#include <istream>
#include <string>
#include <vector>

class Allocation
{
public:
  bool Load(const std::string &fileName);
  bool Load(std::istream &stream);

  size_t GetCount() const;
  const std::string &GetTicker(size_t index) const;
  double GetExistingShares(size_t index) const;
  double GetTargetShares(size_t index) const;
  bool IsTargetInPercents(size_t index) const;
  double GetCommission(size_t index) const;

  bool CanBuy(size_t index) const;
  bool CanSell(size_t index) const;

  double GetExistingCash() const;
  bool HasTargetCash() const;
  double GetTargetCash() const;
  bool IsTargetCashInPercents() const;

  bool UseAllCash() const;
  size_t GetMaxDeals() const;

  bool UseLeastSquaresApproximation() const;
  const std::string &GetProviderName() const;
  const std::string &GetProviderToken() const;

#ifdef _DEBUG
  void Dump() const;
#endif

private:
  template<class Source>
  bool LoadInternal(Source f);

  static int LoadHelper(void *param, const char *section, const char *name, const char *value);
  static char *ReaderHelper(char *str, int num, void *p);

  struct LoadContext;
  bool LoadHandler(LoadContext &ctx, std::string section, std::string name, std::string value);

  static bool StringToDouble(const std::string &s, double &d, bool &percents);
  static bool StringToDouble(const std::string &s, double &d);
  static bool StringToBool(const std::string &s, bool &b);
  static bool StringToULong(const std::string &s, size_t &n);

  struct Asset;
  Asset &GetAsset(const std::string &ticker, bool create = false);

private:
  struct Asset
  {
    std::string ticker;
    double exists = 0;

    double target = 0;
    bool targetInPercents = false;

    double commission = 0;

    bool canBuy = true;
    bool canSell = true;
  };

  std::vector<Asset> assets_;

  double cash_               = 0;
  double cashTarget_         = 0;
  bool cashTargetInPercents_ = false;
  bool cashTargetIsSet_      = false;

  bool noMoreDeals_  = false;
  size_t maxDeals_   = 0;

  bool useLeastSquares_ = true;
  std::string providerName_ = "YAHOO FINANCE";
  std::string providerToken_;
};
