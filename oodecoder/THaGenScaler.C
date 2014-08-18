////////////////////////////////////////////////////////////////////
//
//   THaGenScaler
//   A generic scaler
//
/////////////////////////////////////////////////////////////////////

#include "THaGenScaler.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;


THaGenScaler::THaGenScaler(Int_t crate, Int_t slot, Int_t numchan) : ToyModule(crate, slot) { 
  fHasClock = kFALSE;
  fFirstTime = kTRUE;
  fClockChan = -1;
  fClockRate = 0;
  fNormScaler = 0;  
  fNumChanMask = 0xf0;
  fDeltaT = DEFAULT_DELTAT;  // a default time interval between readings
  fWordsExpect = numchan;  
  fDataArray = new Int_t[fWordsExpect];
  fPrevData = new Int_t[fWordsExpect];
  fRate = new Double_t[fWordsExpect];
  memset(fDataArray, 0, fWordsExpect*sizeof(Int_t));
  memset(fPrevData, 0, fWordsExpect*sizeof(Int_t));
  memset(fRate, 0, fWordsExpect*sizeof(Double_t));
}


THaGenScaler::~THaGenScaler() { 
  delete [] fDataArray;
  delete [] fPrevData;
  delete [] fRate;
}

Int_t THaGenScaler::SetClock(Double_t deltaT, Int_t clockchan, Double_t clockrate) {
  // Sets the clock for the time base
  // retcode:
  //     0   nothing wrong, but has no deltaT nor clock data. (a bit odd)
  //    -1   something wrong, see error print
  //     1   clock rate set for this scaler.  This scaler has a clock.
  //     2   using deltaT, instead, for an approx time diff between readings

    Int_t retcode = 0;
    fHasClock = kFALSE;
    if (clockrate > 0) {
       if (fNormScaler) { 
          cout << "THaGenScaler:: WARNING:  Declaring this object to have"<<endl;
          cout << "   a clock even though this also has a normalization scaler ?"<<endl;
          cout << "  This makes no sense. "<<endl;
          retcode = -1;
       }
       fHasClock = kTRUE;
    }
    if (clockchan < 0 || clockchan >= fWordsExpect) {
       cout << "THaGenScaler:: ERROR:  clock channel out of range "<<endl;
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
      cout << "THaGenScaler:: Warning: using default deltaT = "<<fDeltaT<<endl;
    }
    if (retcode == 0) cout << "THaGetScaler:: Warning:: no deltaT nor clockrate define for this scaler "<<endl;
    return retcode;
}

void THaGenScaler::LoadNormScaler(THaGenScaler *scal) {
  if (fHasClock) { 
    cout << "THaGenScaler:: WARNING:  loading norm scaler although this"<<endl;
    cout << "   object has a clock ?   This makes no sense. "<<endl;
  }
  if ( !scal ) {
    cout << "THaGenScaler:: ERROR:  attempting to load a non-existent "<<endl;
    cout << "normalization scaler !" <<endl;
    return;
  }    
  fNormScaler = scal;
}

Int_t THaGenScaler::Decode(const Int_t *evbuffer) {
  // Return arg:
  //     0 = did not decode this module since slot not found yet.
  //     1 = slot found, decode finished.
  Int_t ldebug=1;
  Int_t doload=0;
  Int_t nfound=1;
  if (IsDecoded()) return nfound;
  if (IsSlot(*evbuffer)) {
    if (fFirstTime) {
      fFirstTime = kFALSE;
    } else {
      doload=1;
      memcpy(fPrevData, fDataArray, fWordsExpect*sizeof(Int_t));
    }
    if (ldebug) cout << "is slot 0x"<<hex<<*evbuffer<<dec<<endl;
    fIsDecoded = kTRUE;
    evbuffer++;
    for (Int_t i=0; i<fWordsExpect; i++) {
      fDataArray[i] = *(evbuffer++);
      nfound++;
      if (ldebug) cout << "   data["<<i<<"] = 0x"<<hex<<fDataArray[i]<<dec<<endl;
    }
  }
  if (doload) LoadRates();
  return nfound;
}

Double_t THaGenScaler::GetTimeSincePrev() {
// Time since previous reading.
// If a normalization scaler was defined, use its time base.
// Otherwise, if this scaler has a clock, use it to get the time precisely.
// Finally, if there is no clock, use fDeltaT as an approximate time.

  Int_t ldebug=0;  
  if (fNormScaler) return fNormScaler->GetTimeSincePrev();
  Double_t dtime = 0;
  if (ldebug) {
     cout << "Into GetTimeSincePrev "<<endl;
     if (IsDecoded()) cout << "Is Decoded "<<endl;
     if (fHasClock) cout << "has Clock "<<endl;
  }
  if (IsDecoded() && fHasClock && fClockRate>0 && checkchan(fClockChan)) {
    dtime = (fDataArray[fClockChan]-fPrevData[fClockChan])/fClockRate;
    if (ldebug) cout << "GetTimeSincePrev  "<<fClockRate<<"   "<<fClockChan<<"   "<<dtime<<endl;
  } else {
    if (fDeltaT > 0) dtime = fDeltaT;   // default
  }
  return dtime;
}

void THaGenScaler::LoadRates() {
  cout << "LoadRates "<<endl;
  if (IsDecoded()) {
    Double_t dtime = GetTimeSincePrev();
    cout << "dtime = "<<dtime<<endl;
    if (dtime==0) {
       memset(fRate, 0, fWordsExpect*sizeof(Double_t));
       return;
    }
    for (Int_t i=0; i<fWordsExpect; i++) {
      fRate[i] = (fDataArray[i]-fPrevData[i])/dtime;
    }
  }
}

Int_t THaGenScaler::GetData(Int_t chan) {
  if (checkchan(chan)) {
    return fDataArray[chan];
  } else {
    return 0;
  }
}

Double_t THaGenScaler::GetRate(Int_t chan) {
  if (checkchan(chan)) {
    return fRate[chan];
  } else {
    return 0;
  }
}

void THaGenScaler::DoPrint() {
  cout << "THaGenScaler::   crate "<<fCrate<<"   slot "<<fSlot<<endl;
  cout << "THaGenScaler::   Header 0x"<<hex<<fHeader<<"    Mask  0x"<<fHeaderMask<<dec<<endl;
  cout << "num words expected  "<<fWordsExpect<<endl;
  if (fHasClock) cout << "Has a clock"<<endl;
  if (fNormScaler) cout << "Using norm scaler with ptr = "<<fNormScaler << endl;
  cout << "Clock channel "<<fClockChan<<"   clock rate "<<fClockRate<<endl;
}

void THaGenScaler::DebugPrint(ofstream *file) {
  if (!file) return;
  *file<<"----  Data ---- "<<fWordsExpect<<endl;
  *file<<"Data now   //   previous    //   rate  "<<endl;
  for (Int_t i=0; i<fWordsExpect; i++) {
     *file << "  0x"<<hex<<fDataArray[i]<<"   0x"<<fPrevData[i]<<dec<<"   "<<fRate[i]<<endl;
  }
}

Bool_t THaGenScaler::IsSlot(Int_t rdata) {
  Bool_t result;
  Int_t numchan;
  result = ((rdata & fHeaderMask)==fHeader);
  if (result) {
    numchan = rdata&fNumChanMask;
    if (numchan != fWordsExpect) 
         cout << "THaGenScaler:: ERROR:  Inconsistent number of chan"<<endl;
  }
  return result;
}


ClassImp(THaGenScaler)
