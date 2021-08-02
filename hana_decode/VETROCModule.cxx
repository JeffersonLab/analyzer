/////////////////////////////////////////////////////////////////////
//
//   VETROCModule
//   for TDC readout
//
//   It has these data types:
//   //   0 block header
//   //   1 block trailer
//   //   2 event header
//   //   3 trigger time
//   //   8 pulse time
//   //   14 data not valid (empty module)
//   //   15 filler (non-data) word
//   
//
/////////////////////////////////////////////////////////////////////

#define LIKEV792 1

#include "VETROCModule.h"
#include "THaSlotData.h"

using namespace Decoder;
using namespace std;

Module::TypeIter_t VETROCModule::fgThisType =
  DoRegister( ModuleType( "Decoder::VETROCModule" , 526 ));

VETROCModule::VETROCModule(UInt_t crate, UInt_t slot) : VmeModule(crate, slot) {
  fDebugFile=0;
  Init();
}

VETROCModule::~VETROCModule() {

}

void VETROCModule::Init() {
  VmeModule::Init();
  fNumChan=128;
  for (UInt_t i=0; i<fNumChan; i++) fData.push_back(0);
  fDebugFile=0;
  Clear();
  IsInit = kTRUE;
  fName = "VETROC TDC Module";
}

#ifdef LIKEV792
UInt_t VETROCModule::LoadSlot(THaSlotData* sldat, const UInt_t* evbuffer,
			       const UInt_t* pstop ) {
  // pstop points to last word of data
const UInt_t* p = evbuffer;
fWordsSeen = 0;
p++; // Skip first word that has junk info
// First word is event header
UInt_t nwords = (*p)&0xff; // Get number of events that follow
p+=4; // Skip block header, event header, and 2 trigger times words (4 words total)
UInt_t wtype; // Variable to hold word type
UInt_t raw,chan,edge;
UInt_t status;
while(p < pstop) {
  wtype = ((*p)>>27)&0xf; // Word type bits 27-30
  if(wtype == 8) { // Found a TDC hit (skip anything else)
    edge = ((*p)>>26)&0x1;  // edge time
    chan = ((*p)>>16)&0xff; // channel
    raw = ((*p)>>0)&0xffff; // pulse data
    status = sldat->loadData("tdc",chan,raw,edge);
    if(status != SD_OK) return -1;

//cout << "Loaded...   (*p):   "<<hex<<(*p)<<"   nwords:   "<<nwords<<"   chan:   "<<chan<<"   raw:   "<<raw<<"   "<< "   edge:   "<<edge<<endl;
  
  }
  p++;
  fWordsSeen++;
 }
  return fWordsSeen;
}
#endif

UInt_t VETROCModule::GetData(UInt_t chan) const {
  if (chan < 0 || chan > fNumChan) return 0;
  return fData[chan];
}

void VETROCModule::Clear(const Option_t* opt) {
  VmeModule::Clear(opt);
  fNumHits = 0;
  for (UInt_t i=0; i<fNumChan; i++) fData[i]=0;
}

ClassImp(VETROCModule)
