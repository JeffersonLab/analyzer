//*-- Author :    Ole Hansen   25-Feb-26
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CrateMap_t                                                                //
//                                                                           //
// Test Decoder::THaCrateMap                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
# include <catch2/catch_test_macros.hpp>
// # include <catch2/generators/catch_generators.hpp>
// # include <catch2/matchers/catch_matchers_floating_point.hpp>
#else
# include <catch2/catch.hpp>
#endif

#include "THaCrateMap.h"      // for THaCrateMap, Rtypes
#include <ctime>              // for mktime, time_t, tm
#include <iostream>           // for ostream, cout, streambuf
#include <memory>             // for unique_ptr, make_unique
#include <set>                // for set
#include <string>             // for string
#include <string_view>        // for string_view
#include <vector>             // for vector

//#include "TError.h"           // for gErrorIgnoreLevel, kBreak

namespace {
struct cout_redirect { // NOLINT(*-special-member-functions)
  explicit cout_redirect( std::streambuf* newbuf )
    : savebuf_{std::cout.rdbuf(newbuf)} {}
  ~cout_redirect() { std::cout.rdbuf(savebuf_); }
private:
  std::streambuf* savebuf_;
};

time_t mktloc( int year, int month, int day, bool dst = false )
{
  tm tval{
    .tm_mday = day, .tm_mon = month - 1 /* sic */,
    .tm_year = year - 1900, .tm_isdst = dst
  };
  return mktime(&tval);
}
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace std::string_literals;
using namespace Decoder;

// Test the decoder crate map class
TEST_CASE("Crate Map", "[Database]") // NOLINT(*-function-cognitive-complexity)
{
  auto crmap = make_unique<THaCrateMap>("testmap");
  crmap->setDebug(THaCrateMap::kAllowMissingModel);

  SECTION("Initial setup")
  {
    CHECK(crmap->GetName() == "testmap"sv);
    CHECK(crmap->getTSROC() == THaCrateMap::DEFAULT_TSROC);
    CHECK_FALSE(crmap->isInit());
    CHECK(crmap->GetSize() == 0);
  }

  SECTION("Initialization from string")
  {
    string testmap = R"TESTMAP(
# comment
! Old-style comment
TSROC 16

config 250  cfg:fw=1
config 1881 cfg: debug

-------[ 2020-01-01 00:00:00 ] # Test time stamp

== Crate 1 type fastbus
# slot model clear header mask nchan ndata
6 1877 1  0xf0  0xc000  96 672
7 1877  1 0xe0  0x0a    96 672

   16 1881 1 0x0 0x0 64 64 cfg: +   highres
14 1881 1 0x0 0x0 64 64
15 1881 1 0x0 0x0 64 64 cfg: highres
)TESTMAP";
    //TODO VME modules, scalers

    auto st = crmap->init(testmap);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == 0);
    CHECK(crmap->GetName() == "testmap"sv);

    CHECK(crmap->GetSize() == 5);
    CHECK(crmap->GetNcrates() == 1);
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
    CHECK(crmap->getMask(1,16) == 0);
    CHECK(crmap->getConfigStr(1,6).empty());
    CHECK(crmap->getConfigStr(1,14) == "debug");
    CHECK(crmap->getConfigStr(1,15) == "highres");
    CHECK(crmap->getConfigStr(1,16) == "debug highres");

    auto used_crates = crmap->GetUsedCrates();
    CHECK(used_crates.size() == 1);
    CHECK(used_crates == set<UInt_t>{1});

    auto used_slots = crmap->GetUsedSlots(1);
    CHECK(used_slots.size() == 5);
    CHECK(used_slots == set<UInt_t>{6,7,14,15,16});

    crmap->print();

    crmap->Clear();
    CHECK_FALSE(crmap->isInit());
    CHECK(crmap->GetSize() == 0);
    CHECK(crmap->getTSROC() == THaCrateMap::DEFAULT_TSROC);
  }

  //TODO init from file
  SECTION("Initialization from file")
  {}

  SECTION("Initialization from file with timestamps")
  {
    // Open tests/DB/db_testmap.dat and extract info for the given date

    //=== Part 1: Date = 01-Jan-2020 00:00 EST -> nothing defined
    time_t tloc = mktloc(2020, 1, 1);
    auto st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_ERR);
    //TODO check error message

    //=== Part 2: Date = 0 (1-Jan-1970 UTC)
    //  -> everything defined (timestamps disabled)
    st = crmap->init(0);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetNcrates() == 32); // 32 crates (some defined and undefined)
    CHECK(crmap->GetSize() == 231);   // Big experiment

    //=== Part 3: Date = 01-Feb-2021 00:00 EST
    //  -> 14 crates, defined in first time-stamped block
    tloc = mktloc(2021, 2, 1);
    st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == tloc);
    CHECK(crmap->getTSROC() == 21);
    // Overall
    crmap->print();
    CHECK(crmap->GetNcrates() == 14);  // 14 crates
    CHECK(crmap->GetSize() == 73);     // 73 slots
    CHECK(crmap->GetUsedCrates() == set<UInt_t>{1,4,5,6,7,10,16,17,19,20,22,23,24,25});
    vector<UInt_t> nslots; nslots.reserve(crmap->GetNcrates());
    for( const auto& cr: crmap->GetUsedCrates() )
      nslots.push_back(crmap->getNslot(cr));
    CHECK(nslots == vector<UInt_t>{1,4,9,16,10,1,18,8,1,1,1,1,1,1});

    // Crate 1
    UInt_t crate = 1, cr1slot = 20;
    CHECK_FALSE(crmap->isFastBus(crate));
    CHECK(crmap->isVme(crate));
    CHECK_FALSE(crmap->isCamac(crate));
    CHECK_FALSE(crmap->isScalerCrate(crate));
    CHECK(crmap->isBankStructure(crate));
    CHECK(crmap->isAllBanks(crate));
    CHECK(crmap->getNslot(crate) == 1);

    CHECK(crmap->getModel(crate, cr1slot) == 250);
    CHECK(crmap->getNchan(crate, cr1slot) == 16);
    CHECK(crmap->getMask(crate, cr1slot) == -1);     // 0xFF FF FF FF
    CHECK(crmap->getConfigStr(crate, cr1slot).empty());

    // Crate 7
    crate = 7;
    CHECK(crmap->isVme(crate));
    CHECK(crmap->isBankStructure(crate));
    CHECK_FALSE(crmap->isAllBanks(crate));

    // Crate 17
    crate = 17;
    CHECK_FALSE(crmap->slotUsed(crate,1));      // not defined
    CHECK(crmap->getModel(crate, 1) == 0);      // not defined
    CHECK(crmap->getBank(crate, 1) == -1);      // not defined
    CHECK(crmap->slotUsed(crate,4));
    CHECK(crmap->getModel(crate, 4) == 6401);
    CHECK(crmap->getBank(crate, 6) == 7);
    CHECK(crmap->getConfigStr(crate, 5) == "suppress_hitFIFOwarn=100");
    CHECK(crmap->slotClear(crate,18));          // n/a -> default value

    //....

    //=== Part 4: Date = 01-May-2024 00:00 EDT
    //  -> Add crates 28 & 29, redefine crate 7, remove 10
    tloc = mktloc(2024, 5, 1, true);
    st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == tloc);
    CHECK(crmap->getTSROC() == 21);

    //crmap->print();
    CHECK(crmap->GetNcrates() == 15);
    CHECK(crmap->GetSize() == 73+27-1-10+7);
    CHECK(crmap->GetUsedCrates() == set<UInt_t>{1,4,5,6,7,16,17,19,20,22,23,24,25,28,29});
    nslots.clear();
    for( const auto& cr: crmap->GetUsedCrates() )
      nslots.push_back(crmap->getNslot(cr));
    CHECK(nslots == vector<UInt_t>{1,4,9,16,7,18,8,1,1,1,1,1,1,19,8});

    //
    // Date = 01-Aug-2025 00:00 EDT
    //TODO
  }

}

