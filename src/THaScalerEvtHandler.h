 #ifndef THaScalerEvtHandler_
#define THaScalerEvtHandler_

/////////////////////////////////////////////////////////////////////
//
//   THaScalerEvtHandler
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
#include "THaVarList.h"
#include "VarDef.h"
#include "GenScaler.h"

using namespace Decoder;

class THaRunBase;

class THaScalerEvtHandler : public THaEvtTypeHandler {

public:

   THaScalerEvtHandler(const char*, const char*);
   virtual ~THaScalerEvtHandler();  

   Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time, Int_t idebug=0 );
   Int_t End( THaRunBase* r=0 );


private:

   void AddVars(char *name, char *desc);
   void DefVars();
   vector<GenScaler *> scalers;
   Double_t evcount;
   Int_t *rdata;
   vector<Int_t> index;
   vector<RVarDef> rvars;
   Int_t Nvars;
   Double_t *dvars;
   ofstream *fDebugFile;
   TTree *fScalerTree;
   Double_t xdum1, xdum2, xdum3, xdum4;

   //   Double_t TSbcmu1, TSbcmu1r, TSbcmu3, TSbcmu3r;

   THaScalerEvtHandler(const THaScalerEvtHandler &fh);
   THaScalerEvtHandler& operator=(const THaScalerEvtHandler &fh);

   ClassDef(THaScalerEvtHandler,0)  // Scaler Event handler

};

#endif
