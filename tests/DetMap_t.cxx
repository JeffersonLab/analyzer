//*-- Author :    Ole Hansen   19-Mar-2026
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// DetMap_t                                                                  //
//                                                                           //
// Test THaDetMap                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "THaDetMap.h"          // for THaDetMap
#include "DetMapFromDB.h"       // for DetMapFromDB
#include "Decoder.h"            // for EModuleType, Rtypes
#include "THaAnalysisObject.h"  // for THaAnalysisObject
#include <iostream>             // for cout, streambuf
#include <sstream>              // for ostringstream, basic_stringbuf
#include <string>               // for char_traits, allocator, operator==
#include <string_view>          // for string_view, operator==, operator""sv
#include <utility>              // for cmp_equal, cmp_greater_equal
#include <vector>               // for vector

using namespace std;

namespace {
struct cout_redirect { // NOLINT(*-special-member-functions)
  explicit cout_redirect( std::streambuf* newbuf )
    : savebuf_{std::cout.rdbuf(newbuf)} {}
  ~cout_redirect() { std::cout.rdbuf(savebuf_); }
private:
  std::streambuf* savebuf_;
};
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("Legacy Detector Map", "[Detectors]") // NOLINT(*-function-cognitive-complexity)
{
  THaDetMap m;

  vector<Int_t> dmap = {
    6, 2, 0, 31, 1881,
    6, 3, 0, 31, 1881,
  };
  Int_t ret = m.Fill(dmap, THaDetMap::kFillModel);
  REQUIRE(ret == 2);
  CHECK(m.GetSize() == 2);
  CHECK(m.GetTotNumChan() == 64);
  CHECK(m.IsADC(0U));
  CHECK_FALSE(m.IsTDC(0U));

  THaDetMap::Module* d = m.GetModule(0);
  REQUIRE(d != nullptr);
  CHECK(d->crate == 6);
  CHECK(d->slot == 2);
  CHECK(d->lo == 0);
  CHECK(d->hi == 31);
  CHECK(d->first == 1);   // logical channels count from 1 by default
  CHECK(d->GetModel() == 1881);
  CHECK(d->type == Decoder::EModuleType::kADC);
  CHECK(d->IsADC());
  CHECK_FALSE(d->IsTDC());
  CHECK(THaDetMap::IsADC(d));
  CHECK_FALSE(THaDetMap::IsTDC(d));
  CHECK(THaDetMap::GetModel(d) == 1881);

  dmap = {
// cr  sl  lo  hi  log  ref sig
    8, 10, 31, 16,  12,  18,  2,
    8, 11, 0,  7,   32,  22,  3,
  };
  ret = m.Fill(dmap, THaDetMap::kFillLogicalChannel |
    THaDetMap::kFillRefChan | THaDetMap::kFillSignal);
  REQUIRE(ret == 2);
  CHECK(m.GetSize() == 2);
  CHECK(m.GetTotNumChan() == 24);

  // GetMinMaxChan is currently not used anywhere in the analyzer
  Int_t ch_lo, ch_hi;
  m.GetMinMaxChan(ch_lo, ch_hi);
  CHECK(ch_lo == 12);
  CHECK(ch_hi == 39); // but there is a gap ..., so
  CHECK_FALSE(std::cmp_equal(ch_hi - ch_lo + 1, m.GetTotNumChan()));
 // But this should always hold
 CHECK(std::cmp_greater_equal(ch_hi - ch_lo + 1, m.GetTotNumChan()));

  // Range of valid refindex values, if any
  m.GetMinMaxChan(ch_lo, ch_hi, THaDetMap::kRefIndex);
  CHECK(ch_lo == -1);
  CHECK(ch_hi == -1);  // if max < 0, all invalid

  d = m.GetModule(0);
  REQUIRE(d != nullptr);
  CHECK(d->crate == 8);
  CHECK(d->slot == 10);
  CHECK(d->lo == 16);
  CHECK(d->hi == 31);
  CHECK(d->first == 12);
  CHECK(d->model == 0);
  CHECK(d->refchan == 18);
  CHECK(d->refindex == -1);
  CHECK(d->plane == 0);
  CHECK(d->signal == 2);
  CHECK(d->reverse == true);
  CHECK(d->cmnstart == false);
  CHECK(d->tag == ""sv);
  CHECK(d->GetNchan() == 16);
  CHECK_FALSE(d->IsADC());
  CHECK_FALSE(d->IsTDC());
  CHECK_FALSE(d->IsCommonStart());
  CHECK(d->type == Decoder::EModuleType::kUndefined);

  d = m.GetModule(1);
  REQUIRE(d != nullptr);
  CHECK(d->crate == 8);
  CHECK(d->slot == 11);
  CHECK(d->lo == 0);
  CHECK(d->hi == 7);
  CHECK(d->first == 32);
  CHECK(d->model == 0);
  CHECK(d->refchan == 22);
  CHECK(d->refindex == -1);
  CHECK(d->plane == 0);
  CHECK(d->signal == 3);
  CHECK(d->reverse == false);
  CHECK(d->cmnstart == false);
  CHECK(d->tag == ""sv);
  CHECK(d->GetNchan() == 8);
  CHECK_FALSE(d->IsADC());
  CHECK_FALSE(d->IsTDC());
  CHECK(d->type == Decoder::EModuleType::kUndefined);

  CHECK(m.GetModule(2) == nullptr);
}

namespace {
void CheckRefMap( THaDetMap& m, bool legacy = false) // NOLINT(*-function-cognitive-complexity)
{
  CHECK(m.GetTotNumChan() == 64);

  UInt_t crate = 6 + 2*legacy;
  THaDetMap::Module* d = m.GetModule(0);
  REQUIRE(d != nullptr);
  CHECK(d->crate == crate);
  CHECK(d->slot == 2);
  CHECK(d->lo == 0);
  CHECK(d->hi == 31);
  CHECK(d->first == 0);
  CHECK(d->model == 1881);
  CHECK(d->refchan == -1);
  CHECK(d->refindex == 1);
  CHECK(d->plane == 0);
  CHECK(d->signal == 0);
  if( !legacy ) {
    CHECK(d->tag == "LS"sv);
  }

  d = m.GetModule(1);
  REQUIRE(d != nullptr);
  CHECK(d->crate == crate);
  CHECK(d->slot == 12);
  CHECK(d->lo == 0);
  CHECK(d->hi == 15);
  CHECK(d->first == 64);
  CHECK(d->model == 1877);
  CHECK(d->refchan == -1);
  CHECK(d->refindex == 2);
  CHECK(d->plane == 0);
  CHECK(d->signal == 0);
  if( !legacy ) {
    CHECK(d->tag == "v195"sv);   // sic. Truncated at 4 chars.
  }

  d = m.GetModule(2);
  REQUIRE(d != nullptr);
  CHECK(d->crate == crate);
  CHECK(d->slot == 13);
  CHECK(d->lo == 0);
  CHECK(d->hi == 15);
  CHECK(d->first == 80);
  CHECK(d->model == 250);
  CHECK(d->refchan == 7);
  CHECK(d->refindex == -1);
  CHECK(d->plane == 0);
  CHECK(d->signal == 0);
  CHECK(d->tag == ""sv);
  CHECK(d->reverse == false);
  CHECK(d->cmnstart == false);

  CHECK(m.GetModule(3) == nullptr);

  d = m.Find(crate,13,6);
  CHECK(d == m.GetModule(2));
  d = m.Find(crate,13,24);
  CHECK(d == nullptr);

  Int_t ch_lo, ch_hi;
  m.GetMinMaxChan(ch_lo, ch_hi);
  CHECK(ch_lo == 0);
  CHECK(ch_hi == 95);
  CHECK_FALSE(std::cmp_equal(ch_hi - ch_lo + 1, m.GetTotNumChan()));
  CHECK(std::cmp_greater_equal(ch_hi - ch_lo + 1, m.GetTotNumChan()));

  m.GetMinMaxChan(ch_lo, ch_hi, THaDetMap::kRefIndex);
  CHECK(ch_lo == 1);
  CHECK(ch_hi == 2);
}
} // namespace

TEST_CASE("Detector Map Parser v2", "[Detectors]") // NOLINT(*-function-cognitive-complexity)
{
  THaDetMap m;

  const char* mapstr = "6 2 0 31 M:1881, 6 3 0 31";
  Int_t ret = m.Fill(mapstr);
  REQUIRE(ret == 2);
  CHECK(m.GetSize() == 2);
  CHECK(m.GetNchan(0) == 32);
  CHECK(m.GetNchan(10) == 0);

  THaDetMap::Module* d = m.GetModule(0);
  CHECK(d->crate == 6);
  CHECK(d->slot == 2);
  CHECK(d->lo == 0);
  CHECK(d->hi == 31);
  CHECK(d->first == 1);
  CHECK(d->GetModel() == 1881);
  CHECK(d->refchan == -1);
  CHECK(d->refindex == -1);
  CHECK(d->plane == 0);
  CHECK(d->signal == 0);
  CHECK(d->tag == ""sv);
  CHECK(d->reverse == false);
  CHECK(d->cmnstart == false);

  {
    const char* printstr = R"OUT(THaDetMap: size = 2
 Crate  Slot Start   End  Logi  Model  Type  Flg RefCh RefIx Plane  Sgnl
     6     2     0    31     1   1881   ADC         -1    -1     0     0
     6     3     0    31    33      0   ???         -1    -1     0     0
)OUT";
    ostringstream ss;
    cout_redirect outp(ss.rdbuf());
    m.Print();
    CHECK(ss.str() == printstr);
  }

  mapstr =
    //tag   cr sl lo hi log  model  refidx
    "  LS    6  2  0 31   0 M:1881     I:1,"
    "  v1955 6 12  0 15  64 M:1877     I:2,"
    "        6 13  0 15     M:250      R:7,";
  ret = m.Fill(mapstr);
  REQUIRE(ret == 3);
  CheckRefMap(m);

  {
    const char* printstr = R"OUT(THaDetMap: size = 3
 Crate  Slot Start   End  Logi  Model  Type  Flg RefCh RefIx Plane  Sgnl   Tag
     6     2     0    31     0   1881   ADC         -1     1     0     0    LS
     6    12     0    15    64   1877   TDC         -1     2     0     0  v195
     6    13     0    15    80    250  SMPL          7    -1     0     0
)OUT";
    ostringstream ss;
    cout_redirect outp(ss.rdbuf());
    m.Print();
    CHECK(ss.str() == printstr);
  }
}

TEST_CASE("Detector map from database", "[Detectors]") // NOLINT(*-function-cognitive-complexity)
{
  SECTION("Legacy (v1) map")
  {
    // Tests v1 map in database file db_DM1.dat (identical to last map above)
    auto* d = new Podd::Tests::DetMapFromDB("DM1");
    REQUIRE(d);
    Int_t st = d->Init();
    REQUIRE(st == THaAnalysisObject::EStatus::kOK);
    THaDetMap* m = d->GetDetMap();
    REQUIRE(m);
    CheckRefMap(*m, true);
    CHECK(d->GetNumber() == 42);
    delete d;
  }

  SECTION("Modernized (v2) map")
  {
    // Tests v2 map in db_DM2.dat
    auto* d = new Podd::Tests::DetMapFromDB("DM2");
    Int_t st = d->Init();
    REQUIRE(st == THaAnalysisObject::EStatus::kOK);
    THaDetMap* m = d->GetDetMap();
    REQUIRE(m);
    CheckRefMap(*m);
    CHECK(d->GetNumber() == 142);
    delete d;
  }
}
