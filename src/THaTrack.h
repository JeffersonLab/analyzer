#ifndef ROOT_THaTrack
#define ROOT_THaTrack

//////////////////////////////////////////////////////////////////////////
//
// THaTrack
//
// A track in the focal plane
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TMath.h"

class TClonesArray;
class THaPIDinfo;
class THaVertex;
class THaSpectrometer;

class THaTrack : public TObject {

public:

  THaTrack() : fP(0.0), fPx(0.0), fPy(0.0), fPz(0.0), fTheta(0.0), fPhi(0.0),
    fX(0.0), fY(0.0), fClusters(NULL), fPIDinfo(NULL), fVertex(NULL),
    fSpectrometer(NULL) {}
  THaTrack( Double_t p, Double_t theta, Double_t phi, Double_t x, Double_t y,
	    const THaSpectrometer* s=NULL, const TClonesArray* clusters=NULL,
	    THaPIDinfo* pid=NULL, THaVertex* vertex=NULL );
  virtual ~THaTrack() {}

  void              Clear( Option_t* opt );
  Double_t          GetP()  const                  { return fP; }
  Double_t          GetPx() const                  { return fPy; }
  Double_t          GetPy() const                  { return fPy; }
  Double_t          GetPz() const                  { return fPz; }
  Double_t          GetX( Double_t z=0.0 ) const;
  Double_t          GetY( Double_t z=0.0 ) const;
  Double_t          GetTheta() const               { return fTheta; }
  Double_t          GetPhi() const                 { return fPhi; }
  const TClonesArray*    GetClusters() const       { return fClusters; }
  THaPIDinfo*       GetPIDinfo() const             { return fPIDinfo; }
  const THaSpectrometer* GetSpectrometer() const   { return fSpectrometer; }
  THaVertex*        GetVertex() const              { return fVertex; }
  void              Print( Option_t* opt="" ) const;
  void              SetMomentum( Double_t p, Double_t theta, Double_t phi );
  void              SetPosition( Double_t x, Double_t y );
  void              Set( Double_t p, Double_t theta, Double_t phi, 
			 Double_t x, Double_t y );
  void              SetClusters( TClonesArray* c ) { fClusters = c; }
  void              SetPIDinfo(  THaPIDinfo* pid ) { fPIDinfo  = pid; }
  void              SetSpectrometer( THaSpectrometer* s )
                                                   { fSpectrometer = s; }
  void              SetVertex( THaVertex* v )      { fVertex   = v; }

protected:

  Double_t          fP;              // momentum (GeV)
  Double_t          fPx;             // x momentum component (GeV)
  Double_t          fPy;             // y momentum component (GeV)
  Double_t          fPz;             // z momentum component (GeV)
  Double_t          fTheta;          // Tangent of TRANSPORT Theta (x')
  Double_t          fPhi;            // Tangent of TRANSPORT Phi (y')
  Double_t          fX;              // x position in focal plane (mm)
  Double_t          fY;              // y position (mm)

  const TClonesArray*     fClusters; // clusters of this track
  THaPIDinfo*             fPIDinfo;  // particle ID information for this track
  THaVertex*              fVertex;   // reconstructed vertex quantities
  const THaSpectrometer*  fSpectrometer; // spectrometer this track belongs to

  void              CalcPxyz();

private:

  ClassDef(THaTrack,0)  //A track in the focal plane
};

//__________________ inlines __________________________________________________
inline
void THaTrack::CalcPxyz()
{
  // Internal function to compute momentum components.

  fPz     = fP/TMath::Sqrt(1.0+fTheta*fTheta+fPhi*fPhi);
  fPx     = fPz*fTheta;
  fPy     = fPz*fPhi;
}

//_____________________________________________________________________________
inline
Double_t THaTrack::GetX( Double_t z ) const
{
  // Return x position at distance z from the focal plane,
  // assuming that the track is not deflected in the detector region.

  return fX + z*fTheta;
}

//_____________________________________________________________________________
inline
Double_t THaTrack::GetY( Double_t z ) const
{
  // Return y position at distance z from the focal plane,
  // assuming that the track is not deflected in the detector region.

  return fY + z*fPhi;
}

//_____________________________________________________________________________
inline
void THaTrack::SetMomentum( Double_t p, Double_t theta, Double_t phi )
{
  fP      = p;
  fTheta  = theta;
  fPhi    = phi;
  CalcPxyz();
}

//_____________________________________________________________________________
inline
void THaTrack::SetPosition( Double_t x, Double_t y )
{
  fX = x;
  fY = y;
}

//_____________________________________________________________________________
inline
void THaTrack::Set( Double_t p, Double_t theta, Double_t phi,
			   Double_t x, Double_t y )
{
  SetMomentum( p, theta, phi );
  SetPosition( x, y );
}

#endif
