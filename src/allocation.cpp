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

#include "allocation.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <ini.h>
#include <iostream> // TODO: debug-only
#include <set>

#ifdef _DEBUG
  #include <iomanip>
#endif

bool Allocation::Load(const std::string &fileName)
{
  return LoadInternal(std::bind(&ini_parse, fileName.c_str(), std::placeholders::_1, std::placeholders::_2));
}

bool Allocation::Load(std::istream &stream)
{
  return LoadInternal(
    std::bind(&ini_parse_stream, &ReaderHelper, &stream, std::placeholders::_1, std::placeholders::_2));
}

size_t Allocation::GetCount() const
{
  return assets_.size();
}

const std::string &Allocation::GetTicker(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].ticker;
}

double Allocation::GetExistingShares(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].exists;
}

double Allocation::GetTargetShares(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].target;
}

bool Allocation::IsTargetInPercents(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].targetInPercents;
}

double Allocation::GetCommission(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].commission;
}

bool Allocation::CanBuy(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].canBuy;
}

bool Allocation::CanSell(size_t index) const
{
  assert(index < assets_.size());
  return assets_[index].canSell;
}

double Allocation::GetExistingCash() const
{
  return cash_;
}

bool Allocation::HasTargetCash() const
{
  return cashTargetIsSet_;
}

double Allocation::GetTargetCash() const
{
  return cashTarget_;
}

bool Allocation::IsTargetCashInPercents() const
{
  return cashTargetInPercents_;
}

bool Allocation::UseAllCash() const
{
  return noMoreDeals_;
}

size_t Allocation::GetMaxDeals() const
{
  return maxDeals_;
}

bool Allocation::UseLeastSquaresApproximation() const
{
  return useLeastSquares_;
}

const std::string &Allocation::GetProviderName() const
{
  return providerName_;
}

const std::string &Allocation::GetProviderToken() const
{
  return providerToken_;
}

#ifdef _DEBUG
void Allocation::Dump() const
{
  std::cout << "Assets:" << std::endl;
  for (auto it = assets_.cbegin(); it != assets_.cend(); it++)
  {
    const Asset &a = *it;
    std::cout << "  ";
    std::cout << std::setw(4) << it->ticker << ": ";
    std::cout << std::setw(3) << a.exists << " -> ";
    std::cout << a.target << (a.targetInPercents ? "%" : "") << std::endl;
  }
  std::cout << "  Cash: " << cash_;
  if (cashTargetIsSet_)
  {
    std::cout << " -> " << cashTarget_ << (cashTargetInPercents_ ? "%" : "");
  }
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  if (noMoreDeals_) std::cout << "  Use all cash" << std::endl;
  if (maxDeals_ > 0) std::cout << "  Max deals: " << maxDeals_ << std::endl;
  std::cout << "  Model: " << (useLeastSquares_ ? "LS" : "LAD") << std::endl;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Allocation::LoadContext
{
  Allocation &a;
  double commission;
  double withdraw;
  std::set<std::string> commissionSet;

  LoadContext(Allocation &a) : a(a), commission(0), withdraw(0) { }
};

template<class Source>
bool Allocation::LoadInternal(Source f)
{
  LoadContext ctx(*this);
  if (int line = f(&LoadHelper, &ctx))
  {
    // TODO: more informative messages are required here
    std::cout << "Error: Pailed to parse config, line " << line << std::endl;
    return false;
  }

  for (auto it = assets_.begin(); it != assets_.end(); it++)
  {
    if (ctx.commissionSet.find(it->ticker) == ctx.commissionSet.end())
    {
      it->commission = ctx.commission;
    }
  }

  cash_ -= ctx.withdraw;

  return true;
}

int Allocation::LoadHelper(void *param, const char *section, const char *name, const char *value)
{
  Allocation::LoadContext &ctx = *static_cast<Allocation::LoadContext *>(param);
  return ctx.a.LoadHandler(ctx, section, name, value);
}

char *Allocation::ReaderHelper(char *str, int num, void *p)
{
  std::istream &stream = *static_cast<std::istream *>(p);

  std::string s;
  std::getline(stream, s);

  str[s.copy(str, num)] = 0;

  return stream.eof() && s.empty() ? 0 : str;
}

bool Allocation::LoadHandler(LoadContext &ctx, std::string section, std::string name, std::string value)
{
  std::string sourceValue = value;

  std::transform(section.begin(), section.end(), section.begin(), &toupper);
  std::transform(   name.begin(),    name.end(),    name.begin(), &toupper);
  std::transform(  value.begin(),   value.end(),   value.begin(), &toupper);

  if (section == "HAVE")
  {
    Asset &a = GetAsset(name, true);
    if (!StringToDouble(value, a.exists)) return false;
  }
  else if (section == "WANT")
  {
    Asset &a = GetAsset(name, true);
    if (!StringToDouble(value, a.target, a.targetInPercents)) return false;
  }
  else if (section == "COMMISSION")
  {
    Asset &a = GetAsset(name, true);
    if (!StringToDouble(value, a.commission)) return false;
    ctx.commissionSet.insert(name);
  }
  else if (section == "TRADE")
  {
    Asset &a = GetAsset(name, true);
    if (value == "BUY")
    {
      a.canBuy  = true;
      a.canSell = false;
    }
    else if (value == "SELL")
    {
      a.canBuy  = false;
      a.canSell = true;
    }
    else if (value == "KEEP")
    {
      a.canBuy  = false;
      a.canSell = false;
    }
    else if (value == "TRADE")
    {
      a.canBuy  = true;
      a.canSell = true;
    }
    else
    {
      return false;
    }
  }
  else if (section == "CASH")
  {
    if (name == "HAVE")
    {
      if (!StringToDouble(value, cash_)) return false;
    }
    else if (name == "WITHDRAW")
    {
      if (!StringToDouble(value, ctx.withdraw)) return false;
    }
    else if (name == "WANT")
    {
      cashTargetIsSet_ = true;
      if (!StringToDouble(value, cashTarget_, cashTargetInPercents_)) return false;
    }
    else
    {
      return false;
    }
  }
  else if (section == "OPTIONS")
  {
    if (name == "COMMISSION")
    {
      if (!StringToDouble(value, ctx.commission)) return false;
    }
    else if (name == "NO MORE DEALS" || name == "USE ALL CASH")
    {
      if (!StringToBool(value, noMoreDeals_)) return false;
    }
    else if (name == "MAX DEALS")
    {
      if (!StringToULong(value, maxDeals_)) return false;
    }
    else if (name == "MODEL")
    {
      if (value == "LAD")
      {
        useLeastSquares_ = false;
      }
      else if (value == "LSAPPROX")
      {
        useLeastSquares_ = true;
      }
      else
      {
        return false;
      }
    }
    else if (name == "MARKET INFO PROVIDER" || name == "PROVIDER")
    {
      providerName_ = value;
    }
    else if (name == "API TOKEN" || name == "TOKEN")
    {
      providerToken_ = sourceValue;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }

  return true;
}

bool Allocation::StringToDouble(const std::string &s, double &d, bool &percents)
{
  char *end;
  d = strtod(s.c_str(), &end);
  percents = false;
  if (*end == '%')
  {
    percents = true;
    end++;
  }
  return !*end;
}

bool Allocation::StringToDouble(const std::string &s, double &d)
{
  bool percents;
  return StringToDouble(s, d, percents) && !percents;
}

bool Allocation::StringToBool(const std::string &s, bool &b)
{
  if (s == "TRUE" || s == "YES" || s == "1") b = true;
    else if (s == "FALSE" || s == "NO" || s == "0") b = false;
      else return false;
  return true;
}

bool Allocation::StringToULong(const std::string &s, size_t &n)
{
  char *end;
  n = strtoul(s.c_str(), &end, 0);
  return !*end;
}

Allocation::Asset &Allocation::GetAsset(const std::string &ticker, bool create)
{
  for (size_t i = 0; i < assets_.size(); i++)
  {
    if (assets_[i].ticker == ticker) return assets_[i];
  }

  assert(create);
  Allocation::Asset a;
  a.ticker = ticker;
  assets_.push_back(a);
  return assets_.back();
}
