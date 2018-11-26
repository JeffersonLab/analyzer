#ifndef Podd_THaScalerEvtHandler_h_
#define Podd_THaScalerEvtHandler_h_

/////////////////////////////////////////////////////////////////////
//
//   THaScalerEvtHandler
//   Class to handle Hall A scaler events (type 140)
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "Decoder.h"
#include <vector>
#include "TString.h"  

class ScalerLoc;
class TTree;

class THaScalerEvtHandler : public THaEvtTypeHandler {

public:

   THaScalerEvtHandler(const char* name, const char* description);
   virtual ~THaScalerEvtHandler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=0 );


private:

   void AddVars(const TString& name, const TString& desc, Int_t iscal,
       Int_t ichan, Int_t ikind);
   void DefVars();

   std::vector<Decoder::GenScaler*> scalers;
   std::vector<ScalerLoc*> scalerloc;
   Double_t evcount;
   Int_t fNormIdx, fNormSlot;
   Double_t *dvars;
   TTree *fScalerTree;

   THaScalerEvtHandler(const THaScalerEvtHandler& fh);
   THaScalerEvtHandler& operator=(const THaScalerEvtHandler& fh);

   ClassDef(THaScalerEvtHandler,0)  // Scaler Event handler

};

#endif
