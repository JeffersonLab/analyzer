////////////////////////////////////////////////////////////////////
//
//   ToyPhysicsEvtHandler
//
/////////////////////////////////////////////////////////////////////

#include "ToyEvtTypeHandler.h"
#include "ToyPhysicsEvtHandler.h"
#include "ToyModule.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

ToyPhysicsEvtHandler::ToyPhysicsEvtHandler() { 
}

ToyPhysicsEvtHandler::~ToyPhysicsEvtHandler() { 
}

Int_t ToyPhysicsEvtHandler::Decode(THaEvData *evdata) {

  // Break evdata's evbuffer into rocs
  // Find all the slots.

  // Pass the buffer for that slot to the module belonging to it.

  // Toy code.

   Int_t nroc=4;  
   Int_t irn[4], istart[4], iend[4];
   irn[0] = 1; irn[1] = 2; irn[2] = 5; irn[3] = 8; // list of rocs found
   for (Int_t i = 0; i < 4; i++) { istart[i]=0; iend[i]=10; }
   Int_t first_slot_used = 0, n_slots_done = 0;
   Int_t Nslot = 20;  // will depend on cratemap info
   Int_t iroc, slot, n_slots_checked;
   n_slots_checked = 0;

   for (iroc = 0; iroc < nroc; iroc++) {
       
     Int_t roc = irn[iroc];

     for (slot=first_slot_used; n_slots_checked<Nslot-n_slots_done; slot++) {

       n_slots_done++;

       for (Int_t index = istart[iroc]; index < iend[iroc]; index++) {

	 Int_t idx = roc*100 + slot;  // need to fix this
         if (evdata->GetSlot(idx)->GetModule()->Found(evdata->GetRawData(index))) {
	   evdata->GetSlot(idx)->GetModule()->Decode(evdata, index);
               // module::Decode starts at "index" and increments "index".
               // and does  "evdata->crateslot->loadData()"
	 }
       }
     }
   }

  return 0;
}

void ToyPhysicsEvtHandler::Init(THaCrateMap *map) {

}


ClassImp(ToyPhysicsEvtHandler)
