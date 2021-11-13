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
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "Module.h"
#include "THaSlotData.h"
#include "THaCrateMap.h"
#include "TClass.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace std;

namespace Decoder {

static const bool VERBOSE = true;
const UInt_t THaSlotData::DEFNCHAN = 128;  // Default number of channels
const UInt_t THaSlotData::DEFNDATA = 1024; // Default number of data words
const UInt_t THaSlotData::DEFNHITCHAN = 1; // Default number of hits per channel

//_____________________________________________________________________________
THaSlotData::THaSlotData() :
  crate(-1), slot(-1), fModule(nullptr), numhitperchan(0), numraw(0), numchanhit(0),
  firstfreedataidx(0), numholesdataidx(0), fDebugFile(nullptr),
  didini(false), fNchan(0) {}

//_____________________________________________________________________________
THaSlotData::THaSlotData(UInt_t cra, UInt_t slo) :
  crate(cra), slot(slo), fModule(nullptr), numhitperchan(0), numraw(0), numchanhit(0),
  firstfreedataidx(0), numholesdataidx(0), fDebugFile(nullptr),
  didini(false), fNchan(0)
{
}

//_____________________________________________________________________________
THaSlotData::~THaSlotData() = default;

//_____________________________________________________________________________
void THaSlotData::define(UInt_t cra, UInt_t slo, UInt_t nchan,
                         UInt_t /*ndata*/, // legacy parameter, no longer needed
                                           // since data arrays grow as needed
			 UInt_t nhitperchan ) {
  // Must call define once if you are really going to use this slot.
  // Otherwise its an empty slot which does not use much memory.
  crate = cra;
  slot = slo;
  didini = true;
  fNchan = nchan;
  numhitperchan=nhitperchan;
  numHits.resize(fNchan);
  chanlist.resize(fNchan);
  idxlist.resize(fNchan);
  chanindex.resize(fNchan);
  rawData.resize(fNchan);
  data.resize(fNchan);
  dataindex.resize(fNchan);
  numMaxHits.resize(fNchan);
  numchanhit = numraw = firstfreedataidx = numholesdataidx= 0;
  numHits.assign(numHits.size(),0);
}

//_____________________________________________________________________________
int THaSlotData::loadModule(const THaCrateMap *map) {

  UInt_t modelnum = map->getModel(crate, slot);

  // Search for the model number, which is the sorting key for the std::set
  auto found = Module::fgModuleTypes().find(Module::ModuleType(nullptr, modelnum));

  if( found != Module::fgModuleTypes().end() ) {
    const auto& loctype = *found;
    assert(modelnum == loctype.fMapNum);  // else set::find lied

    if (fDebugFile) {
      *fDebugFile << "THaSlotData:: loctype.fClassName  "<< loctype.fClassName<<endl;
      *fDebugFile << "THaSlotData:: loctype.fMapNum  "<< loctype.fMapNum<<endl;
      *fDebugFile << "THaSlotData:: fTClass ptr =  "<<loctype.fTClass<<endl;
    }

    // Get the ROOT class for this type
    if( !loctype.fTClass ) {
      loctype.fTClass = TClass::GetClass( loctype.fClassName );
      if (fDebugFile) {
	 *fDebugFile << "defining fTClass ptr =  "<<loctype.fTClass<<endl;
      }
      if (!loctype.fTClass) {
        if (fDebugFile) {
          *fDebugFile << "THaSlotData:: SERIOUS problem :  fTClass still zero " << endl;
        }
        return SD_OK;
      }
    }

    if (fDebugFile) *fDebugFile << "THaSlotData::  Found Module !!!! "<<dec<<modelnum<<endl;

    if (fDebugFile) *fDebugFile << "THaSlotData:: Creating fModule"<<endl;
    if( !fModule || fModule->IsA() != loctype.fTClass ) {
      fModule.reset(static_cast<Module*>( loctype.fTClass->New() ));
    } else if (fDebugFile) {
      *fDebugFile << "THaSlotData:: Reusing existing fModule" << endl;
    }

    if (!fModule) {
      cerr << "ERROR: Failure to make module on crate "<<dec<<crate<<"  slot "<<slot<<endl;
      cerr << "usually because the module class is abstract; make sure base class methods are defined"<<endl;
      if (fDebugFile)
        *fDebugFile << "failure to make module on crate "<<dec<<crate<<"  slot "<<slot<<endl;
      return SD_ERR;
    }

    if (fDebugFile) {
      *fDebugFile << "THaSlotData: fModule successfully created" << endl;
      *fDebugFile << "THaSlotData:: about to init  module   "
                  << crate << "  " << slot
                  << "  header " << hex << map->getHeader(crate, slot)
                  << "  model num " << dec << map->getModel(crate, slot)
                  << "  bank = " << map->getBank(crate, slot)
                  << endl;
    }

    // Init first, then SetSlot
    try {
      fModule->Init(map->getConfigStr(crate, slot));
    }
    catch( const exception& e ) {
      ostringstream ostr;
      ostr << "ERROR initializing module for crate " << dec << crate
           << " slot " << slot << ": " << e.what() << endl;
      cerr << ostr.str();
      if( fDebugFile )
        *fDebugFile << ostr.str();
      return SD_ERR;
    }
    fModule->SetSlot(crate, slot,
                     map->getHeader(crate, slot),
                     map->getMask(crate, slot),
                     map->getModel(crate, slot));
    fModule->SetBank(map->getBank(crate, slot));
    if (fDebugFile) {
      fModule->SetDebugFile(fDebugFile);
      fModule->DoPrint();
    }
  }
  return SD_OK;
}

//_____________________________________________________________________________
UInt_t THaSlotData::LoadIfSlot( const UInt_t* evbuffer, const UInt_t *pstop) {
  // returns how many words seen.
  if ( !fModule ) {
// This is bad and should not happen; it means you didn't define a module
// for this slot.  Check db_cratemap.dat, e.g. erase things that dont exist.
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  if (fDebugFile)
    *fDebugFile << "THaSlotData::LoadIfSlot:  "
                << dec << crate << "  " << slot
                << "   p " << hex << evbuffer << "  " << *evbuffer
                << "  " << dec << ((UInt_t(*evbuffer)) >> 27)
                << hex << "  " << pstop << "  " << fModule.get()
                << dec << endl;
  if ( !fModule->IsSlot( *evbuffer ) ) {
    if(fDebugFile) *fDebugFile << "THaSlotData:: Not slot ... return ... "<<endl;
    return 0;
  }
  if (fDebugFile) fModule->DoPrint();
  fModule->Clear();
  UInt_t wordseen = fModule->LoadBlock(this, evbuffer, pstop);
  if (fDebugFile)
    *fDebugFile << "THaSlotData:: after LoadIfSlot:  wordseen =  "
                << dec << "  " << wordseen << endl;
  return wordseen;
}

//_____________________________________________________________________________
UInt_t THaSlotData::LoadBank( const UInt_t* p, UInt_t pos, UInt_t len) {
  // returns how many words seen.
  if ( !fModule ) {
// This is bad and should not happen; it means you didn't define a module
// for this slot.  Check db_cratemap.dat, e.g. erase things that dont exist.
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  if (fDebugFile)
    *fDebugFile << "THaSlotData::LoadBank:  " << dec << crate << "  "<<slot
                << "  pos " << pos << "   len " << len << "   start word "
                << hex << *p << "  module ptr  " << fModule.get() << dec << endl;
  if (fDebugFile) fModule->DoPrint();
  fModule->Clear();
  UInt_t wordseen = fModule->LoadBank(this, p, pos, len);
  if (fDebugFile) *fDebugFile << "THaSlotData:: after LoadBank:  wordseen =  "<<dec<<"  "<<wordseen<<endl;
  return wordseen;
}

//_____________________________________________________________________________
UInt_t THaSlotData::LoadNextEvBuffer() {
// for modules that are in multiblock mode, load the next event in the block
  if ( !fModule ) {
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  return fModule->LoadNextEvBuffer(this);
}

//_____________________________________________________________________________
Int_t THaSlotData::loadData(const char* type, UInt_t chan, UInt_t dat, UInt_t raw) {

  const int very_verb=1;

  if( !didini ) {
    if (very_verb) {  // this might be your problem.
      cout << "THaSlotData: ERROR: Did not init slot."<<endl;
      cout << "  Fix your cratemap."<<endl;
    }
    return SD_ERR;
  }
  if ( chan >= fNchan) {
    if (VERBOSE) {
      cout << "THaSlotData: Warning in loadData: channel ";
      cout <<chan<<" out of bounds, ignored,"
	   << " on crate " << crate << " slot "<< slot << endl;
    }
    return SD_WARN;
  }
  if( numchanhit > fNchan) {
    if (VERBOSE) {
      cout << "THaSlotData: Warning in loadData: too many "
	   << "channels for crate/slot = " << crate << " " << slot;
	   cout << ": " << numchanhit << " seen."
	   << endl;
    }
    return SD_WARN;
  }
  if( device.empty() && type ) device = type;

  if (( numchanhit == 0 )||(numHits[chan]==0)) {
    compressdataindex(numhitperchan);
    dataindex[firstfreedataidx]=numraw;
    idxlist[chan]=firstfreedataidx;
    numMaxHits[chan]=numhitperchan;
    firstfreedataidx+=numhitperchan;
    chanindex[chan]=numchanhit;
    chanlist[numchanhit++]=chan;
  } else {
    if (numHits[chan]<numMaxHits[chan]) {
      dataindex[idxlist[chan]+numHits[chan]]=numraw;
    } else {
      if (idxlist[chan]+numMaxHits[chan]==firstfreedataidx) {
	compressdataindex(numhitperchan);
	dataindex[idxlist[chan]+numHits[chan]]=numraw;
	numMaxHits[chan]+=numhitperchan;
	firstfreedataidx+=numhitperchan;
      } else {
	compressdataindex(numMaxHits[chan]+numhitperchan);
	numholesdataidx+=numMaxHits[chan];
	for (UInt_t i=0; i<numHits[chan]; i++  ) {
	  dataindex[firstfreedataidx+i]=dataindex[idxlist[chan]+i];
	}
	dataindex[firstfreedataidx+numHits[chan]]=numraw;
	idxlist[chan]=firstfreedataidx;
	numMaxHits[chan]+=numhitperchan;
	firstfreedataidx+=numMaxHits[chan];
      }
    }
  }

  // Grow data arrays if necessary
  if( numraw >= data.size() ) {
    size_t allocd = 2*data.size();
    rawData.resize(allocd);
    data.resize(allocd);
  }
  rawData[numraw] = raw;
  data[numraw++]  = dat;
  if( numHits[chan] == kMaxUInt ) {
    cout << "THaSlotData: numchanhit, numraw = "<<numchanhit<<"  "<<numraw<<endl;
    if( VERBOSE )
      cout << "THaSlotData: Warning in loadData: too many hits "
	   << "for module " << device << " in crate/slot = "
	   << dec << crate << " " << slot
	   << " chan = " << chan << endl;
    return SD_WARN;
  }
  numHits[chan]++;
  return SD_OK;
}

//_____________________________________________________________________________
int THaSlotData::loadData(UInt_t chan, UInt_t dat, UInt_t raw) {
  // NEW (6/2014).
  return loadData(nullptr, chan, dat, raw);
}

//_____________________________________________________________________________
void THaSlotData::print() const
{
  if (fDebugFile) {
    print_to_file();
    return;
  }
  cout << "\n THaSlotData contents : " << endl;
  cout << "This is crate "<<dec<<crate<<" and slot "<<slot<<endl;
  cout << "Total Amount of Data : " << dec << getNumRaw() << endl;
  if (getNumRaw() > 0) {
    cout << "Raw Data Dump: " << hex << endl;
  } else {
    //    if(getNumRaw() == SD_ERR) cout << "getNumRaw ERROR"<<endl;
    return;
  }
  UInt_t k = 0;
  for (UInt_t i=0; i<getNumRaw()/5; i++) {
    for(UInt_t j=0; j<5; j++) cout << getRawData(k++) << "  ";
    cout << endl;
  }
  for (UInt_t i=k; i<getNumRaw(); i++) cout << getRawData(i) << "  ";
  bool first = true;
  ios_base::fmtflags fmt = cout.flags();
  for ( UInt_t chan=0; chan < fNchan; chan++) {
    if (getNumHits(chan) > 0) {
      if (first) {
	cout << "\nThis is "<<devType()<<" Data : "<<endl;
	first = false;
      }
      cout << dec << "Channel " << chan << "  ";
      cout << "numHits : " << getNumHits(chan) << endl;
      for (UInt_t hit=0; hit<getNumHits(chan); hit++) {
	cout << "Hit # "<<dec<<hit;
        //	if(getData(chan,hit) == SD_ERR) cout<<"ERROR: getData"<<endl;
	cout << "  Data  = (hex) "<<hex<<getData(chan,hit);
	cout << "   or (decimal) = "<<dec<<getData(chan,hit)<<endl;
      }
    }
  }
  cout.flags(fmt);
}

//_____________________________________________________________________________
void THaSlotData::print_to_file() const {
  if (!fDebugFile) return;
  *fDebugFile << "\n THaSlotData contents : " << endl;
  *fDebugFile << "This is crate "<<dec<<crate<<" and slot "<<slot<<endl;
  *fDebugFile << "Total Amount of Data : " << dec << getNumRaw() << endl;
  if (getNumRaw() > 0) {
    *fDebugFile << "Raw Data Dump: " << hex << endl;
  } else {
    //    if(getNumRaw() == SD_ERR) *fDebugFile << "getNumRaw ERROR"<<endl;
    return;
  }
  UInt_t k = 0;
  for (UInt_t i=0; i<getNumRaw()/5; i++) {
    for(UInt_t j=0; j<5; j++) *fDebugFile << getRawData(k++) << "  ";
    *fDebugFile << endl;
  }
  for (UInt_t i=k; i<getNumRaw(); i++) *fDebugFile << getRawData(i) << "  ";
  bool first = true;
  for ( UInt_t chan=0; chan < fNchan; chan++) {
    if (getNumHits(chan) > 0) {
      if (first) {
	*fDebugFile << "\nThis is "<<devType()<<" Data : "<<endl;
	first = false;
      }
      *fDebugFile << dec << "Channel " << chan << "  ";
      *fDebugFile << "numHits : " << getNumHits(chan) << endl;
      for (UInt_t hit=0; hit<getNumHits(chan); hit++) {
	*fDebugFile << "Hit # "<<dec<<hit;
        //	if(getData(chan,hit) == SD_ERR) *fDebugFile<<"ERROR: getData"<<endl;
	*fDebugFile << "  Data  = (hex) "<<hex<<getData(chan,hit);
	*fDebugFile << "   or (decimal) = "<<dec<<getData(chan,hit)<<endl;
      }
    }
  }
}

//_____________________________________________________________________________
void THaSlotData::compressdataindexImpl( UInt_t numidx )
{
  // first check if it is more favourable to expand it, or to reshuffle
  auto alloci = dataindex.size();
  if( numholesdataidx/static_cast<double>(alloci) > 0.5 &&
      numholesdataidx > numidx ) {
    // Maybe reshuffle. But how many active dataindex entries would we need?
    UInt_t nidx = numidx;
    for (UInt_t i=0; i<numchanhit; i++) {
      nidx += numMaxHits[ chanlist[i] ];
    }
    if( nidx <= alloci ) {
      // reshuffle, lots of holes
      VectorUIntNI tmp(alloci);
      firstfreedataidx=0;
      for (UInt_t i=0; i<numchanhit; i++) {
	UInt_t chan=chanlist[i];
	for (UInt_t j=0; j<numHits[chan]; j++) {
	  tmp[firstfreedataidx+j]=dataindex[idxlist[chan]+j];
	}
	idxlist[chan] = firstfreedataidx;
	firstfreedataidx=firstfreedataidx+numMaxHits[chan];
      }
      dataindex = std::move(tmp);
      return;
    }
  }
  // If we didn't reshuffle, grow the array instead
  alloci *= 2;
  if( firstfreedataidx+numidx > alloci )
    // Still too small?
    alloci = 2*(firstfreedataidx+numidx);
  // FIXME one should check that it doesnt grow too much
  dataindex.resize(alloci);
}

} // namespace Decoder

//_____________________________________________________________________________
ClassImp(Decoder::THaSlotData)
