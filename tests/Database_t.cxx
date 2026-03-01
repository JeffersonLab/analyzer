//*-- Author :    Ole Hansen   06-Feb-26
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Database_t                                                                //
//                                                                           //
// Test Podd::Database                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
// # include <catch2/generators/catch_generators.hpp>
// # include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "Database.h"                    // for LoadDatabase, OpenDBFile
#include "PoddTests.h"                   // for PODD_TESTS_DBDIR
#include "TDatime.h"                     // for TDatime
#include "TError.h"                      // for gErrorIgnoreLevel, kBreak
#include <algorithm>                     // for __all_of, all_of
#include <cstdio>                        // for fclose, FILE
#include <cstring>                       // for memset
#include <string>                        // for allocator, basic_string, ope...
#include <vector>                        // for vector, operator==
#include <ctime>

using namespace std;
using namespace std::string_literals;

// #include <sstream>
//
// namespace {
// struct cout_redirect { // NOLINT(*-special-member-functions)
//   explicit cout_redirect( std::streambuf* newbuf )
//     : savebuf_{std::cout.rdbuf(newbuf)} {}
//   ~cout_redirect() { std::cout.rdbuf(savebuf_); }
// private:
//   std::streambuf* savebuf_;
// };
// }

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("Database functions", "[Database]") // NOLINT(*-function-cognitive-complexity)
{
  TDatime now;
  string openpath;
  const char* prefix = "R.s1.";
  FILE* f = Podd::OpenDBFile(prefix, now, "Database tests",
                             "r", 0, openpath );
  gErrorIgnoreLevel = kBreak;  // Suppress nuisance error message

  SECTION("OpenDBFile")
  {
    const char* filepath = PODD_TESTS_DBDIR "/db_R.s1.dat";
    REQUIRE(f != nullptr);
    REQUIRE(openpath == filepath);
    int status = fclose(f);
    CHECK(status == 0);

    f = Podd::OpenDBFile("R.s1", now, "Database tests",
                         "r", 0, openpath);
    REQUIRE(f != nullptr);
    REQUIRE(openpath == filepath);
    (void)fclose(f);

    f = Podd::OpenDBFile("no such file", now, "Database tests");
    REQUIRE(f == nullptr);
  }

  SECTION("LoadDBvalue")
  {
    Double_t Cn;
    UInt_t npaddles;
    auto status = Podd::LoadDBvalue(f, now, "R.s1.Cn", Cn);
    CHECK(status == 0);
    CHECK(Cn == 1.49859e+08);
    status = Podd::LoadDBvalue(f, now, "R.s1.npaddles", npaddles);
    CHECK(status == 0);
    CHECK(npaddles == 6);
    (void)fclose(f);
  }

  SECTION("LoadDBarray")
  {
    vector<Int_t> ivec, ivec_ref{502, 476, 547, 499, 656, 678};
    vector<Double_t> vec, vec_ref{0.88, 0.18, 0.005};
    auto status = Podd::LoadDBarray(f, now, "R.s1.L.ped", ivec);
    CHECK(status == 0);
    CHECK(ivec == ivec_ref);
    status = Podd::LoadDBarray(f, now, "R.s1.size", vec);
    CHECK(status == 0);
    CHECK(vec == vec_ref);
    (void)fclose(f);
  }

  SECTION("LoadDatabase")
  {
    // Successful requests
    Int_t intval = 0, glval = 0;
    Double_t atten{-10.}, avgres{-20.}, tdcres{-30.}, nosuch{-40.};
    vector<Double_t> Lgains, Rgains;
    const vector<Double_t> Lgains_ref{0.44, 0.423, 0.44, 0.507, 0.43, 0.447},
                           Rgains_ref{0.443, 0.396, 0.452, 0.502, 0.512, 0.383};
    vector<Int_t> intvec;
    const vector<Int_t> detmap_ref{1,21,38,43,1,1881, 1,21,32,37,7,1881,
                                   1,23,40,45,1,1877, 1,23,32,37,7,1877};
    Double_t szarr[3], param1{0}, param2{0};
    const vector<Double_t> size_ref{0.88, 0.18, 0.005};
    string strval;
    vector<DBRequest> requests = {
      {.name = "npaddles", .var = &intval, .type = kInt},
      {.name = "detmap",   .var = &intvec, .type = kIntV},
      {.name = "atten",    .var = &atten,  .type = kDouble},
      {.name = "nosuch",   .var = &nosuch, .optional = true},
      {.name = "avgres",   .var = &avgres},
      {.name = "tdc.res",  .var = &tdcres},
      {.name = "L.gain",   .var = &Lgains, .type = kDoubleV, .nelem = 6},
      {.name = "size",     .var = szarr,   .nelem = 3},
      {.name = "strval",   .var = &strval, .type = kString},
      {.name = "param1",   .var = &param1, .search = -1},
      {.name = "param2",   .var = &param2, .search = 2},
      {.name = "global",   .var = &glval,  .type = kInt,    .search = 1},
    };
    Int_t status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == 0);
    CHECK(intval == 6);
    CHECK(intvec == detmap_ref);
    CHECK(atten == 0.7);
    CHECK(avgres == 1e-10);
    CHECK(tdcres == 5e-10);
    CHECK(Lgains == Lgains_ref);
    CHECK(nosuch == -40.);
    vector<Double_t> size{szarr, szarr+3};
    CHECK(size == size_ref);
    CHECK(strval == "Some text  here");
    CHECK(param1 == 137.67);
    CHECK(param2 == 423.07);
    CHECK(glval == 4267);

    // Key alternatives
    intval = 0;
    requests = {
      {.name = "stdval,altval,extval", .var = &intval, .type = kInt},
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == 0);
    CHECK(intval == 1324);

    // Prefix without a trailing dot
    intvec.assign(4, -123); // arbitrary
    requests = {
      {.name = "retiming_offsets", .var = &intvec, .type = kIntV},
    };
    status = Podd::LoadDatabase(f, now, requests, "R.s1");
    CHECK(status == 0);
    CHECK(intvec.size() == 6);
    CHECK(std::ranges::all_of(intvec, [](Int_t i){return i == 0;}));

    // Key prefix
    vector<Double_t> gains;
    requests = {
      {.name = "gain", .var = &gains, .type = kDoubleV},
    };
    status = Podd::LoadDatabase(
      {.file = f, .date = now, .prefix = prefix, .key_prefix = "L."}, requests);
    CHECK(status == 0);
    CHECK(gains == Lgains_ref);

    status = Podd::LoadDatabase(
      {.file = f, .date = now, .prefix = prefix, .key_prefix = "R."}, requests);
    CHECK(status == 0);
    CHECK(gains == Rgains_ref);

    // Error: Key not found
    atten = -10., avgres = -20., nosuch = -40.;
    intvec.clear();
    requests = {
      {.name = "atten",  .var = &atten},
      {.name = "detmap", .var = &intvec, .type = kIntV},
      {.name = "nosuch", .var = &nosuch}, // error here (3rd entry)
      {.name = "avgres", .var = &avgres}
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == 3);
    CHECK(atten == 0.7);
    CHECK(intvec.size() == 24);
    CHECK(nosuch == -40.);
    CHECK(avgres == -20.);

    glval = 0;
    requests = {
      {.name = "global", .var = &glval, .type = kInt, .search = 2},
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == 1);
    CHECK(glval == 0);

    // Error: Array size mismatch
    memset(szarr, 0, 3U*sizeof(Double_t));
    requests = {
      {.name = "size", .var = szarr, .nelem = 4},
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == -130); // size mismatch
    CHECK((szarr[0] == 0 && szarr[1] == 0 && szarr[2] == 0));

    // Error: Key length too short
    requests = {
      {.name = "npaddle", .var = &intval, .type = kInt},
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == 1); // size mismatch

    // Error: Key length too long
    requests = {
      {.name = "npaddlesd", .var = &intval, .type = kInt},
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == 1); // size mismatch

    // Error: numerical conversion error
    Double_t dval{0};
    requests = {
      {.name = "converr", .var = &dval},
    };
    status = Podd::LoadDatabase(f, now, requests, prefix);
    CHECK(status == -131); // conversion error

    (void)fclose(f);
  }

}

// Test the decoder crate map class
// TODO Move this to a separate "Decoder" test implementation once available
#include "THaCrateMap.h"
#include <memory>

TEST_CASE("Crate Map", "[Database]") // NOLINT(*-function-cognitive-complexity)
{
  auto crmap = make_unique<Decoder::THaCrateMap>("testmap");

  SECTION("Initial setup")
  {
    CHECK(crmap->GetName() == "testmap"sv);
    CHECK(crmap->getTSROC() == Decoder::THaCrateMap::DEFAULT_TSROC);
    CHECK_FALSE(crmap->isInit());
    CHECK(crmap->GetSize() == 0);
  }

  SECTION("Initialization from string")
  {
    string testmap = R"TESTMAP(
# comment
! Old-style comment
TSROC 16

config 250 cfg:fw=1
config 1881 cfg: debug

-------[ 2020-01-01 00:00:00 ] # Test time stamp

== Crate 1 type fastbus
# slot model clear header mask nchan ndata
6 1877 1  0x0  0x0  96  672
7 1877  1 0x0 0x0 96 672

   16 1881 1 0x0 0x0 64 64 cfg: +   highres
14 1881 1 0x0 0x0 64 64
15 1881 1 0x0 0x0 64 64 cfg: highres
)TESTMAP";
    //TODO VME modules, scalers

    auto st = crmap->init(testmap);
    REQUIRE(st == Decoder::THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == 0);
    CHECK(crmap->GetName() == "testmap"sv);

    CHECK(crmap->GetSize() == 5);
    CHECK(crmap->getTSROC() == 16);
    CHECK(crmap->getNslot(1) == 5);
    CHECK(crmap->getMinSlot(1) == 6);
    CHECK(crmap->getMaxSlot(1) == 16);
    CHECK(crmap->slotUsed(1,14));
    CHECK_FALSE(crmap->slotUsed(1,1));
    CHECK_FALSE(crmap->slotUsed(2,6));
    CHECK(crmap->isFastBus(1) == true);
    CHECK(crmap->isVme(1) == false);
    CHECK(crmap->isCamac(1) == false);
    CHECK(crmap->isScalerCrate(1) == false);
    CHECK(crmap->isBankStructure(1) == false);
    CHECK(crmap->isAllBanks(1) == false);
    CHECK(crmap->getModel(1,6) == 1877);
    CHECK(crmap->getModel(1,7) == 1877);
    CHECK(crmap->getModel(1,16) == 1881);
    CHECK(crmap->getNchan(1,6) == 96);
    CHECK(crmap->getNdata(1,6) == 672);
    CHECK(crmap->getMask(1,16) == 0);
    CHECK(crmap->getConfigStr(1,6) == ""sv);
    CHECK(crmap->getConfigStr(1,14) == "debug"sv);
    CHECK(crmap->getConfigStr(1,15) == "highres"sv);
    CHECK(crmap->getConfigStr(1,16) == "debug highres"sv);

    auto used_crates = crmap->GetUsedCrates();
    CHECK(used_crates.size() == 1);
    CHECK(used_crates == vector<UInt_t>{1});

    auto used_slots = crmap->GetUsedSlots(1);
    CHECK(used_slots.size() == 5);
    CHECK(used_slots == vector<UInt_t>{6,7,14,15,16}); // expected sorted

    //TODO timestamps in string (should not have any effect)
    crmap->clear();
    CHECK_FALSE(crmap->isInit());
    CHECK(crmap->GetSize() == 0);
    CHECK(crmap->getTSROC() == Decoder::THaCrateMap::DEFAULT_TSROC);
  }
}

