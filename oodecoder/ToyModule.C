////////////////////////////////////////////////////////////////////
//
//   ToyModule
//
//   Abstract class to help decode the module.
//   No module data is stored here; instead it is in THaSlotData
//   arrays, as it was traditionally.
//
/////////////////////////////////////////////////////////////////////

#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyModule::ToyModule() { 
}

ToyModule::ToyModule(Int_t crate, Int_t slot) : fCrate(crate), fSlot(slot), fNumWord(0) { 
}

ToyModule::~ToyModule() { 
}


ToyModule::ToyModule(const ToyModule& rhs) {
   Create(rhs);
}


ToyModule::ToyModule::operator=(const ToyModule& rhs) {
  if ( &rhs != this) {
    //    Uncreate();  // need a destructor method if we allocate memory 
    //                 (so far that's not the design)
    Create(rhs);
  }
  return *this;
} 

void Create(const ToyModule& rhs) {
  fCrate = rhs.fCrate;
  fSlot = rhs.fSlot;
  fNumWord = rhs.fNumWord;

}


ClassImp(ToyModule)
