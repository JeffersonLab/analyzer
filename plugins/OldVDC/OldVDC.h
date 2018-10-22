#ifndef Podd_OldVDC_h_
#define Podd_OldVDC_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDC                                                                    //
//                                                                           //
// This is the Podd 1.5.37 VDC code, ported to Podd 1.6. This code is known  //
// to be buggy. Do not use except for comparisons of new with old results.   //
//                                                                           //
// The only difference, aside from the name chage OldVDC -> OldVDC, is       //
// that all database functions have been re-written to use the new database  //
// format.                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTrackingDetector.h"
#include <vector>

class OldVDCUVPlane;
class THaTrack;
class TClonesArray;
class OldVDCUVTrack;

class OldVDC : public THaTrackingDetector {

public:
  OldVDC( const char* name, const char* description = "",
	  THaApparatus* a = NULL );

  virtual ~OldVDC();

  virtual void  Clear( Option_t* opt="" );
  virtual Int_t Decode( const THaEvData& );
  virtual Int_t CoarseTrack( TClonesArray& tracks );
  virtual Int_t FineTrack( TClonesArray& tracks );
  virtual Int_t FindVertices( TClonesArray& tracks );
  virtual EStatus Init( const TDatime& date );
  virtual void  SetDebug( Int_t level );

  // Get and Set Functions
  virtual OldVDCUVPlane* GetUpper() { return fUpper; }
  virtual OldVDCUVPlane* GetLower() { return fLower; }

  virtual Double_t GetVDCAngle() { return fVDCAngle; }
  virtual Double_t GetSpacing()  { return fUSpacing;  }

  virtual void Print(const Option_t* opt="") const;

  void  SetNMaxGap( Int_t val );
  void  SetMinTime( Int_t val );
  void  SetMaxTime( Int_t val );
  void  SetTDCRes( Double_t val );

  // Bits & and bit masks for THaTrack
  enum {
    kStageMask     = BIT(14) | BIT(15),  // Track processing stage bits
    kInvalid       = 0x0000,  // Not processed
    kCoarse        = BIT(14), // Coarse track
    kFine          = BIT(15), // Fine track
    kReassigned    = BIT(16), // Track is a new track in Fine stage
    kMultiTrack    = BIT(17), // Track was generated in the multitrack analysis
    kBadTrack      = BIT(18)  // Track prematurely exits the spectrometer or similar
  };

  // Bits and bit masks for this object
  enum {
    kOnlyFastest    = BIT(14), // Only use fastest hit for each wire (highest TDC)
    kTDCbits        = BIT(15) | BIT(16),  // Mask for TDC mode bits
    kHardTDCcut     = BIT(15), // Use hard TDC cuts (fMinTime, fMaxTime)
    kSoftTDCcut     = BIT(16), // Use soft TDC cut (reasonable estimated drifts)
    kIgnoreNegDrift = BIT(17), // Completely ignore negative drift times

    kCoarseOnly     = BIT(23) // Do only coarse tracking
  };

  // declarations for target vertex reconstruction
  enum ECoordTypes { kTransport, kRotatingTransport };
  enum EFPMatrixElemTags { T000 = 0, Y000, P000 };
  enum { kPORDER = 7 };

  // private class for storing matrix element data
  // class OldVDCMatrixElement;
  // friend class OldVDCMatrixElement;
  class OldVDCMatrixElement {
  public:
    OldVDCMatrixElement() : iszero(true), pw(3), order(0), v(0), poly(kPORDER) {}
    bool match( const OldVDCMatrixElement& rhs ) const;
    void clear()
      { iszero = true; pw.clear(); order = 0; v = 0.0; poly.clear(); }

    bool iszero;             // whether the element is zero
    std::vector<int> pw;     // exponents of matrix element
                             //   e.g. D100 = { 1, 0, 0 }
    int  order;
    double v;                // its computed value
    std::vector<double> poly;// the associated polynomial
  };

protected:

  OldVDCUVPlane* fLower;    // Lower UV plane
  OldVDCUVPlane* fUpper;    // Upper UV plane

  TClonesArray*  fUVpairs;  // Pairs of matched UV tracks (OldVDCTrackPair obj)

  Double_t fVDCAngle;       // Angle from the VDC cs to TRANSPORT cs (rad)
  Double_t fSin_vdc;        // Sine of VDC angle
  Double_t fCos_vdc;        // Cosine of VDC angle
  Double_t fTan_vdc;        // Tangent of VDC angle
  Double_t fUSpacing;       // Spacing between U1 and U2 (m)
  Double_t fVSpacing;       // Spacing between V1 and V2 (m)
  Int_t    fNtracks;        // Number of tracks found in ConstructTracks

  Int_t    fNumIter;        // Number of iterations for FineTrack()
  Double_t fErrorCutoff;    // Cut on track matching error
  ECoordTypes fCoordType;   // Coordinates to use as input for matrix calcs

  Double_t fCentralDist;    // the path length of the central ray from
                            // the origin of the transport coordinates to 
                            // the s1 plane

  // initial matrix elements
  std::vector<OldVDCMatrixElement> fTMatrixElems;
  std::vector<OldVDCMatrixElement> fDMatrixElems;
  std::vector<OldVDCMatrixElement> fPMatrixElems;
  std::vector<OldVDCMatrixElement> fPTAMatrixElems; // involves abs(theta_fp)
  std::vector<OldVDCMatrixElement> fYMatrixElems;
  std::vector<OldVDCMatrixElement> fYTAMatrixElems; // involves abs(theta_fp)
  std::vector<OldVDCMatrixElement> fFPMatrixElems;  // matrix elements used in
                                            // focal plane transformations
                                            // { T, Y, P }

  std::vector<OldVDCMatrixElement> fLMatrixElems;   // Path-length corrections (meters)

  void CalcFocalPlaneCoords( THaTrack* track, const ECoordTypes mode);
  void CalcTargetCoords(THaTrack *the_track, const ECoordTypes mode);
  void CalcMatrix(const double x, std::vector<OldVDCMatrixElement> &matrix);
  Double_t DoPoly(const int n, const std::vector<double> &a, const double x);
  Double_t PolyInv(const double x1, const double x2, const double xacc, 
		 const double y, const int norder, 
		 const std::vector<double> &a);
  Double_t CalcTargetVar(const std::vector<OldVDCMatrixElement> &matrix, 
			 const double powers[][5]);
  Double_t CalcTarget2FPLen(const std::vector<OldVDCMatrixElement>& matrix,
			    const Double_t powers[][5]);
  Int_t ReadDatabase( const TDatime& date );

  virtual Int_t ConstructTracks( TClonesArray* tracks = NULL, Int_t flag = 0 );

  void CorrectTimeOfFlight(TClonesArray& tracks);
  void FindBadTracks(TClonesArray &tracks);

  ClassDef(OldVDC,0)             // VDC class
}; 

////////////////////////////////////////////////////////////////////////////////

#endif
