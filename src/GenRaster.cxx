///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// GenRaster                                                                 //
//                                                                           //
// Class for a beam raster device                                            //
// measuring two magnet currents                                             //
// which are proportional to the horizontal/vertical beam displacement       //
// the two planes are assumed to be decoupled                                //
// there is no phase shift between the current and the actual beam position  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "GenRaster.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "TMath.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
GenRaster::GenRaster( const char* name, const char* description,
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
Int_t GenRaster::ReadDatabase( const TDatime& date )
{
  static const char* const here = "ReadDatabase()";

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];
  int locdebug = 1; 
  
  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // Seek our database section
  //sprintf(keyword,"[%s_detmap]",GetName());
  //cout << "keyword = " <<keyword<<endl;

  Int_t n=strlen(keyword);
  if (locdebug) {
     printf("Reading database: %s   desc %s \n",GetName(),GetTitle());
  }

  // Read in fRasterFreq (2), fRasterPedestal(2), fSlopePedestal(2)
  // Read in fPosOff[0].SetZ, fPosOff[1].SetZ, fPosOff[2].SetZ
  // Read in fRaw2Pos[0](2,2), fPosOff[0].SetX, fPosOff[0].SetY
  // Read in fRaw2Pos[1](2,2), fPosOff[1].SetX, fPosOff[1].SetY
  // Read in fRaw2Pos[2](2,2), fPosOff[2].SetX, fPosOff[3].SetY

  char chead[255], cdummy[100];
  Float_t fd1,fd2,fd3,fd4,fd5,fd6;
  fd1 = 0; fd2 = 0; fd3 = 0;
  fd4 = 0; fd5 = 0; fd6 = 0;
  
  int status = 1;
  
  while (fgets( buf, LEN, fi) != NULL) {

    if (strstr(buf,"detmap") != NULL) {
      sscanf(buf,"%s %d %s %d ", cdummy, &fDaqCrate, chead, &fDaqOff);
      fDaqHead = header_str_to_base16(chead);
      if (locdebug) {
	printf("   detmap: crate %d    header %s  = 0x%x  offset  %d  \n",
	       fDaqCrate,chead,fDaqHead,fDaqOff);
      }  
    }
    
    if (strstr(buf,"frequency") != NULL) {
      sscanf(buf, "%s %f%f", cdummy, &fd1, &fd2);
      fRasterFreq(0) = fd1;
      fRasterFreq(1) = fd2;
      if (locdebug) printf("raster freq %f %f\n",fRasterFreq(0),fRasterFreq(1));
    }

    if (strstr(buf,"pedestals") != NULL) {
      sscanf(buf, "%s %f%f%f%f", cdummy, &fd1, &fd2, &fd3, &fd4);
      fRasterPedestal(0) = fd1;
      fRasterPedestal(1) = fd2;
      fSlopePedestal(0) = fd3;
      fSlopePedestal(1) = fd4;
      if (locdebug) printf("raster ped %f %f %f %f\n",fRasterPedestal(0),fRasterPedestal(1),fSlopePedestal(0),fSlopePedestal(1));
    }
    
    if (strstr(buf,"z-offsets") != NULL) {
      sscanf(buf, "%s %f%f%f", cdummy, &fd1, &fd2, &fd3);
      fPosOff[0].SetZ(fd1);
      fPosOff[1].SetZ(fd2);
      fPosOff[2].SetZ(fd3);
    }
    
    if (strstr(buf,"x-y-offsets-0") != NULL) {
      sscanf(buf, "%s %f%f%f%f%f%f", cdummy, &fd1, &fd2, &fd3, &fd4, &fd5, &fd6);
      fRaw2Pos[0](0,0) = fd1;
      fRaw2Pos[0](1,0) = fd2;
      fRaw2Pos[0](0,1) = fd3;
      fRaw2Pos[0](1,1) = fd4;
      fPosOff[0].SetX(fd5);
      fPosOff[0].SetY(fd6);
    }  
    
    if (strstr(buf,"x-y-offsets-1") != NULL) {
      sscanf(buf, "%s %f%f%f%f%f%f", cdummy, &fd1, &fd2, &fd3, &fd4, &fd5, &fd6);
      fRaw2Pos[1](0,0) = fd1;
      fRaw2Pos[1](1,0) = fd2;
      fRaw2Pos[1](0,1) = fd3;
      fRaw2Pos[1](1,1) = fd4;
      fPosOff[1].SetX(fd5);
      fPosOff[1].SetY(fd6);
    }  
    
    if (strstr(buf,"x-y-offsets-2") != NULL) {
      sscanf(buf, "%s %f%f%f%f%f%f", cdummy, &fd1, &fd2, &fd3, &fd4, &fd5, &fd6);
      fRaw2Pos[2](0,0) = fd1;
      fRaw2Pos[2](1,0) = fd2;
      fRaw2Pos[2](0,1) = fd3;
      fRaw2Pos[2](1,1) = fd4;
      fPosOff[2].SetX(fd5);
      fPosOff[2].SetY(fd6);
    }  
    
    // Find timestamp, if any, for the raster constants. 
    // Give up and rewind to current file position on any non-matching tag.
    // Timestamps should be in ascending order
    SeekDBdate( fi, date, true );
    
}

  fclose(fi);
  return kOK;
}

//_____________________________________________________________________________
Int_t GenRaster::DefineVariables( EMode mode )
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
GenRaster::~GenRaster()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();

}

//_____________________________________________________________________________
inline 
void GenRaster::ClearEvent()
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
Int_t GenRaster::Decode( const THaEvData& evdata )
{

  // clears the event structure
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // pedestal subtraction is not foreseen for the raster


  Int_t locdebug = 0;

  ClearEvent();

#ifdef OLDDECODE1
  for (Int_t i = 0; i < fDetMap->GetSize(); i++ ){
    THaDetMap::Module* d = fDetMap->GetModule( i );
    for (Int_t j=0; j< evdata.GetNumChan( d->crate, d->slot ); j++) {
      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j);
      if ((chan>=d->lo)&&(chan<=d->hi)) {
	Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );
	Int_t k = d->first + chan - d->lo -1;
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
  }
#endif

  Int_t len;
  UInt_t data;
  len = evdata.GetRocLength(fDaqCrate);
  if (len < 4) {
    cout << "GenRaster::ERROR: Length of data from crate "<<fDaqCrate<<" too small"<<endl;
    return 0;
  }
  for (Int_t i = 0; i < len; i++) {
    if ((UInt_t)evdata.GetRawData(fDaqCrate,i) == fDaqHead) {  // Found the header @ i
      if (locdebug) cout << "Detector "<<GetName()<<"  ADC data ===> "<< hex << fDaqHead << dec << endl;
      for (Int_t j = 0; j < 4; j++) {   // 4 data words
        data = evdata.GetRawData(fDaqCrate,i+fDaqOff+j);
	  if (j < 2) fRawPos(j) = data;
          if (j >= 2) fRawSlope(j-2) = data;
	if (locdebug) cout << "chan "<<j+1<<"  = 0x"<<hex<<data<<"   = (dec) "<<dec<<data<<endl;
      }
    }
  }

  return 0;
}

//____________________________________________________

Int_t GenRaster::Process( )
{

  for ( Int_t i = 0; i<3; i++) {
    
    //      fPosition[i] = fRaw2Pos[i]*fRawPos+fPosOff[i] ;
    //    this is how i wish it would look like,
    //    but unluckily multiplications between tmatrix and tvector
    //    are not defined, as well as adding a tvector and a tvector3
    //    so i have to do it by hand instead ):

    TVector dum(fRawPos);
    dum*=fRaw2Pos[i]; // This should come from data - altered decode method to read it in - BC
    fPosition[i].SetXYZ( dum(0)+fPosOff[i](0),
			 dum(1)+fPosOff[i](1),
			 dum(2)+fPosOff[i](2)  );

  }
  
  fDirection = fPosition[1] - fPosition[0];
  
  return 0 ;
}


//_____________________________________________________________________________
UInt_t GenRaster::header_str_to_base16(const char* hdr) {
// Utility to convert string header to base 16 integer
  static const char chex[] = "0123456789abcdef";
  if( !hdr ) return 0;
  const char* p = hdr+strlen(hdr);
  UInt_t result = 0;  UInt_t power = 1;
  while( p-- != hdr ) {
    const char* q = strchr(chex,tolower(*p));
    if( q ) {
      result += (q-chex)*power; 
      power *= 16;
    }
  }
  return result;
};
////////////////////////////////////////////////////////////////////////////////
ClassImp(GenRaster)

