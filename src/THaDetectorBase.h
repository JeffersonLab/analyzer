#ifndef ROOT_THaDetectorBase
#define ROOT_THaDetectorBase

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "TVector3.h"
#include "THaMatrix.h"
#include <cstdio>

class THaDetMap;
class THaEvData;
class THaTrack;

class THaDetectorBase : public THaAnalysisObject {
  
public:
  virtual ~THaDetectorBase();
  
  virtual Int_t            Decode( const THaEvData& ) = 0;
          Int_t            GetNelem()  const    { return fNelem; }
          const TVector3&  GetOrigin() const    { return fOrigin; }
          const Float_t*   GetSize()   const    { return fSize; }
          void             PrintDetMap( Option_t* opt="") const;

          bool             CheckIntercept( THaTrack* track );
          bool             CalcInterceptCoords( THaTrack* track, 
						Double_t& x, Double_t& y );
          bool             CalcPathLen( THaTrack* track, Double_t& t );


protected:

  // Mapping
  THaDetMap*      fDetMap;    // Hardware channel map for this detector

  // Configuration
  Int_t           fNelem;     // Number of detector elements (paddles, mirrors)

  // Geometry 
  TVector3        fOrigin;    // Origin of detector plane in detector coordinates (m)
  Float_t         fSize[3];   // Detector size in x,y,z (m) - x,y are half-widths
  
  // Extra Geometry for calculating intercepts
  TVector3  fXax;                  // X axis of the detector plane
  TVector3  fYax;                  // Y axis of the detector plane
  TVector3  fZax;                  // Normal to the detector plane
  THaMatrix fDenom;                // Denominator matrix for intercept calc
  THaMatrix fNom;                  // Nominator matrix for intercept calc

  virtual void  DefineAxes( Double_t rotation_angle );

          bool  CalcTrackIntercept( THaTrack* track, Double_t& t, 
				    Double_t& ycross, Double_t& xcross);

  //Only derived classes may construct me
  THaDetectorBase() : fDetMap(NULL), fNelem(0) {}
  THaDetectorBase( const char* name, const char* description );

  ClassDef(THaDetectorBase,0)   //ABC for a detector or subdetector
};

#endif
