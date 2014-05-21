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
#include "ToyModuleX.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

typedef ToyModule::TypeSet_t  TypeSet_t;
typedef ToyModule::TypeIter_t TypeIter_t;

TypeIter_t Lecroy1877Module::fgThisType = DoRegister( ToyModuleType( "Lecroy1877Module" ));
TypeIter_t ToyFastbusModule::fgThisType = DoRegister( ToyModuleType( "ToyFastbusModule" ));
TypeIter_t ToyModuleX::fgThisType = DoRegister( ToyModuleType( "ToyModuleX" ));

ToyModule::ToyModule(Int_t crate, Int_t slot) : fCrate(crate), fSlot(slot), fNumWords(0) { 
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
  fNumWords = rhs.fNumWords;

}

void ToyModule::DoPrint() {

  cout << "Module   name = "<<fName<<endl;
  cout << "Crate  "<<fCrate<<"     slot "<<fSlot<<endl;

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


ClassImp(ToyModule)
