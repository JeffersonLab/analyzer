#ifndef Podd_Fadc250ScalerEvtHandler_h_
#define Podd_Fadc250ScalerEvtHandler_h_

/////////////////////////////////////////////////////////////////////
//
//   Fadc250ScalerEvtHandler
//   Class to handle Hall A scaler events (type 140)
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"  // for THaEvtTypeHandler
#include "TString.h"            // for TString, Rtypes, ClassDefOverride
#include <string>               // for string
#include <utility>              // for move
#include <vector>               // for vector

namespace Decoder { class GenScaler; }
struct FadcScalerLoc;
class TTree;

class Fadc250ScalerEvtHandler : public THaEvtTypeHandler {

public:
  Fadc250ScalerEvtHandler(const char* name, const char* description);
  Fadc250ScalerEvtHandler(const Fadc250ScalerEvtHandler& fh) = delete;
  Fadc250ScalerEvtHandler& operator=(const Fadc250ScalerEvtHandler& fh) = delete;
  ~Fadc250ScalerEvtHandler() override;

   enum EKind : Byte_t { ICOUNT = 1, IRATE = 2 };

   Int_t Analyze(THaEvData *evdata) override;
   EStatus Init( const TDatime& run_time) override;
   Int_t End( THaRunBase* r=nullptr ) override;
  
protected:
   void AddVars( const TString& name, const TString& desc, UInt_t iscal,
                 UInt_t ichan, UInt_t ikind );
   void DefVars();
   void ParseVariable( const std::vector<std::string>& dbline );
   void ParseMap( const char* cbuf, const std::vector<std::string>& dbline );
   void VerifySlots();
   void SetIndices();
   void AssignNormScaler();
   Int_t ReadDatabase( const TDatime& date ) override;

   Int_t  imodel = 0;
   UInt_t icrate = 0;
   std::vector<Decoder::GenScaler*> scalers;
   std::vector<FadcScalerLoc*> scalerloc;
   ULong64_t fEvCount;
   ULong64_t fEvNum;  // last seen physics event number
   UInt_t fNormIdx, fNormSlot;
   Double_t *dvars;
   TTree *fScalerTree;

   ClassDefOverride(Fadc250ScalerEvtHandler,0)  // Fadc250 scaler Event handler
};

#endif
