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

#include <catch.hpp>
#include <sstream>

TEST_CASE("EmptyTable", "[tableformatter]")
{
  TableFormatter tf;
  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() == "");
}

TEST_CASE("OneCell", "[tableformatter]")
{
  TableFormatter tf;
  tf.GetCell(0, 0)
    .SetText("Text")
    .SetFrame(TableFormatter::all);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "+------+\n"
    "| Text |\n"
    "+------+\n");
}

TEST_CASE("SimpleTable", "[tableformatter]")
{
  TableFormatter tf;

  tf.GetCell(0, 0).SetText("A");
  tf.GetCell(1, 0).SetText("A longer string");
  tf.GetCell(1, 1).SetText("A");
  tf.GetCell(0, 1).SetText("");

  tf.GetCols({0, 1}).AddFrame(TableFormatter::topleft);
  tf.GetRows({1, 0}).AddFrame(TableFormatter::bottomright);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "+-----------------+---+\n"
    "| A               |   |\n"
    "+-----------------+---+\n"
    "| A longer string | A |\n"
    "+-----------------+---+\n");
}

TEST_CASE("AlignmentTest", "[tableformatter]")
{
  TableFormatter tf;

  tf.GetCell(0, 0).SetText("Left").SetFrame(TableFormatter::top);
  tf.GetCell(1, 0).SetText("~~~~~~ ~~~~~~ ~~~~~~");
  tf.GetCell(2, 0).SetText("Left-2").SetAlign(TableFormatter::aleft);
  tf.GetCell(3, 0).SetText("Right" ).SetAlign(TableFormatter::aright);
  tf.GetCell(4, 0).SetText("Center").SetAlign(TableFormatter::acenter)
    .SetFrame(TableFormatter::bottom);

  tf.GetCol(0).AddFrame(TableFormatter::leftright);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "+----------------------+\n"
    "| Left                 |\n"
    "| ~~~~~~ ~~~~~~ ~~~~~~ |\n"
    "| Left-2               |\n"
    "|                Right |\n"
    "|        Center        |\n"
    "+----------------------+\n");
}

TEST_CASE("HorizontalMerge", "[tableformatter]")
{
  TableFormatter tf;

  tf.GetCell(0, 0).Merge(0, 1).SetText("Top-left cell");
  tf.GetCell(0, 2).SetText("One");
  tf.GetCell(1, 0).SetText("Two");
  tf.GetCell(1, 1).Merge(0, 1).SetText("Bottom-right cell");

  tf.GetRows({0, 1}).AddFrame(TableFormatter::all);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "+--------------------+-----+\n"
    "| Top-left cell      | One |\n"
    "+-----+--------------+-----+\n"
    "| Two | Bottom-right cell  |\n"
    "+-----+--------------------+\n");
}

TEST_CASE("VerticalMerge", "[tableformatter]")
{
  TableFormatter tf;

  tf.GetCell(0, 0).Merge(2, 0).SetText("One").SetAlign(TableFormatter::acenter);
  tf.GetCell(3, 0).SetText("Two");
  tf.GetCell(0, 1).SetText("Three");
  tf.GetCell(1, 1).Merge(2, 0).SetText("Four");

  tf.GetCols({0, 1}).AddFrame(TableFormatter::all);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "+-----+-------+\n"
    "|     | Three |\n"
    "|     +-------+\n"
    "| One |       |\n"
    "|     | Four  |\n"
    "+-----+       |\n"
    "| Two |       |\n"
    "+-----+-------+\n");
}

TEST_CASE("NumbersFormatting", "[tableformatter]")
{
  TableFormatter tf;

  tf[0][0] = -5.4321;
  tf[1][0] = -5.4321;
  tf[2][0] = -5.4321;

  tf[1][0].SetDigits(2);
  tf[2][0].SetDigits(5);

  tf.GetCol(0)
    .SetPrefix("$")
    .SetSuffix("p.")
    .SetSpace(" [ ", " ] ")
    .SetAlign(TableFormatter::acenter)
    .SetFrame(TableFormatter::leftright);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "|    [ -$5p. ]    |\n"
    "|  [ -$5.43p. ]   |\n"
    "| [ -$5.43210p. ] |\n");
}

TEST_CASE("MoreComplicatedTest", "[tableformatter]")
{
  TableFormatter tf;

  tf.GetCell(0, 0).Merge(0, 4);
  tf.GetCell(1, 1).Merge(1, 4);
  tf.GetCell(3, 2).Merge(0, 4);

  tf.GetCell(0, 0).SetText("Hello");
  tf.GetCell(1, 1).SetText("World");
  tf.GetCell(3, 2).SetText("More long text");

  tf.GetCell(2, 0).Merge(2, 0);
  tf.GetCell(2, 0).SetText("Vertical");

  tf.GetCell(0, 5).Merge(0, 1);
  tf.GetCell(0, 5).SetText("Horizontal");

  tf.GetCell(3, 1).SetText("x");
  tf.GetCell(1, 6).SetText("y");
  tf.GetCell(2, 6).SetText("z");

  tf.GetCell(0, 0).SetFrame(TableFormatter::all);
  tf.GetCell(1, 1).SetFrame(TableFormatter::all);
  tf.GetCell(3, 2).SetFrame(TableFormatter::all);
  tf.GetCell(2, 0).SetFrame(TableFormatter::all);
  tf.GetCell(0, 5).SetFrame(TableFormatter::all);

  tf.GetCell(4, 2).SetNumber(1.999).SetFrame(TableFormatter::bottom).SetDigits(3);
  tf.GetCell(4, 5).SetNumber(3).SetFrame(TableFormatter::bottom).SetAlign(TableFormatter::acenter);

  tf.GetCell(0, 0).SetAlign(TableFormatter::aright);
  tf.GetCell(1, 1).SetAlign(TableFormatter::acenter);
  tf.GetCell(3, 2).SetAlign(TableFormatter::acenter);
  tf.GetCell(0, 5).SetAlign(TableFormatter::aleft);

  std::stringstream ss;
  tf.Render(ss);
  REQUIRE(ss.str() ==
    "+----------------------+-------------+\n"
    "|                Hello | Horizontal  |\n"
    "+----------+-----------+---------+---+\n"
    "           |        World        | y  \n"
    "+----------+                     |    \n"
    "|          |                     | z  \n"
    "|          +---+-----------------+---+\n"
    "| Vertical | x |   More long text    |\n"
    "|          |   +---------------------+\n"
    "|          |     1.999      3         \n"
    "+----------+    ------- ---------     \n");

  std::stringstream ss2;
  tf.Render(ss2);
  REQUIRE(ss.str() == ss2.str());
}
