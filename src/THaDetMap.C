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

  fMaplength = rhs.fMaplength;
  fNmodules  = rhs.fNmodules;
  fMap       = new Module[fMaplength];
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
  // Add a module to the map.

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

  if( fNmodules >= kDetMapSize ) return -1;  //Map is full

  if ( fNmodules >= fMaplength ) { // need to expand the Map
    Int_t oldlen = fMaplength;
    fMaplength += 10;
    Module* tmpmap = new Module[fMaplength];   // expand in groups of 10
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
Int_t THaDetMap::Fill( const vector<int>& values, UInt_t flags )
{
  // Fill the map with 'values'. Depending on 'flags', the values vector
  // is interpreted as a 4-, 5-, 6- or 7-tuple:
  //
  // The first 4 values are interpreted as (crate,slot,start_chan,end_chan)
  // Each of the following flags causes one more value to be used as part of
  // the tuple for each module:
  // 
  // kFillLogicalChannel - Logical channel number for 'start_chan'.
  // kFillModel          - The module's hardware model number (see AddModule())
  // kFillRefIndex       - Reference channel (for pipeline TDCs etc.)
  //
  // If more than one flag is present, the numbers will be interpreted
  // in the order the flags are listed above. 
  // Example:
  //      flags = kFillModel | kFillRefIndex
  // ==>
  //      the vector is interpreted as a series of 6-tuples in the order
  //      (crate,slot,start_chan,end_chan,model,refindex).
  //
  // If kFillLogicalChannel is not set, but kAutoCount is, then the 
  // first logical channel numbers are automatically calculated for each 
  // module, assuming the numbers are sequential.
  // 
  // By default, an existing map is overwritten. If the flag kDoNotClear 
  // is present, then the data are appended.
  //
  // The return value is the number of modules successfully added,
  // or negative if an error occurred.

  typedef vector<int>::size_type vsiz_t;

  if( (flags & kDoNotClear) == 0 )
    Clear();

  vsiz_t tuple_size = 4;
  if( flags & kFillLogicalChannel )
    tuple_size++;
  if( flags & kFillModel )
    tuple_size++;
  if( flags & kFillRefIndex )
    tuple_size++;

  UInt_t prev_first = 0, prev_nchan = 0;
  // Defaults for optional values
  UInt_t first = 0, model = 0;
  Int_t ref = -1;

  Int_t ret = 0;
  for( vsiz_t i = 0; i < values.size(); i += tuple_size ) {
    // For compatibility with older maps, crate < 0 means end of data
    if( values[i] < 0 )
      break;
    // Now we require a full tuple
    if( i+tuple_size > values.size() ) {
      ret = -2;
      break;
    }

    vsiz_t k = 4;
    if( flags & kFillLogicalChannel )
      first = values[i+k++];
    else if( flags & kAutoCount ) {
      first = prev_first + prev_nchan;
    }
    if( flags & kFillModel )
      model = values[i+k++];
    if( flags & kFillRefIndex )
      ref   = values[i+k++];

    ret = AddModule( values[i], values[i+1], values[i+2], values[i+3],
		     first, model, ref );
    if( ret<=0 )
      break;
    prev_first = first;
    prev_nchan = GetNchan( ret-1 );
  }
  
  return ret;
}

//_____________________________________________________________________________
Int_t THaDetMap::GetTotNumChan() const
{
  // Get sum of the number of channels of all modules in the map. This is 
  // typically the total number of hardware channels used by the detector.

  Int_t sum = 0;
  for( UShort_t i = 0; i < fNmodules; i++ )
    sum += GetNchan(i);
    
  return sum;
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
	 << setw(5) << GetModel(m)
	 << setw(4) << ( IsADC(m) ? " ADC" : " " )
	 << setw(4) << ( IsTDC(m) ? " TDC" : " " )
	 << endl;
  }
}

//_____________________________________________________________________________
void THaDetMap::Reset()
{
  // Clear() the map and reset the array size to zero, freeing memory.

  Clear();
  delete [] fMap;
  fMap = NULL;
  fMaplength = 0;
}

//_____________________________________________________________________________
ClassImp(THaDetMap)

