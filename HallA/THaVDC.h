#ifndef Podd_HALLA_VDC_h_
#define Podd_HALLA_VDC_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTrackingDetector.h"
#include <cassert>

class THaVDCChamber;
class THaTrack;
class TClonesArray;
class THaVDCPoint;

class THaVDC : public THaTrackingDetector {

public:
  THaVDC( const char* name, const char* description = "",
	  THaApparatus* a = NULL );

  virtual ~THaVDC();

  virtual void  Clear( Option_t* opt="" );
  virtual Int_t Decode( const THaEvData& );
  virtual Int_t CoarseTrack( TClonesArray& tracks );
  virtual Int_t FineTrack( TClonesArray& tracks );
  virtual Int_t FindVertices( TClonesArray& tracks );
  virtual EStatus Init( const TDatime& date );
  virtual void  SetDebug( Int_t level );

  // Get and Set Functions
  THaVDCChamber* GetUpper() const { return fUpper; }
  THaVDCChamber* GetLower() const { return fLower; }

  Double_t GetVDCAngle() const { return fVDCAngle; }
  Double_t GetSpacing()  const { return fSpacing;  }

  void Print(const Option_t* opt="") const;

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
#ifdef MCDATA
    kMCdata         = BIT(21), // Assume input is Monte Carlo data
#endif
    kDecodeOnly     = BIT(22), // Only decode data, disable tracking
    kCoarseOnly     = BIT(23)  // Do only coarse tracking
  };

  enum { kPORDER = 7 };

  // Class for storing matrix element data
  class THaMatrixElement {
  public:
    THaMatrixElement() : iszero(true), order(0), v(0)
      { pw.reserve(5); poly.reserve(kPORDER); }
    bool match( const THaMatrixElement& rhs ) const
      { assert(pw.size() == rhs.pw.size()); return ( pw == rhs.pw ); }
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

  enum ECoordType { kTransport, kRotatingTransport };
  enum EFPMatrixElemTag { T000 = 0, Y000, P000 };

  // Subdetectors
  THaVDCChamber* fLower;    // Lower chamber
  THaVDCChamber* fUpper;    // Upper chamber

  // Event data
  TClonesArray*  fLUpairs;  // Candidate pairs of lower/upper points
  Int_t    fNtracks;        // Number of tracks found in ConstructTracks
  UInt_t   fEvNum;          // Event number from decoder (for diagnostics)

  // Geometry
  Double_t fVDCAngle;       // Angle from the VDC cs to TRANSPORT cs (rad)
  Double_t fSin_vdc;        // Sine of VDC angle
  Double_t fCos_vdc;        // Cosine of VDC angle
  Double_t fTan_vdc;        // Tangent of VDC angle
  Double_t fSpacing;        // Spacing between lower and upper chambers (m)

  Double_t fCentralDist;    // the path length of the central ray from
			    // the origin of the transport coordinates to
			    // the s1 plane

  // Configuration
  Int_t    fNumIter;        // Number of iterations for FineTrack()
  Double_t fErrorCutoff;    // Cut on track matching error
  ECoordType fCoordType;    // Coordinates to use as input for matrix calcs

  // Optics matrix elements (FIXME: move to HRS)
  std::vector<THaMatrixElement> fTMatrixElems;
  std::vector<THaMatrixElement> fDMatrixElems;
  std::vector<THaMatrixElement> fPMatrixElems;
  std::vector<THaMatrixElement> fPTAMatrixElems; // involves abs(theta_fp)
  std::vector<THaMatrixElement> fYMatrixElems;
  std::vector<THaMatrixElement> fYTAMatrixElems; // involves abs(theta_fp)
  std::vector<THaMatrixElement> fFPMatrixElems;  // matrix elements used in
					    // focal plane transformations
					    // { T, Y, P }

  std::vector<THaMatrixElement> fLMatrixElems;   // Path-length corrections (meters)

  void CalcFocalPlaneCoords( THaTrack* track );
  void CalcTargetCoords(THaTrack *the_track );
  void CalcMatrix(const double x, std::vector<THaMatrixElement> &matrix);
  Double_t DoPoly(const int n, const std::vector<double> &a, const double x);
  Double_t PolyInv(const double x1, const double x2, const double xacc,
		 const double y, const int norder,
		 const std::vector<double> &a);
  Double_t CalcTargetVar(const std::vector<THaMatrixElement> &matrix,
			 const double powers[][5]);
  Double_t CalcTarget2FPLen(const std::vector<THaMatrixElement>& matrix,
			    const Double_t powers[][5]);
  Int_t ReadDatabase( const TDatime& date );

  virtual Int_t ConstructTracks( TClonesArray* tracks = NULL, Int_t flag = 0 );

  void CorrectTimeOfFlight(TClonesArray& tracks);
  void FindBadTracks(TClonesArray &tracks);

  virtual Int_t ReadGeometry( FILE* file, const TDatime& date,
			      Bool_t required = kFALSE );

  ClassDef(THaVDC,0)             // VDC class
};

////////////////////////////////////////////////////////////////////////////////

#endif
