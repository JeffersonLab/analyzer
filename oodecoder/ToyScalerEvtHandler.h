#ifndef ToyScalerEvtHandler_
#define ToyScalerEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   ToyScalerEvtHandler
//   Class to handle Hall A scaler events (type 140)
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"

class ToyScalerEvtHandler : public ToyEvtTypeHandler {

public:

   ToyScalerEvtHandler();
   virtual ~ToyScalerEvtHandler();  

   Int_t Decode(THaEvData *evdata);
   Int_t Init(THaCrateMap *map);


private:

   ToyScalerEvtHandler(const ToyScalerEvtHandler &fh);
   ToyScalerEvtHandler& operator=(const ToyScalerEvtHandler &fh);

   ClassDef(ToyScalerEvtHandler,0)  // Scaler Event handler

};

#endif
