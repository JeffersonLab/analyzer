/////////////////////////////////////////////////////////////////////
//
//   THaSlotData
//   Data in one slot of one crate from DAQ.
//
//   THaSlotData contains data from one slot of one crate
//   from a CODA event.  Public methods allow to obtain
//   raw data for this crate & slot, or to obtain TDC, ADC,
//   or scaler data for each channel in the slot.  Methods
//   clearEvent() and loadData() should only be used by
//   the decoder.  WARNING: For efficiency, only the
//   hit counters are zero'd each event, not the data
//   arrays, see below.
//
//   author  Robert Michaels (rom@jlab.org) ca. 2000
//   extended for multihit modules - Bodo Reitz 2004
//   added decoder modules and bank decoding - Bob Michaels, 2015
//   added multiblock decoding, Bob Michaels, 2016
//   maintained and updated by Ole Hansen (ole@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaSlotData.h"
#include "Decoder.h"      // for UInt_t, Int_t, ROOT_VERSION, ROOT_VERSION_CODE, kMaxUInt
#include "Module.h"       // for Module
#include "TClass.h"       // for TClass
#include "TError.h"       // for Warning, Error
#include "THaCrateMap.h"  // for THaCrateMap
#include <cstring>        // for memcpy, size_t
#include <exception>      // for exception
#include <iomanip>        // for operator<<, setfill, setw
#include <iostream>       // for ostream, operator<<, cout, cerr, endl, ios_base
#include <set>            // for operator!=
#include <sstream>        // for ostringstream
#include <utility>        // for move

using namespace std;

namespace Decoder {

const UInt_t THaSlotData::DEFNCHAN = 128;  // Default number of channels
const UInt_t THaSlotData::DEFNDATA = 1024; // Default number of data words
const UInt_t THaSlotData::DEFNHITCHAN = 1; // Default number of hits per channel

//_____________________________________________________________________________
THaSlotData::THaSlotData()
  : crate(-1)
  , slot(-1)
  , fModule(nullptr)
  , numhitperchan(0)
  , numchanhit(0)
  , firstfreedataidx(0)
  , numholesdataidx(0)
  , didini(false)
  , fNchan(0)
{}

//_____________________________________________________________________________
THaSlotData::THaSlotData( UInt_t cra, UInt_t slo )
  : crate(cra)
  , slot(slo)
  , fModule(nullptr)
  , numhitperchan(0)
  , numchanhit(0)
  , firstfreedataidx(0)
  , numholesdataidx(0)
  , didini(false)
  , fNchan(0)
{}

//_____________________________________________________________________________
void THaSlotData::define(UInt_t cra, UInt_t slo, UInt_t nchan,
                         UInt_t ndata, UInt_t nhitperchan )
{
  // Must call define once if you are really going to use this slot.
  // Otherwise, it is an empty slot which does not use much memory.
  crate = cra;
  slot = slo;
  didini = true;
  fNchan = nchan;
  numhitperchan = nhitperchan;
  numHits.assign(fNchan, 0);
  chanlist.resize(fNchan);
  idxlist.resize(fNchan);
  dataindex.resize(static_cast<size_t>(fNchan) * numhitperchan);
  numMaxHits.resize(fNchan);
  numchanhit = firstfreedataidx = numholesdataidx = 0;
  rawData.clear();
  rawData.shrink_to_fit();  // in case define() is called again
  rawData.reserve(ndata);
  data.clear();
  data.shrink_to_fit();
  data.reserve(ndata);
}

//_____________________________________________________________________________
int THaSlotData::loadModule(const THaCrateMap *map)
{
  Int_t modelnum = map->getModel(crate, slot);

  // Search for the model number
  auto found = Module::fgModuleTypes().find(modelnum);

  if( found != Module::fgModuleTypes().end() ) {
    const auto& loctype = *found;
    assert(modelnum == loctype.fModel);  // else set::find lied

    // Get the ROOT class for this type
    if( !loctype.fTClass ) {
      loctype.fTClass = TClass::GetClass( loctype.fClassName );
      if (!loctype.fTClass) {
        return SD_OK;
      }
    }

    if( !fModule || fModule->IsA() != loctype.fTClass ) {
      fModule.reset(static_cast<Module*>( loctype.fTClass->New() ));
    }

    if (!fModule) {
      cerr << "ERROR: Failure to make module on crate "
        << dec << crate << "  slot " << slot << endl;
      cerr << "usually because the module class is abstract; make sure base "
        "class methods are defined" << endl;
      return SD_ERR;
    }

    // Init first, then SetSlot
    try {
      fModule->Init(map->getConfigStr(crate, slot).c_str());
    }
    catch( const exception& e ) {
      ostringstream ostr;
      ostr << "ERROR initializing module for crate " << dec << crate
        << " slot " << slot << ": " << e.what() << endl;
      cerr << ostr.str();
      return SD_ERR;
    }
    fModule->SetSlot(crate, slot,
                     map->getHeader(crate, slot),
                     map->getMask(crate, slot),
                     map->getModel(crate, slot));
    fModule->SetBank(map->getBank(crate, slot));
  }
  return SD_OK;
}

//_____________________________________________________________________________
UInt_t THaSlotData::LoadIfSlot( const UInt_t* evbuffer, const UInt_t *pstop) {
  // returns how many words seen.
  if ( !fModule ) {
// This is bad and should not happen; it means you didn't define a module
// for this slot.  Check db_cratemap.dat, e.g. erase things that don't exist.
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  if ( !fModule->IsSlot( *evbuffer ) )
    return 0;
  fModule->Clear();
  UInt_t wordseen = fModule->LoadBlock(this, evbuffer, pstop);
  return wordseen;
}

//_____________________________________________________________________________
UInt_t THaSlotData::LoadBank( const UInt_t* p, UInt_t pos, UInt_t len) {
  // returns how many words seen.
  if ( !fModule ) {
// This is bad and should not happen; it means you didn't define a module
// for this slot.  Check db_cratemap.dat, e.g. erase things that don't exist.
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  fModule->Clear();
  UInt_t wordseen = fModule->LoadBank(this, p, pos, len);
  return wordseen;
}

//_____________________________________________________________________________
UInt_t THaSlotData::LoadNextEvBuffer() {
// for modules that are in multiblock mode, load the next event in the block
  if ( !fModule ) {
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  fModule->Clear("E"); // clear the module's event data only
  return fModule->LoadNextEvBuffer(this);
}

//_____________________________________________________________________________
Int_t THaSlotData::loadData(const char* type, UInt_t chan, UInt_t dat, UInt_t raw) {

  const char* const here = "THaSlotData::loadData";

  if( !didini ) {
    Error(here, "Did not init slot. Fix your cratemap.");
    return SD_ERR;
  }
  if ( chan >= fNchan) {
    Warning(here, "Channel %u out of bounds, ignored, "
            "on crate %u slot %u. Check decoder module coding.",
            chan, crate, slot);
    return SD_WARN;
  }
  if( numchanhit > fNchan ) {
    Warning(here, "Too many channels for crate/slot = "
            "%u/%u: %u seen. Check decoder module coding.",
            crate, slot, numchanhit);
    return SD_WARN;
  }
  if( device.empty() && type ) device = type;

  // Record index into the data arrays for this hit. The idea is that every new
  // hit, regardless from which channel, is always appended to the data/rawData
  // arrays. Thus, if a channel has multiple hits, they may end up scattered
  // non-consecutively in these arrays, depending on the order in which the DAQ
  // delivers the data. The indices for the hits of a given channel are recorded
  // in dataindex[idxlist[chan]+ihit], where 0 <= ihit < numHits[chan].
  //
  // This is probably too complicated; sure, the data themselves are not being
  // relocated if there are more hits than expected (numhitperchan), but entries
  // in dataindex may have to be relocated, possibly a lot. A vector of vectors
  // might be a less bug-prone, more performant, and more maintainable solution.
  // Our postdoc Bodo Reitz wrote this logic in July 2004. FWIW, it works.
  //
  // Shorthands for readability
  const UInt_t nhit = numHits[chan];     // Number of hits on this channel already seen
  const UInt_t nmax = numMaxHits[chan];  // Currently reserved number of words for this channel
  const UInt_t incr = numhitperchan;     // Expected number of hits per channel
  const UInt_t numraw = getNumRaw();     // Number of data words already recorded

  // Prevent overflow of numHits (extremely unlikely)
  if( nhit == kMaxUInt ) [[unlikely]] {
    Warning(here, "Too many hits in crate/slot = %u/%u chan = %u. "
        "numchanhit numraw = %u %u", crate, slot, chan, numchanhit, numraw);
    return SD_WARN;
  }

  if( numchanhit == 0 || nhit == 0 ) {
    // No hits yet on this channel
    checkdataindex(incr);
    dataindex[firstfreedataidx] = numraw;
    idxlist[chan] = firstfreedataidx;
    numMaxHits[chan] = incr;
    firstfreedataidx += incr;
    chanlist[numchanhit++] = chan;      // List of channels with hits
  } else if( nhit < nmax ) {
    // Additional hit on this channel, and space available
    dataindex[idxlist[chan] + nhit] = numraw;
  } else if( idxlist[chan] + nmax == firstfreedataidx ) {
    // Additional hit, out of space, at end of dataindex array: just append
    assert(nhit == nmax);
    if( checkdataindex(incr) && idxlist[chan] + nhit != firstfreedataidx )
      goto relocate;  // if reshuffled, this chan may no longer be at the end
    assert( idxlist[chan]+nhit == firstfreedataidx );
append:
    dataindex[firstfreedataidx] = numraw;
    numMaxHits[chan] += incr;
    firstfreedataidx += incr;
  } else {
    // Make space for an additional hit on a channel that is not at the end of
    // the dataindex array: move this channel to the end, leaving a hole
    assert(nhit == nmax);
relocate:
    if( checkdataindex(nmax + incr) && idxlist[chan] + nmax == firstfreedataidx )
      goto append; // if reshuffled, this chan may now be at the end
    numholesdataidx += nmax;
    memcpy(&dataindex[firstfreedataidx], &dataindex[idxlist[chan]],
           nhit * sizeof(UInt_t));
    dataindex[firstfreedataidx + nhit] = numraw;
    idxlist[chan] = firstfreedataidx;
    numMaxHits[chan] += incr;
    firstfreedataidx += numMaxHits[chan];
  }

  assert(rawData.size() == data.size());
  rawData.push_back(raw);
  data.push_back(dat);
  numHits[chan]++;
  return SD_OK;
}

//_____________________________________________________________________________
int THaSlotData::loadData(UInt_t chan, UInt_t dat, UInt_t raw) {
  // NEW (6/2014).
  return loadData(nullptr, chan, dat, raw);
}

//_____________________________________________________________________________
void THaSlotData::print( ostream& os ) const
{
  ios_base::fmtflags fmt = os.flags();
  const auto numraw = getNumRaw();
  os << "\n THaSlotData contents :" << endl;
  os << "This is crate " << dec << crate << " and slot " << slot << endl;
  os << "Total Amount of Data : " << dec << numraw << endl;
  if( numraw == 0 )
    return;

  os << "Raw Data Dump:" << hex << endl;
  UInt_t k = 0;
  for( UInt_t i = 0; i < numraw / 5; i++ ) {
    for( UInt_t j = 0; j < 5; j++ ) {
      os << setfill('0') << right << setw(8) << getRawData(k++);
      if( j != 4 )
        os << "  ";
    }
    os << endl;
  }
  for( UInt_t i = k; i < numraw; i++ ) {
    os << setfill('0') << right << setw(8) << getRawData(i);
    if( i != numraw - 1 )
      os << "  ";
  }
  bool first = true;
  for( UInt_t chan = 0; chan < fNchan; chan++ ) {
    if( getNumHits(chan) > 0 ) {
      if( first ) {
        os << endl << "This is " << devType() << " Data :" << endl;
        first = false;
      }
      os << dec << "Channel " << chan << "  ";
      os << "numHits : " << getNumHits(chan) << endl;
      for( UInt_t hit = 0; hit < getNumHits(chan); hit++ ) {
        os << "Hit # " << dec << left << setfill(' ') << setw(3) << hit;
        os << " Data = (hex) " << hex << right << setfill('0')
          << setw(8) << getData(chan, hit);
        os << "  (decimal) = " << dec << getData(chan, hit) << endl;
      }
    }
    os << setfill(' ');
  }
  os.flags(fmt);
}

//_____________________________________________________________________________
bool THaSlotData::compressdataindex( UInt_t numidx )
{
  // Expand or compress (reshuffle) the dataindex. Returns true if reshuffled.
  // first check if it is more favourable to expand it, or to reshuffle
  auto alloci = dataindex.size();
  if( numholesdataidx/static_cast<double>(alloci) > 0.5 &&
      numholesdataidx > numidx ) {
    // reshuffle, enough holes to accommodate numidx extra words
#ifndef NDEBUG
    UInt_t oldidx = firstfreedataidx;
#endif
    VectorUIntNI tmp;
    tmp.reserve(dataindex.capacity());
    tmp.resize(alloci);
    firstfreedataidx = 0;
    for( UInt_t i = 0; i < numchanhit; i++ ) {
      UInt_t chan = chanlist[i];
      memcpy(&tmp[firstfreedataidx], &dataindex[idxlist[chan]],
             numHits[chan] * sizeof(UInt_t));
      idxlist[chan] = firstfreedataidx;
      firstfreedataidx += numMaxHits[chan];
    }
    assert(firstfreedataidx + numholesdataidx == oldidx); // shrank by numholesdataidx
    assert(firstfreedataidx + numidx < alloci);
    dataindex = std::move(tmp);
    numholesdataidx = 0;
    return true;
  }
  // If we didn't reshuffle, grow the array instead
  do { alloci *= 2; } while( alloci <= firstfreedataidx + numidx );
  dataindex.resize(alloci);
  return false;
}

} // namespace Decoder

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Decoder::THaSlotData)
#endif
