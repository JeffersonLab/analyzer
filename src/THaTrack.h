#ifndef ROOT_THaTrack
#define ROOT_THaTrack

//////////////////////////////////////////////////////////////////////////
//
// THaTrack
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"

class TClonesArray;
class THaPIDinfo;
class THaTrackingDetector;
class THaTrackID;
class THaCluster;

class THaTrack : public TObject {

public:

  typedef THaTrackingDetector TD;

  THaTrack() : fX(0.0), fY(0.0), fTheta(0.0), fPhi(0.0), fP(0.0),
    fClusters(NULL), fPIDinfo(NULL), fCreator(NULL), fID(NULL), fFlag(0) {}
  THaTrack( Double_t x, Double_t y, Double_t theta, Double_t phi,
	    TD* creator=NULL, THaTrackID* id=NULL, THaPIDinfo* pid=NULL );
  virtual ~THaTrack();

  Int_t             AddCluster( THaCluster* c );
  void              Clear( Option_t* opt="" );
  TD*               GetCreator()       const { return fCreator; }
  TList*            GetClusters()      const { return fClusters; }
  UInt_t            GetFlag()          const { return fFlag; }
  THaTrackID*       GetID()            const { return fID; }
  Double_t          GetP()             const { return fP; }
  Double_t          GetPhi()           const { return fPhi; }
  THaPIDinfo*       GetPIDinfo()       const { return fPIDinfo; }
  Double_t          GetTheta()         const { return fTheta; }
  Double_t          GetX()             const { return fX; }
  Double_t          GetY()             const { return fY; }
  Double_t          GetX( Double_t z ) const { return fX + z*fTheta; }
  Double_t          GetY( Double_t z ) const { return fY + z*fPhi; }

  Double_t          GetDX()            const { return fDX; }
  Double_t          GetDY()            const { return fDY; }
  Double_t          GetDTheta()        const { return fDTheta; }
  Double_t          GetDPhi()          const { return fDPhi; } 
  Double_t          GetRX()            const { return fRX; }
  Double_t          GetRY()            const { return fRY; }
  Double_t          GetRTheta()        const { return fRTheta; }
  Double_t          GetRPhi()          const { return fRPhi; } 
  Double_t          GetTX()            const { return fTX; }
  Double_t          GetTY()            const { return fTY; }
  Double_t          GetTTheta()        const { return fTTheta; }
  Double_t          GetTPhi()          const { return fTPhi; }
  Double_t          GetDp()            const { return fDp; }

  TVector3&         GetPvect()               { return fPvect; }

  void              Print( Option_t* opt="" ) const;

  void              SetID( THaTrackID* id )   { fID = id; }
  void              SetFlag( UInt_t flag )    { fFlag = flag; }
  void              SetMomentum( Double_t p ) { fP = p; }
  void              SetPosition( Double_t x, Double_t y );
  void              Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; }
  void              SetR( Double_t x, Double_t y,
			  Double_t theta, Double_t phi );
  void              SetD( Double_t x, Double_t y,
			  Double_t theta, Double_t phi );
  void              SetTarget( Double_t x, Double_t y,
			  Double_t theta, Double_t phi );
  void              SetDp(Double_t dp) { fDp = dp; }

  void              SetCreator( TD* d )               { fCreator = d; }
  void              SetPIDinfo( THaPIDinfo* pid )     { fPIDinfo = pid; }
  void              SetPvect( const TVector3& pvect ) { fPvect = pvect; }

protected:

  Double_t          fX;              // x position in TRANSPORT plane (m)
  Double_t          fY;              // y position in TRANSPORT plane (m)
  Double_t          fTheta;          // Tangent of TRANSPORT Theta (x')
  Double_t          fPhi;            // Tangent of TRANSPORT Phi (y')
  Double_t          fP;              // Track momentum (GeV)

  TList*            fClusters;       //! Clusters of this track
  THaPIDinfo*       fPIDinfo;        //! Particle ID information for this track
  TD*               fCreator;        //! Detector creating this track

  // coordinates in the rotated TRANSPORT system
  Double_t fRX;     // x position in focal plane (m)
  Double_t fRY;     // y position in focal plane (m)
  Double_t fRTheta; // Tangent of TRANSPORT Theta (x')
  Double_t fRPhi;   // Tangent of TRANSPORT Phi (y')

  // coordinates in the target system
  Double_t fTX;     // x position at target (m)
  Double_t fTY;     // y position at target (m)
  Double_t fTTheta; // Tangent of TRANSPORT Theta (out-of-plane angle) at target
  Double_t fTPhi;   // Tangent of TRANSPORT Phi (in-plane angle) at target

  // coordinates in the detector system
  Double_t fDX;     // x position in DCS
  Double_t fDY;     // y position in DCS
  Double_t fDTheta; // Tangent of DCS Theta 
  Double_t fDPhi;   // Tangent of DCS Phi 

  Double_t fDp;     // dp/p_center -- fractional change in momentum

  THaTrackID*       fID;     //! Track identifier
  UInt_t            fFlag;   // Status flag

  TVector3          fPvect;  // Momentum vector at target in lab system (GeV)

  ClassDef(THaTrack,2)       // A generic particle track
};

//__________________ inlines __________________________________________________
inline
void THaTrack::SetPosition( Double_t x, Double_t y )
{
  fX = x;
  fY = y;
}

//_____________________________________________________________________________
inline
void THaTrack::SetD( Double_t x, Double_t y, Double_t theta, Double_t phi )
{
  // set the coordinates in the rotated focal-plane frame
  fDX = x;
  fDY = y;
  fDTheta = theta;
  fDPhi = phi;
}

//_____________________________________________________________________________
inline
void THaTrack::SetR( Double_t x, Double_t y, Double_t theta, Double_t phi )
{
  // set the coordinates in the rotated focal-plane frame
  fRX = x;
  fRY = y;
  fRTheta = theta;
  fRPhi = phi;
}

//_____________________________________________________________________________
inline
void THaTrack::SetTarget( Double_t x, Double_t y, Double_t theta, Double_t phi )
{
  // set the coordinates in the target frame
  fTX = x;
  fTY = y;
  fTTheta = theta;
  fTPhi = phi;
}

#endif
