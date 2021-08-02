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
#include "TMath.h"

using namespace std;

static constexpr Int_t NPOS = 3;
static constexpr Int_t NBPM = 2;
static constexpr Int_t NCHAN = 2*NBPM;

//_____________________________________________________________________________
THaRaster::THaRaster( const char* name, const char* description,
                      THaApparatus* apparatus )
  : THaBeamDet(name,description,apparatus), fRawPos(NBPM), fRawSlope(NBPM),
    fRasterFreq(NBPM), fSlopePedestal(NBPM), fRasterPedestal(NBPM),
    fNfired(0)
{
  // Constructor
  fRaw2Pos[0].ResizeTo(NPOS, NBPM);
  fRaw2Pos[1].ResizeTo(NPOS, NBPM);
  fRaw2Pos[2].ResizeTo(NPOS, NBPM);
}


//_____________________________________________________________________________
Int_t THaRaster::ReadDatabase( const TDatime& date )
{
  // ReadDatabase:  if detectors cant be added to detmap
  //                or entry for bpms is missing           -> kInitError
  //                otherwise                              -> kOk

  const char* const here = "ReadDatabase";
  vector<Int_t> detmap;
  Double_t zpos[NPOS];
  Double_t freq[NBPM], rped[NBPM], sped[NBPM];
  Double_t raw2posA[NBPM*NPOS], raw2posB[NBPM*NPOS], raw2posT[NBPM*NPOS];

  FILE* file = OpenFile( date );
  if( !file )
    return kInitError;

  // fOrigin.SetXYZ(0,0,0);
  //Int_t err = ReadGeometry( file, date );

//  if( !err ) {
  memset(zpos, 0, sizeof(zpos));
  // Read configuration parameters
  DBRequest config_request[] = {
    {"detmap", &detmap, kIntV},
    {"zpos",   zpos,    kDouble, NPOS, true},
    {nullptr}
  };
  Int_t err = LoadDB(file, date, config_request, fPrefix);
//  }

  // Fill the detector map. It must specify NBPM(=2) channels for the x/y
  // position readouts. Another NBPM(=2) channels for the x/y raw slopes are
  // optional. These channels must be defined in exactly this order.
  // Although the existing database format includes the logical channel number,
  // which would in principle allow a different order, its value was ignored in
  // the original code and replaced with the order in which modules are listed
  // in the database. It is unclear why the number was first introduced and then
  // extra code was written to support ignoring it. Bizarre.
  // To maintain compatibility with existing databases, we ignore the logical
  // channel number altogether here. Alternatively, we could use it, but then
  // many old databases would become invalid, causing confusion and errors.
  UInt_t flags = THaDetMap::kSkipLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }
  if( !err ) {
    UInt_t nchan = fDetMap->GetTotNumChan();
    if( nchan != NBPM && nchan != NCHAN ) {
      Error(Here(here), "Incorrect number of channels = %u defined in "
                        "detector map. Must be either %u or %u. Fix database.",
            nchan, NBPM, NCHAN);
      err = kInitError;
    } else if( fDebug > 0 && nchan == NBPM ) {
      Warning( Here(here), "Only %u channels defined in detector map. Raster "
                           "slope information will not be available", nchan);
    }
  }

  if( !err ) {
    fPosOff[0].SetZ( zpos[0] );
    fPosOff[1].SetZ( zpos[1] );
    fPosOff[2].SetZ( zpos[2] );

    memset( rped, 0, sizeof(rped) );
    memset( sped, 0, sizeof(sped) );

    DBRequest calib_request[] = {
      { "freqs",      freq,     kDouble, NBPM },
      { "rast_peds",  rped,     kDouble, NBPM, true },
      { "slope_peds", sped,     kDouble, NBPM, true },
      { "raw2posA",   raw2posA, kDouble, NBPM*NPOS },
      { "raw2posB",   raw2posB, kDouble, NBPM*NPOS },
      { "raw2posT",   raw2posT, kDouble, NBPM*NPOS },
      { nullptr }
    };
    err = LoadDB( file, date, calib_request, fPrefix );
  }
  fclose(file);
  if( err )
    return err;

  fRasterFreq.SetElements( freq );
  fRasterPedestal.SetElements( rped );
  fSlopePedestal.SetElements( sped );

  fPosOff[0].SetX(   raw2posA[0]);
  fPosOff[0].SetY(   raw2posA[1]);
  fRaw2Pos[0](0,0) = raw2posA[2];
  fRaw2Pos[0](1,1) = raw2posA[3];
  fRaw2Pos[0](0,1) = raw2posA[4];
  fRaw2Pos[0](1,0) = raw2posA[5];

  fPosOff[1].SetX(   raw2posB[0]);
  fPosOff[1].SetY(   raw2posB[1]);
  fRaw2Pos[1](0,0) = raw2posB[2];
  fRaw2Pos[1](1,1) = raw2posB[3];
  fRaw2Pos[1](0,1) = raw2posB[4];
  fRaw2Pos[1](1,0) = raw2posB[5];

  fPosOff[2].SetX(   raw2posT[0]);
  fPosOff[2].SetY(   raw2posT[1]);
  fRaw2Pos[2](0,0) = raw2posT[2];
  fRaw2Pos[2](1,1) = raw2posT[3];
  fRaw2Pos[2](0,1) = raw2posT[4];
  fRaw2Pos[2](1,0) = raw2posT[5];

  return kOK;
}

//_____________________________________________________________________________
Int_t THaRaster::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

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
    { nullptr }
  };
    
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaRaster::~THaRaster()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
void THaRaster::Clear( Option_t* opt )
{
  // Reset per-event data.
  THaBeamDet::Clear(opt);
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
  // Decode Raster
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // pedestal subtraction is not foreseen for the raster

  fNfired += THaBeamDet::Decode(evdata);

  if( fNfired != NBPM && fNfired != NCHAN ) {
    Warning(Here("Decode"), "Unexpected number of fired channels = %d."
                            "Setting beam position to nominal values.", fNfired);
    Clear();
  }
  return 0;
}

//_____________________________________________________________________________
bool THaRaster::CheckHitInfo( const DigitizerHitInfo_t& hitinfo ) const
{
  Int_t k = hitinfo.lchan;
  if( k < 0 || k >= NCHAN ) {
    Warning(Here("Decode"), "Illegal detector channel %d", k);
    return false;
  }
  return true;
}

//_____________________________________________________________________________
OptUInt_t THaRaster::LoadData( const THaEvData& evdata,
                               const DigitizerHitInfo_t& hitinfo )
{
  if( !CheckHitInfo(hitinfo) )
    return nullopt;
  return THaBeamDet::LoadData(evdata, hitinfo);
}

//_____________________________________________________________________________
Int_t THaRaster::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Store 'data' from single hit in channel 'hitinfo'. Called from Decode()

  Int_t k = hitinfo.lchan;
  assert(k >= 0 && k < NCHAN); // else bug in LoadData

  if( k < NBPM )
    fRawPos(k) = data;
  else
    fRawSlope(k-NBPM) = data;

  return 0;
}

//_____________________________________________________________________________
Int_t THaRaster::Process()
{
  for( Int_t i = 0; i < NPOS; i++ ) {

    //      fPosition[i] = fRaw2Pos[i]*fRawPos+fPosOff[i] ;
    //    this is how i wish it would look like,
    //    but unluckily multiplications between tmatrix and tvector
    //    are not defined, as well as adding a tvector and a tvector3
    //    so i have to do it by hand instead ):

    TVectorD dum(fRawPos);
    dum *= fRaw2Pos[i];
    fPosition[i].SetXYZ(dum(0) + fPosOff[i](0),
                        dum(1) + fPosOff[i](1),
                        dum(2) + fPosOff[i](2));

  }

  fDirection = fPosition[1] - fPosition[0];

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
ClassImp(THaRaster)

