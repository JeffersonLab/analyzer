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

  fMaplength = 10;      // default number of modules/size of the array
  fMap = new Module [fMaplength];
  
}

//_____________________________________________________________________________
THaDetMap::THaDetMap( const THaDetMap& rhs )
{
  // Copy constructor. Initialize one detector map with another.

  fMaplength = rhs.fMaplength;
  fMap = new Module [fMaplength];
  
  fNmodules = rhs.fNmodules;

  for (unsigned int i=0; i<fNmodules; i++) {
    fMap[i] = rhs.fMap[i];
  }
}

//_____________________________________________________________________________
THaDetMap& THaDetMap::operator=( const THaDetMap& rhs )
{
  // THaDetMap assignment operator. Assign one map to another.

  if ( this != &rhs && fMaplength != rhs.fMaplength ) {
    fNmodules = rhs.fNmodules;
    if ( fMaplength != rhs.fMaplength && fMap ) {
      delete [] fMap;
      fMaplength = rhs.fMaplength;
      fMap =  new Module[fMaplength];
    }
    
    for (unsigned int i=0; i<fNmodules; i++) {
      fMap[i] = rhs.fMap[i];
    }
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
			    UShort_t chan_lo, UShort_t chan_hi, UInt_t firstw )
{
  // Add a module to the map.

  if( fNmodules >= kDetMapSize ) return -1;  //Map is full -- sanity check
  
  if( fNmodules >= fMaplength ) {            // need to expand the Map
    Int_t oldlen = fMaplength;
    fMaplength += 10;
    Module* tmpmap = new Module[fMaplength];  // expand in groups of 10 modules
    
    for (Int_t i=0; i<oldlen; i++) {
      tmpmap[i] = fMap[i];
    }
    delete [] fMap;
    fMap = tmpmap;
  }
  
  Module& m = fMap[fNmodules];
  m.crate = crate;
  m.slot = slot;
  m.lo = chan_lo;
  m.hi = chan_hi;
  m.firstw = firstw;
//    int i = 4*fNmodules;
//    *(fMap+i)   = crate;
//    *(fMap+i+1) = slot;
//    *(fMap+i+2) = chan_lo;
//    *(fMap+i+3) = chan_hi;

  return ++fNmodules;
}

//_____________________________________________________________________________
void THaDetMap::Print( Option_t* opt ) const
{
  // Print the contents of the map

  cout << "Size: " << fNmodules << endl;
  for ( UShort_t i=0; i<fNmodules; i++ ) {
    Module& m = fMap[i];
    cout << " "
	 << setw(5) << m.crate
	 << setw(5) << m.slot
	 << setw(5) << m.lo
	 << setw(5) << m.hi
	 << setw(5) << m.firstw
	 << endl;
  }
  
//    for( UShort_t i=0; i<fNmodules; i++ ) {
//      Module* m = GetModule(i);
//      cout << " " 
//  	 << setw(5) << m->crate
//  	 << setw(5) << m->slot 
//  	 << setw(5) << m->lo 
//  	 << setw(5) << m->hi 
//  	 << endl;
//    }
}


