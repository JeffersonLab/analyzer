#ifndef ROOT_THaVDCChamber
#define ROOT_THaVDCChamber

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCChamber                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "TClonesArray.h"
#include "THaVDCPlane.h"
#include "THaVDCPoint.h"
#include "THaVDC.h"
#include <cassert>

class THaVDCPlane;
class THaEvData;

// Coordinates of a point in this chamber
struct PointCoords_t {
  Double_t x;
  Double_t y;
  Double_t theta;  // TRANSPORT x' = dx/dz
  Double_t phi;    // TRANSPORT y' = dy/dz
};

class THaVDCChamber : public THaSubDetector {

public:

  THaVDCChamber( const char* name="", const char* description="",
		 THaDetectorBase* parent = NULL );
  virtual ~THaVDCChamber();

  virtual void    Clear( Option_t* opt="" );    // Reset event-by-event data
  virtual Int_t   Decode( const THaEvData& evData );
  virtual Int_t   CoarseTrack();          // Find clusters & estimate track
  virtual Int_t   FineTrack();            // More precisely calculate track
  virtual EStatus Init( const TDatime& date );

  PointCoords_t   CalcDetCoords( const THaVDCCluster* u,
				 const THaVDCCluster* v ) const;

  // Get and Set Functions
  THaVDCPlane*    GetUPlane()      const { return fU; }
  THaVDCPlane*    GetVPlane()      const { return fV; }
  Int_t           GetNPoints()     const { return fPoints->GetLast()+1; }
  TClonesArray*   GetPoints()      const { return fPoints; }
  THaVDC*         GetVDC()         const
  { return static_cast<THaVDC*>(GetDetector()); }
  Double_t        GetSpacing()     const { return fSpacing;}
  THaVDCPoint*    GetPoint( Int_t i ) const
    { assert( i>=0 && i<GetNPoints() );
      return static_cast<THaVDCPoint*>( fPoints->UncheckedAt(i) ); }
  Double_t        GetZ()           const { return fU->GetZ(); }

protected:

  THaVDCPlane*  fU;           // The U plane
  THaVDCPlane*  fV;           // The V plane

  TClonesArray* fPoints;      // Pairs of possibly matching U and V clusters

  // Geometry
  Double_t fSpacing;          // Space between U & V planes (m)
  Double_t fSin_u;            // Trig functions for the U plane axis angle
  Double_t fCos_u;            //
  Double_t fSin_v;            // Trig functions for the V plane axis angle
  Double_t fCos_v;            //
  Double_t fInv_sin_vu;       // 1/Sine of the difference between the
                              // V axis angle and the U axis angle
  THaVDC*  fVDC;              // Parent VDC detector of this chamber

  void  FindClusters();       // Find clusters in U and V planes
  void  FitTracks();          // Fit local tracks for each cluster
  Int_t MatchUVClusters();    // Match clusters in U with clusters in V
  Int_t CalcPointCoords();

  ClassDef(THaVDCChamber,0)   // VDC chamber (pair of a U and a V plane)
};

///////////////////////////////////////////////////////////////////////////////

#endif
