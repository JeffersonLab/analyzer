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

//_____________________________________________________________________________
THaCluster& THaCluster::operator=( const THaCluster& rhs )
{
  // Assignment operator

  TObject::operator=( rhs );
  if( this != &rhs ) {
    fCenter     = rhs.fCenter;
  }
  return *this;
}

//_____________________________________________________________________________
void THaCluster::Clear( const Option_t* opt )
{
  // Clear the contents of the cluster

  fCenter.SetXYZ( 0.0, 0.0, 0.0 );
}

//_____________________________________________________________________________
void THaCluster::Print( Option_t* opt ) const
{
  // Print contents of cluster

  cout << fCenter.X() << " " << fCenter.Y() << " " << fCenter.Z() << endl;

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaCluster)
