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
#include <string>
#include <vector>
#include "TTree.h"
#include "TString.h"  

class ScalerLoc { // Utility class used by THaScalerEvtHandler
 public:
  ScalerLoc(TString nm, TString desc, Int_t idx, Int_t sl, Int_t ich,
      Int_t iki, Int_t iv)
   : name(nm), description(desc), index(idx), islot(sl), ichan(ich),
     ikind(iki), ivar(iv) {};
  ~ScalerLoc();
  TString name, description;
  UInt_t index, islot, ichan, ikind, ivar;
};

class THaScalerEvtHandler : public THaEvtTypeHandler {

public:

   THaScalerEvtHandler(const char* name, const char* description);
   virtual ~THaScalerEvtHandler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=0 );


private:

   void AddVars(TString name, TString desc, Int_t iscal, Int_t ichan, Int_t ikind);
   void DefVars();

   std::vector<Decoder::GenScaler*> scalers;
   std::vector<ScalerLoc*> scalerloc;
   Double_t evcount;
   UInt_t *rdata;
   Int_t fNormIdx, fNormSlot;
   Double_t *dvars;
   TTree *fScalerTree;

   THaScalerEvtHandler(const THaScalerEvtHandler& fh);
   THaScalerEvtHandler& operator=(const THaScalerEvtHandler& fh);

   ClassDef(THaScalerEvtHandler,0)  // Scaler Event handler

};

#endif
