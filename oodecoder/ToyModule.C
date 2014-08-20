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
#include "Lecroy1877Module.h"
#include "Lecroy1881Module.h"
#include "Lecroy1875Module.h"
#include "Scaler560.h"
#include "Scaler1151.h"
#include "Scaler3800.h"
#include "Scaler3801.h"
#include "Fadc250Module.h"
#include "THaEvData.h"
#include "THaSlotData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

typedef ToyModule::TypeSet_t  TypeSet_t;
typedef ToyModule::TypeIter_t TypeIter_t;

TypeIter_t Lecroy1877Module::fgThisType = DoRegister( ToyModuleType( "Lecroy1877Module" , 1877));
TypeIter_t Lecroy1881Module::fgThisType = DoRegister( ToyModuleType( "Lecroy1881Module" , 1881));
TypeIter_t Lecroy1875Module::fgThisType = DoRegister( ToyModuleType( "Lecroy1875Module" , 1875));
TypeIter_t Scaler560::fgThisType = DoRegister( ToyModuleType( "Scaler560" , 560 ));
TypeIter_t Scaler1151::fgThisType = DoRegister( ToyModuleType( "Scaler1151" , 1151 ));
TypeIter_t Scaler3800::fgThisType = DoRegister( ToyModuleType( "Scaler3800" , 3800 ));
TypeIter_t Scaler3801::fgThisType = DoRegister( ToyModuleType( "Scaler3801" , 3801 ));
TypeIter_t Fadc250Module::fgThisType = DoRegister( ToyModuleType( "Fadc250Module" , 250 ));

// Add your module here.  



ToyModule::ToyModule(Int_t crate, Int_t slot) : fCrate(crate), fSlot(slot), fWordsExpect(0) { 
  fHeader=-1;
  fHeaderMask=-1;
  fWdcntMask=0;
  fWdcntShift=0;
  fDebugFile=0;
  fModelNum = -1;
}

ToyModule::~ToyModule() { 
}


ToyModule::ToyModule(const ToyModule& rhs) {
   Create(rhs);
}


ToyModule &ToyModule::operator=(const ToyModule& rhs) {
  if ( &rhs != this) {
    //    Uncreate();  // need a destructor method if we allocate memory 
    //                 (so far that's not the design)
    Create(rhs);
  }
  return *this;
} 

void ToyModule::Create(const ToyModule& rhs) {
  fCrate = rhs.fCrate;
  fSlot = rhs.fSlot;
  fWordsExpect = rhs.fWordsExpect;

}

void ToyModule::DoPrint() {
  if (fDebugFile) {
    *fDebugFile << "ToyModule   name = "<<fName<<endl;
    *fDebugFile << "ToyModule::    Crate  "<<fCrate<<"     slot "<<fSlot<<endl;
  }
}


//_____________________________________________________________________________
TypeSet_t& ToyModule::fgToyModuleTypes()
{
  // Local storage for all defined Module types. Initialize here on first use
  // (cf. http://www.parashift.com/c++-faq/static-init-order-on-first-use-members.html)

  static TypeSet_t* fgToyModuleTypes = new TypeSet_t;

  return *fgToyModuleTypes;
}

//_____________________________________________________________________________
TypeIter_t ToyModule::DoRegister( const ToyModuleType& info )
{
  // Add given info in fgBdataLocTypes

  if( !info.fClassName ||!*info.fClassName ) {
    ::Error( "ToyModule::DoRegister", "Attempt to register empty class name. "
	     "Coding error. Call expert." );
    return fgToyModuleTypes().end();
  }

  pair< TypeIter_t, bool > ins = fgToyModuleTypes().insert(info);

  if( !ins.second ) {
    ::Error( "ToyModule::DoRegister", "Attempt to register duplicate database "
	     "class \"%s\". Coding error. Call expert.", info.fClassName );
    return fgToyModuleTypes().end();
  }
  // NB: std::set guarantees that iterators remain valid on further insertions,
  // so this return value will remain good, unlike, e.g., std::vector iterators.
  return ins.first;
}

Bool_t ToyModule::IsSlot(UInt_t rdata) {
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

Int_t ToyModule::LoadSlot(THaSlotData *sldat, const Int_t* evbuffer, const Int_t *pstop) {
  const Int_t *p = evbuffer;
  if (fDebugFile) *fDebugFile << "ToyModule:: loadslot "<<endl; 
  if (!fHeader) cerr << "ToyModule::LoadSlot::ERROR : no header ?"<<endl;
  while (IsSlot( *p )) {
    if (p >= pstop) break;
    Decode(p);
    if (fDebugFile) *fDebugFile << "ToyModule:: loadData  "<<fChan<<"  "<<fData<<endl; 
    sldat->loadData(fChan, fData, fRawData);
    fWordsSeen++;
    p++;
  }
  return fWordsSeen;
}



ClassImp(ToyModule)
