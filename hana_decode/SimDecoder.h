//*-- Author:  Ole Hansen<mailto:ole@jlab.org>; Jefferson Lab; 5-Jul-2014
//
#ifndef __SimDecoder_h
#define __SimDecoder_h

/////////////////////////////////////////////////////////////////////
//
//   Podd::SimDecoder
//
//   Generic simulation decoder interface
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "THaAnalysisObject.h"
#include "TClonesArray.h"

namespace Podd {

// MC truth information for detector hits
class MCHitInfo {
public:
  MCHitInfo() : fMCTrack(0), fContam(0), fMCPos(0), fMCTime(0) {}
  MCHitInfo( Int_t mctrk, Double_t mcpos, Double_t time, Int_t contam = 0 )
    : fMCTrack(mctrk), fContam(contam), fMCPos(mcpos), fMCTime(0) {}
  virtual ~MCHitInfo() {}

  void MCPrint() const;

  Int_t    fMCTrack;     // MC signal track number generating this hit
  Int_t    fContam;      // Indicator for contributions other than signal
  Double_t fMCPos;       // True MC track crossing position (m)
  Double_t fMCTime;      // Hit time (s)

  ClassDef(MCHitInfo,2)  // Generic Monte Carlo hit info
};

// Recommended prefix of for MC global variables (MC truth data)
extern const char* const MC_PREFIX;

class SimDecoder : public THaEvData {
 public:
  SimDecoder();
  virtual ~SimDecoder();

  virtual void       Clear( Option_t* opt="" );
  virtual MCHitInfo  GetMCHitInfo( Int_t crate, Int_t slot, Int_t chan ) const;
  virtual Int_t      DefineVariables( THaAnalysisObject::EMode mode =
				      THaAnalysisObject::kDefine );

  TObject* GetMCHit( Int_t i )   const {
    return (fMCHits) ? fMCHits->UncheckedAt(i) : 0;
  }
  TObject* GetMCTrack( Int_t i ) const {
    return (fMCTracks) ? fMCTracks->UncheckedAt(i) : 0;
  }
  Int_t    GetNMCHits()    const {
    return (fMCHits) ? fMCHits->GetLast()+1 : 0;
  }
  Int_t    GetNMCTracks()  const {
    return (fMCTracks) ? fMCTracks->GetLast()+1 : 0;
  }

protected:

  TClonesArray*  fMCHits;     //-> MC hits
  TClonesArray*  fMCTracks;   //-> MC physics tracks
  Bool_t         fIsSetup;    // DefineVariables has run

  ClassDef(SimDecoder,0) // Generic decoder for simulation data
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

#endif
