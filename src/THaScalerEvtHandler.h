#ifndef THaScalerEvtHandler_
#define THaScalerEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   THaScalerEvtHandler
//   Class to handle Hall A scaler events (type 140)
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#define ICOUNT  1
#define IRATE   2
#define MAXCHAN  32
#define MAXTEVT 5000
#define defaultDT 4

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include "Rtypes.h"
#include "TTree.h"
#include "Decoder.h"
#include "THaEvData.h"
#include "THaVarList.h"
#include "VarDef.h"
#include "GenScaler.h"

using namespace Decoder;

class THaRunBase;

class ScalerLoc { // Utility class used by THaScalerEvtHandler
 public:
 ScalerLoc(TString nm, TString desc, Int_t isc, Int_t ich, Int_t iki) : 
   name(nm), description(desc), iscaler(isc), ichan(ich), ikind(iki) { };
  ~ScalerLoc();
  TString name, description;
  Int_t iscaler, ichan, ivar, ikind;
};

class THaScalerEvtHandler : public THaEvtTypeHandler {

public:

   THaScalerEvtHandler(const char*, const char*);
   virtual ~THaScalerEvtHandler();  

   Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time, Int_t idebug=0 );
   Int_t End( THaRunBase* r=0 );


private:

   void AddVars(TString name, TString desc, Int_t iscal, Int_t ichan, Int_t ikind);
   void DefVars();
   vector<string> vsplit(const string& s);
   size_t FindNoCase(const string sdata, const string skey);

   vector<GenScaler *> scalers;
   vector<ScalerLoc *> scalerloc;
   Double_t evcount;
   Int_t *rdata;
   vector<Int_t> index;
   Int_t Nvars, ifound;
   Double_t *dvars;
   TTree *fScalerTree;
   Double_t xdum1, xdum2, xdum3, xdum4;

   THaScalerEvtHandler(const THaScalerEvtHandler &fh);
   THaScalerEvtHandler& operator=(const THaScalerEvtHandler &fh);

   ClassDef(THaScalerEvtHandler,0)  // Scaler Event handler

};

#endif
