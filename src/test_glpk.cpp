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

#include <catch.hpp>
#include <glpk.h>

TEST_CASE("GlpkTest1", "[glpk]")
{
  // F(X) = x1 + 2x2 - 2x3 → min
  // 4x1 + 3x2 - x3≤10
  // - 2x2 + 5x3≥3
  // x1 + 2x3=9
  // by http://math.semestr.ru/simplex/simplex-standart.php

  glp_prob *lp = glp_create_prob();
  glp_set_obj_dir(lp, GLP_MIN);

  glp_add_rows(lp, 3);
  glp_set_row_bnds(lp, 1, GLP_UP, 0, 10);
  glp_set_row_bnds(lp, 2, GLP_LO, 3,  0);
  glp_set_row_bnds(lp, 3, GLP_FX, 9,  0);

  glp_add_cols(lp, 3);
  glp_set_col_bnds(lp, 1, GLP_LO, 0, 0);
  glp_set_col_bnds(lp, 2, GLP_LO, 0, 0);
  glp_set_col_bnds(lp, 3, GLP_LO, 0, 0);

  glp_set_obj_coef(lp, 1, 1);
  glp_set_obj_coef(lp, 2, 2);
  glp_set_obj_coef(lp, 3, -2);

  double cond1[] = { 0,  4,  3, -1 };
  double cond2[] = { 0,  0, -2,  5 };
  double cond3[] = { 0,  1,  0,  2 };

  int idx[] = { 0, 1, 2, 3 };
  glp_set_mat_row(lp, 1, 3, idx, cond1);
  glp_set_mat_row(lp, 2, 3, idx, cond2);
  glp_set_mat_row(lp, 3, 3, idx, cond3);

  int res = glp_simplex(lp, NULL);
  REQUIRE(res == 0);

  int status = glp_get_status(lp);
  REQUIRE(status == GLP_OPT);

  double z = glp_get_obj_val(lp);
  double x1 = glp_get_col_prim(lp, 1);
  double x2 = glp_get_col_prim(lp, 2);
  double x3 = glp_get_col_prim(lp, 3);

  glp_delete_prob(lp);

  REQUIRE(z == -9);
  REQUIRE(x1 == 0);
  REQUIRE(x2 == 0);
  REQUIRE(x3 == 4.5);
}

TEST_CASE("GlpkTest2", "[glpk]")
{
  // F(X) = x1 + x2 - x3 + x5+15 → max (min)
  // -3x1 + x2 + x3=3
  // 4x1 + 2x2 - x4=12
  // 2x1 - x2 + x5=2
  // x1 ≥ 0, x2 ≥ 0, x3 ≥ 0, x4 ≥ 0, x5 ≥ 0
  // by http://math.semestr.ru/simplex/simplex-standart.php

  glp_prob *lp = glp_create_prob();

  glp_add_rows(lp, 3);
  glp_set_row_bnds(lp, 1, GLP_FX,  3, 0);
  glp_set_row_bnds(lp, 2, GLP_FX, 12, 0);
  glp_set_row_bnds(lp, 3, GLP_FX,  2, 0);

  glp_add_cols(lp, 5);
  glp_set_col_bnds(lp, 1, GLP_LO, 0, 0);
  glp_set_col_bnds(lp, 2, GLP_LO, 0, 0);
  glp_set_col_bnds(lp, 3, GLP_LO, 0, 0);
  glp_set_col_bnds(lp, 4, GLP_LO, 0, 0);
  glp_set_col_bnds(lp, 5, GLP_LO, 0, 0);

  glp_set_obj_coef(lp, 1,  1);
  glp_set_obj_coef(lp, 2,  1);
  glp_set_obj_coef(lp, 3, -1);
  glp_set_obj_coef(lp, 4,  0);
  glp_set_obj_coef(lp, 5,  1);

  double cond1[] = { 0,  -3,  1, 1,  0, 0 };
  double cond2[] = { 0,   4,  2, 0, -1, 0 };
  double cond3[] = { 0,   2, -1, 0,  0, 1 };

  int idx[] = { 0, 1, 2, 3, 4, 5 };
  glp_set_mat_row(lp, 1, 5, idx, cond1);
  glp_set_mat_row(lp, 2, 5, idx, cond2);
  glp_set_mat_row(lp, 3, 5, idx, cond3);

  glp_set_obj_dir(lp, GLP_MIN);
  int res = glp_exact(lp, NULL);
  REQUIRE(res == 0);

  int status = glp_get_status(lp);
  REQUIRE(status == GLP_OPT);

  double z = glp_get_obj_val(lp);
  double x1 = glp_get_col_prim(lp, 1);
  double x2 = glp_get_col_prim(lp, 2);
  double x3 = glp_get_col_prim(lp, 3);
  double x4 = glp_get_col_prim(lp, 4);
  double x5 = glp_get_col_prim(lp, 5);

  REQUIRE(z == -3);
  REQUIRE(x1 == 2);
  REQUIRE(x2 == 2);
  REQUIRE(x3 == 7);
  REQUIRE(x4 == 0);
  REQUIRE(x5 == 0);

  glp_set_obj_dir(lp, GLP_MAX);
  res = glp_exact(lp, NULL);
  REQUIRE(res == 0);

  status = glp_get_status(lp);
  REQUIRE(status == GLP_UNBND);

  glp_delete_prob(lp);
}
