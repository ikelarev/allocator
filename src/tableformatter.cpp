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

#include "tableformatter.h"
#include "mipsolver.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>

TableFormatter::Cell TableFormatter::GetCell(size_t row, size_t col)
{
  return Cell(cells_[row][col]);
}

TableFormatter::IndexHelper TableFormatter::operator [](size_t row)
{
  return std::move(IndexHelper(*this, row));
}

TableFormatter::CellGroup TableFormatter::GetRow(size_t row)
{
  CellGroup res;
  auto it = cells_.find(row);
  if (it != cells_.end())
  {
    auto &row = it->second;
    for (auto jt = row.begin(); jt != row.end(); jt++)
    {
      res &= Cell(jt->second);
    }
  }
  return std::move(res);
}

TableFormatter::CellGroup TableFormatter::GetCol(size_t col)
{
  CellGroup res;
  for (auto it = cells_.begin(); it != cells_.end(); it++)
  {
    auto &row = it->second;
    auto jt = row.find(col);
    if (jt != row.end())
    {
      res &= Cell(jt->second);
    }
  }
  return std::move(res);
}

TableFormatter::CellGroup TableFormatter::GetRows(std::set<size_t> rows)
{
  CellGroup res;
  for (auto it = rows.begin(); it != rows.end(); it++)
  {
    res &= GetRow(*it);
  }
  return std::move(res);
}

TableFormatter::CellGroup TableFormatter::GetCols(std::set<size_t> cols)
{
  CellGroup res;
  for (auto it = cols.begin(); it != cols.end(); it++)
  {
    res &= GetCol(*it);
  }
  return std::move(res);
}

void TableFormatter::Render(std::ostream &stream)
{
  size_t rowcount = 0;
  size_t colcount = 0;
  size_t maxwidth = 0;

  for (auto it = cells_.begin(); it != cells_.end(); it++)
  {
    auto &row = it->second;
    for (auto jt = row.begin(); jt != row.end(); jt++)
    {
      CellInfo &cell = jt->second;

      rowcount = std::max(rowcount, it->first + cell.rowspan + 1);
      colcount = std::max(colcount, jt->first + cell.colspan + 1);

      RenderCell(it->first, jt->first);
      assert(cell.render.size() == cell.rowspan + 1);

      for (size_t i = 0; i < cell.render.size(); i++)
      {
        maxwidth = std::max(maxwidth, cell.render[i].size());
      }
    }
  }

  MIPSolver s;
  MIPSolver::Expression sum;
  std::vector<MIPSolver::Variable> x(colcount);
  for (size_t i = 0; i < x.size(); i++)
  {
    x[i] = s.GetIntegerVariable(static_cast<double>(maxwidth));
    sum += x[i];
  }

  for (auto it = cells_.begin(); it != cells_.end(); it++)
  {
    auto &row = it->second;
    for (auto jt = row.begin(); jt != row.end(); jt++)
    {
      CellInfo &cell = jt->second;

      size_t width = 0;
      for (size_t i = 0; i < cell.render.size(); i++)
      {
        width = std::max(width, cell.render[i].size());
      }

      MIPSolver::Expression sum;
      for (size_t i = 0; i <= cell.colspan; i++)
      {
        assert(jt->first + i < x.size());
        sum += x[jt->first + i];
      }

      s.Restrict(sum >= static_cast<double>(width));
    }
  }

  MIPSolver::Solution sol = s.Minimize(sum);
  assert(sol);


  std::vector<std::vector<bool>> vert(rowcount);
  for (size_t row = 0; row < rowcount; row++)
  {
    vert[row].resize(colcount + 1);
  }

  std::vector<std::vector<bool>> horz(rowcount + 1);
  for (size_t row = 0; row <= rowcount; row++)
  {
    horz[row].resize(colcount);
  }

  for (size_t row = 0; row < rowcount; row++)
  {
    for (size_t col = 0; col < colcount; col++)
    {
      size_t offsetrow;
      size_t offsetcol;
      CellInfo *c = GetCell(row, col, offsetrow, offsetcol);
      if (c && offsetrow == 0 && offsetcol == 0)
      {
        assert(row < vert.size());
        assert(col < vert[row].size());
        assert(col + c->colspan <= vert[row].size());

        assert(row < horz.size());
        assert(col < horz[row].size());
        assert(row + c->rowspan <= horz.size());
        assert(col < horz[row + c->rowspan + 1].size());

        for (size_t i = row; i <= row + c->rowspan; i++)
        {
          if (c->frame & left  ) vert[i][col                 ] = true;
          if (c->frame & right ) vert[i][col + c->colspan + 1] = true;
        }

        for (size_t j = col; j <= col + c->colspan; j++)
        {
          if (c->frame & top   ) horz[row                 ][j] = true;
          if (c->frame & bottom) horz[row + c->rowspan + 1][j] = true;
        }
      }
    }
  }

  std::vector<std::vector<char>> node(rowcount + 1);
  for (size_t row = 0; row <= rowcount; row++)
  {
    node[row].resize(colcount + 1, ' ');

    for (size_t col = 0; col <= colcount; col++)
    {
      int v = 0;
      int h = 0;

      if (row < rowcount && vert[row    ][col]) v++;
      if (row > 0        && vert[row - 1][col]) v++;

      if (col < colcount && horz[row][col    ]) h++;
      if (col > 0        && horz[row][col - 1]) h++;

      if (h > 0 && v > 0)
      {
        node[row][col] = '+';
      }
      else if (h == 0 && v == 2)
      {
        node[row][col] = '|';
      }
      else if (h == 2 && v == 0)
      {
        node[row][col] = '-';
      }
    }
  }

  std::vector<bool> hashorz(rowcount + 1);
  for (size_t row = 0; row <= rowcount; row++)
  {
    for (size_t col = 0; col < colcount; col++)
    {
      if (horz[row][col])
      {
        hashorz[row] = true;
        break;
      }
    }
  }

  std::vector<bool> hasvert(colcount + 1);
  for (size_t col = 0; col <= colcount; col++)
  {
    for (size_t row = 0; row < rowcount; row++)
    {
      if (vert[row][col])
      {
        hasvert[col] = true;
        break;
      }
    }
  }


  for (size_t row = 0; row <= rowcount; row++)
  {
    if (hashorz[row])
    {
      for (size_t col = 0; col <= colcount; col++)
      {
        if (hasvert[col]) stream << node[row][col];
        if (col >= colcount) break;

        size_t width = static_cast<size_t>(sol(x[col]));
        stream << std::setw(width) << std::setfill(horz[row][col] ? '-' : ' ') << std::string();
      }
      stream << std::endl;
    }
    if (row >= rowcount) break;

    for (size_t col = 0; col <= colcount; col++)
    {
      if (hasvert[col]) stream << (vert[row][col] ? '|' : ' ');
      if (col >= colcount) break;

      size_t offsetrow;
      size_t offsetcol;
      if (CellInfo *c = GetCell(row, col, offsetrow, offsetcol))
      {
        if (offsetcol == 0)
        {
          size_t width = 0;
          for (size_t i = 0; i <= c->colspan; i++)
          {
            width += static_cast<size_t>(sol(x[col + i]));
            if (i > 0) width += hasvert[col + i];
          }

          assert(offsetrow < c->render.size());
          assert(width >= c->render[offsetrow].size());

          const std::string &text = c->render[offsetrow];

          stream
            << std::setw(c->align == acenter ? (width + text.size()) / 2 : width)
            << (c->align == aleft ? std::left : std::right)
            << std::setfill(' ')
            << text;

          if (c->align == acenter)
          {
            stream << std::setw((width - text.size() + 1) / 2) << std::string();
          }

          col += c->colspan;
        }
      }
      else
      {
        size_t width = static_cast<size_t>(sol(x[col]));
        stream << std::setw(width) << std::setfill(' ') << std::string();
      }
    }
    stream << std::endl;

  }
}

void TableFormatter::RenderCell(size_t row, size_t col)
{
  assert(cells_.find(row) != cells_.end());
  assert(cells_[row].find(col) != cells_[row].end());

  CellInfo &cell = cells_[row][col];
  std::string prefix = cell.prefix;

  std::string text;
  if (cell.isNumber)
  {
    double n = cell.number;
    if (n < 0)
    {
      n = -n;
      prefix = "-" + prefix;
    }

    std::stringstream ss;
    ss << std::setprecision(cell.digits) << std::fixed << n;
    text = ss.str();
  }
  else
  {
    text = cell.text;
  }

  if (!text.empty())
  {
    text = cell.spaces[0] + prefix + text + cell.suffix + cell.spaces[1];
  }

  // TODO: multiline cells
  cell.render.resize(cell.rowspan + 1);
  size_t index = cell.rowspan / 2;
  assert(index < cell.render.size());
  cell.render[index] = text;

  // TODO: min/max width
}

TableFormatter::CellInfo *TableFormatter::GetCell(size_t row, size_t col, size_t &offsetrow, size_t &offsetcol)
{
  for (auto it = cells_.begin(); it != cells_.end(); it++)
  {
    for (auto jt = it->second.begin(); jt != it->second.end(); jt++)
    {
      CellInfo &cell = jt->second;

      size_t row1 = it->first;
      size_t row2 = it->first + cell.rowspan;

      size_t col1 = jt->first;
      size_t col2 = jt->first + cell.colspan;

      if (row >= row1 && row <= row2 && col >= col1 && col <= col2)
      {
        offsetrow = row - row1;
        offsetcol = col - col1;
        return &cell;
      }
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TableFormatter::CellGroup &TableFormatter::CellGroup::SetText(const std::string &s)
{
  return Invoke(
    [&s](CellInfo &cell)
    {
      cell.text = s;
      cell.isNumber = false;
    }
  );
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetNumber(double n)
{
  return Invoke(
    [n](CellInfo &cell)
    {
      cell.number = n;
      cell.isNumber = true;
    }
  );
}

TableFormatter::CellGroup &TableFormatter::CellGroup::operator =(const std::string &s)
{
  return SetText(s);
}

TableFormatter::CellGroup &TableFormatter::CellGroup::operator =(double n)
{
  return SetNumber(n);
}

TableFormatter::CellGroup &TableFormatter::CellGroup::Merge(size_t rows, size_t cols)
{
  return Invoke(
    [rows, cols](CellInfo &cell)
    {
      cell.rowspan = rows;
      cell.colspan = cols;
    }
  );
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetFrame(FrameFlags frame)
{
  return Invoke([frame](CellInfo &cell) { cell.frame = frame; });
}

TableFormatter::CellGroup &TableFormatter::CellGroup::AddFrame(FrameFlags frame)
{
  return Invoke(
    [frame](CellInfo &cell)
    {
      cell.frame = static_cast<FrameFlags>(cell.frame | frame);
    }
  );
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetAlign(AlignFlags align)
{
  return Invoke([align](CellInfo &cell) { cell.align = align; });
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetDigits(size_t digits)
{
  return Invoke([digits](CellInfo &cell) { cell.digits = digits; });
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetPrefix(const std::string &s)
{
  return Invoke([&s](CellInfo &cell) { cell.prefix = s; });
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetSuffix(const std::string &s)
{
  return Invoke([&s](CellInfo &cell) { cell.suffix = s; });
}

TableFormatter::CellGroup &TableFormatter::CellGroup::SetSpace(const std::string &left, const std::string &right)
{
  return Invoke(
    [&left, &right](CellInfo &cell)
    {
      cell.spaces[0] = left;
      cell.spaces[1] = right;
    }
  );
}

TableFormatter::CellGroup TableFormatter::CellGroup::operator &(const CellGroup &cells) const
{
  TableFormatter::CellGroup res(*this);
  for (auto it = cells.cells_.begin(); it != cells.cells_.end(); it++)
  {
    res.cells_.insert(*it);
  }
  return std::move(res);
}

TableFormatter::CellGroup TableFormatter::CellGroup::operator ^(const CellGroup &cells) const
{
  TableFormatter::CellGroup res(*this);
  for (auto it = cells.cells_.begin(); it != cells.cells_.end(); it++)
  {
    res.cells_.erase(*it);
  }
  return std::move(res);
}

TableFormatter::CellGroup &TableFormatter::CellGroup::operator &=(const CellGroup &cells)
{
  *this = *this & cells;
  return *this;
}

TableFormatter::CellGroup &TableFormatter::CellGroup::Invoke(const std::function<void (CellInfo &)> &f)
{
  for (auto it = cells_.begin(); it != cells_.end(); it++)
  {
    f(**it);
  }
  return *this;
}