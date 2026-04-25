////////////////////////////////////////////////////////////////////
//
//   GenScaler
//   A generic scaler
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"
#include "THaSlotData.h"  // for THaSlotData
#include <cassert>        // for assert
#include <fstream>        // for ofstream
#include <iostream>       // for cout, cerr, endl, dec, hex, operator-, operator<<
#include <limits>         // for numeric_limits
#include <type_traits>    // for is_unsigned_v

using namespace std;

namespace Decoder {

  const int DEFAULT_DELTAT = 4;

  //___________________________________________________________________________
  GenScaler::GenScaler( UInt_t crate, UInt_t slot )
    : VmeModule(crate, slot)
    , fIsDecoded(false)
    , fFirstTime(true)
    , fDeltaT(0.0)
    , fClockChan(0)
    , fNumChanMask(0)
    , fNumChanShift(0)
    , fHasClock(false)
    , fClockRate(0)
    , fNormScaler(nullptr)
    , firsttime(true)
    , firstwarn(true)
  {
    fWordsExpect = 32;
    fNumChan = 0;
  }

  //___________________________________________________________________________
  void GenScaler::Clear( Option_t* opt ) {
    // Clear event-by-event data
    VmeModule::Clear(opt);
    // Do not clear fDataArray, fPrevArray, fRate here since they should
    // carry over between events
    fIsDecoded = false;
  }

  //___________________________________________________________________________
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
    fDataArray.assign(fWordsExpect, 0);
    fPrevData.assign(fWordsExpect, 0);
    fRate.assign(fWordsExpect, 0.0);
  }

  //___________________________________________________________________________
  void GenScaler::SetBank( Int_t bank ) {
    /// Define scaler header format for modules in banks
    fBank = bank;
    if(fBank > 0) {
      fHeader = fSlot << 8;
      fHeaderMask = 0xff00;
    }
  }

  //___________________________________________________________________________
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
	cerr << "GenScaler:: WARNING:  Declaring this object to have"<<endl;
	cerr << "   a clock even though this also has a normalization scaler ?"<<endl;
	cerr << "  This makes no sense. "<<endl;
	return -1;
      }
      fHasClock = true;
    }
    if( clockchan >= fWordsExpect ) {
      cerr << "GenScaler:: ERROR:  clock channel out of range "<<endl;
      return -1;
    }
    fClockChan = clockchan;
    fClockRate = clockrate;
    if( fClockRate > 0 )
      retcode = 1;
    if( deltaT > 0 ) {
      fDeltaT = deltaT;
      if( fClockRate <= 0 )
        retcode = 2;
    } else {
      cerr << "GenScaler:: Warning: using default deltaT = " << fDeltaT << endl;
    }
    if( retcode == 0 )
      cerr << "GetScaler:: Warning:: no deltaT nor clockrate defined for this scaler " << endl;
    return retcode;
  }

  //___________________________________________________________________________
  void GenScaler::LoadNormScaler(GenScaler *scal) {
    if (fHasClock) {
      cerr << "GenScaler:: WARNING:  loading norm scaler although this" << endl;
      cerr << "   object has a clock ?   This makes no sense. " << endl;
    }
    if( !scal ) {
      cerr << "GenScaler:: ERROR:  attempting to load a non-existent " << endl;
      cerr << "normalization scaler !" << endl;
      return;
    }
    fNormScaler = scal;
  }

  //___________________________________________________________________________
  Int_t GenScaler::Decode(const UInt_t *evbuffer) {
    Int_t doload = 0;
    Int_t nfound = 1;
    if( IsDecoded() )
      return nfound;
    if( fFirstTime ) {
      fFirstTime = false;
    } else {
      doload = 1;
      fPrevData.swap(fDataArray);
    }
    //if ( !IsSlot(*evbuffer) ) return nfound; // redundant, checked in LoadSlot
    assert(IsSlot(*evbuffer));
    evbuffer++;
    fIsDecoded = true;
    fDataArray.assign(evbuffer, evbuffer + fNumChan);
    nfound += fNumChan;
    if( doload )
      LoadRates();
    return nfound;
  }

  //___________________________________________________________________________
  namespace {
  template<typename T> requires std::is_unsigned_v<T>
  constexpr T DiffWithOverflow( T cur, T prev )
  {
    // Return difference between cur and prev scaler values (monotonically
    // increasing numbers), where cur may have wrapped around due to unsigned
    // integer overflow. The result is undefined if the actual difference is
    // larger than numeric_limits<T>::max(). This case cannot be detected.
    if( cur < prev ) {
      prev = numeric_limits<T>::max() - (prev - 1);
      return cur + prev;  //  by construction, this will not overflow
    }
    return cur - prev;
  }
  }

  //___________________________________________________________________________
  Double_t GenScaler::GetTimeSincePrev() const { // NOLINT(*-no-recursion)
    // Time since previous reading.
    // If a normalization scaler was defined, use its time base.
    // Otherwise, if this scaler has a clock, use it to get the time precisely.
    // Finally, if there is no clock, use fDeltaT as an approximate time.

    if( fNormScaler ) //FIXME generalize. Can have norm scaler AND clock
      return fNormScaler->GetTimeSincePrev();
    Double_t dtime = 0; //FIXME -> division by zero in LoadRates()
    const UInt_t ch = fClockChan;  // shorthand for readability
    if( IsDecoded() && fHasClock && fClockRate > 0 && checkchan(ch) ) {
      UInt_t clockdif = DiffWithOverflow(fDataArray[ch], fPrevData[ch]);
      dtime = clockdif / fClockRate;
    } else if( fDeltaT > 0. ) {
      dtime = fDeltaT; // default
    }
    return dtime;
  }

  //___________________________________________________________________________
  void GenScaler::LoadRates() {
    if( IsDecoded() ) {
      Double_t dtime = GetTimeSincePrev();
      if( dtime == 0. ) {
        fRate.assign(fWordsExpect, 0.);
        return;
      }
      for( UInt_t i = 0; i < fWordsExpect; i++ ) {
        UInt_t diff = DiffWithOverflow(fDataArray[i], fPrevData[i]);
        fRate[i] = diff / dtime;
      }
    }
  }

  //___________________________________________________________________________
  UInt_t GenScaler::GetData( UInt_t chan ) const {
    if( checkchan(chan) ) {
      return fDataArray[chan];
    }
    return 0;
  }

  //___________________________________________________________________________
  Double_t GenScaler::GetRate( UInt_t chan ) const {
    if( checkchan(chan) ) {
      return fRate[chan];
    }
    return 0;
  }

  //___________________________________________________________________________
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

  //___________________________________________________________________________
  void GenScaler::DebugPrint( ofstream* file ) const {
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

  //___________________________________________________________________________
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
      fNumChan = fWordsExpect;
      if( firsttime ) {
        firsttime = false;
        cout << "Warning::GenScaler:: (" << fCrate << "," << fSlot
             << ") using default num " << fgNumChanDefault << " channels"
             << endl;
      }
    }
    if( fNumChan != fWordsExpect ) {
      if( fNumChan > fWordsExpect )
        // Data are almost certainly garbage
        return false;
        //fNumChan = fWordsExpect;

      // Print warning to alert user to potential problems
      if( fNumChan != fWordsExpect ) {
        cout << "GenScaler:: ERROR:  (" << fCrate << "," << fSlot << ") "
             << "inconsistent number of chan, got " << fNumChan
             << ", expected " << fWordsExpect << endl;
        //        DoPrint();
        //firstwarn = false;
      }
    }
    return true;
  }

  //___________________________________________________________________________
  UInt_t GenScaler::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                              const UInt_t* pstop )
  {
    // This is a simple, default method for loading a slot
    // pstop points to last word of data
    static const char* const here = "LoadSlot";

    fWordsSeen = 0;
    Clear();
    if( !fHeader )
      return 0;
    const UInt_t* p = evbuffer;
    while( p < pstop ) {
      if( IsSlot(*p) ) {   // Sets fNumChan
        if( p + fNumChan > pstop ) {
          Error(here, "(%u,%u) Num channels %u points past end of buffer. "
                "Possibly unsupported data format. Ignoring data.",
                fCrate, fSlot, fNumChan);
          return 0;
        }
        Decode(p);
        if( sldat ) {
          for( UInt_t ichan = 0; ichan < fNumChan; ichan++ ) {
            sldat->loadData(ichan, fDataArray[ichan], fDataArray[ichan]);
          }
        }
        fWordsSeen = fNumChan;
        return fWordsSeen;
      }
      p++;
    }
    return 0;
  }

  //___________________________________________________________________________
  UInt_t GenScaler::LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer,
                              UInt_t pos, UInt_t len) {
    /// Fill data structures of this class, utilizing bank structure
    /// Read until out of data or until decode says that the slot is finished
    /// len = ndata in event, pos = word number for block header in event
    static const char* const here = "LoadSlot";

    fWordsSeen = 0;
    Clear();

    // How can we set this just once?
    //    fHeader = fSlot << 8;
    //    fHeaderMask = 0x3f00;
    if( !fHeader )
      return 0;

    while(fWordsSeen < len) {
      UInt_t index = pos + fWordsSeen;
      if( IsSlot(evbuffer[index]) ) {  // Sets fNumChan
        if( index + fNumChan >= pos + len ) {
          Error(here, "(%u,%u) Num channels %u points past end of buffer. "
                "Possibly unsupported data format. Disabling module.",
                fCrate, fSlot, fNumChan);
          return 0;
        }
        fWordsSeen += Decode(evbuffer + index);
        if( sldat ) {
          for( UInt_t ichan = 0; ichan < fNumChan; ichan++ ) {
            sldat->loadData(ichan, fDataArray[ichan], fDataArray[ichan]);
          }
        }
        break;
      }
      fWordsSeen += fNumChan + 1; // Skip to next header
    }
    return fWordsSeen;
  }

  //___________________________________________________________________________
}

ClassImp(Decoder::GenScaler)
