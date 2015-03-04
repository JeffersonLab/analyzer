/////////////////////////////////////////////////////////////////////
//
//   SkeletonModule
//   This is an example of a module which a User may add.
//   See the header for more advice.
//
/////////////////////////////////////////////////////////////////////

#define LIKEV792 1

#include "SkeletonModule.h"
#include "THaSlotData.h"

using namespace std;

namespace Decoder {

Module::TypeIter_t SkeletonModule::fgThisType =
  DoRegister( ModuleType( "Decoder::SkeletonModule" , 4444 ));

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

#ifdef LIKEV792
Int_t SkeletonModule::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop) {
// This is a simple, default method for loading a slot
  const UInt_t *p = evbuffer;
  fWordsSeen = 0;
//  cout << "version like V792"<<endl;
  ++p;
  Int_t nword=*p-2;
  ++p;
  for (Int_t i=0;i<nword;i++) {
       ++p;
       UInt_t chan=((*p)&0x00ff0000)>>16;
       UInt_t raw=((*p)&0x00000fff);
       Int_t status = sldat->loadData("adc",chan,raw,raw);
       fWordsSeen++;
       if (chan < fData.size()) fData[chan]=raw;
//       cout << "word   "<<i<<"   "<<chan<<"   "<<raw<<endl;
       if( status != SD_OK ) return -1;
  }
  return fWordsSeen;
}
#endif

Int_t SkeletonModule::GetData(Int_t chan) const {
  if (chan < 0 || chan > fNumChan) return 0;
  return fData[chan];
}

void SkeletonModule::Clear(const Option_t *opt) {
  fNumHits = 0;
  for (Int_t i=0; i<fNumChan; i++) fData[i]=0;
}

}

ClassImp(Decoder::SkeletonModule)
