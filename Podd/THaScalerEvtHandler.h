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

class TTree;

static const UInt_t ICOUNT    = 1;
static const UInt_t IRATE     = 2;
static const UInt_t MAXCHAN   = 32;
static const UInt_t MAXTEVT   = 5000;
static const UInt_t defaultDT = 4;

class ScalerLoc { // Utility class used by THaScalerEvtHandler
 public:
  ScalerLoc(const TString& nm, const TString& desc, Int_t idx, Int_t sl,
      Int_t ich, Int_t iki, Int_t iv)
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


protected:

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
