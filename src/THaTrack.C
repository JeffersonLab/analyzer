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
#include "THaPIDinfo.h"
#include "THaTrackID.h"
#include "TList.h"
#include "TClass.h"

#include <iostream>

ClassImp(THaTrack)

//_____________________________________________________________________________
THaTrack::THaTrack( Double_t x, Double_t y, Double_t theta, Double_t phi, 
		    THaTrackingDetector* creator, 
		    THaTrackID* id, THaPIDinfo* pid ) :
  fX(x), fY(y), fTheta(theta), fPhi(phi), fP(0.0), 
  fPIDinfo(pid), fCreator(creator), fID(id), fFlag(0), fType(0)
{
  // Normal constructor with initialization

  fClusters = new TList;
}

//_____________________________________________________________________________
THaTrack::~THaTrack()
{
  // Destructor

  delete fID;        fID       = NULL;
  delete fClusters;  fClusters = NULL;
}

//_____________________________________________________________________________
Int_t THaTrack::AddCluster( THaCluster* cluster )
{
  // Add a cluster to the internal list

#ifdef WITH_DEBUG
  if( !cluster )
    return 1;
  TClass* c = cluster->IsA();
  if( !c || !c->InheritsFrom("THaCluster") )
    return 2;
#endif

  fClusters->Add( cluster );
  return 0;
}

//_____________________________________________________________________________
void THaTrack::Clear( const Option_t* opt )
{
  // Reset track quantities. 

  fP = fDp = fTheta = fPhi = fX = fY = 0.0;
  fRX = fRY = fRTheta = fRPhi = 0.0;
  fTX = fTY = fTTheta = fTPhi = 0.0;
  fDX = fDY = fDTheta = fDPhi = 0.0;
  fFlag = fType = 0;
  delete fID; fID = NULL;
  fClusters->Clear( opt );
  if( fPIDinfo ) fPIDinfo->Clear( opt );
  fPvect.SetXYZ( 0.0, 0.0, 0.0 );
  fVertex.SetXYZ( 0.0, 0.0, 0.0 );
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

  fClusters->Print( opt );
  if( fPIDinfo ) fPIDinfo->Print( opt );
}



