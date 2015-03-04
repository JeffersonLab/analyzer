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
#include <sstream>

using namespace std;

namespace Decoder {

  const int DEFAULT_DELTAT = 4;

GenScaler::GenScaler(Int_t crate, Int_t slot)
  : VmeModule(crate, slot), fIsDecoded(kFALSE), fDataArray(0), fPrevData(0),
    fRate(0), fNormScaler(0)
{
  fWordsExpect = 32;
}

void GenScaler::GenInit()
{
  fHasClock = kFALSE;
  fFirstTime = kTRUE;
  fIsDecoded = kFALSE;
  fClockChan = -1;
  fClockRate = 0;
  fNormScaler = 0;
  fNumChanMask = 0xf0;
  fDeltaT = DEFAULT_DELTAT;  // a default time interval between readings
  fDataArray = new Int_t[fWordsExpect];
  fPrevData = new Int_t[fWordsExpect];
  fRate = new Double_t[fWordsExpect];
  memset(fDataArray, 0, fWordsExpect*sizeof(Int_t));
  memset(fPrevData, 0, fWordsExpect*sizeof(Int_t));
  memset(fRate, 0, fWordsExpect*sizeof(Double_t));
}

GenScaler::~GenScaler() {
  delete [] fDataArray;
  delete [] fPrevData;
  delete [] fRate;
}

Int_t GenScaler::SetClock(Double_t deltaT, Int_t clockchan, Double_t clockrate) {
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
	  cout << "GenScaler:: WARNING:  Declaring this object to have"<<endl;
	  cout << "   a clock even though this also has a normalization scaler ?"<<endl;
	  cout << "  This makes no sense. "<<endl;
	  retcode = -1;
       }
       fHasClock = kTRUE;
    }
    if (clockchan < 0 || clockchan >= fWordsExpect) {
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
  if (IsSlot(*evbuffer)) {
    if (fFirstTime) {
      fFirstTime = kFALSE;
    } else {
      doload=1;
      memcpy(fPrevData, fDataArray, fWordsExpect*sizeof(Int_t));
    }
    if (fDebugFile) *fDebugFile << "is slot 0x"<<hex<<*evbuffer<<dec<<endl;
    fIsDecoded = kTRUE;
    evbuffer++;
    for (Int_t i=0; i<fWordsExpect; i++) {
      fDataArray[i] = *(evbuffer++);
      nfound++;
      if (fDebugFile) *fDebugFile << "   data["<<i<<"] = 0x"<<hex<<fDataArray[i]<<dec<<endl;
    }
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
    dtime = (fDataArray[fClockChan]-fPrevData[fClockChan])/fClockRate;
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
       memset(fRate, 0, fWordsExpect*sizeof(Double_t));
       return;
    }
    for (Int_t i=0; i<fWordsExpect; i++) {
      fRate[i] = (fDataArray[i]-fPrevData[i])/dtime;
    }
  }
}

Int_t GenScaler::GetData(Int_t chan) const {
  if (checkchan(chan)) {
    return fDataArray[chan];
  } else {
    return 0;
  }
}

Double_t GenScaler::GetRate(Int_t chan) const {
  if (checkchan(chan)) {
    return fRate[chan];
  } else {
    return 0;
  }
}

void GenScaler::DoPrint() const {
  cout << "GenScaler::   crate "<<fCrate<<"   slot "<<fSlot<<endl;
  cout << "GenScaler::   Header 0x"<<hex<<fHeader<<"    Mask  0x"<<fHeaderMask<<dec<<endl;
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
  for (Int_t i=0; i<fWordsExpect; i++) {
     *file << "  0x"<<hex<<fDataArray[i]<<"   0x"<<fPrevData[i]<<dec<<"   "<<fRate[i]<<endl;
  }
}

Bool_t GenScaler::IsSlot(UInt_t rdata) {
  Bool_t result;
  result = ((rdata & fHeaderMask)==fHeader);
  if (result) {
    fNumChan = rdata&fNumChanMask;
    if (fNumChan != fWordsExpect) {
	cout << "GenScaler:: ERROR:  Inconsistent number of chan"<<endl;
	if (fNumChan > fWordsExpect) fNumChan = fWordsExpect;
    }
  }
  return result;
}

Int_t GenScaler::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop) {
// This is a simple, default method for loading a slot
  const UInt_t *p = evbuffer;
  while ( p < pstop ) {
    if (IsSlot( *p )) {
      if (fDebugFile) *fDebugFile << "GenScaler:: Loadslot "<<endl;
      if (!fHeader) cerr << "GenScaler::LoadSlot::ERROR : no header ?"<<endl;
      Decode(p);
      for (Int_t ichan = 0; ichan < fNumChan; ichan++) {
	sldat->loadData(ichan, fDataArray[ichan], fDataArray[ichan]);
      }
      fWordsSeen = fNumChan;
      return fWordsSeen;
    }
  p++;
  }
  return 0;
}

}

ClassImp(Decoder::GenScaler)
