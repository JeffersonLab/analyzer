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
#include "TString.h"
#include <vector>
#include <string>

class TTree;

static const UInt_t ICOUNT    = 1;
static const UInt_t IRATE     = 2;
static const UInt_t MAXCHAN   = 32;
static const UInt_t MAXTEVT   = 5000;
static const UInt_t defaultDT = 4;

class ScalerLoc { // Utility class used by THaScalerEvtHandler
 public:
  ScalerLoc( TString nm, TString desc, UInt_t idx, UInt_t sl,
             UInt_t ich, UInt_t iki )
    : name(std::move(nm))
    , description(std::move(desc))
    , index(idx)
    , islot(sl)
    , ichan(ich)
    , ikind(iki)
    {}
  TString name, description;
  UInt_t index, islot, ichan, ikind;
};

class THaScalerEvtHandler : public THaEvtTypeHandler {

public:

  THaScalerEvtHandler(const char* name, const char* description);
  THaScalerEvtHandler(const THaScalerEvtHandler& fh) = delete;
  THaScalerEvtHandler& operator=(const THaScalerEvtHandler& fh) = delete;
  virtual ~THaScalerEvtHandler();

   virtual Int_t Analyze(THaEvData *evdata);
   virtual EStatus Init( const TDatime& run_time);
   virtual Int_t End( THaRunBase* r=nullptr );


protected:

   void AddVars( const TString& name, const TString& desc, UInt_t iscal,
                 UInt_t ichan, UInt_t ikind );
   void DefVars();
   void ParseVariable( const std::vector<std::string>& dbline );
   void ParseMap( const char* cbuf, const std::vector<std::string>& dbline );
   void VerifySlots();
   void SetIndices();
   void AssignNormScaler();

   std::vector<Decoder::GenScaler*> scalers;
   std::vector<ScalerLoc*> scalerloc;
   Double_t evcount;
   UInt_t fNormIdx, fNormSlot;
   Double_t *dvars;
   TTree *fScalerTree;

   ClassDef(THaScalerEvtHandler,0)  // Scaler Event handler

};

#endif
