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

  // Bits for fType
  enum {
    kHasDet        = BIT(0),  // Detector coordinates set
    kHasFP         = BIT(1),  // Focal plane coordinates set 
    kHasRot        = BIT(2),  // Rotating TRANSPORT coordinates set
    kHasTarget     = BIT(3),  // Target coordinates reconstructed
    kHasVertex     = BIT(4)   // Vertex reconstructed
  };

  typedef THaTrackingDetector TD;

  THaTrack() : 
    fX(0.0), fY(0.0), fTheta(0.0), fPhi(0.0), fP(0.0), fClusters(NULL), 
    fPIDinfo(NULL), fCreator(NULL), fID(NULL), fFlag(0), fType(0) {}
  THaTrack( Double_t x, Double_t y, Double_t theta, Double_t phi,
	    TD* creator=NULL, THaTrackID* id=NULL, THaPIDinfo* pid=NULL );
  virtual ~THaTrack();

  Int_t             AddCluster( THaCluster* c );
  void              Clear( Option_t* opt="" );
  TD*               GetCreator()       const { return fCreator; }
  TList*            GetClusters()      const { return fClusters; }
  UInt_t            GetFlag()          const { return fFlag; }
  UInt_t            GetType()          const { return fType; }
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
  Double_t          GetLabPx()         const { return fPvect.X(); }
  Double_t          GetLabPy()         const { return fPvect.Y(); }
  Double_t          GetLabPz()         const { return fPvect.Z(); }
  Double_t          GetVertexX()       const { return fVertex.X(); }
  Double_t          GetVertexY()       const { return fVertex.Y(); }
  Double_t          GetVertexZ()       const { return fVertex.Z(); }

  TVector3&         GetPvect()               { return fPvect; }
  TVector3&         GetVertex()              { return fVertex; }

  bool              HasDet()           const { return (fType&kHasDet); }
  bool              HasFP()            const { return (fType&kHasFP); }
  bool              HasRot()           const { return (fType&kHasRot); }
  bool              HasTarget()        const { return (fType&kHasTarget); }
  bool              HasVertex()        const { return (fType&kHasVertex); }

  void              Print( Option_t* opt="" ) const;

  void              SetID( THaTrackID* id )   { fID   = id; }
  void              SetFlag( UInt_t flag )    { fFlag = flag; }
  void              SetType( UInt_t flag )    { fType = flag; }
  void              SetMomentum( Double_t p ) { fP    = p; }
  void              SetDp( Double_t dp )      { fDp   = dp; }
  void              Set( Double_t x, Double_t y, Double_t theta, Double_t phi )
  { fX = x; fY = y; fTheta = theta; fPhi = phi; fType |= kHasFP; }
  void              SetR( Double_t x, Double_t y,
			  Double_t theta, Double_t phi );
  void              SetD( Double_t x, Double_t y,
			  Double_t theta, Double_t phi );
  void              SetTarget( Double_t x, Double_t y,
			       Double_t theta, Double_t phi );

  void              SetCreator( TD* d )                { fCreator = d; }
  void              SetPIDinfo( THaPIDinfo* pid )      { fPIDinfo = pid; }
  void              SetPvect( const TVector3& pvect )  { fPvect   = pvect; }
  void              SetVertex( const TVector3& vertx ) { fVertex  = vertx; }

protected:

  // Focal plane coordinates (TRANSPORT system projected to z=0)
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
  UInt_t            fFlag;   // General status flag (for use by tracking det.)
  UInt_t            fType;   // Flag indicating which vectors reconstructed

  TVector3          fPvect;  // Momentum vector at target in lab system (GeV)
  TVector3          fVertex; // Vertex location in lab (m) valid if fHasVertex

  ClassDef(THaTrack,1)       // A generic particle track
};

//__________________ inlines __________________________________________________
inline
void THaTrack::SetD( Double_t x, Double_t y, Double_t theta, Double_t phi )
{
  // set the coordinates in the rotated focal-plane frame
  fDX = x;
  fDY = y;
  fDTheta = theta;
  fDPhi = phi;
  fType |= kHasDet;
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
  fType |= kHasRot;
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
  fType |= kHasTarget;
}

#endif
