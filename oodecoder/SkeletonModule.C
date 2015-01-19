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
  if (fTdcData) delete [] fTdcData;
}

void SkeletonModule::Init() {
  fTdcData = new Int_t[NTDCCHAN*MAXHIT];
  fDebugFile=0;
  Clear("");
  IsInit = kTRUE;
  fName = "Skeleton Module (example)";
}


Bool_t SkeletonModule::IsSlot(UInt_t rdata) {
  if (fDebugFile) *fDebugFile << "is slot ? "<<hex<<fHeader<<"  "<<fHeaderMask<<"  "<<rdata<<dec<<endl;
  return ((rdata != 0xffffffff) & ((rdata & fHeaderMask)==fHeader));
}

Int_t SkeletonModule::GetData(Int_t chan, Int_t hit) {
  Int_t idx = chan*MAXHIT + hit;
  if (idx < 0 || idx >= MAXHIT*NTDCCHAN) return 0;
  return fTdcData[idx];
}

void SkeletonModule::Clear(const Option_t *opt) { 
  fNumHits = 0;
  memset(fTdcData, 0, NTDCCHAN*MAXHIT*sizeof(Int_t));
}

Int_t SkeletonModule::LoadSlot(THaSlotData *sldat, const Int_t *evbuffer, const Int_t *pstop) {
// this increments evbuffer
  if (fDebugFile) *fDebugFile << "SkeletonModule:: loadslot "<<endl; 
  fWordsSeen = 0;
  const Int_t *loc;
  loc = evbuffer;

  while (loc <= pstop) {

    // want something like this
    // load slot data and store data in this class's array(s)
    // status = sldat->loadData("tdc",chan,raw,raw);
    // Int_t idx = chan*MAXHIT + ihit; 
    // if (idx >= 0 && idx < MAXHIT*NTDCCHAN) fTdcData[idx] = raw;
    // fWordsSeen++;

     loc++;
  }
 
  return fWordsSeen;
}


ClassImp(Decoder::SkeletonModule)
