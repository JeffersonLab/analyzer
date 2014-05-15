////////////////////////////////////////////////////////////////////
//
//   ToyFastbusModule
//
/////////////////////////////////////////////////////////////////////

#include "DecoderGlobals.h"
#include "ToyFastbusModule.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include "Rtypes.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

ToyFastbusModule::ToyFastbusModule(Int_t crate, Int_t slot) : ToyModule(crate, slot) {
  fNoWdCnt=1;
  if (fCrate < 0 || fCrate > MAXROC) {
       cerr << "ERROR: crate out of bounds"<<endl;
       fCrate = -1;
  }
  if (fSlot < 0 || fSlot > MAXSLOTS_FASTBUS) {
       cerr << "ERROR: slot out of bounds"<<endl;
       fSlot = -1;
  }
}

ToyFastbusModule::ToyFastbusModule(const ToyFastbusModule& rhs) {
   Create(rhs);
}


ToyFastbusModule &ToyFastbusModule::operator=(const ToyFastbusModule& rhs) {
  if ( &rhs != this) {
    //    Uncreate();  // need a destructor method if we allocate memory 
    //                 (so far that's not the design)
    Create(rhs);
  }
  return *this;
} 

void ToyFastbusModule::Create(const ToyFastbusModule& rhs) {
  ToyModule::Create(rhs);
  fSlotMask = rhs.fSlotMask;
  fSlotShift = rhs.fSlotShift;
  fChanMask = rhs.fChanMask;
  fChanShift = rhs.fChanShift;
  fDataMask = rhs.fDataMask;
  fDataShift = rhs.fDataShift;
}


ClassImp(ToyFastbusModule)
