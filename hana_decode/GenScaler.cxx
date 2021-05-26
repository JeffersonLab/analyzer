////////////////////////////////////////////////////////////////////
//
//   GenScaler
//   A generic scaler
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>

using namespace std;

namespace Decoder {

  const int DEFAULT_DELTAT = 4;

  GenScaler::GenScaler( UInt_t crate, UInt_t slot) :
    VmeModule(crate, slot),
    fIsDecoded(false), fFirstTime(true), fDeltaT(0.0),
    fClockChan(0), fNumChanMask(0), fNumChanShift(0),
    fHasClock(false), fClockRate(0), fNormScaler(nullptr),
    firsttime(true), firstwarn(true)
  {
    fWordsExpect = 32;
    fNumChan = 0;
  }

  void GenScaler::Clear(Option_t* opt) {
    // Clear event-by-event data
    VmeModule::Clear(opt);
    fIsDecoded=false;
  }

  void GenScaler::GenInit()
  {
    fHasClock = false;
    fFirstTime = true;
    fIsDecoded = false;
    fClockChan = -1;
    fClockRate = 0;
    fNormScaler = nullptr;
    fNumChanMask = 0xff;
    fNumChanShift = 0;
    fDeltaT = DEFAULT_DELTAT;  // a default time interval between readings
    fDataArray.resize(fWordsExpect);
    fPrevData.resize(fWordsExpect);
    fRate.resize(fWordsExpect);
    fDataArray.assign(fDataArray.size(), 0);
    fPrevData.assign(fPrevData.size(), 0);
    fRate.assign(fRate.size(), 0.0);
  }

  void GenScaler::SetBank(Int_t bank) {
    /// Define scaler header format for modules in banks
    fBank = bank;
    if(fBank > 0) {
      fHeader = fSlot << 8;
      fHeaderMask = 0xff00;
    }
  }

  Int_t GenScaler::SetClock( Double_t deltaT, UInt_t clockchan, Double_t clockrate) {
    // Sets the clock for the time base
    // retcode:
    //     0   nothing wrong, but has no deltaT nor clock data. (a bit odd)
    //    -1   something wrong, see error print
    //     1   clock rate set for this scaler.  This scaler has a clock.
    //     2   using deltaT, instead, for an approx time diff between readings

    Int_t retcode = 0;
    fHasClock = false;
    if (clockrate > 0) {
      if (fNormScaler) {
	cout << "GenScaler:: WARNING:  Declaring this object to have"<<endl;
	cout << "   a clock even though this also has a normalization scaler ?"<<endl;
	cout << "  This makes no sense. "<<endl;
	retcode = -1;
      }
      fHasClock = true;
    }
    if( clockchan >= fWordsExpect ) {
      cout << "GenScaler:: ERROR:  clock channel out of range "<<endl;
      retcode = -1;
    } else {
      fClockChan = clockchan;
    }
    fClockRate = clockrate;
    if (retcode >= 0 && fClockRate > 0) retcode=1;
    if (deltaT > 0) {
      fDeltaT = deltaT;
      if (fClockRate <= 0 && retcode>=0) retcode=2;
    } else {
      cout << "GenScaler:: Warning: using default deltaT = "<<fDeltaT<<endl;
    }
    if (retcode == 0) cout << "GetScaler:: Warning:: no deltaT nor clockrate define for this scaler "<<endl;
    return retcode;
  }

  void GenScaler::LoadNormScaler(GenScaler *scal) {
    if (fHasClock) {
      cout << "GenScaler:: WARNING:  loading norm scaler although this"<<endl;
      cout << "   object has a clock ?   This makes no sense. "<<endl;
    }
    if ( !scal ) {
      cout << "GenScaler:: ERROR:  attempting to load a non-existent "<<endl;
      cout << "normalization scaler !" <<endl;
      return;
    }
    fNormScaler = scal;
  }

  Int_t GenScaler::Decode(const UInt_t *evbuffer) {
    Int_t doload=0;
    Int_t nfound=1;
    if (IsDecoded()) return nfound;
    if (fFirstTime) {
      fFirstTime = false;
    } else {
      doload=1;
      fPrevData = fDataArray;
    }
    if ( !IsSlot(*evbuffer) ) return nfound;
    if (fDebugFile) *fDebugFile << "is slot 0x"<<hex<<*evbuffer<<dec<<" num chan "<<fNumChan<<endl;
    evbuffer++;
    fIsDecoded = true;
    fDataArray.assign( evbuffer, evbuffer+fNumChan );
    nfound += fNumChan;
    if (fDebugFile) {
      for( UInt_t i = 0; i < fNumChan; i++ )
        *fDebugFile << "   data[" << i << "] = 0x" << hex << fDataArray[i] << dec << endl;
    }
    if (doload) LoadRates();
    return nfound;
  }

  Double_t GenScaler::GetTimeSincePrev() const {
    // Time since previous reading.
    // If a normalization scaler was defined, use its time base.
    // Otherwise, if this scaler has a clock, use it to get the time precisely.
    // Finally, if there is no clock, use fDeltaT as an approximate time.

    if (fNormScaler) return fNormScaler->GetTimeSincePrev();
    Double_t dtime = 0;
    if (fDebugFile) {
      *fDebugFile << "Into GetTimeSincePrev "<<endl;
      if (IsDecoded()) *fDebugFile << "Is Decoded "<<endl;
      if (fHasClock) *fDebugFile << "has Clock "<<endl;
    }
    if (IsDecoded() && fHasClock && fClockRate>0 && checkchan(fClockChan)) {
      // Check for scaler overflow
      UInt_t clockdif;
      if(fDataArray[fClockChan] < fPrevData[fClockChan]) {
	clockdif = (kMaxUInt-(fPrevData[fClockChan]-1))
	  + fDataArray[fClockChan];
      } else {
	clockdif = fDataArray[fClockChan]-fPrevData[fClockChan];
      }
      dtime = clockdif/fClockRate;
      if (fDebugFile) *fDebugFile << "GetTimeSincePrev  "<<fClockRate<<"   "<<fClockChan<<"   "<<dtime<<endl;
    } else {
      if (fDeltaT > 0) dtime = fDeltaT;   // default
    }
    return dtime;
  }

  void GenScaler::LoadRates() {
    if (IsDecoded()) {
      Double_t dtime = GetTimeSincePrev();
      if (dtime==0) {
	fRate.assign(fRate.size(), 0);
	return;
      }
      for( UInt_t i = 0; i < fWordsExpect; i++ ) {
	// Check for scaler overflow
	UInt_t diff;
	if(fDataArray[i] < fPrevData[i]) {
	  diff = (kMaxUInt-(fPrevData[i]-1)) + fDataArray[i];
	} else {
	  diff = fDataArray[i]-fPrevData[i];
	}
	fRate[i] = diff/dtime;
      }
    }
  }

  UInt_t GenScaler::GetData( UInt_t chan) const {
    if (checkchan(chan)) {
      return fDataArray[chan];
    } else {
      return 0;
    }
  }

  Double_t GenScaler::GetRate( UInt_t chan) const {
    if (checkchan(chan)) {
      return fRate[chan];
    } else {
      return 0;
    }
  }

  void GenScaler::DoPrint() const {
    cout << "GenScaler::   crate "<<fCrate<<"   slot "<<fSlot<<endl;
    cout << "GenScaler::   Header 0x"<<hex<<fHeader<<"    Mask  0x"<<fHeaderMask<<dec<<endl;
    cout << "GenScaler::   fNumChanMask = "<< hex<< fNumChanMask<<dec<<endl;
    cout << "GenScaler::   fNumChanShift = "<< hex<< fNumChanShift<<dec<<endl;
    cout << "num words expected  "<<fWordsExpect<<endl;
    if (fHasClock) cout << "Has a clock"<<endl;
    if (fNormScaler) cout << "Using norm scaler with ptr = "<<fNormScaler << endl;
    cout << "Clock channel "<<fClockChan<<"   clock rate "<<fClockRate<<endl;
  }

  void GenScaler::DebugPrint(ofstream *file) const {
    if (!file) return;
    *file << "GenScaler::   crate "<<fCrate<<"   slot "<<fSlot<<endl;
    *file << "GenScaler::   Header 0x"<<hex<<fHeader<<"    Mask  0x"<<fHeaderMask<<dec<<endl;
    *file << "num words expected  "<<fWordsExpect<<endl;
    if (fHasClock) *file << "Has a clock"<<endl;
    if (fNormScaler) *file << "Using norm scaler with ptr = "<<fNormScaler << endl;
    *file << "Clock channel "<<fClockChan<<"   clock rate "<<fClockRate<<endl;
    *file<<"  ----   Data  ---- "<<fWordsExpect<<endl;
    *file<<"Data now   //   previous    //   rate  "<<endl;
    for( UInt_t i = 0; i < fWordsExpect; i++ ) {
      *file << "  0x"<<hex<<fDataArray[i]<<"   0x"<<fPrevData[i]<<dec<<"   "<<fRate[i]<<endl;
    }
  }

  Bool_t GenScaler::IsSlot( UInt_t rdata ) {
    /// Check if this word is the header for the slot we are looking for
    /// Get the number of channels in this module from the header and
    /// save so that bank version of LoadSlot can skip over this module if
    /// it is not the correct one.
    if( (rdata & fHeaderMask) != fHeader )
      return false;
    // This is a header word. Try extracting the number of channels.
    fNumChan = (rdata & fNumChanMask) >> fNumChanShift;
    if( fNumChan == 0 ) {
      fNumChan = fgNumChanDefault;
      if( firsttime ) {
        firsttime = false;
        cout << "Warning::GenScaler:: (" << fCrate << "," << fSlot
             << ") using default num " << fgNumChanDefault << " channels"
             << endl;
      }
    }
    if( fNumChan != fWordsExpect ) {
      if( fNumChan > fWordsExpect )
        fNumChan = fWordsExpect;

      // Print warning once to alert user to potential problems,
      // or for every suspect event if debugging enabled
      if( firstwarn && fNumChan != 16 && fNumChan != fWordsExpect ) {
        cout << "GenScaler:: ERROR:  (" << fCrate << "," << fSlot << ") "
             << "inconsistent number of chan." << endl;
        //        DoPrint();
        firstwarn = false;
      }
    }
    return true;
  }

UInt_t GenScaler::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer, const UInt_t* pstop )
  {
    // This is a simple, default method for loading a slot
    const UInt_t* p = evbuffer;
    Clear();
    while( p < pstop ) {
      if( IsSlot(*p) ) {
	if (fDebugFile) *fDebugFile << "GenScaler:: Loadslot "<<endl;
	if (!fHeader) cerr << "GenScaler::LoadSlot::ERROR : no header ?"<<endl;
	Decode(p);
        for( UInt_t ichan = 0; ichan < fNumChan; ichan++ ) {
          sldat->loadData(ichan, fDataArray[ichan], fDataArray[ichan]);
	}
	fWordsSeen = fNumChan;
	return fWordsSeen;
      }
      p++;
    }
    return 0;
  }

  UInt_t GenScaler::LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer, UInt_t pos, UInt_t len) {
    /// Fill data structures of this class, utilizing bank structure
    /// Read until out of data or until decode says that the slot is finished
    /// len = ndata in event, pos = word number for block header in event
    fWordsSeen = 0;
    Clear();

    // How can set set this just once?
    //    fHeader = fSlot << 8;
    //    fHeaderMask = 0x3f00;

    while(fWordsSeen < len) {
      UInt_t index = pos + fWordsSeen;
      if( IsSlot(evbuffer[index]) ) {
        Decode(&evbuffer[index]);
        for( UInt_t ichan = 0; ichan < fNumChan; ichan++ ) {
          sldat->loadData(ichan, fDataArray[ichan], fDataArray[ichan]);
        }
        fWordsSeen += (fNumChan + 1);
        break;
      }
      fWordsSeen += (fNumChan + 1); // Skip to next header
    }
    return fWordsSeen;
  }

}

ClassImp(Decoder::GenScaler)
