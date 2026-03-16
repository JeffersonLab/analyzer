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
#include "PoddTests.h"        // for PODD_TESTS_DBDIR
#include "TError.h"           // for gErrorIgnoreLevel, kBreak
#include <cstdio>             // for fclose, fopen, FILE
#include <ctime>              // for time_t, timegm, tm
#include <memory>             // for unique_ptr, make_unique
#include <set>                // for set
#include <sstream>            // for ostringstream
#include <string>             // for string, string_literals
#include <string_view>        // for string_view
#include <vector>             // for vector

namespace {
time_t mktloc( int year, int month, int day )
{
  // Return Unix time value for midnight UTC at given date
  tm tval{
    .tm_mday = day, .tm_mon = month - 1 /* sic */,
    .tm_year = year - 1900, .tm_isdst = 0
  };
  return timegm(&tval);
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
  gErrorIgnoreLevel = kBreak;  // Suppress ROOT messages from Error/Warning/Info

  static const string testmap = R"TESTMAP(
# comment
! Old-style comment
TSROC 16

config 250  cfg:fw=1
config 1881 cfg: debug

-------[ 2020-01-01 00:00:00 ] # Test time stamp


== Crate 1 type fastbus ignored text
# slot model clear header mask nchan ndata <- ndata ignored
6 1877 1  0xf0  0xc000  96 672  this text is ignored too: right, it is
7 1877  1 0xe0  0x0a    96 672

# Test whitespace lines, space in config, unsorted slots

   16 1881 1 0x0 0x0 64    cfg: +   highres  # trailing comment
14 1881 1 0x0 0x0 64    !Another inline comment
15 1881 1 0x0 0x0 64 64 cfg:highres,  double=5

Crate 5 type fastbus
2 1875 1 0xdeaf0000 0xffff0000
4 1875 1 0xdeae0000 0xffff0000 64
== Crate 2 type vme
#slot model bank
  2   250   25
  3   250   25
  4   250   25

)TESTMAP";

  // Reference formatted printout of the map above. Must not have invisible
  // trailing spaces at the end of any lines
  static const string refstr = R"REFSTR(TSROC 16
config 250  cfg:fw=1
config 1881 cfg:debug
==== Crate 1 type fastbus
#slot  model  clear  header      mask        nchan  cfgstr
 6     1877   1      0x000000f0  0x0000c000  96
 7     1877   1      0x000000e0  0x0000000a  96
 14    1881   1      0x00000000  0x00000000  64
 15    1881   1      0x00000000  0x00000000  64     cfg:highres,  double=5
 16    1881   1      0x00000000  0x00000000  64     cfg:+   highres
==== Crate 2 type vme
#slot  model  bank
 2     250    25
 3     250    25
 4     250    25
==== Crate 5 type fastbus
#slot  model  clear  header      mask        nchan
 2     1875   1      0xdeaf0000  0xffff0000  64
 4     1875   1      0xdeae0000  0xffff0000  64
)REFSTR";

  static const string refstr2 = R"REFSTR(TSROC 22
config 3561 cfg:thresh=250,window=70
==== Crate 3 type vme
#slot  model  bank  cfgstr
 11    3561   3561  cfg:+debug
==== Crate 58 type vme
#slot  model  bank
 11    3561   3561
==== Crate 64 type vme
#slot  model  bank
 11    514    86
==== Crate 65 type vme
#slot  model  bank
 11    514    86
==== Crate 66 type vme
#slot  model  bank
 11    514    86
==== Crate 67 type vme
#slot  model  bank
 11    514    86
==== Crate 68 type vme
#slot  model  bank
 11    514    86
==== Crate 69 type vme
#slot  model  bank
 11    514    86
==== Crate 70 type vme
#slot  model  bank
 11    514    86
)REFSTR";

  SECTION("Initial setup")
  {
    CHECK(crmap->GetName() == "testmap"sv);
    CHECK(crmap->getTSROC() == THaCrateMap::DEFAULT_TSROC);
    CHECK_FALSE(crmap->isInit());
    CHECK(crmap->GetSize() == 0);
  }

  SECTION("Initialization from string")
  {
    //TODO scalers

    auto st = crmap->init(testmap);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == 0);
    CHECK(crmap->GetName() == "testmap"sv);

    CHECK(crmap->GetSize() == 10);    // 5 + 3 + 2 slots
    CHECK(crmap->GetNcrates() == 3);  // 3 crates
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
    CHECK(crmap->getNdata(1,6) == THaCrateMap::MAXDATA);  // deprecated, value in map ignored
    CHECK(crmap->getHeader(1,6) == 0xf0);  // random nonsense header
    CHECK(crmap->getHeader(1,7) == 0xe0);  // and mask values
    CHECK(crmap->getMask(1,6) == 0xc000);
    CHECK(crmap->getMask(1,7) == 0x000a);
    CHECK(crmap->getMask(1,16) == 0);
    CHECK(crmap->getHeader(5,2) == 0xdeaf0000);
    CHECK(crmap->getMask(5,2) == 0xffff0000);
    CHECK(crmap->getNchan(5,2) == 64);     // automatically retrieved from Module
    CHECK(crmap->getConfigStr(1,6).empty());
    CHECK(crmap->getConfigStr(1,14) == "debug");
    CHECK(crmap->getConfigStr(1,15) == "highres,  double=5"); // atm, no in-string whitespace collapsing
    CHECK(crmap->getConfigStr(1,16) == "debug highres");

    const auto& used_crates = crmap->GetUsedCrates();
    CHECK(used_crates.size() == 3);
    CHECK(used_crates == set<UInt_t>{1,2,5});

    const auto& used_slots = crmap->GetUsedSlots(1);
    CHECK(used_slots.size() == 5);
    CHECK(used_slots == set<UInt_t>{6,7,14,15,16});
    const auto& used_slots2 = crmap->GetUsedSlots(5);
    CHECK(used_slots2.size() == 2);
    CHECK(used_slots2 == set<UInt_t>{4,2});

    crmap->Clear();
    CHECK_FALSE(crmap->isInit());
    CHECK(crmap->GetSize() == 0);
    CHECK(crmap->getTSROC() == THaCrateMap::DEFAULT_TSROC);
  }

  SECTION("Print method")
  {
    auto st = crmap->init(testmap);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    // Print normalized map (no comments, no empty lines, sorted crates and
    // slots, no timestamps)
    ostringstream oss;
    crmap->print(oss);
    CHECK(refstr == oss.str());
    crmap->Clear();
    // The output string should be suitable for initializing the map again
    // with identical results
    crmap->init(oss.str());
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    oss.str(""); oss.clear();
    crmap->print(oss);
    CHECK(oss.str() == refstr);
  }

  SECTION("Initialization from externally-opened file")
  {
    const char* testfilename = PODD_TESTS_DBDIR "/testmap.txt";
    FILE* fi = fopen(testfilename, "r");
    REQUIRE(fi != nullptr);
    int st = crmap->init(fi, testfilename);
    (void) fclose(fi);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == 0);
    CHECK(crmap->GetName() == string_view(testfilename));
    CHECK(crmap->GetSize() == 8);     // 5 + 3 slots
    CHECK(crmap->GetNcrates() == 2);  // 2 crates
    CHECK(crmap->GetUsedCrates() == set<UInt_t>{1,2});
  }

  // Open tests/DB/db_testmap.dat and extract info for the given date
  SECTION("Load from file, date = 01-Jan-2020 00:00 EST")
  {
    //=== Nothing defined, timestamp date is before first date in file
    time_t tloc = mktloc(2020, 1, 1);
    auto st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_ERR);
    //TODO check error message (how?)
  }

  SECTION("Load from file, date = 0 (01-Jan-1970 UTC)")
  {
    //=== Everything defined (timestamps disabled), unless there's a reset
    crmap->setDebug(THaCrateMap::kAllowMissingModel);
    int st = crmap->init(0);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
//    CHECK(crmap->GetNcrates() == 32); // 32 crates (some defined and undefined)
//    CHECK(crmap->GetSize() == 231);   // Big experiment
    CHECK(crmap->GetNcrates() == 9);  // Only last 9 definitions, after reset
    CHECK(crmap->GetSize() == 9);
  }

  SECTION("Load from file, date = 01-Feb-2021 00:00 EST")
  {
    //=== 14 crates, defined in first time-stamped block
    crmap->setDebug(THaCrateMap::kAllowMissingModel);
    time_t tloc = mktloc(2021, 2, 1);
    int st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == tloc);
    CHECK(crmap->getTSROC() == 21);
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
  }

  SECTION("Load from file, date = 01-May-2024 00:00 EDT")
  {
    //=== Add crates 28 & 29, redefine crate 7, remove 10
    crmap->setDebug(THaCrateMap::kAllowMissingModel);
    time_t tloc = mktloc(2024, 5, 1);
    int st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == tloc);
    CHECK(crmap->getTSROC() == 21);
    CHECK(crmap->GetNcrates() == 15);
    CHECK(crmap->GetSize() == 73+27-1-10+7);
    CHECK(crmap->GetUsedCrates() == set<UInt_t>{1,4,5,6,7,16,17,19,20,22,23,24,25,28,29});
    vector<UInt_t> nslots; nslots.reserve(crmap->GetNcrates());
    for( const auto& cr: crmap->GetUsedCrates() )
      nslots.push_back(crmap->getNslot(cr));
    CHECK(nslots == vector<UInt_t>{1,4,9,16,7,18,8,1,1,1,1,1,1,19,8});
  }

  SECTION("Load from file, with reset, date = 01-Aug-2025 00:00 EDT")
  {
    //=== Resets map, then defines crates 3, 58, 64-70.
    crmap->setDebug(THaCrateMap::kAllowMissingModel);
    time_t tloc = mktloc(2025, 8, 1);
    int st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetInitTime() == tloc);
    CHECK(crmap->getTSROC() == 22);  // Not reset
    CHECK(crmap->GetSize() == 9);
    CHECK(crmap->GetNcrates() == 9);
    CHECK(crmap->GetUsedCrates() == std::set<UInt_t>{3,58,64,65,66,67,68,69,70});
    CHECK_FALSE(crmap->crateUsed(5));
    CHECK(crmap->getNslot(58) == 1);
    CHECK(crmap->GetUsedSlots(58) == std::set<UInt_t>{11});
    CHECK(crmap->getModel(58,11) == 3561);
    CHECK(crmap->getBank(58,11) == 3561);
    // Global string set in earlier section, before "reset", is not cleared.
    CHECK(crmap->getConfigStr(58,11) == "thresh=250,window=70");
    // and it is available for prefixing a per-module string
    CHECK(crmap->getConfigStr(3,11) == "thresh=250,window=70 debug");
    CHECK(crmap->getNslot(64) == 1);
    CHECK(crmap->getModel(64,11) == 514);
    CHECK(crmap->getBank(64,11) == 86);
    CHECK(crmap->getConfigStr(64,11).empty());

    ostringstream oss;
    crmap->print(oss);
    CHECK(refstr2 == oss.str());  // Better than a few dozen individual CHECKs
  }

  SECTION("Load from file with always_reset flag")
  {
    //=== Several timestamp sections, with always_reset
    crmap->SetDBfileName("testmap2");
    CHECK(crmap->GetName() == "testmap2"sv);
    time_t tloc = mktloc(2021, 2, 1);
    int st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->isInit());
    CHECK(crmap->GetNcrates() == 2);
    CHECK(crmap->GetSize() == 13);
    CHECK(crmap->GetUsedCrates() == std::set<UInt_t>{4,5});
    CHECK(crmap->getNchan(4,1) == 64);
    CHECK(crmap->getNchan(4,4) == 64);

    tloc = mktloc(2022, 8, 1);
    st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->GetNcrates() == 2);
    CHECK(crmap->GetSize() == 22);
    CHECK(crmap->GetUsedCrates() == std::set<UInt_t>{6,7});

    tloc = mktloc(2023, 12, 1);
    st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->getTSROC() == 19);
    CHECK(crmap->GetNcrates() == 4);
    CHECK(crmap->GetSize() == 22);
    CHECK(crmap->GetUsedCrates() == std::set<UInt_t>{4,1,16,3});
    CHECK(crmap->getConfigStr(4,14) == "thresh=250,window=70");
  }

  SECTION("Time zone handling in file timestamps")
  {
    crmap->SetDBfileName("testmap2");
    time_t tloc = mktloc(2023, 10, 16); // 2023-10-16 00:00:00 UTC
    int st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->GetNcrates() == 1);
    CHECK(crmap->GetSize() == 2);
    CHECK(crmap->GetUsedCrates() == std::set<UInt_t>{2});

    tloc += 600; // 2023-10-16 00:10:00 UTC
    st = crmap->init(tloc);
    REQUIRE(st == THaCrateMap::CM_OK);
    CHECK(crmap->GetNcrates() == 4);
    CHECK(crmap->GetSize() == 22);
    CHECK(crmap->GetUsedCrates() == std::set<UInt_t>{1,3,4,16});
  }
}

