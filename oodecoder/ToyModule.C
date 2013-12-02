////////////////////////////////////////////////////////////////////
//
//   ToyModule
//
/////////////////////////////////////////////////////////////////////

#include "ToyModule.h"
#include "THaEvData.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyModule::ToyModule() { 
}

ToyModule::~ToyModule() { 
}

Int_t ToyModule::Decode(THaEvData *evdata, Int_t jstart) {
  std::cout<<"ToyModule decode from "<<jstart<<std::endl;
  
  return 0;
}

ToyModule::ToyModule(const ToyModule& rhs) 
{
  Create (rhs);
}; 

ToyModule &ToyModule::operator=(const ToyModule &rhs)
{
  if ( &rhs != this )
    {
      Uncreate();
      Create (rhs);
    }
  return *this;
};

void ToyModule::Create(const ToyModule& rhs)
{
  // new something ...
};

void ToyModule::Uncreate()
{
  //   delete [] something; // ...

};

Bool_t ToyModule::IsSlot(Int_t slot, Int_t rdata) {
  return kTRUE;
}




ClassImp(ToyModule)
