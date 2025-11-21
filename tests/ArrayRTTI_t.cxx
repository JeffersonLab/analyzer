//*-- Author :    Ole Hansen   03-Nov-23
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ArrayRTTI_t                                                               //
//                                                                           //
// Test global variables defined on array-type class member variables        //
//                                                                           //
// At this time, the following are supported:                                //
//                                                                           //
// - fixed-size C-arrays of arbitrary dimensionality                         //
// - variable-size 1D C-arrays with separate size variable (a ROOT extension)//
// - std::vector                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
# include <catch2/generators/catch_generators.hpp>
# include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "ArrayRTTI.h"
#include "THaVarList.h"
#include "THaGlobals.h"   // gHaVars

using namespace std;

namespace {

//_____________________________________________________________________________
// Helper class for cycling through RVarDef structs using Catch's GENERATE()
class RVarDefIterator : public Catch::Generators::IGenerator<RVarDef> {
  const RVarDef* defs_;
  static inline RVarDefIterator* me_;
public:
  explicit RVarDefIterator( const RVarDef* defs ) : defs_(defs) {
    me_ = this;
  }

  RVarDef const& get() const override {
    return *defs_;
  }
  bool next() override {
    if( !defs_ )
      return false;
    return ((++defs_)->name != nullptr);
  }
  static size_t index() {
    // IGenerator does keep track of the element index, but the value is
    // hard to access. This won't work with nested GENERATE()s!
    return me_ ? me_->currentElementIndex() : -1;
  };
};

auto vardefs( const RVarDef* defs ){
  return Catch::Generators::GeneratorWrapper<RVarDef>(
    Catch::Detail::make_unique<RVarDefIterator>(defs)
  );
}

} // end private namespaces

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("Global Variables of Arrays", "[ArrayRTTI]") // NOLINT(*-function-cognitive-complexity)
{
  using namespace Podd::Tests;
  VarType kDataType  = std::is_same_v<Data_t, Float_t> ? kFloat  : kDouble;
  VarType kDataTypeV = std::is_same_v<Data_t, Float_t> ? kFloatV : kDoubleV;
  VarType kDataTypeP = std::is_same_v<Data_t, Float_t> ? kFloatP : kDoubleP;

  const TString obj_name = "array_rtti";
  const TString obj_prefix = obj_name + ".";

  ArrayRTTI TestObj(obj_name);

  auto err = TestObj.Init();
  TString prefix(TestObj.GetPrefix());

  SECTION("Test Class") {
    SECTION("Construction") {
      REQUIRE_FALSE(TestObj.IsZombie());
    }

    SECTION("Initialization") {
      CHECK(err == THaAnalysisObject::kOK);
      CHECK(TestObj.IsInit());
    }

    SECTION("Object name") {
      CHECK(TestObj.GetName() == obj_name);
      CHECK(prefix == obj_prefix);
    }

    SECTION("Successful setup of variables") {
      // This also tests the teardown of the variables when TestObj goes out of scope
      REQUIRE(gHaVars->GetSize() == ArrayRTTI::GetNvars());
    }
  }

  SECTION("Properties of Variables") {
    auto item = GENERATE(vardefs(ArrayRTTI::GetRVarDef()));
    TString name = item.name;
    TString varname = prefix + name;
    const auto* var = gHaVars->Find(varname);
    REQUIRE(var);

    auto i = RVarDefIterator::index();           // index of current variable

    SECTION("Scaler or array") {
      switch( i ) {
        case 7: // vecarr
          CHECK(var->IsVector());                // std::vector
          [[fallthrough]];
        case 6: // vararr
          CHECK(var->IsVarArray());              // variable array
          [[fallthrough]];
        case 0: // oned
        case 3: // twod
        {
          CHECK(var->IsArray());                 // any array
          CHECK_FALSE(var->IsPointerArray());    // not array of pointers
          break;
        }
        case 1: // elem0
        case 2: // elem1
        case 4: // elem02
        case 5: // elem30
          CHECK_FALSE(var->IsArray());
          break;
        default:
          FAIL("Undefined variable index");
          break;
      }
    }

    SECTION("Variable type") {
      switch( i ) {
        case 0: // oned
        case 1: // elem0
        case 2: // elem1
        case 3: // twod
        case 4: // elem02
        case 5: // elem30
          CHECK(var->GetType() == kDataType);    // kDouble or kFloat
          break;
        case 6: // vararr
          CHECK(var->GetType() == kDataTypeP);   // pointer to kDouble/kFloat
          break;
        case 7: // vecarr
          CHECK(var->GetType() == kDataTypeV);   // std::vector of kDouble/kFloat
          break;
        default:
          FAIL("Undefined variable index");
          break;
      }
    }

    SECTION("Dimensions and size") {
      auto sz = var->GetLen();         // Current total number of elements
      auto ndim = var->GetNdim();      // Number of dimensions (0 = scalar)
      const auto* dim = var->GetDim(); // Array of dimensions
      switch( i ) {
        case 0: // oned
          CHECK(ndim == 1);
          CHECK(sz == ArrayRTTI::k1Dsize);
          REQUIRE(dim);
          CHECK(dim[0] == ArrayRTTI::k1Dsize);
          break;
        case 1: // elem0
        case 2: // elem1
        case 4: // elem02
        case 5: // elem30
          CHECK(ndim == 0);
          CHECK(sz == 1);
          CHECK_FALSE(dim);
          break;
        case 3: // twod
          CHECK(ndim == 2);
          CHECK(sz == ArrayRTTI::k2Dcols * ArrayRTTI::k2Drows);
          REQUIRE(dim);
          CHECK(dim[0] == ArrayRTTI::k2Dcols);
          CHECK(dim[1] == ArrayRTTI::k2Drows);
          break;
        case 6: // vararr
          CHECK(ndim == 1);
          CHECK(sz == ArrayRTTI::kVarSize);
          CHECK(TestObj.fN == sz);
          REQUIRE(dim);
          CHECK(dim[0] == ArrayRTTI::kVarSize);
          break;
        case 7: // vecarr
          CHECK(ndim == 1);
          CHECK(sz == ArrayRTTI::kVecSize);
          REQUIRE(dim);
          CHECK(dim[0] == ArrayRTTI::kVecSize);
          break;
        default:
          FAIL("Undefined variable index");
          break;
      }
    }

    SECTION("Data contents") {
      auto sz = var->GetLen();
      auto val0 = var->GetValue();
      const auto* dim = var->GetDim();
      switch( i ) {
        case 0: // oned
          for( Int_t k = 0; k < sz; ++k ) {
            Data_t expect = 0.25 + k;
            Data_t value = var->GetValue(k);
            CHECK_THAT(value, Catch::Matchers::WithinRel(expect));
          }
          break;
        case 1: // elem0
          CHECK_THAT(val0, Catch::Matchers::WithinRel(0.25));
          break;
        case 2: // elem1
          CHECK_THAT(val0, Catch::Matchers::WithinRel(1.25));
          break;
        case 3: // twod
        {
          Int_t idx = 0;
          for( Int_t k = 0; k < dim[0]; ++k ) {
            for( Int_t l = 0; l < dim[1]; ++l ) {
              Data_t expect = 0.33 + ArrayRTTI::k2Drows * k + l;
              Data_t value = var->GetValue(idx++);
              CHECK_THAT(value, Catch::Matchers::WithinRel(expect));
            }
          }
          break;
        }
        case 4: // elem02
        {
          Data_t expect = 0.33 + ArrayRTTI::k2Drows * 0 + 2;
          REQUIRE_THAT(val0, Catch::Matchers::WithinRel(expect));
          break;
        }
        case 5: // elem30
        {
          Data_t expect = 0.33 + ArrayRTTI::k2Drows * 3 + 0;
          REQUIRE_THAT(val0, Catch::Matchers::WithinRel(expect));
          break;
        }
        case 6: // vararr
          for( Int_t k = 0; k < sz; ++k ) {
            Data_t expect = 0.75 + k;
            Data_t value = var->GetValue(k);
            REQUIRE_THAT(value, Catch::Matchers::WithinRel(expect));
          }
          break;
        case 7: // vecarr
          for( Int_t k = 0; k < sz; ++k ) {
            Data_t expect = -11.652 + 2*k;
            Data_t value = var->GetValue(k);
            REQUIRE_THAT(value, Catch::Matchers::WithinRel(expect));
          }
          break;
        default:
          FAIL("Undefined variable index");
          break;
      }
    }
  }

  SECTION("Resizing arrays") {
    constexpr auto delta = 3;
    SECTION("C-array with separate size variable") {
      const auto* var = gHaVars->Find(prefix + "vararr");
      TestObj.fN -= delta;
      CHECK(var->GetLen() == TestObj.fN);
      TestObj.fN += delta;
    }

    SECTION("std::vector array") {
      const auto* var = gHaVars->Find(prefix + "vecarr");
      std::vector<Data_t>& vec = TestObj.fVectorD;
      vec.resize(vec.size() + delta);
      CHECK(var->GetLen() == ArrayRTTI::kVecSize + delta);
      vec.resize(vec.size() - delta);
    }
  }
}

