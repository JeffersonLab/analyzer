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
#include "TVector3.h"

namespace Podd {

// Recommended prefix of MC global variables (MC truth data)
extern const char* const MC_PREFIX;

// Support classes for SimDecoder. These need to be ROOT classes so that
// CINT doesn't choke on the return values of the respective Get functions

// Track points typically are hits of the physics tracks (recorded in fMCTracks)
// recorded in tracker planes. They are stored for every track and can be
// exported as global variables. This is opposed to MCHitInfo, which is the
// _digitized_ hit information for any MC hits (primaries, secondaries,
// background) and which generated on demand for certain hardware channels,
// typically to compare measured to true hit data

// MC truth information for digitized etector hits
class MCHitInfo {
public:
  MCHitInfo() : fMCTrack(0), fContam(0), fMCPos(0), fMCTime(0) {}
  MCHitInfo( Int_t mctrk, Double_t mcpos, Double_t time, Int_t contam = 0 )
    : fMCTrack(mctrk), fContam(contam), fMCPos(mcpos), fMCTime(time) {}
  virtual ~MCHitInfo() {}

  void MCPrint() const;

  Int_t    fMCTrack;     // MC signal track number generating this hit
  Int_t    fContam;      // Indicator for contributions other than signal
  Double_t fMCPos;       // True MC track crossing position (m)
  Double_t fMCTime;      // Hit time (s)

  ClassDef(MCHitInfo,0)  // Generic Monte Carlo hit info
};

// A MC physics track's interaction point at a tracker plane in the lab system.
class MCTrackPoint : public TObject {
public:
  MCTrackPoint() : fMCTrack(0) {}
  MCTrackPoint( Int_t mctrk, Int_t plane, const TVector3& point )
    : fMCTrack(mctrk), fPlane(plane), fMCPoint(point) {}
  virtual ~MCTrackPoint() {}

  virtual void Print( Option_t* opt ) const;

  Int_t    fMCTrack;  // MC track number generating this point
  Int_t    fPlane;    // Tracker plane/layer number
  TVector3 fMCPoint;  // Truth position of a MC physics track in a tracker plane

  ClassDef(MCTrackPoint,1)  // Monte Carlo track interaction coordinates
};

// Simulation decoder class
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
  MCTrackPoint* GetMCPoint( Int_t i ) const {
    return (fMCPoints) ?
      static_cast<Podd::MCTrackPoint*>(fMCPoints->UncheckedAt(i)) : 0;
  }
  Int_t    GetNMCHits()    const {
    return (fMCHits) ? fMCHits->GetLast()+1 : 0;
  }
  Int_t    GetNMCTracks()  const {
    return (fMCTracks) ? fMCTracks->GetLast()+1 : 0;
  }
  Int_t    GetNMCPoints()  const {
    return (fMCPoints) ? fMCPoints->GetLast()+1 : 0;
  }

protected:

  TClonesArray*  fMCHits;     //-> MC hits
  TClonesArray*  fMCTracks;   //-> MC physics tracks
  TClonesArray*  fMCPoints;   //-> Points of the MC physics tracks
  Bool_t         fIsSetup;    // DefineVariables has run

  ClassDef(SimDecoder,0) // Generic decoder for simulation data
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

#endif
