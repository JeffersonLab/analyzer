////////////////////////////////////////////////////////////////////
//
//   ToyModuleCollect
//
//  Collection of module types.
//
/////////////////////////////////////////////////////////////////////

#include "ToyModuleCollect.h"
#include "ToyModule.h"
#include "ToyFastbusModule.h"
#include "Lecroy1877Module.h"
#include "ToyModuleX.h"
#include "THaEvData.h"
#include "TMath.h"
#include "TClass.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyModuleCollect::ToyModuleCollect() { 
}

ToyModuleCollect::~ToyModuleCollect() {
}

Int_t ToyModuleCollect::Init() {
  // Create a list of the (finite number of) possible modules
  // If a new module comes into existance it must be added here.

   Int_t err=0;  

   for( ToyModule::TypeIter_t it = ToyModule::fgToyModuleTypes().begin();
       !err && it != ToyModule::fgToyModuleTypes().end(); ++it ) {
    const ToyModule::ToyModuleType& loctype = *it;

    // Get the ROOT class for this type
    cout << "loctype.fClassName  "<< loctype.fClassName<<endl;

    //    assert( loctype.fClassName && *loctype.fClassName );
    if( !loctype.fTClass ) {
      loctype.fTClass = TClass::GetClass( loctype.fClassName );

      cout << "loctype.fTClass "<<loctype.fTClass<<endl;
#ifdef THING
      if( !loctype.fTClass ) {
	// Probably typo in the call to ToyModule::DoRegister
        
	Error( Here(here), "No class defined for data type \"%s\". Programming "
	       "error. Call expert.", loctype.fClassName );
	err = -1;
	break;
      }
#endif
    }

    if( !loctype.fTClass->InheritsFrom( ToyModule::Class() )) {
      cout << "does not inherit from class"<<endl;

#ifdef THING
      Error( Here(here), "Class %s is not a ToyModule. Programming error. "
	     "Call expert.", loctype.fTClass->GetName() );
      err = -1;
      break;
#endif

    } else {
      cout << "DOES inherit from class"<<endl;
    }
   }    

  return ProcessCrateMap();  

}

Int_t ToyModuleCollect::ProcessCrateMap(void) {

  // Pretend we're reading in a cratemap and encounter these modules

  string strmodule[5];

  strmodule[0] = "1877";
  strmodule[1] = "1877";
  strmodule[2] = "ModuleX";
  strmodule[3] = "ModuleX";
  strmodule[4] = "1877";

  const ToyModule *modtype;

  for (Int_t islot=0; islot < 5; islot++) {

    // do something ...

  }

  return 1;
    

}

ClassImp(ToyModuleCollect)
