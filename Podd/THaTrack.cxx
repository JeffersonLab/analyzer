//*-- Author :    Ole Hansen   29 March 2001

//////////////////////////////////////////////////////////////////////////
//
// THaTrack
//
// A generic track.
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrack.h"
#include "THaCluster.h"
#include "THaTrackID.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
TObject* THaTrack::Clone( const char* ) const
{
  return new THaTrack(*this);
}

//_____________________________________________________________________________
THaTrack::THaTrack(const THaTrack& src)
  : TObject(src)
  , fX(src.fX)
  , fY(src.fY)
  , fTheta(src.fTheta)
  , fPhi(src.fPhi)
  , fP(src.fP)
  , fDX(src.fDX)
  , fDY(src.fDY)
  , fDTheta(src.fDTheta)
  , fDPhi(src.fDPhi)
  , fRX(src.fRX)
  , fRY(src.fRY)
  , fRTheta(src.fRTheta)
  , fRPhi(src.fRPhi)
  , fTX(src.fTX)
  , fTY(src.fTY)
  , fTTheta(src.fTTheta)
  , fTPhi(src.fTPhi)
  , fDp(src.fDp)
  , fPvect(src.fPvect)
  , fVertex(src.fVertex)
  , fVertexError(src.fVertexError)
  , fPathl(src.fPathl)
  , fTime(src.fTime)
  , fdTime(src.fdTime)
  , fBeta(src.fBeta)
  , fdBeta(src.fdBeta)
  , fNclusters(src.fNclusters)
  , fClusters{src.fClusters[0], src.fClusters[1],
              src.fClusters[2], src.fClusters[3]}
  , fPIDinfo(src.fPIDinfo)
  , fCreator(src.fCreator)
  , fIndex(src.fIndex)
  , fTrkNum(src.fTrkNum)
  , fID(static_cast<THaTrackID*>(src.fID->Clone()))
  , fFlag(src.fFlag)
  , fType(src.fType)
  , fChi2(src.fChi2)
  , fNDoF(src.fNDoF)
  , fDedx(src.fDedx)
  , fEnergy(src.fEnergy)
  , fNPMT(src.fNPMT)
  , fBetaChi2(src.fBetaChi2)
  , fFPTime(src.fFPTime)
  , fGoodPlane3(src.fGoodPlane3)
  , fGoodPlane4(src.fGoodPlane4)
{}

//_____________________________________________________________________________
THaTrack::~THaTrack()
{
  // Destructor. Delete objects owned by this track.

  delete fID;
}

//_____________________________________________________________________________
void THaTrack::Clear( Option_t* opt )
{
  // If *opt == 'F' then reset all track quantities, else just
  // delete memory managed by this track.
  // (We need this behavior so we can Clear("C") the track TClonesArray
  // without the overhead of clearing everything.)
 
  //FIXME: too complicated. Do we really need to reallocate the trackID?

  if( opt && (*opt == 'F') ) {
    fTheta = fPhi = fX = fY = fP = fDp = kBig;
    fRX = fRY = fRTheta = fRPhi = kBig;
    fTX = fTY = fTTheta = fTPhi = kBig;
    fDX = fDY = fDTheta = fDPhi = kBig;
    fNclusters = 0; fFlag = fType = 0U;
    fIndex = -1;
    if( fPIDinfo ) fPIDinfo->Clear( opt );
    fPvect.SetXYZ( kBig, kBig, kBig );
    fVertex.SetXYZ( kBig, kBig, kBig );
    fVertexError.SetXYZ( kBig, kBig, kBig );
    fPathl = fTime = fdTime = fBeta = fdBeta = kBig;
    fChi2 = kBig; fNDoF = 0;
    memset( fClusters, 0, kMAXCL*sizeof(void*) );
  }
  delete fID; fID = nullptr;
}

//_____________________________________________________________________________
Int_t THaTrack::AddCluster( THaCluster* c )
{
  // Add a cluster to the internal list of pointers.
  // (Clusters are memory-managed by tracking detectors, not by tracks.)

  if( fNclusters >= kMAXCL )
    return 1;

  fClusters[ fNclusters++ ] = c;
  return 0;
}
   
//_____________________________________________________________________________
void THaTrack::Print( Option_t* opt ) const
{
  // Print track parameters
  TObject::Print( opt );
  cout << "Momentum = " << fP     << " GeV/c" << endl;
  cout << "x_fp     = " << fX     << " m"     << endl;
  cout << "y_fp     = " << fY     << " m"     << endl;
  cout << "theta_fp = " << fTheta << " rad"   << endl;
  cout << "phi_fp   = " << fPhi   << " rad"   << endl;

  for( int i=0; i<fNclusters; i++ )
    fClusters[i]->Print();
  if( fPIDinfo ) fPIDinfo->Print( opt );
}

//_____________________________________________________________________________
static Double_t SafeNDoF( Int_t dof )
{
  if( dof <= 0 )
    return 1e-10;
  return static_cast<Double_t>(dof);
}

//_____________________________________________________________________________
Int_t THaTrack::Compare(const TObject * obj) const
{
  // compare two tracks by chi2/ndof
  // for track array sorting

  const auto* tr = dynamic_cast<const THaTrack*>(obj);
  if (!tr) return 0;

  Double_t v1 = GetChi2() / SafeNDoF( GetNDoF() );
  Double_t v2 = tr->GetChi2()/ SafeNDoF( tr->GetNDoF() );

  if( v1<v2 ) return -1;
  else if( v1==v2 ) return 0;
  else return 1;
}


//_____________________________________________________________________________

ClassImp(THaTrack)
