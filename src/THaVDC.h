#ifndef ROOT_THaVDC
#define ROOT_THaVDC

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTrackingDetector.h"

class THaVDCUVPlane;
class THaTrack;
class TClonesArray;

class THaVDC : public THaTrackingDetector {

public:
  THaVDC( const char* name, const char* description = "",
	  THaApparatus* a = NULL );
  virtual ~THaVDC();

  virtual Int_t Decode( const THaEvData& );
  virtual Int_t CoarseTrack( TClonesArray& tracks );
  virtual Int_t FineTrack( TClonesArray& tracks );
  virtual EStatus Init( const TDatime& date );

  // Get and Set Functions
  virtual THaVDCUVPlane* GetUpper() { return fUpper; }
  virtual THaVDCUVPlane* GetLower() { return fLower; }

  virtual Double_t GetVDCAngle() { return fVDCAngle; }
  virtual Double_t GetSpacing()  { return fSpacing;  }

protected:

  THaVDC() {}                    // Must construct with name
  THaVDC( const THaVDC& ) {}
  THaVDC& operator=( const THaVDC& ) { return *this; }

  THaVDCUVPlane* fLower;    // Lower UV plane
  THaVDCUVPlane* fUpper;    // Upper UV plane

  TClonesArray*  fUVpairs;  // Pairs of matched UV tracks (THaVDCTrackPair obj)

  Double_t fVDCAngle;       // Angle from the VDC cs to TRANSPORT cs (rad)
  Double_t fSin_vdc;        // Sine of VDC angle
  Double_t fCos_vdc;        // Cosine of VDC angle
  Double_t fTan_vdc;        // Tangent of VDC angle
  Double_t fSpacing;        // Spacing between U1 and U2 (m)
  Int_t    fNtracks;        // Number of tracks found in ConstructTracks

  Int_t    fNumIter;        // Number of iterations for FineTrack()
  Double_t fErrorCutoff;    // Cut on track matching error

          void  Clear()  {}
  virtual Int_t ConstructTracks( TClonesArray * tracks = NULL, Int_t flag = 0 );

  void ProjToTransPlane(Double_t& x, Double_t& y, Double_t& z, 
			Double_t& th, Double_t& ph);

  virtual Int_t SetupDetector( const TDatime& date );

  ClassDef(THaVDC,0)             // VDC class
}; 

////////////////////////////////////////////////////////////////////////////////

#endif
