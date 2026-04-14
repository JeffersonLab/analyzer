//*-- Author :    Ole Hansen   11-Apr-26
//
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SlotData_t                                                                //
//                                                                           //
// Test Decoder::THaSlotData                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CATCH3
#include <catch2/catch_test_macros.hpp>            // for CHECK, operator==, StringRef, Section, SECTION, AutoReg, REQUIRE, TEST_CASE, operator<=
#include <catch2/generators/catch_generators.hpp>  // for makeGenerators, value, GENERATE, generate
#include "catch2/catch_message.hpp"                // for INFO, ScopedMessage
#else
# include <catch2/catch.hpp>
#endif

#include "THaSlotData.h"     // for THaSlotData, SD_OK, SD_WARN, SD_ERR, Rtypes
#include "Rtypes.h"          // for UInt_t, kMaxUInt, BIT, Int_t
#include "TError.h"          // for gErrorIgnoreLevel, kBreak
#include <algorithm>         // for equal, sort, find, max
#include <cassert>           // for assert
#include <cstddef>           // for size_t
#include <memory>            // for unique_ptr, make_unique
#include <numeric>           // for accumulate, iota
#include <random>            // for uniform_int_distribution, mt19937
#include <sstream>           // for ostringstream
#include <string>            // for char_traits, operator==, string, string_literals
#include <string_view>       // for string_view, operator==
#include <vector>            // for vector, operator==

using namespace std;
using namespace std::string_literals;
using namespace Decoder;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Test cases                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// Test the decoder slotdata class
TEST_CASE("Slot Data", "[Decoder]") // NOLINT(*-function-cognitive-complexity)
{
  // The object to be tested
  auto sldat = make_unique<THaSlotData>();
  gErrorIgnoreLevel = kBreak;  // Suppress ROOT messages from Error/Warning/Info

  // Some constants we need later
  static constexpr UInt_t NHIT = 100'000, NCHAN = 8, MAXCHAN = 32,
                          SHIFT = 27, MASK = (1 << SHIFT) - 1;

  SECTION("Default parameters")
  {
    REQUIRE(sldat);
    CHECK(sldat->getCrate() == kMaxUInt);
    CHECK(sldat->getSlot() == kMaxUInt);
    CHECK(sldat->getNchan() == 0);
    CHECK(sldat->devType() == ""sv);
    CHECK(sldat->getNumChan() == 0);
    CHECK(sldat->getNumRaw() == 0);
    CHECK(sldat->GetModule() == nullptr);
  }

  SECTION("Define slot")
  {
    sldat->define(1, 5, 96);
    CHECK(sldat->getCrate() == 1);
    CHECK(sldat->getSlot() == 5);
    CHECK(sldat->getNchan() == 96);
    CHECK(sldat->devType() == ""sv);
    CHECK(sldat->getNumChan() == 0);
    CHECK(sldat->getNumRaw() == 0);
    CHECK(sldat->GetModule() == nullptr);

    sldat->define(2, 1);
    sldat->setDevType("adc");
    CHECK(sldat->getCrate() == 2);
    CHECK(sldat->getSlot() == 1);
    CHECK(sldat->getNchan() == THaSlotData::DEFNCHAN); // 128
    CHECK(sldat->devType() == "adc"sv);
  }

  SECTION("loadData, clearEvent")
  {
    sldat->define(18, 2, 96);
    auto ret = sldat->loadData("module", 12, 0x0100, 0xf0000100);
    CHECK(ret == SD_OK);
    CHECK(sldat->getNumRaw() == 1);
    CHECK(sldat->getNumChan() == 1);
    CHECK(sldat->getNextChan(0) == 12);
    CHECK(sldat->getNumHits(12) == 1);
    CHECK(sldat->getData(12, 0) == 0x0100);
    CHECK(sldat->getRawData(12, 0) == 0xf0000100);
    CHECK(sldat->devType() == "module"sv);

    sldat->clearEvent();
    CHECK(sldat->getNumRaw() == 0);
    CHECK(sldat->getNumChan() == 0);
    CHECK(sldat->getNumHits(12) == 0);
  }

  SECTION("Error returns from loadData")
  {
    auto ret = sldat->loadData(16, 0, 0);
    CHECK(ret == SD_ERR); // not initialized
    sldat->define(1, 5, 64);
    CHECK(sldat->getNchan() == 64);
    ret = sldat->loadData(64, 0, 0);
    CHECK(ret == SD_WARN); // channel out of range (0-63 ok)
    ret = sldat->loadData(224, 0, 0);
    CHECK(ret == SD_WARN);
    ret = sldat->loadData(-1, 0, 0);
    CHECK(ret == SD_WARN);
  }

  SECTION("Print method")
  {
    // List of channels and number of hits to make on each
    vector<UInt_t> channels{ 1,  5 };
    vector<UInt_t> nhits   { 11, 8 };
    auto totalhits = accumulate(nhits.begin(), nhits.end(), 0U);

    // Init slotdata
    sldat->define(10, 1, MAXCHAN, THaSlotData::DEFNDATA, 32 );
    sldat->setDevType("Slot10Module");

    // Insert hits for each channel
    Int_t err = 0;
    for( UInt_t i = 0; i < channels.size(); ++i ) {
      UInt_t chan = channels[i];
      UInt_t nhit = nhits[i];
      UInt_t base = chan << SHIFT | chan * NHIT;
      for( UInt_t ihit = 0; ihit < nhit && err == 0; ++ihit ) {
        UInt_t data = base + ihit;
        err = sldat->loadData(chan, data & MASK, data );
      }
      if( err )
        break;
    }
    CHECK( err == SD_OK );
    CHECK( sldat->getNumRaw() == totalhits );
    CHECK( sldat->getNumChan() == channels.size() );

    ostringstream oss;
    sldat->print(oss);

    static const string refstr = R"REFSTR(
 THaSlotData contents :
This is crate 10 and slot 1
Total Amount of Data : 19
Raw Data Dump:
080186a0  080186a1  080186a2  080186a3  080186a4
080186a5  080186a6  080186a7  080186a8  080186a9
080186aa  2807a120  2807a121  2807a122  2807a123
2807a124  2807a125  2807a126  2807a127
This is Slot10Module Data :
Channel 1  numHits : 11
Hit # 0   Data = (hex) 000186a0  (decimal) = 100000
Hit # 1   Data = (hex) 000186a1  (decimal) = 100001
Hit # 2   Data = (hex) 000186a2  (decimal) = 100002
Hit # 3   Data = (hex) 000186a3  (decimal) = 100003
Hit # 4   Data = (hex) 000186a4  (decimal) = 100004
Hit # 5   Data = (hex) 000186a5  (decimal) = 100005
Hit # 6   Data = (hex) 000186a6  (decimal) = 100006
Hit # 7   Data = (hex) 000186a7  (decimal) = 100007
Hit # 8   Data = (hex) 000186a8  (decimal) = 100008
Hit # 9   Data = (hex) 000186a9  (decimal) = 100009
Hit # 10  Data = (hex) 000186aa  (decimal) = 100010
Channel 5  numHits : 8
Hit # 0   Data = (hex) 0007a120  (decimal) = 500000
Hit # 1   Data = (hex) 0007a121  (decimal) = 500001
Hit # 2   Data = (hex) 0007a122  (decimal) = 500002
Hit # 3   Data = (hex) 0007a123  (decimal) = 500003
Hit # 4   Data = (hex) 0007a124  (decimal) = 500004
Hit # 5   Data = (hex) 0007a125  (decimal) = 500005
Hit # 6   Data = (hex) 0007a126  (decimal) = 500006
Hit # 7   Data = (hex) 0007a127  (decimal) = 500007
)REFSTR";
    CHECK(refstr == oss.str());
  }

  SECTION("Add 100k random hits on 8 channels")
  {
    // Generator for reproducible random number sequences (default seed)
    std::mt19937 gen; // NOLINT(*-random-generator-seed, *-msc32-c, *-msc51-cpp)
    using RandomInt = std::uniform_int_distribution<UInt_t>;

    vector<UInt_t> chanmap;
    chanmap.reserve(NCHAN);

    // Generate NCHAN channel numbers in [0, MAXCHAN), so we do not just
    // use a simple sequence of consecutive numbers.
    RandomInt rndmaxint(0, MAXCHAN - 1); // for generating channel numbers
    for( UInt_t i = 0; i < NCHAN; ++i ) {
      UInt_t chan;
      do { chan = rndmaxint(gen); }
      while( ranges::find(chanmap, chan) != chanmap.end() );
      chanmap.push_back(chan);
    }

    // Initialize slot data to hold data from MAXCHAN channels
    // Intentionally use nhitperchan = 1 to stress-test the reshuffle algorithm.
    // Performance is two orders of magnitude better with nhitperchan = 64.
    // nhitperchan = 1500 results in higher memory usage than the smaller values.
    UInt_t nhitperchan = GENERATE( 1, 8, 64, 1500 );
    INFO("nhitperchan = " << nhitperchan);
    sldat->define(10, 1, MAXCHAN, THaSlotData::DEFNDATA, nhitperchan );

    // Fill SlotData with hits. THaSlotData was specifically designed to store
    // an arbitrary number of hits from different channels in random order,
    // preserving the order of the hits within each channel and the overall
    // ordering of the arrival of the hits (although without attaching a channel
    // number to each hit). At the time the class was written, it was coded
    // in C, and this was a little difficult to do. The resulting code was
    // fairly bug-prone, and performance with truly randomly jumping channel
    // numbers is poor. Of course, any decent real-world DAQ delivers the hits
    // for each channel in one block (although the channel numbers are not
    // necessarily ordered). This test puts maximum stress on the internal logic.
    Int_t ret = SD_OK;
    vector<UInt_t> nhits(NCHAN);
    RandomInt randint(0, NCHAN - 1); // for generating channel indices
    for( size_t i = 0; i < NHIT && ret == SD_OK; ++i ) {
      UInt_t idx = randint(gen); // Channel index
      assert( idx < NCHAN );
      UInt_t chan = chanmap[idx];   // Actual channel number
      assert( chan < MAXCHAN );
      // The data are sequential in hit number for each channel
      // Data format (32-bit words)
      // bits 0-26:  hit number + NHIT * channel index
      // bits 27-31: channel index (0-31 allowable)
      UInt_t data = idx << SHIFT | nhits[idx] + idx * NHIT;
      ret = sldat->loadData(chan, data & MASK, data );
      nhits[idx]++;
    }
    CHECK(ret == SD_OK);

    // Overall hit number and number of active channels
    CHECK(sldat->getNumRaw() == NHIT);
    CHECK(sldat->getNumChan() == NCHAN);

    // Actual channel numbers
    vector<UInt_t> channels(NCHAN);
    for( UInt_t i = 0; i < NCHAN; ++i )
      channels[i] = sldat->getNextChan(i);
    vector refchans{chanmap};
    ranges::sort(refchans);
    ranges::sort(channels);
    CHECK(channels == refchans);

    // Number of hits on each channel
    vector<UInt_t> numhits(NCHAN);
    for( UInt_t i = 0; i < NCHAN; ++i )
      numhits[i] = sldat->getNumHits(chanmap[i]);
    CHECK(numhits == nhits);

    // Check data in each channel. Each data word is unique.
    auto maxnhits = ranges::max(nhits);
    vector<UInt_t> dataref(maxnhits), thedata;
    ranges::iota(dataref, 0);
    thedata.reserve(maxnhits);
    for(UInt_t idx = 0; idx < NCHAN; ++idx ) {
      UInt_t chan = chanmap[idx];
      UInt_t nhit = nhits[idx];  // = sldat->getNumHits(chan), see above
      UInt_t mindata = idx * NHIT;
      UInt_t maxdata = mindata + nhit - 1;
      thedata.clear();
      ret = 0;
      for( UInt_t ihit = 0; ihit < nhit && ret == 0; ++ihit ) {
        UInt_t raw = sldat->getRawData(chan, ihit);
        UInt_t tag = raw >> SHIFT;
        UInt_t data = raw & MASK;
        thedata.push_back( data - mindata );
        if( tag != idx )
          ret |= BIT(1);
        if( data < mindata )
          ret |= BIT(2);
        if( data > maxdata )
          ret |= BIT(3);
        if( data != sldat->getData(chan, ihit) )
          ret |= BIT(4);
      }
      CHECK(ret == 0);   // Else data error (wrong channel, out of range)
      CHECK(thedata.size() == nhits[idx]);  // trivially
      CHECK(thedata.size() <= maxnhits);    // trivially
      // Check if hit numbers are sequential
      CHECK(std::equal(thedata.begin(), thedata.end(), dataref.begin()));
    }
  }
}

