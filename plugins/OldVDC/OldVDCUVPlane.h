#ifndef Podd_OldVDCUVPlane_h_
#define Podd_OldVDCUVPlane_h_
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCUVPlane                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "THaSubDetector.h"
#include "OldVDCPlane.h"

#include "TClonesArray.h"
#include <cassert>

class OldVDCUVTrack;
class OldVDC;
class THaEvData;

class OldVDCUVPlane : public THaSubDetector {

  friend class OldVDCUVTrack;

public:

  OldVDCUVPlane( const char* name="", const char* description="",
		 THaDetectorBase* parent = NULL );
  virtual ~OldVDCUVPlane();

  virtual void    Clear( Option_t* opt="" );    // Reset event-by-event data
  virtual Int_t   Decode( const THaEvData& evData );
  virtual Int_t   CoarseTrack();          // Find clusters & estimate track
  virtual Int_t   FineTrack();            // More precisely calculate track
  virtual EStatus Init( const TDatime& date );
  virtual void    SetDebug( Int_t level );

  // Get and Set Functions
  OldVDCPlane*   GetUPlane()      const { return fU; }
  OldVDCPlane*   GetVPlane()      const { return fV; } 
  Int_t          GetNUVTracks()   const { return fUVTracks->GetLast()+1; }
  TClonesArray*  GetUVTracks()    const { return fUVTracks; }
  OldVDC*        GetVDC()         const { return (OldVDC*)GetDetector(); }
  Double_t       GetSpacing()     const { return fSpacing;}
  OldVDCUVTrack* GetUVTrack( Int_t i ) const 
    { assert( i>=0 && i<GetNUVTracks() );
      return (OldVDCUVTrack*)fUVTracks->UncheckedAt(i); }

  void  SetNMaxGap( Int_t val );
  void  SetMinTime( Int_t val );
  void  SetMaxTime( Int_t val );
  void  SetTDCRes( Double_t val );

protected:

  OldVDCPlane*  fU;           // The U plane
  OldVDCPlane*  fV;           // The V plane

  TClonesArray* fUVTracks;    // UV tracks
  
  //UV Plane Geometry
  Double_t fSpacing;          // Space between U & V planes (m)
  Double_t fSin_u;            // Trig functions for the U plane axis angle
  Double_t fCos_u;            // 
  Double_t fSin_v;            // Trig functions for the V plane axis angle
  Double_t fCos_v;            // 
  Double_t fInv_sin_vu;       // 1/Sine of the difference between the
                              // V axis angle and the U axis angle

  // For CoarseTrack
  void FindClusters()        // Find clusters in U & V planes
    { fU->FindClusters(); fV->FindClusters(); }
  Int_t MatchUVClusters();   // Match U plane clusters 
                             //  with V plane clusters
  // For FineTrack
  void FitTracks()      // Fit data to recalculate cluster position
    { fU->FitTracks(); fV->FitTracks(); }

  // For Both
  Int_t CalcUVTrackCoords(); // Compute UV track coords in detector cs
  
  ClassDef(OldVDCUVPlane,0)             // VDCUVPlane class
};

////////////////////////////////////////////////////////////////////////////////

#endif
