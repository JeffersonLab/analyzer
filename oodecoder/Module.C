////////////////////////////////////////////////////////////////////
//
//   Module
//
//   Abstract class to help decode the module.
//   No module data is stored here; instead it is in THaSlotData
//   arrays, as it was traditionally.
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "Module.h"
#include "Lecroy1877Module.h"
#include "Lecroy1881Module.h"
#include "Lecroy1875Module.h"
#include "Scaler560.h"
#include "Scaler1151.h"
#include "Scaler3800.h"
#include "Scaler3801.h"
#include "Fadc250Module.h"
#include "F1TDCModule.h"
#include "THaEvData.h"
#include "THaSlotData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace Decoder;

typedef Module::TypeSet_t  TypeSet_t;
typedef Module::TypeIter_t TypeIter_t;

TypeIter_t Lecroy1877Module::fgThisType = DoRegister( ModuleType( "Decoder::Lecroy1877Module" , 1877));
TypeIter_t Lecroy1881Module::fgThisType = DoRegister( ModuleType( "Decoder::Lecroy1881Module" , 1881));
TypeIter_t Lecroy1875Module::fgThisType = DoRegister( ModuleType( "Decoder::Lecroy1875Module" , 1875));
TypeIter_t Scaler560::fgThisType = DoRegister( ModuleType( "Decoder::Scaler560" , 560 ));
TypeIter_t Scaler1151::fgThisType = DoRegister( ModuleType( "Decoder::Scaler1151" , 1151 ));
TypeIter_t Scaler3800::fgThisType = DoRegister( ModuleType( "Decoder::Scaler3800" , 3800 ));
TypeIter_t Scaler3801::fgThisType = DoRegister( ModuleType( "Decoder::Scaler3801" , 3801 ));
TypeIter_t Fadc250Module::fgThisType = DoRegister( ModuleType( "Decoder::Fadc250Module" , 250 ));
TypeIter_t F1TDCModule::fgThisType = DoRegister( ModuleType( "Decoder::F1TDCModule" , 1001 ));

// Add your module here.  



Module::Module(Int_t crate, Int_t slot) : fCrate(crate), fSlot(slot), fWordsExpect(0) { 
  fHeader=-1;
  fHeaderMask=-1;
  fWdcntMask=0;
  fWdcntShift=0;
  fDebugFile=0;
  fModelNum = -1;
  fName = "";
}

Module::~Module() { 
}


Module::Module(const Module& rhs) {
   Create(rhs);
}


Module &Module::operator=(const Module& rhs) {
  if ( &rhs != this) {
    //    Uncreate();  // need a destructor method if we allocate memory 
    //                 (so far that's not the design)
    Create(rhs);
  }
  return *this;
} 

void Module::Create(const Module& rhs) {
  fCrate = rhs.fCrate;
  fSlot = rhs.fSlot;
  fWordsExpect = rhs.fWordsExpect;

}

void Module::DoPrint() {
  if (fDebugFile) {
    *fDebugFile << "Module   name = "<<fName<<endl;
    *fDebugFile << "Module::    Crate  "<<fCrate<<"     slot "<<fSlot<<endl;
  }
}


//_____________________________________________________________________________
TypeSet_t& Module::fgModuleTypes()
{
  // Local storage for all defined Module types. Initialize here on first use
  // (cf. http://www.parashift.com/c++-faq/static-init-order-on-first-use-members.html)

  static TypeSet_t* fgModuleTypes = new TypeSet_t;

  return *fgModuleTypes;
}

//_____________________________________________________________________________
TypeIter_t Module::DoRegister( const ModuleType& info )
{
  // Add given info in fgBdataLocTypes

  if( !info.fClassName ||!*info.fClassName ) {
    ::Error( "Module::DoRegister", "Attempt to register empty class name. "
	     "Coding error. Call expert." );
    return fgModuleTypes().end();
  }

  pair< TypeIter_t, bool > ins = fgModuleTypes().insert(info);

  if( !ins.second ) {
    ::Error( "Module::DoRegister", "Attempt to register duplicate database "
	     "class \"%s\". Coding error. Call expert.", info.fClassName );
    return fgModuleTypes().end();
  }
  // NB: std::set guarantees that iterators remain valid on further insertions,
  // so this return value will remain good, unlike, e.g., std::vector iterators.
  return ins.first;
}

Bool_t Module::IsSlot(UInt_t rdata) {
  // may want to have "rules" like possibly 2 headers and 
  // a different rule for where to get fWordsExpect.
  // rules can be defined in the cratemap.
  // For some modules, fWordsExpect may be a property of the module.
  if ((rdata & fHeaderMask)==fHeader) {
    fWordsExpect = (rdata & fWdcntMask)>>fWdcntShift;
    return kTRUE;
  }
  if (fWordsSeen < fWordsExpect) return kTRUE;
  return kFALSE;
}

Int_t Module::LoadSlot(THaSlotData *sldat, const Int_t* evbuffer, const Int_t *pstop) {
  const Int_t *p = evbuffer;
  if (fDebugFile) *fDebugFile << "Module:: loadslot "<<endl; 
  if (!fHeader) cerr << "Module::LoadSlot::ERROR : no header ?"<<endl;
  while (IsSlot( *p )) {
    if (p >= pstop) break;
    Decode(p);
    if (fDebugFile) *fDebugFile << "Module:: loadData  "<<fChan<<"  "<<fData<<endl; 
    sldat->loadData(fChan, fData, fRawData);
    fWordsSeen++;
    p++;
  }
  return fWordsSeen;
}



ClassImp(Decoder::Module)
