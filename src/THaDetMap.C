//*-- Author :    Ole Hansen   16/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaDetMap
//
// The standard detector map for a Hall A detector.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetMap.h"
#include <iostream.h>
#include <iomanip.h>
#include <cstring>

ClassImp(THaDetMap)

const int THaDetMap::kDetMapSize;

//_____________________________________________________________________________
THaDetMap::THaDetMap() : fNmodules(0)
{
  // Default constructor. Create an empty detector map.

  fMap = (UShort_t*) new char[sizeof(UShort_t)*4*kDetMapSize];
}

//_____________________________________________________________________________
THaDetMap::THaDetMap( const THaDetMap& rhs )
{
  // Copy constructor. Initialize one detector map with another.

  fNmodules = rhs.fNmodules;
  fMap = (UShort_t*) new char[sizeof(UShort_t)*4*kDetMapSize];
  memcpy(fMap,rhs.fMap,sizeof(UShort_t)*4*fNmodules);
}

//_____________________________________________________________________________
THaDetMap& THaDetMap::operator=( const THaDetMap& rhs )
{
  // THaDetMap assignment operator. Assign one map to another.

  if ( this != &rhs ) {
    fNmodules = rhs.fNmodules;
    delete [] fMap;
    fMap = (UShort_t*) new char[sizeof(UShort_t)*4*kDetMapSize];
    memcpy(fMap,rhs.fMap,sizeof(UShort_t)*4*fNmodules);
  }
  return *this;
}

//_____________________________________________________________________________
THaDetMap::~THaDetMap()
{
  // Destructor. Free all memory used by the detector map.

  delete [] fMap;
}

//_____________________________________________________________________________
Int_t THaDetMap::AddModule( UShort_t crate, UShort_t slot, 
			    UShort_t chan_lo, UShort_t chan_hi )
{
  // Add a module to the map.

  if( fNmodules >= kDetMapSize ) return -1;  //Map is full

  int i = 4*fNmodules;
  *(fMap+i)   = crate;
  *(fMap+i+1) = slot;
  *(fMap+i+2) = chan_lo;
  *(fMap+i+3) = chan_hi;

  return ++fNmodules;
}

//_____________________________________________________________________________
void THaDetMap::Print( Option_t* opt ) const
{
  // Print the contents of the map

  cout << "Size: " << fNmodules << endl;
  for( UShort_t i=0; i<fNmodules; i++ ) {
    Module* m = GetModule(i);
    cout << " " 
	 << setw(5) << m->crate
	 << setw(5) << m->slot 
	 << setw(5) << m->lo 
	 << setw(5) << m->hi 
	 << endl;
  }
}
