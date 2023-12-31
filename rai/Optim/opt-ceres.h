/*  ------------------------------------------------------------------
    Copyright (c) 2011-2020 Marc Toussaint
    email: toussaint@tu-berlin.de

    This code is distributed under the MIT License.
    Please see <root-path>/LICENSE for details.
    --------------------------------------------------------------  */

#pragma once

#include "MathematicalProgram.h"

namespace ceres {
class Problem;
}

//===========================================================================

struct CeresInterface {
  shared_ptr<MathematicalProgram_Factored> P;
//  MathematicalProgram& P;

  CeresInterface(const shared_ptr<MathematicalProgram_Factored>& P) : P(P) {}

  arr solve();
};

//===========================================================================

struct Conv_MathematicalProgram_CeresProblem {
  shared_ptr<MathematicalProgram_Factored> P;

  arr x_full, phi_full; //the full linear memory for all decision variables and all features

  ptr<ceres::Problem> ceresProblem;

  Conv_MathematicalProgram_CeresProblem(const shared_ptr<MathematicalProgram_Factored>& _P);
};
