#ifndef ROOT_THaVDCUVPlane
#define ROOT_THaVDCUVPlane
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVPlane                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "THaSubDetector.h"
#include "THaVDCPlane.h"

#include "TClonesArray.h"
#include <cassert>

class THaVDCUVTrack;
class THaVDC;
class THaEvData;

class THaVDCUVPlane : public THaSubDetector {

  friend class THaVDCUVTrack;

public:

  THaVDCUVPlane( const char* name="", const char* description="",
		 THaDetectorBase* parent = NULL );
  virtual ~THaVDCUVPlane();

  virtual void    Clear( Option_t* opt="" );    // Reset event-by-event data
  virtual Int_t   Decode( const THaEvData& evData );
  virtual Int_t   CoarseTrack();          // Find clusters & estimate track
  virtual Int_t   FineTrack();            // More precisely calculate track
  virtual EStatus Init( const TDatime& date );

  // Get and Set Functions
  THaVDCPlane*   GetUPlane()      const { return fU; }
  THaVDCPlane*   GetVPlane()      const { return fV; } 
  Int_t          GetNUVTracks()   const { return fUVTracks->GetLast()+1; }
  TClonesArray*  GetUVTracks()    const { return fUVTracks; }
  THaVDC*        GetVDC()         const { return (THaVDC*)GetDetector(); }
  Double_t       GetSpacing()     const { return fSpacing;}
  THaVDCUVTrack* GetUVTrack( Int_t i ) const 
    { assert( i>=0 && i<GetNUVTracks() );
      return (THaVDCUVTrack*)fUVTracks->UncheckedAt(i); }

protected:

  THaVDCPlane*  fU;           // The U plane
  THaVDCPlane*  fV;           // The V plane

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
  
  ClassDef(THaVDCUVPlane,0)             // VDCUVPlane class
};

////////////////////////////////////////////////////////////////////////////////

#endif
