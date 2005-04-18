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
  // If *opt == 'F' then reset all track quantities, else just
  // delete memory managed by this track.
  // (We need this behavior so we can Clear("C") the track TClonesArray
  // without the overhead of clearing everything.)

  if( opt && (*opt == 'F') ) {
    fP = fDp = fTheta = fPhi = fX = fY = 0.0;
    fRX = fRY = fRTheta = fRPhi = 0.0;
    fTX = fTY = fTTheta = fTPhi = 0.0;
    fDX = fDY = fDTheta = fDPhi = 0.0;
    fNclusters = fFlag = fType = 0;
    if( fPIDinfo ) fPIDinfo->Clear( opt );
    fPvect.SetXYZ( 0.0, 0.0, 0.0 );
    fVertex.SetXYZ( 0.0, 0.0, 0.0 );
    fVertexError.SetXYZ( 1.0, 1.0, 1.0 );
    fChi2 = kBig; fNDoF = 0.0;
  }
  delete fID; fID = NULL;
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
  cout << "Theta    = " << fTheta << " rad\n";
  cout << "Phi      = " << fPhi << " rad\n";

  for( int i=0; i<fNclusters; i++ )
    fClusters[i]->Print();
  if( fPIDinfo ) fPIDinfo->Print( opt );
}

//_____________________________________________________________________________

ClassImp(THaTrack)

