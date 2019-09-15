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

#include <functional>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

class TableFormatter
{
public:
  enum FrameFlags
  {
    none   = 0x0,

    left   = 0x1,
    right  = 0x2,
    top    = 0x4,
    bottom = 0x8,

    topleft     = top | left,
    topright    = top | right,
    bottomleft  = bottom | left,
    bottomright = bottom | right,

    leftright = left | right,
    topbottom = top | bottom,

    all = topleft | bottomright,
  };

  enum AlignFlags
  {
    aleft,
    aright,
    acenter,
  };

  class Cell;
  Cell GetCell(size_t row, size_t col);

  class IndexHelper;
  IndexHelper operator [](size_t row);

  class CellGroup;
  CellGroup GetRow(size_t row);
  CellGroup GetCol(size_t col);

  CellGroup GetRows(std::set<size_t> rows);
  CellGroup GetCols(std::set<size_t> cols);

  void Render(std::ostream &stream);

private:
  void RenderCell(size_t row, size_t col);

  struct CellInfo;
  CellInfo *GetCell(size_t row, size_t col, size_t &offsetrow, size_t &offsetcol);

private:
  struct CellInfo
  {
    std::string text;
    double number = 0;
    bool isNumber = false;
    size_t digits = 0;

    FrameFlags frame = none;
    AlignFlags align = aleft;

    std::string prefix;
    std::string suffix;

    size_t rowspan = 0;
    size_t colspan = 0;

    std::string spaces[2] = { " ", " " };

    std::vector<std::string> render;
  };

  std::map<size_t, std::map<size_t, CellInfo>> cells_; // cells_[row][col]
};

class TableFormatter::CellGroup
{
public:
  CellGroup &SetText(const std::string &s);
  CellGroup &SetNumber(double n);

  CellGroup &operator =(const std::string &s);
  CellGroup &operator =(double n);

  CellGroup &Merge(size_t rows, size_t cols);

  CellGroup &SetFrame(FrameFlags frame);
  CellGroup &AddFrame(FrameFlags frame);

  CellGroup &SetAlign(AlignFlags align);

  CellGroup &SetDigits(size_t digits);
  CellGroup &SetPrefix(const std::string &s);
  CellGroup &SetSuffix(const std::string &s);
  CellGroup &SetSpace(const std::string &left = " ", const std::string &right = " ");

  CellGroup operator &(const CellGroup &cells) const;
  CellGroup operator ^(const CellGroup &cells) const;

  CellGroup &operator &=(const CellGroup &cells);

protected:
  CellGroup() { }
  void AddCell(CellInfo &cell)
  {
    cells_.insert(&cell);
  }

  CellGroup &Invoke(const std::function<void (CellInfo &)> &f);

protected:
  std::set<CellInfo *> cells_;

  friend class TableFormatter;
};

class TableFormatter::Cell : public CellGroup
{
private:
  Cell(CellInfo &cell)
  {
    AddCell(cell);
  }

  friend class TableFormatter;
};

class TableFormatter::IndexHelper
{
public:
  CellGroup operator [](size_t col)
  {
    return std::move(tf_.GetCell(row_, col));
  }

private:
  IndexHelper(TableFormatter &tf, size_t row) : tf_(tf), row_(row) { }

private:
  TableFormatter &tf_;
  size_t row_;

  friend class TableFormatter;
};
