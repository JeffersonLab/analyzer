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
#include <iostream>
#include <fstream>
#include <vector>
#include "Rtypes.h"
#include "TTree.h"
#include "Decoder.h"
#include "THaEvData.h"
#include "GenScaler.h"

using namespace Decoder;

class THaRunBase;

class ToyScalerEvtHandler : public ToyEvtTypeHandler {

public:

   ToyScalerEvtHandler(const char*, const char*);
   virtual ~ToyScalerEvtHandler();  

   Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time, Int_t idebug=0 );
   Int_t End( THaRunBase* r=0 );


private:

   vector<GenScaler *> scalers;
   Double_t evnum;
   Int_t *rdata;
   ofstream *fDebugFile;
   TTree *fScalerTree;

   Double_t TSbcmu1, TSbcmu1r, TSbcmu3, TSbcmu3r;

   ToyScalerEvtHandler(const ToyScalerEvtHandler &fh);
   ToyScalerEvtHandler& operator=(const ToyScalerEvtHandler &fh);

   ClassDef(ToyScalerEvtHandler,0)  // Scaler Event handler

};

#endif
