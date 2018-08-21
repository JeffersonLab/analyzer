////////////////////////////////////////////////////////////////////
//
//   Module
//
//   Abstract class to help decode the module.
//   No module data is stored here; instead it is in THaSlotData
//   arrays, as it was traditionally.
//
/////////////////////////////////////////////////////////////////////

#include "Module.h"
#include "THaSlotData.h"
#include "TError.h"

using namespace std;

namespace Decoder {

Module::Module()
  : fCrate(0), fSlot(0), fHeader(0), fHeaderMask(0xffffffff), fBank(-1),
    fWordsExpect(0), fWordsSeen(0), fWdcntMask(0), fWdcntShift(0),
    fModelNum(-1), fNumChan(0), fMode(0), block_size(1), IsInit(false),
    fMultiBlockMode(false), fBlockIsDone(false), fFirmwareVers(0),
    fDebugFile(0), fExtra(0)
{
}

Module::Module(Int_t crate, Int_t slot)
  : fCrate(crate), fSlot(slot), fHeader(0), fHeaderMask(0xffffffff), fBank(-1),
    fWordsExpect(0), fWordsSeen(0), fWdcntMask(0), fWdcntShift(0),
    fModelNum(-1), fNumChan(0), fMode(0), block_size(1), IsInit(false),
    fMultiBlockMode(false), fBlockIsDone(false), fFirmwareVers(0),
    fDebugFile(0), fExtra(0)
{
  // Warning: see comments at Init()
  fName = "";
}

Module::~Module() {
}


void Module::Init() {
// Suggestion: call this Init() before calling the inherting class's Init.
// Otherwise some variables may be undefined.  The "factory" method
// using TClass::New does NOT call the c'tor of this base class !!
// Make sure all your variables are defined.
  fHeader=0;
  fHeaderMask=0xffffffff;
  fBank=-1;
  fWordsSeen = 0;
  fWordsExpect = 0;
  fWdcntMask=0;
  fWdcntShift=0;
  fDebugFile=0;
  fModelNum = -1;
  fName = "";
  fNumChan = 0;
}


Bool_t Module::IsSlot(UInt_t rdata) {
// Simplest version of IsSlot relies on a unique header.
 if ((rdata & fHeaderMask)==fHeader) {
    fWordsExpect = (rdata & fWdcntMask)>>fWdcntShift;
    return kTRUE;
  }
  return kFALSE;
}

void Module::DoPrint() const {
  if (fDebugFile) {
    *fDebugFile << "Module   name = "<<fName<<endl;
    *fDebugFile << "Module::    Crate  "<<fCrate<<"     slot "<<fSlot<<endl;
    *fDebugFile << "Module::    fWdcntMask "<<hex<<fWdcntMask<<dec<<endl;
    *fDebugFile << "Module::    fHeader "<<hex<<fHeader<<endl;
    *fDebugFile << "Module::    fHeaderMask "<<hex<<fHeaderMask<<dec<<endl;
    *fDebugFile << "Module::    num chan "<<fNumChan<<endl;
  }
}


//_____________________________________________________________________________
Module::TypeSet_t& Module::fgModuleTypes()
{
  // Local storage for all defined Module types. Initialize here on first use
  // (cf. http://www.parashift.com/c++-faq/static-init-order-on-first-use-members.html)

  static TypeSet_t* fgModuleTypes = new TypeSet_t;

  return *fgModuleTypes;
}

//_____________________________________________________________________________
Module::TypeIter_t Module::DoRegister( const ModuleType& info )
{
  // Add given info in fgModuleTypes

  if( !info.fClassName ||!*info.fClassName ) {
    ::Error( "Module::DoRegister", "Attempt to register empty class name. "
	     "Coding error. Call expert." );
    return fgModuleTypes().end();
  }

  pair< TypeIter_t, bool > ins = fgModuleTypes().insert(info);

  if( !ins.second ) {
    ::Error( "Module::DoRegister", "Attempt to register duplicate decoder module "
	     "class \"%s\". Coding error. Call expert.", info.fClassName );
    return fgModuleTypes().end();
  }
  // NB: std::set guarantees that iterators remain valid on further insertions,
  // so this return value will remain good, unlike, e.g., std::vector iterators.
  return ins.first;
}

Int_t Module::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer,
			  const Int_t pos, const Int_t len)
{
  // Load slot from pos to pos+len

  // Basic example of how this could be done
  // if (fDebugFile) {
  //      *fDebugFile << "Module:: Loadslot "<<endl;
  //      *fDebugFile << "pos"<<dec<<pos<<"   len "<<len<<endl;
  // }
  // fWordsSeen=0;
  // while ( fWordsSeen<len ) {
  //   if (fDebugFile) *fDebugFile <<endl;
  //   for (size_t ichan = 0, nchan = GetNumChan(); ichan < nchan; ichan++) {
  //     Int_t mdata,rdata;
  //     rdata = evbuffer[pos+fWordsSeen];
  //     mdata = rdata;
  //     sldat->loadData(ichan, mdata, rdata);
  //     if (ichan < fData.size()) fData[ichan]=rdata;
  //     fWordsSeen++;
  //   }
  // }
  // return fWordsSeen;

  // By default, this is just a wrapper for the 3-parameter version of LoadSlot,
  // which every module is required to implement

  return LoadSlot(sldat, evbuffer+pos, evbuffer+pos+len);
}

}

ClassImp(Decoder::Module)
