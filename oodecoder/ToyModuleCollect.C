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
  
  fModuleList->insert(make_pair("1877",new LeCroy1877Module()));
  fModuleList->insert(make_pair("ModuleX",new ToyModuleX()));

  ProcessCrateMap();  

}

Int_t ToyModuleCollect::ProcessCrateMap() {

  // Pretend we're reading in a cratemap and encounter these modules

  string strmodule[5];

  strmodule[0] = "1877";
  strmodule[1] = "1877";
  strmodule[2] = "ModuleX";
  strmodule[3] = "ModuleX";
  strmodule[4] = "1877";

  const ToyModule& modtype;

  for (Int_t islot=0; islot < 5; islot++) {

    map< string, ToyModule *module >::const_iterator it =
           fModuleList.find(strmodule[islot]);
  
    if (it != fModuleList.end()) modtype = it->second;

    cout << "modtype.fClassName "<<modtype.fClassName<<endl;
    cout << "modtype.fName "<<modtype.fName<<endl;
    cout << "modtype.fTClass "<<modtype.fTClass<<endl;

  }
    

}

ClassImp(ToyModuleCollect)
