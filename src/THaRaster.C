//*-- Author :    Chris  Behre   July 2000

//////////////////////////////////////////////////////////////////////////
//
// THaRaster
// 
// Decodes the Raster Data for Hall A
//
//////////////////////////////////////////////////////////////////////////

#include "THaRaster.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaVarList.h"
#include <cstring>

ClassImp(THaRaster)

//______________________________________________________________________________
THaRaster::THaRaster( const char* name, const char* description, 
		      const char* device, THaApparatus* apparatus )
  : THaDetector(name,description,apparatus), fDevice("struck")
{
  // Constructor with name, description and Detector type
  // This will allow processing of runs from older detector configurations.

  if( device && strlen(device) > 0 ) {
    fDevice = device;
    fDevice.ToLower();
  }
}

//______________________________________________________________________________
THaDetectorBase::EStatus THaRaster::Init( const TDatime& run_time )
{
  // Initialize detector.  "run_time" is currently ignored.

  static const char* const here = "Init()";

  if( IsInit() ) {
    Warning( Here(here), "Detector already initialized. Doing nothing." );
    return fStatus;
  }

  MakePrefix();

  // Which device are we using?
  // I guess the choice should be time-dependent since the raster configuration
  // is known for each experiment, but for now we rely on the user to tell us
  // which device we need to decode.

  int dev = 0;

  if( fDevice == "struck" )
    dev = 1;  
  else if( fDevice == "vmic" ) 
    dev = 2;
  else if( fDevice == "lecroy" ) 
    dev = 3;
  else {
    Warning( Here(here), "No such device: %s. Raster initialization failed.",
	     fDevice.Data() ); 
    return fStatus;
  }
  
  // Initialize detector map

  // FIXME: Check this! This is what Chris Behre coded.
  // Caution: Must match chan_map in Decode().
  const THaDetMap::Module m[4] = {
    { 14, 5, 0, 3 },     // Struck
    { 14, 3, 0, 3 },     // VMIC
    { 14, 2, 7, 7 },     // LeCroy #1
    { 14, 2, 0, 2 }      // LeCroy #2       //Chris had 0-3, obviously wrong
  };
  int i = dev-1;
  fDeviceNo = i;
  fDetMap->AddModule( m[i].crate, m[i].slot, m[i].lo, m[i].hi );

  // Special case: LeCroy has two entries

  if( i == 2 )
    fDetMap->AddModule( m[i+1].crate, m[i+1].slot, m[i+1].lo, m[i+1].hi );

  // Register raster variables in global list

  VarDef vars[] = {
    { "Xcur", "X-axis current",        kFloat, 0, &fXcur },
    { "Ycur", "Y-axis current",        kFloat, 0, &fYcur },
    { "Xder", "X-axis derivative",     kFloat, 0, &fXder },
    { "Yder", "Y-axis derivative",     kFloat, 0, &fYder },
    { 0, 0, 0, 0, 0 }
  };
  DefineVariables( vars );

  // Initialize decoder channel map
  //FIXME: Check this! 

  Float_t* const tmp[NCHAN] = { 
    &fXcur, &fYcur, &fXder, &fYder, 0, 0, 0, 0,  // Struck
    &fXcur, &fXder, &fYder, &fYcur, 0, 0, 0, 0,  // VMIC
    &fYcur, &fXder, &fYder, 0, 0, 0, 0, &fXcur   // LeCroy  // Chris had fXcur on 6
  };                                                // but must be 7 (see fDetMap!)
  memcpy( fChanMap, tmp, NCHAN*sizeof(Float_t*));

  return fStatus = kOK;
}

//______________________________________________________________________________
THaRaster::~THaRaster()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//______________________________________________________________________________
Int_t THaRaster::Decode( const THaEvData& evdata )
{
  // Decode raster data.
  // 

  Int_t nhit = 0;
  Int_t device_offset = 8*fDeviceNo;

  // Reset event-by-event variables.

  ClearEvent();
  
  // Loop over all modules defined for this detector

  for( UShort_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    // Loop over all channels that have a hit.
    for( UShort_t j = 0; j < evdata.GetNumChan( d->crate, d->slot); j++) {
      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan > d->hi || chan < d->lo ) continue;    // Not one of my channels.

      // Get the data. Rasters are assumed to have only single hits (hit=0).

      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );
      nhit++;

  // Copy the data to the local variables.
      Float_t* pdat = fChanMap[ device_offset+chan ];
      if( pdat ) 
	*pdat = static_cast<Float_t>( data );
      else
	Warning( "Decode()", "Invalid channel: %d for detector: %s. "
		 "Data ignored.", chan, fDevice.Data() );
    }
  }
  return nhit;
}
