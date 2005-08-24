//*-- Author :    Ole Hansen   16/05/00

//////////////////////////////////////////////////////////////////////////
//
// THaDetMap
//
// The standard detector map for a Hall A detector.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetMap.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

const int THaDetMap::kDetMapSize;

//_____________________________________________________________________________
THaDetMap::THaDetMap() : fNmodules(0)
{
  // Default constructor. Create an empty detector map.
  fMaplength = 10; // default number of modules/size of the array
  fMap = new Module[fMaplength];
}

//_____________________________________________________________________________
THaDetMap::THaDetMap( const THaDetMap& rhs )
{
  // Copy constructor. Initialize one detector map with another.

  fNmodules = rhs.fNmodules;
  fMap = new Module[fMaplength];

  fNmodules = rhs.fNmodules;

  memcpy(fMap,rhs.fMap,fNmodules*sizeof(Module));
}

//_____________________________________________________________________________
THaDetMap& THaDetMap::operator=( const THaDetMap& rhs )
{
  // THaDetMap assignment operator. Assign one map to another.

  if ( this != &rhs ) {
    if ( fMaplength != rhs.fMaplength ) {
      delete [] fMap;
      fMaplength = rhs.fMaplength;
      fMap = new Module[fMaplength];
    }
    fNmodules = rhs.fNmodules;
    memcpy(fMap,rhs.fMap,fNmodules*sizeof(Module));
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
			    UShort_t chan_lo, UShort_t chan_hi,
			    UInt_t first, UInt_t model, Int_t refindex )
{
  struct ModuleType {
    UInt_t model;
    int adc;
    int tdc;
  };

  static const ModuleType module_list[] = {
    { 1875, 0, 1 },
    { 1877, 0, 1 },
    { 1881, 1, 0 },
    { 1872, 0, 1 },
    { 3123, 1, 0 },
    { 1182, 1, 0 },
    {  792, 1, 0 },
    {  775, 0, 1 },
    {  767, 0, 1 },
    { 3201, 0, 1 },
    { 6401, 0, 1 },
    { 0 }
  };

  // Add a module to the map.

  if( fNmodules >= kDetMapSize ) return -1;  //Map is full

  if ( fNmodules >= fMaplength ) { // need to expand the Map
    Int_t oldlen = fMaplength;
    fMaplength += 10;
    Module* tmpmap = new Module[fMaplength]; // expand in groups of 10
    
    memcpy(tmpmap,fMap,oldlen*sizeof(Module));
    delete [] fMap;
    fMap = tmpmap;
  }

  Module& m = fMap[fNmodules];
  m.crate = crate;
  m.slot  = slot;
  m.lo    = chan_lo;
  m.hi    = chan_hi;
  m.first = first;
  m.model = model;
  m.refindex = refindex;

  const ModuleType* md = module_list;
  while ( md->model && model != md->model ) md++;
  m.model |= ( md->adc ? kADCBit : 0 ) | ( md->tdc ? kTDCBit : 0 );
  
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
	 << setw(5) << m->first
	 << setw(5) << (m->model & ~(kADCBit | kTDCBit))
	 << setw(4) << ( IsADC(m) ? "ADC" : " " )
	 << setw(4) << ( IsTDC(m) ? "TDC" : " " )
	 << endl;
  }
}

//_____________________________________________________________________________
ClassImp(THaDetMap)

