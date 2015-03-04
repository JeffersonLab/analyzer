#ifndef THaScalerEvtHandler_
#define THaScalerEvtHandler_

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
#include "TString.h"   // really both std::string and TString?

class ScalerLoc { // Utility class used by THaScalerEvtHandler
 public:
 ScalerLoc(TString nm, TString desc, Int_t isc, Int_t ich, Int_t iki) :
   name(nm), description(desc), iscaler(isc), ichan(ich), ikind(iki) { };
  ~ScalerLoc();
  TString name, description;
  UInt_t iscaler, ichan, ivar, ikind;
};

class THaScalerEvtHandler : public THaEvtTypeHandler {

public:

   THaScalerEvtHandler(const char*, const char*);
   virtual ~THaScalerEvtHandler();

   Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=0 );
   virtual void SetDebugFile(std::ofstream *file);


private:

   void AddVars(TString name, TString desc, Int_t iscal, Int_t ichan, Int_t ikind);
   void DefVars();
   static size_t FindNoCase(const std::string& sdata, const std::string& skey);

   std::vector<Decoder::GenScaler*> scalers;
   std::vector<ScalerLoc*> scalerloc;
   Double_t evcount;
   UInt_t *rdata;
   std::vector<Int_t> index;
   Int_t Nvars, ifound, fNormIdx, nscalers;
   Double_t *dvars;
   TTree *fScalerTree;

   THaScalerEvtHandler(const THaScalerEvtHandler& fh);
   THaScalerEvtHandler& operator=(const THaScalerEvtHandler& fh);

   ClassDef(THaScalerEvtHandler,0)  // Scaler Event handler

};

#endif
