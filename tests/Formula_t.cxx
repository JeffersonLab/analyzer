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
# include <catch2/generators/catch_generators.hpp>
# include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "THaFormula.h"
#include "THaVarList.h"
#include "TError.h"
#include "DataType.h"   // kBig
#include <memory>
#include <cmath>
#include <vector>

using namespace std;

namespace Podd::Tests {

} // namespace Podd::Tests

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("Formulas of Global Scalar Variables", "[Formula]") // NOLINT(*-function-cognitive-complexity)
{
  // Use private variable list
  auto vars = make_unique<THaVarList>();

  // Some arbitrary numbers to work with
  Int_t i = 5;
  UInt_t u = 12;
  Float_t f = 3.14F;
  Double_t d1 = 2.718;
  Double_t d2 = 1.4142;

  vars->Define("i", i);
  vars->Define("u", u);
  vars->Define("f", f);
  vars->Define("d1", d1);
  vars->Define("d2", d2);

  auto fi  = make_shared<THaFormula>("fi",  "2*i", false, vars.get());
  auto fu  = make_shared<THaFormula>("fu",  "-11*u", false, vars.get());
  auto ff  = make_shared<THaFormula>("ff",  "2.5*f", false, vars.get());
  auto fd1 = make_shared<THaFormula>("fd1", "-22+d1", false, vars.get());
  auto fd2 = make_shared<THaFormula>("fd2", "d2/5", false, vars.get());
  auto fcs = make_shared<THaFormula>("f6",  "cos(d1)**2+sin(d1)**2", false, vars.get());

  auto f1  = make_shared<THaFormula>("f1",  "2*i-11*u", false, vars.get());
  auto f2  = make_shared<THaFormula>("f2",  "i+d1", false, vars.get());
  auto f3  = make_shared<THaFormula>("f3",  "3*f-u+16+11*d2", false, vars.get());
  auto f4  = make_shared<THaFormula>("f4",  "2**i", false, vars.get());
  auto f5  = make_shared<THaFormula>("f5",  "sqrt(i)", false, vars.get());
  auto f6  = make_shared<THaFormula>("f7",  "cos(d1)-sin(d2)", false, vars.get());

  const auto formulas = {fi, fu, ff, fd1, fd2, fcs, f1, f2, f3, f4, f5, f6 };

  // TODO Redirect error output to stringstream, check message (requires logger)
  gErrorIgnoreLevel = kBreak;  // Suppress nuisance error message
  auto fbad = make_shared<THaFormula>("f2", "i+d", false, vars.get());

  SECTION("Successful compilation") {
    for( const auto& form : formulas) { ;
      REQUIRE_FALSE(form->IsZombie());
      REQUIRE_FALSE(form->IsError());
    }
  }

  SECTION("Expected compilation failure") {
    REQUIRE(fbad->IsError());
    //CHECK(ostr.str() == "Error in <THaFormula::Compile>:  Bad numerical expression : \"d\"");
    CHECK_THAT(fbad->Eval(), Catch::Matchers::WithinRel(kBig));
  }

  SECTION("Formula status") {
    for( const auto& form : formulas) {
      CHECK_FALSE(form->IsInvalid());
      CHECK_FALSE(form->IsArray());
      CHECK_FALSE(form->IsVarArray());
      CHECK(form->GetNdata() == 1);
    }
  }

//TODO registration in list of functions

  SECTION("Single Variable Arithmetic") {
    CHECK_THAT(fi->Eval(),  Catch::Matchers::WithinRel(2. * i));
    CHECK_THAT(fu->Eval(),  Catch::Matchers::WithinRel(-11. * u));
    CHECK_THAT(ff->Eval(),  Catch::Matchers::WithinRel(2.5F * f));
    CHECK_THAT(fd1->Eval(), Catch::Matchers::WithinRel(-22 + d1));
    CHECK_THAT(fd2->Eval(), Catch::Matchers::WithinRel(d2 / 5));
    // Test repeatability
    CHECK_THAT(fi->Eval(),  Catch::Matchers::WithinRel(2. * i));
    CHECK_THAT(fu->Eval(),  Catch::Matchers::WithinRel(-11. * u));
    CHECK_THAT(ff->Eval(),  Catch::Matchers::WithinRel(2.5F * f));
    CHECK_THAT(fd1->Eval(), Catch::Matchers::WithinRel(-22 + d1));
    CHECK_THAT(fd2->Eval(), Catch::Matchers::WithinRel(d2 / 5));
    CHECK_THAT(fcs->Eval(), Catch::Matchers::WithinRel(1.0));
  }

  SECTION("Multiple Variable Arithmetic") {
    CHECK_THAT(f1->Eval(), Catch::Matchers::WithinRel(2. * i - 11. * u));
    CHECK_THAT(f2->Eval(), Catch::Matchers::WithinRel(i + d1));
    CHECK_THAT(f3->Eval(), Catch::Matchers::WithinAbs(3 * f - u + 16 + 11 * d2, 1e-6));
    CHECK_THAT(f4->Eval(), Catch::Matchers::WithinRel(pow(2, i)));
    CHECK_THAT(f5->Eval(), Catch::Matchers::WithinRel(sqrt(i)));
    CHECK_THAT(f6->Eval(), Catch::Matchers::WithinRel(cos(d1) - sin(d2)));
  }

  SECTION("Instance range check") {
    for( const auto& form : formulas) {
      CHECK_THAT(form->EvalInstance(0),    Catch::Matchers::WithinAbs(0.0, 150.0));
      CHECK_THAT(form->EvalInstance(1),    Catch::Matchers::WithinRel(kBig));
      CHECK_THAT(form->EvalInstance(2222), Catch::Matchers::WithinRel(kBig));
      CHECK_THAT(form->EvalInstance(-1),   Catch::Matchers::WithinRel(kBig));
    }
  }

  SECTION("Changing Values") {
    d1 = GENERATE(5.6, 9.3, 11.7, -110.4222, 98327.22);
    CHECK_THAT(fd1->Eval(), Catch::Matchers::WithinRel(-22 + d1));
    CHECK_THAT(fcs->Eval(), Catch::Matchers::WithinRel(1.0));
    CHECK_THAT(f6->Eval(),  Catch::Matchers::WithinRel(cos(d1) - sin(d2)));
  }

  //TODO formulas of formulas

}

TEST_CASE("Formulas of Global Vector Variables", "[Formula]") // NOLINT(*-function-cognitive-complexity)
{
  // Use private variable list
  auto vars = make_shared<THaVarList>();

  // Some arbitrary data to work with
  Int_t aval[] = {2, 5, -3, 24, 42 };
  // Can't currently define variables of stack arrays ...
  auto* ai = new Int_t[5];
  memcpy(ai, aval, 5*sizeof(*ai));
  vector<Int_t> vi = {7, 8, -11, -22, 43, 19 };

  // THaVarList::Define currently does not work with heap arrays ...
  THaVar* var = vars->DefineByType("ai[5]", "Array of int", &ai, kIntP, nullptr);
  // TODO: put these checks in test case for non-RTTI array variables
  REQUIRE(var != nullptr);
  REQUIRE_FALSE(var->IsZombie());
  REQUIRE(var->GetLen() == 5);
  REQUIRE(vars->Find("ai") == var);
  CHECK(var->IsArray());
  CHECK_FALSE(var->IsVarArray());
  for( size_t i = 0; i < var->GetLen(); ++i )
    CHECK(var->GetValueInt(i) == ai[i]);

  var = vars->Define("vi", "vector<int>", vi);
  REQUIRE(var != nullptr);
  REQUIRE_FALSE(var->IsZombie());
  REQUIRE(var->GetLen() == vi.size());
  REQUIRE(vars->Find("vi") == var);
  CHECK(var->IsArray());
  CHECK(var->IsVarArray());
  for( size_t i = 0; i < var->GetLen(); ++i )
    CHECK(var->GetValueInt(i) == vi[i]);

  auto form  = make_shared<THaFormula>("form",  "7*ai-3", false, vars.get());
  REQUIRE(form != nullptr);
  REQUIRE_FALSE(form->IsZombie());
  REQUIRE_FALSE(form->IsError());
  CHECK(form->GetNdata() == 5);
  CHECK(form->IsArray());
  CHECK_FALSE(form->IsVarArray());
  for( size_t i = 0; i < form->GetNdata(); ++i )
    CHECK(form->EvalInstance(i) == 7 * ai[i] - 3);

  // Changing values
  Int_t ival[] = { 44, 55, -66, -77, 8088};
  memcpy(ai, ival, 5*sizeof(*ai));
  for( size_t i = 0; i < form->GetNdata(); ++i )
    CHECK(form->EvalInstance(i) == 7 * ival[i] - 3);

  auto form2 = make_shared<THaFormula>("form2", "vi**2+5", false, vars.get());
  REQUIRE(form2 != nullptr);
  REQUIRE_FALSE(form2->IsZombie());
  REQUIRE_FALSE(form2->IsError());
  CHECK(form2->GetNdata() == vi.size());
  CHECK(form2->IsArray());
  CHECK(form2->IsVarArray());
  for( size_t i = 0; i < form2->GetNdata(); ++i )
    CHECK(form2->EvalInstance(i) == vi[i] * vi[i] + 5);

  delete [] ai;
}
