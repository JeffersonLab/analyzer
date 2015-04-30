///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaRaster                                                                 //
//                                                                           //
// Class for a beam raster device                                            //
// measuring two magnet currents                                             //
// which are proportional to the horizontal/vertical beam displacement       //
// the two planes are assumed to be decoupled                                //
// there is no phase shift between the current and the actual beam position  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaRaster.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "TMath.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaRaster::THaRaster( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaBeamDet(name,description,apparatus), fRawPos(2), fRawSlope(2),
    fRasterFreq(2), fSlopePedestal(2), fRasterPedestal(2)
{
  // Constructor
  fRaw2Pos[0].ResizeTo(3,2);
  fRaw2Pos[1].ResizeTo(3,2);
  fRaw2Pos[2].ResizeTo(3,2);

}


//_____________________________________________________________________________
Int_t THaRaster::ReadDatabase( const TDatime& date )
{
  // ReadDatabase:  if detectors cant be added to detmap
  //                or entry for bpms is missing           -> kInitError
  //                otherwise                              -> kOk

  const char* const here = "ReadDatabase";

  vector<Int_t> detmap;
  Double_t zpos[3], raw2pos[18];
  Float_t freq[2], rped[2], sped[2];  // FIXME: change to double

  FILE* file = OpenFile( date );
  if( !file )
    return kInitError;

  // fOrigin.SetXYZ(0,0,0);
  Int_t err = kOK;// = ReadGeometry( file, date );

  if( !err ) {
    memset( zpos, 0, sizeof(zpos) );
    // Read configuration parameters
    DBRequest config_request[] = {
      { "detmap",   &detmap,  kIntV },
      { "zpos",     zpos,     kDouble, 3, 1 },
      { 0 }
    };
    err = LoadDB( file, date, config_request, fPrefix );
  }

  if( !err &&
      FillDetMap(detmap, THaDetMap::kFillLogicalChannel |
			 THaDetMap::kFillModel, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  if( !err ) {
    fPosOff[0].SetZ( zpos[0] );
    fPosOff[1].SetZ( zpos[1] );
    fPosOff[2].SetZ( zpos[2] );

    memset( rped, 0, sizeof(rped) );
    memset( sped, 0, sizeof(sped) );

    DBRequest calib_request[] = {
      { "freqs",      freq,  kFloat, 2 },
      { "rast_peds",  rped,  kFloat, 2, 1 },
      { "slope_peds", sped,  kFloat, 2, 1 },
      { "raw2pos",    raw2pos, kDouble, 18 },
      { 0 }
    };
    err = LoadDB( file, date, calib_request, fPrefix );
  }
  fclose(file);
  if( err )
    return err;

  fRasterFreq.SetElements( freq );
  fRasterPedestal.SetElements( rped );
  fSlopePedestal.SetElements( sped );

  fPosOff[0].SetX(   raw2pos[0]);
  fPosOff[0].SetY(   raw2pos[1]);
  fRaw2Pos[0](0,0) = raw2pos[2];
  fRaw2Pos[0](1,1) = raw2pos[3];
  fRaw2Pos[0](0,1) = raw2pos[4];
  fRaw2Pos[0](1,0) = raw2pos[5];

  fPosOff[1].SetX(   raw2pos[6]);
  fPosOff[1].SetY(   raw2pos[7]);
  fRaw2Pos[1](0,0) = raw2pos[8];
  fRaw2Pos[1](1,1) = raw2pos[9];
  fRaw2Pos[1](0,1) = raw2pos[10];
  fRaw2Pos[1](1,0) = raw2pos[11];

  fPosOff[2].SetX(   raw2pos[12]);
  fPosOff[2].SetY(   raw2pos[13]);
  fRaw2Pos[2](0,0) = raw2pos[14];
  fRaw2Pos[2](1,1) = raw2pos[15];
  fRaw2Pos[2](0,1) = raw2pos[16];
  fRaw2Pos[2](1,0) = raw2pos[17];

  return kOK;
}

//_____________________________________________________________________________
Int_t THaRaster::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list
  
  RVarDef vars[] = {
    { "rawcur.x", "current in horizontal raster", "GetRawPosX()" },
    { "rawcur.y", "current in vertical raster", "GetRawPosY()"},
    { "rawslope.x", "derivative of current in horizontal raster", "GetRawSlopeX()" },
    { "rawslope.y", "derivative of current in vertical raster", "GetRawSlopeY()"},
    { "bpma.x", "reconstructed x-position at 1st bpm", "GetPosBPMAX()"},
    { "bpma.y", "reconstructed y-position at 1st bpm", "GetPosBPMAY()"},
    { "bpma.z", "reconstructed z-position at 1st bpm", "GetPosBPMAZ()"},
    { "bpmb.x", "reconstructed x-position at 2nd bpm", "GetPosBPMBX()"},
    { "bpmb.y", "reconstructed y-position at 2nd bpm", "GetPosBPMBY()"},
    { "bpmb.z", "reconstructed z-position at 2nd bpm", "GetPosBPMBZ()"},
    { "target.x", "reconstructed x-position at nom. interaction point", "GetPosTarX()"},
    { "target.y", "reconstructed y-position at nom. interaction point", "GetPosTarY()"},
    { "target.z", "reconstructed z-position at nom. interaction point", "GetPosTarZ()"},
    { "target.dir.x", "reconstructed x-component of beam direction", "fDirection.fX"},
    { "target.dir.y", "reconstructed y-component of beam direction", "fDirection.fY"},
    { "target.dir.z", "reconstructed z-component of beam direction", "fDirection.fZ"},
    { 0 }
  };
    
  return DefineVarsFromList( vars, mode );

}

//_____________________________________________________________________________
THaRaster::~THaRaster()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();

}

//_____________________________________________________________________________
void THaRaster::Clear( Option_t* )
{
  // Reset per-event data.
  fRawPos(0)=-1;
  fRawPos(1)=-1;
  fRawSlope(0)=-1;
  fRawSlope(1)=-1;
  fPosition[0].SetXYZ(0.,0.,-10000.);
  fPosition[1].SetXYZ(0.,0.,-10000.);
  fPosition[2].SetXYZ(0.,0.,-10000.);
  fDirection.SetXYZ(0.,0.,1.);
  fNfired=0;
}

//_____________________________________________________________________________
Int_t THaRaster::Decode( const THaEvData& evdata )
{

  // clears the event structure
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // pedestal subtraction is not foreseen for the raster

  UInt_t chancnt = 0;

  for (Int_t i = 0; i < fDetMap->GetSize(); i++ ){
    THaDetMap::Module* d = fDetMap->GetModule( i );

    for (Int_t j=0; j< evdata.GetNumChan( d->crate, d->slot ); j++) {
      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j);
      if ((chan>=d->lo)&&(chan<=d->hi)) {
	Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );
       Int_t k = chancnt+d->first + chan - d->lo -1;
	if (k<2) {
	  fRawPos(k)= data;
	  fNfired++;
	}
	else if (k<4) {
	  fRawSlope(k-2)= data;
	  fNfired++;
	}
	else {
	  Warning( Here("Decode()"), "Illegal detector channel: %d", k );
	}
      }


    }

    chancnt+=d->hi-d->lo+1;
  }

  if (fNfired!=4) {
      Warning( Here("Decode()"), "Number of fired Channels out of range. Setting beam position to nominal values");
  }
  return 0;
}

//____________________________________________________

Int_t THaRaster::Process( )
{

  for ( Int_t i = 0; i<3; i++) {
    
    //      fPosition[i] = fRaw2Pos[i]*fRawPos+fPosOff[i] ;
    //    this is how i wish it would look like,
    //    but unluckily multiplications between tmatrix and tvector
    //    are not defined, as well as adding a tvector and a tvector3
    //    so i have to do it by hand instead ):

    TVector dum(fRawPos);
    dum*=fRaw2Pos[i];
    fPosition[i].SetXYZ( dum(0)+fPosOff[i](0),
			 dum(1)+fPosOff[i](1),
			 dum(2)+fPosOff[i](2)  );

  }
  
  fDirection = fPosition[1] - fPosition[0];
  
  return 0 ;
}


////////////////////////////////////////////////////////////////////////////////
ClassImp(THaRaster)

