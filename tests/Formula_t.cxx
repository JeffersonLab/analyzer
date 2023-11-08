//*-- Author :    Ole Hansen   07-Nov-23
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Formula_t                                                                 //
//                                                                           //
// Test THaFormula class                                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
# include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "THaFormula.h"
#include "THaVarList.h"
#include "TError.h"
#include <memory>
#include <cmath>

using namespace std;

namespace Podd::Tests {

} // namespace Podd::Tests

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("Formulas of Global Variables", "[Formula]") // NOLINT(*-function-cognitive-complexity)
{
  // Some arbitrary numbers to work with
  Int_t i = 5;
  UInt_t u = 12;
  Float_t f = 3.14F;
  Double_t d1 = 2.718, d2 = 1.4142;

  // Use private variable list
  auto vars = make_unique<THaVarList>();

  vars->Define("i", i);
  vars->Define("u", u);
  vars->Define("f", f);
  vars->Define("d1", d1);
  vars->Define("d2", d2);

  SECTION("Scalars") {
    auto fi = make_unique<THaFormula>("fi", "2*i", false, vars.get());
    auto fu = make_unique<THaFormula>("fu", "-11*u", false, vars.get());
    auto ff = make_unique<THaFormula>("ff", "2.5*f", false, vars.get());
    auto fd1 = make_unique<THaFormula>("fd1", "-22+d1", false, vars.get());
    auto fd2 = make_unique<THaFormula>("fd2", "d2/5", false, vars.get());

    auto f1 = make_unique<THaFormula>("f1", "2*i-11*u", false, vars.get());
    auto f2 = make_unique<THaFormula>("f2", "i+d1", false, vars.get());
    auto f3 = make_unique<THaFormula>("f3", "3*f-u+16+11*d2", false, vars.get());
    auto f4 = make_unique<THaFormula>("f4", "2**i", false, vars.get());
    auto f5 = make_unique<THaFormula>("f5", "sqrt(i)", false, vars.get());

    // TODO Redirect error output to stringstream, check message (requires logger)
    gErrorIgnoreLevel = kBreak;  // Suppress nuisance error message
    auto fbad = make_unique<THaFormula>("f2", "i+d", false, vars.get());

    SECTION("Successful compilation") {
      CHECK_FALSE(fi->IsError());
      CHECK_FALSE(fu->IsError());
      CHECK_FALSE(ff->IsError());
      CHECK_FALSE(fd1->IsError());
      CHECK_FALSE(fd2->IsError());
      CHECK_FALSE(f1->IsError());
      CHECK_FALSE(f2->IsError());
      CHECK_FALSE(f3->IsError());
      CHECK_FALSE(f4->IsError());
      CHECK_FALSE(f5->IsError());
      CHECK(fbad->IsError());
      //CHECK(ostr.str() == "Error in <THaFormula::Compile>:  Bad numerical expression : \"d\"");
    }

//TODO !IsArray() etc.
//    SECTION("Formula status") {
//
//    }

//TODO registration in list of functions

    SECTION("Single Variable Arithmetic") {
      CHECK_THAT(fi->Eval(),  Catch::Matchers::WithinRel(2.*i));
      CHECK_THAT(fu->Eval(),  Catch::Matchers::WithinRel(-11.*u));
      CHECK_THAT(ff->Eval(),  Catch::Matchers::WithinRel(2.5F*f));
      CHECK_THAT(fd1->Eval(), Catch::Matchers::WithinRel(-22+d1));
      CHECK_THAT(fd2->Eval(), Catch::Matchers::WithinRel(d2/5));
    }

    SECTION("Multiple Variable Arithmetic") {
      CHECK_THAT(f1->Eval(), Catch::Matchers::WithinRel(2.*i-11.*u));
      CHECK_THAT(f2->Eval(), Catch::Matchers::WithinRel(i+d1));
      CHECK_THAT(f3->Eval(), Catch::Matchers::WithinAbs(3*f-u+16+11*d2,1e-6));
      CHECK_THAT(f4->Eval(), Catch::Matchers::WithinRel(pow(2,i)));
      CHECK_THAT(f5->Eval(), Catch::Matchers::WithinRel(sqrt(i)));
    }

    //TODO changing values

    //TODO formulas of formulas

  }
}
