///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaBPM                                                                    //
//                                                                           //
// Class for a BPM                                                           //
// measuring signals from four antennas                                      //
// and  the position at the BPM (without phase correction)                   //
// works with standard ADCs (lecroy like types)                              //
// is good for unrastered beam, or to get average positions                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaBPM.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include <vector>
#include <stdexcept>
#include <cassert>

static const Int_t NCHAN = 4;

using namespace std;

//_____________________________________________________________________________
THaBPM::THaBPM( const char* name, const char* description,
				  THaApparatus* apparatus ) :
  THaBeamDet(name,description,apparatus),
  fRawSignal(NCHAN),fPedestals(NCHAN),fCorSignal(NCHAN),fRotPos(NCHAN/2),
  fRot2HCSPos(NCHAN/2,NCHAN/2), fCalibRot(0)
{
  // Constructor
}


//_____________________________________________________________________________
Int_t THaBPM::ReadDatabase( const TDatime& date )
{
  // ReadDatabase:  if detectors cant be added to detmap
  //                or entry for bpms is missing           -> kInitError
  //                otherwise                              -> kOk

  const char* const here = "ReadDatabase";

  vector<Int_t> detmap;
  Double_t pedestals[NCHAN], rotations[NCHAN], offsets[2];

  FILE* file = OpenFile( date );
  if( !file )
    return kInitError;

  fOrigin.SetXYZ(0,0,0);
  Int_t err = ReadGeometry( file, date );

  if( !err ) {
    // Read configuration parameters
    DBRequest config_request[] = {
      { "detmap",    &detmap,  kIntV },
      { nullptr }
    };
    err = LoadDB( file, date, config_request, fPrefix );
  }

  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  if( !err ) {
    memset( pedestals, 0, sizeof(pedestals) );
    memset( rotations, 0, sizeof(rotations) );
    memset( offsets  , 0, sizeof( offsets ) );
    DBRequest calib_request[] = {
      { "calib_rot",   &fCalibRot },
      { "pedestals",   pedestals, kDouble, NCHAN, true },
      { "rotmatrix",   rotations, kDouble, NCHAN, true },
      { "offsets"  ,   offsets,   kDouble, 2    , true },
      { nullptr }
    };
    err = LoadDB( file, date, calib_request, fPrefix );
  }
  fclose(file);
  if( err )
    return err;

  fOffset(0) = offsets[0];
  fOffset(1) = offsets[1];

  fPedestals.SetElements( pedestals );
  fRot2HCSPos(0,0) = rotations[0];
  fRot2HCSPos(0,1) = rotations[1];
  fRot2HCSPos(1,0) = rotations[2];
  fRot2HCSPos(1,1) = rotations[3];

  return kOK;
}

//_____________________________________________________________________________
Int_t THaBPM::DefineVariables( EMode mode )
{
  // Initialize global variables

  // Register variables in global list
  RVarDef vars[] = {
    { "rawcur.1", "current in antenna 1", "GetRawSignal0()"},
    { "rawcur.2", "current in antenna 2", "GetRawSignal1()"},
    { "rawcur.3", "current in antenna 3", "GetRawSignal2()"},
    { "rawcur.4", "current in antenna 4", "GetRawSignal3()"},
    { "x", "reconstructed x-position", "fPosition.fX"},
    { "y", "reconstructed y-position", "fPosition.fY"},
    { "z", "reconstructed z-position", "fPosition.fZ"},
    { "rotpos1", "position in bpm system","GetRotPosX()"},
    { "rotpos2", "position in bpm system","GetRotPosY()"},
    { nullptr }
  };

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaBPM::~THaBPM()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
void THaBPM::Clear( Option_t* opt )
{
  // Reset per-event data.
  THaBeamDet::Clear(opt);
  fPosition.SetXYZ(0., 0., -10000.);
  fDirection.SetXYZ(0., 0., 1.);
  for( Int_t k = 0; k < NCHAN; ++k ) {
    fRawSignal(k) = -1;
    fCorSignal(k) = -1;
  }
  fRotPos(0) = fRotPos(1) = 0.0;
}

//_____________________________________________________________________________
Int_t THaBPM::Decode( const THaEvData& evdata )
{
  // Decode BPM
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // performs pedestal subtraction

  Int_t nfired = THaBeamDet::Decode(evdata);

  if( nfired == NCHAN ) {
    fCorSignal = fRawSignal - fPedestals;
  } else {
    Warning(Here("Decode"), "Number of fired Channels out of range. "
                            "Setting beam position to nominal values");
  }

  return 0;
}

//_____________________________________________________________________________
bool THaBPM::CheckHitInfo( const DigitizerHitInfo_t& hitinfo ) const
{
  Int_t k = hitinfo.lchan;
  if( k >= NCHAN || fRawSignal(k) != -1 ) {
    Warning(Here("Decode"), "Illegal detector channel %d", k);
    return false;
  }
  return true;
}

//_____________________________________________________________________________
OptUInt_t THaBPM::LoadData( const THaEvData& evdata,
                            const DigitizerHitInfo_t& hitinfo )
{
  if( !CheckHitInfo(hitinfo) )
    return nullopt;
  return THaBeamDet::LoadData(evdata, hitinfo);
}

//_____________________________________________________________________________
Int_t THaBPM::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Store 'data' from single hit in channel 'hitinfo'. Called from Decode()

  Int_t k = hitinfo.lchan;
  assert(k >= 0 && k < NCHAN && fRawSignal(k) == -1); // else bug in LoadData

  fRawSignal(k) = data;
  return 0;
}

//_____________________________________________________________________________
Int_t THaBPM::Process()
{
  // called by the beam apparatus
  // uses the pedestal subtracted signals from the antennas
  // to get the position in the bpm coordinate system 
  // and uses the transformation matrix defined in the database
  // to transform it into the HCS
  // directions are not calculated, they are always set parallel to z

  for( Int_t k = 0; k < NCHAN; k += 2 ) {
    Double_t ap = fCorSignal(k);
    Double_t am = fCorSignal(k + 1);
    if( ap + am != 0.0 ) {
      fRotPos(k / 2) = fCalibRot * (ap - am) / (ap + am);
    } else {
      fRotPos(k / 2) = 0.0;
    }
  }
  TVectorD dum(fRotPos);
  dum *= fRot2HCSPos;
  fPosition.SetXYZ(
    dum(0) + fOrigin(0) + fOffset(0),
    dum(1) + fOrigin(1) + fOffset(1),
    fOrigin(2)
  );

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaBPM)
