///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCluster                                                                //
//                                                                           //
// A generic cluster.                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCluster.h"
#include <iostream>

using namespace std;

const Double_t THaCluster::kBig = 1e38;

//_____________________________________________________________________________
void THaCluster::Clear( Option_t* )
{
  // Clear the contents of the cluster

  fCenter.SetXYZ( kBig, kBig, kBig );
}

//_____________________________________________________________________________
void THaCluster::Print( Option_t* ) const
{
  // Print contents of cluster

  cout << fCenter.X() << " " << fCenter.Y() << " " << fCenter.Z() << endl;

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaCluster)
