/////////////////////////////////////////////////////////////////////
//
//   SkeletonModule
//   This is an example of a module which a User may add.
//   See the header for more advice.
//
/////////////////////////////////////////////////////////////////////

#include "SkeletonModule.h"
#include "VmeModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace Decoder;

SkeletonModule::SkeletonModule(Int_t crate, Int_t slot) : VmeModule(crate, slot) {
  fDebugFile=0;
  Init();
}

SkeletonModule::~SkeletonModule() { 

}

void SkeletonModule::Init() {
  Module::Init();
  fNumChan=32;
  for (Int_t i=0; i<fNumChan; i++) fData.push_back(0);
  fDebugFile=0;
  Clear("");
  IsInit = kTRUE;
  fName = "Skeleton Module (example)";
}

Int_t SkeletonModule::GetData(Int_t chan) {
  if (chan < 0 || chan > fNumChan) return 0;
  return fData[chan];
}

void SkeletonModule::Clear(const Option_t *opt) { 
  fNumHits = 0;
  for (Int_t i=0; i<fNumChan; i++) fData[i]=0;
}



ClassImp(Decoder::SkeletonModule)
