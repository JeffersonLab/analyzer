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

#ifndef KBIG
#define KBIG THaAnalysisObject::kBig
#endif

namespace Podd {

// Recommended prefix of MC global variables (MC truth data)
extern const char* const MC_PREFIX;

// Support classes for SimDecoder. These need to be ROOT classes so that
// CINT doesn't choke on the return values of the respective Get functions

// A generic Monte Carlo physics track. fMCTracks must contain either objects
// of this class or objects that inherit from it
class MCTrack : public TObject {
public:
  MCTrack( Int_t number, Int_t pid, Double_t weight,
	   const TVector3& vertex, const TVector3& momentum );
  MCTrack();

  Double_t VX()     { return fOrigin.X(); }
  Double_t VY()     { return fOrigin.Y(); }
  Double_t VZ()     { return fOrigin.Z(); }
  Double_t P()      { return fMomentum.Mag(); }
  Double_t PTheta() { return fMomentum.Theta(); }
  Double_t PPhi()   { return fMomentum.Phi(); }

  virtual void Print( const Option_t* opt="" ) const;

  // Truth information
  Int_t    fNumber;        // Track counter
  Int_t    fPID;           // Track particle ID (PDG)
  Double_t fWeight;        // Weight factor
  TVector3 fOrigin;        // Vertex position (m)
  TVector3 fMomentum;      // Momentum (GeV)

  // Fit quality checks (implementation-dependent)
  static const Int_t NFP = 16;
  Double_t fMCFitPar[NFP]; // Results of fit(s) to MC hits
  Double_t fRcFitPar[NFP]; // Results of fit(s) to reconstructed hits

  // Reconstruction status (implementation-dependent)
  Int_t    fReconFlags;    // Reconstruction status flags
  Int_t    fContamFlags;   // Flags indicating contaminated fits
  Double_t fMatchval;      // Projection matchvalue, if applicable
  Int_t    fFitRank;       // Rank of final track fit (0=best)
  Int_t    fTrackRank;     // Rank of final track after pruning (0=best)

  ClassDef(MCTrack,1)  // A MC physics track
};

// Track points typically are hits of the physics tracks (recorded in fMCTracks)
// recorded in tracker planes. They are stored for every track and can be
// exported as global variables. This is opposed to MCHitInfo, which is the
// _digitized_ hit information for any MC hits (primaries, secondaries,
// background) and which generated on demand for certain hardware channels,
// typically to compare measured to true hit data

// MC truth information for digitized detector hits
class MCHitInfo {
public:
  MCHitInfo() : fMCTrack(0), fContam(0), fMCPos(0), fMCTime(0) {}
  MCHitInfo( Int_t mctrk, Double_t mcpos, Double_t time, Int_t contam = 0 )
    : fMCTrack(mctrk), fContam(contam), fMCPos(mcpos), fMCTime(time) {}
  virtual ~MCHitInfo() {}

  void MCPrint() const;
  void MCClear() { fMCTrack = fContam = 0; fMCPos = fMCTime = 0; }

  Int_t    fMCTrack;     // MC signal track number generating this hit
  Int_t    fContam;      // Indicator for contributions other than signal
  Double_t fMCPos;       // True MC track crossing position (m)
  Double_t fMCTime;      // Hit time (s)

  ClassDef(MCHitInfo,1)  // Generic Monte Carlo hit info
};

// A MC physics track's interaction point at a tracker plane in the lab system.
class MCTrackPoint : public TObject {
public:
  MCTrackPoint() : fMCTrack(0), fPlane(-1), fType(-1), fStatus(-1),
		   fMCPoint(KBIG,KBIG,KBIG), fMCP(KBIG,KBIG,KBIG),
		   fMCTime(KBIG), fDeltaE(KBIG), fDeflect(KBIG), fToF(KBIG),
		   fHitResid(KBIG), fTrackResid(KBIG)  {}
  MCTrackPoint( Int_t mctrk, Int_t plane, Int_t type, const TVector3& point,
		const TVector3& pvect )
    : fMCTrack(mctrk), fPlane(plane), fType(type), fStatus(-1), fMCPoint(point),
      fMCP(pvect), fMCTime(KBIG), fDeltaE(KBIG), fDeflect(KBIG),
      fHitResid(KBIG), fTrackResid(KBIG)  {}
  virtual ~MCTrackPoint() {}

  virtual Int_t  Compare( const TObject* obj ) const;
  virtual Bool_t IsSortable() const { return kTRUE; }
  virtual void   Print( Option_t* opt ) const;

  Double_t X()         const { return fMCPoint.X(); }
  Double_t Y()         const { return fMCPoint.Y(); }
  Double_t P()         const { return fMCP.Mag(); }
  Double_t ThetaT()    const { return fMCP.Px()/fMCP.Pz(); }
  Double_t PhiT()      const { return fMCP.Py()/fMCP.Pz(); }
  Double_t R()         const { return fMCPoint.Perp(); }
  Double_t Theta()     const { return fMCPoint.Theta(); }
  Double_t Phi()       const { return fMCPoint.Phi(); }
  Double_t ThetaDir()  const { return fMCP.Theta(); }
  Double_t PhiDir()    const { return fMCP.Phi(); }

  Int_t    fMCTrack;  // MC track number generating this point
  Int_t    fPlane;    // Tracker plane/layer number
  Int_t    fType;     // Plane type (u,v,x etc.)
  Int_t    fStatus;   // Reconstruction status (typ != 0 -> failed to find)
  TVector3 fMCPoint;  // Truth position of MC physics track in tracker plane (m)
  TVector3 fMCP;      // True momentum vector at this position (GeV)
  Double_t fMCTime;   // Arrival time wrt trigger (s)
  Double_t fDeltaE;   // Energy loss wrt prior plane, kBig if unknown (GeV)
  Double_t fDeflect;  // Deflection angle wrt prior plane (rad) or kBig
  Double_t fToF;      // Time-of-flight from prior plane (s) or kBig
  Double_t fHitResid; // True residual nearest reconstr. hit pos - MC hit pos (m)
  Double_t fTrackResid; // True residual reconstructed track - MC hit pos (m)

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
  TClonesArray*  fMCPoints;   //-> MC physics track points
  Bool_t         fIsSetup;    // DefineVariables has run

  ClassDef(SimDecoder,0) // Generic decoder for simulation data
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

#endif
