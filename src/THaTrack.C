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

const Double_t THaTrack::kBig = 1e38;

//_____________________________________________________________________________
THaTrack::~THaTrack()
{
  // Destructor. Delete the track ID associated with this track.

  delete fID;
}

//_____________________________________________________________________________
void THaTrack::Clear( const Option_t* opt )
{
  // If *opt == 'P' ("Partial"), clear everything except the focal plane 
  // coordinates (used by constructor).
  // If *opt == 'F' ("Full") or empty then reset all track quantities.
  // If *opt == 'C', do a Full clear and also deallocate any memory that
  // is reallocated for every event
  
  //FIXME: too complicated. Do we really need to reallocate the trackID?

  if( opt ) {
    switch( *opt ) {
    case 'C':
      delete fID; fID = NULL;
    case 'F':
    case '\0':
      fTheta = fPhi = fX = fY = kBig;
    case 'P':
      fP = fDp = kBig;
      fRX = fRY = fRTheta = fRPhi = kBig;
      fTX = fTY = fTTheta = fTPhi = kBig;
      fDX = fDY = fDTheta = fDPhi = kBig;
      fNclusters = fFlag = fType = 0;
      if( fPIDinfo ) fPIDinfo->Clear( opt );
      fPvect.SetXYZ( kBig, kBig, kBig );
      fVertex.SetXYZ( kBig, kBig, kBig );
      fVertexError.SetXYZ( kBig, kBig, kBig );
      fChi2 = kBig; fNDoF = 0;
      break;
    default:
      break;
    }
  }
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
  cout << "Momentum = " << fP << " GeV/c\n";
  cout << "x_fp     = " << fX   << " m\n";
  cout << "y_fp     = " << fY   << " m\n";
  cout << "theta_fp = " << fTheta << " rad\n";
  cout << "phi_fp   = " << fPhi << " rad\n";

  for( int i=0; i<fNclusters; i++ )
    fClusters[i]->Print();
  if( fPIDinfo ) fPIDinfo->Print( opt );
}

//_____________________________________________________________________________

ClassImp(THaTrack)

