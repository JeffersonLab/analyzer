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
#include "TMath.h"

#include <vector>

static const UInt_t NCHAN = 4;

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
      { 0 }
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
      { "pedestals",   pedestals, kDouble, NCHAN, 1 },
      { "rotmatrix",   rotations, kDouble, NCHAN, 1 },
      { "offsets"  ,   offsets,   kDouble, 2    , 1 },
      { 0 }
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
  // Initialize global variables and lookup table for decoder


  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

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
    { 0 }
  };
    

  return DefineVarsFromList( vars, mode );

}

//_____________________________________________________________________________
THaBPM::~THaBPM()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
}

//_____________________________________________________________________________
void THaBPM::Clear( Option_t* opt )
{
  // Reset per-event data.
  THaBeamDet::Clear(opt);
  fPosition.SetXYZ(0.,0.,-10000.);
  fDirection.SetXYZ(0.,0.,1.);
  for( UInt_t k=0; k<NCHAN; ++k ) {
    fRawSignal(k)=-1;
    fCorSignal(k)=-1;
  }
}

//_____________________________________________________________________________
Int_t THaBPM::Decode( const THaEvData& evdata )
{

  // clears the event structure
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // performs pedestal subtraction

  const char* const here = "Decode()";

  UInt_t nfired = 0;
  for (Int_t i = 0; i < fDetMap->GetSize(); i++ ){
    THaDetMap::Module* d = fDetMap->GetModule( i );
    for (Int_t j=0; j< evdata.GetNumChan( d->crate, d->slot ); j++) {
      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j);
      if ((chan>=d->lo)&&(chan<=d->hi)) {
	Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );
	UInt_t k = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) -1;
	if ((k<NCHAN)&&(fRawSignal(k)==-1)) {
	  fRawSignal(k)= data;
	  nfired++;
	}
	else {
	  Warning( Here(here), "Illegal detector channel: %d", k );
	}
      }
    }
  }

  if (nfired!=NCHAN) {
    Warning( Here(here), "Number of fired Channels out of range. "
	     "Setting beam position to nominal values");
  }
  else {
    fCorSignal=fRawSignal;
    fCorSignal-=fPedestals;
  }

  return 0;
}

//____________________________________________________
//  
Int_t THaBPM::Process( )
{
 
  // called by the beam apparaturs 
  // uses the pedestal substracted signals from the antennas
  // to get the position in the bpm coordinate system 
  // and uses the transformation matrix defined in the database
  // to transform it into the HCS
  // directions are not calculated, they are always set parallel to z

  Double_t ap, am;

  for( UInt_t k=0; k<NCHAN; k+=2 ) {
    if( fCorSignal(k)+fCorSignal(k+1)!=0.0 ) {
      ap=fCorSignal(k);
      am=fCorSignal(k+1);
      fRotPos(k/2)=fCalibRot*(ap-am)/(ap+am);
    }
    else {
      fRotPos(k/2)=0.0;
    }
  }

  TVectorD dum(fRotPos);

  dum*=fRot2HCSPos;

  fPosition.SetXYZ(
		   dum(0)+fOrigin(0)+fOffset(0),
		   dum(1)+fOrigin(1)+fOffset(1),
		   fOrigin(2)
		   );

  return 0 ;
}


////////////////////////////////////////////////////////////////////////////////
ClassImp(THaBPM)
