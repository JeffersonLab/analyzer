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
				  THaApparatus* apparatus ) :
  THaBeamDet(name,description,apparatus), fRawPos(2), fRawSlope(2),fRasterFreq(2),fSlopePedestal(2),fRasterPedestal(2)
{
  // Constructor
  fRaw2Pos[0].ResizeTo(3,2);
  fRaw2Pos[1].ResizeTo(3,2);
  fRaw2Pos[2].ResizeTo(3,2);

}


//_____________________________________________________________________________
// ReadDatabase:  if detectors cant be added to detmap 
//                or entry for bpms is missing           -> kInitError
//                otherwise                              -> kOk
//                CAUTION: i do not check for incomplete or 
//                         inconsistent data
//
Int_t THaRaster::ReadDatabase( const TDatime& date )
{
  static const char* const here = "ReadDatabase()";

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];
  
  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // okay, this needs to be changed, but since i dont want to re- or pre-invent 
  // the wheel, i will keep it ugly and read in my own configuration file with 
  // a very fixed syntax: 

  // Seek our detmap section (e.g. "Raster_detmap")
  sprintf(keyword,"[%s_detmap]",GetName());
  Int_t n=strlen(keyword);

  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here("ReadDataBase()"), "Unexpected end of raster configuration file");
    fclose(fi);
    return kInitError;
  }

  // again that is not really nice, but since it will be changed anyhow:
  // i dont check each time for end of file, needs to be improved

  fDetMap->Clear();
  int first_chan, crate, dummy, slot, first, last, modulid;

  do {
    fgets( buf, LEN, fi);
    sscanf(buf,"%d %d %d %d %d %d %d",&first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if (first_chan>=0) {
      if ( fDetMap->AddModule (crate, slot, first, last, first_chan )<0) {
	Error( Here(here), "Couldnt add Raster to DetMap. Good bye, blue sky, good bye!");
	fclose(fi);
	return kInitError;
      }
    }
  } while (first_chan>=0);

  // Seek our database section
  sprintf(keyword,"[%s]",GetName());
  n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here("ReadDataBase()"), "Unexpected end of raster configuration file");
    fclose(fi);
    return kInitError;
  }
  double dummy1,dummy2,dummy3,dummy4,dummy5,dummy6,dummy7;
  fgets( buf, LEN, fi);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf%lf",&dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6,&dummy7);
  fRasterFreq(0)=dummy2;
  fRasterFreq(1)=dummy3;

  fRasterPedestal(0)=dummy4;
  fRasterPedestal(1)=dummy5;

  fSlopePedestal(0)=dummy6;
  fSlopePedestal(1)=dummy7;

  fgets( buf, LEN, fi);
  sscanf(buf,"%lf",&dummy1);
  fPosOff[0].SetZ(dummy1);
  fgets( buf, LEN, fi);
  sscanf(buf,"%lf",&dummy1);
  fPosOff[1].SetZ(dummy1);
  fgets( buf, LEN, fi);
  sscanf(buf,"%lf",&dummy1);
  fPosOff[2].SetZ(dummy1);

  // Find timestamp, if any, for the raster constants. 
  // Give up and rewind to current file position on any non-matching tag.
  // Timestamps should be in ascending order
  SeekDBdate( fi, date, true );

  fgets( buf, LEN, fi);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf",&dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);
  fRaw2Pos[0](0,0)=dummy3;
  fRaw2Pos[0](1,1)=dummy4;
  fRaw2Pos[0](0,1)=dummy5;
  fRaw2Pos[0](1,0)=dummy6;
  fPosOff[0].SetX(dummy1);
  fPosOff[0].SetY(dummy2);

  fgets( buf, LEN, fi);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf",&dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);
  fRaw2Pos[1](0,0)=dummy3;
  fRaw2Pos[1](1,1)=dummy4;
  fRaw2Pos[1](0,1)=dummy5;
  fRaw2Pos[1](1,0)=dummy6;
  fPosOff[1].SetX(dummy1);
  fPosOff[1].SetY(dummy2);

  fgets( buf, LEN, fi);
  sscanf(buf,"%lf%lf%lf%lf%lf%lf",&dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);
  fRaw2Pos[2](0,0)=dummy3;
  fRaw2Pos[2](1,1)=dummy4;
  fRaw2Pos[2](0,1)=dummy5;
  fRaw2Pos[2](1,0)=dummy6;
  fPosOff[2].SetX(dummy1);
  fPosOff[2].SetY(dummy2);

  fclose(fi);
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
inline 
void THaRaster::ClearEvent()
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


  ClearEvent();

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

