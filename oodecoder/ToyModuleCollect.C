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

Int_t ToyModuleCollect::Init(void) {
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

      cout << "Stuff "<<loctype.fClassName<<"    "<< TClass::GetClass( loctype.fClassName )<<endl;

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


    cout << "eeee "<<ToyModule::Class()<<endl;

    if (loctype.fTClass) {

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
    }

    return ProcessCrateMap();  

}


Int_t ToyModuleCollect::ProcessCrateMap(void) {

  // Pretend we're reading in a cratemap and encounter these modules

  TString strmodule[5], modname[5];

  strmodule[0] = "Lecroy1877Module";        modname[0] = "fred";
  strmodule[1] = "ToyFastbusModule";        modname[1] = "bob";
  strmodule[2] = "ToyModuleX";              modname[2] = "hip";
  strmodule[3] = "ToyModuleX";              modname[3] = "norf";
  strmodule[4] = "Lecroy1877Module";        modname[4] = "abddef";

  ToyModule *locmodule;

    Int_t err=0;  

    cout << "ProcessCrateMap ++++++++++++++++++++++++++++ "<<endl;

  for (Int_t islot=0; islot < 5; islot++) {

    for( ToyModule::TypeIter_t it = ToyModule::fgToyModuleTypes().begin();
       !err && it != ToyModule::fgToyModuleTypes().end(); ++it ) {
    const ToyModule::ToyModuleType& loctype = *it;

    // Get the ROOT class for this type
    cout << "loctype.fClassName  "<< loctype.fClassName<<endl;

    if (strmodule[islot] == loctype.fClassName) {

      cout << "Found Module !!!! "<<modname[islot]<<endl;
      cout << "fTClass ptr =  "<<loctype.fTClass<<endl;

      locmodule = static_cast<ToyModule*>( loctype.fTClass->New() );

#ifdef THING1
      if (strmodule[islot]=="Lecroy1877Module") locmodule = new Lecroy1877Module();
      if (strmodule[islot]=="ToyFastbusModule") locmodule = new ToyFastbusModule();
      if (strmodule[islot]=="ToyModuleX") locmodule = new ToyModuleX();
#endif

      cout << "Class name "<<loctype.fTClass->GetName()<<endl;

      cout << "=====================   module pointer "<<locmodule<<"     slot "<<islot<<endl;

      fModuleList.Add(locmodule);

    }
    


    }
  }

  cout << "\n\n Size of fModuleList "<<fModuleList.GetSize()<<endl;

  return 1;
    

}


ClassImp(ToyModuleCollect)
