///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// GenBPM                                                                    //
//                                                                           //
// Class for a BPM                                                           //
// measuring signals from four antennas                                      //
// and  the position at the BPM (without phase correction)                   //
// works with standard ADCs (lecroy like types)                              //
// is good for unrastered beam, or to get average positions                  //
//                                                                           //
// Modified by B. Craver and R. Michaels                                     //
//                                                                           //
//     ReadDataBase uses a different format (datamap not used, data          //
//              comes from VME).                                             //
//     Decode does VME decoding using parameters from ReadDataBase           //
//                                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//#define OLDDECODE1
#include "GenBPM.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "TMath.h"

using namespace std;

ClassImp(GenBPM)

//_____________________________________________________________________________
GenBPM::GenBPM( const char* name, const char* description,
				  THaApparatus* apparatus ) :
  THaBeamDet(name,description,apparatus),
  fRawSignal(4),fPedestals(4),fCorSignal(4),fRotPos(2),fRot2HCSPos(2,2)
{
  // Constructor

  fDaqCrate = 0;
  fDaqHead = 0;
  fDaqOff = 0;

}


//_____________________________________________________________________________
//
// ReadDatabase:  if detectors cant be added to detmap 
//                or entry for bpms is missing           -> kInitError
//                otherwise                              -> kOk
//                CAUTION: i do not check for incomplete or 
//                         inconsistent data
//
Int_t GenBPM::ReadDatabase( const TDatime& date )
{
  static const char* const here = "ReadDatabase()";

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];
  int locdebug = 1; 
  
  FILE* fi = OpenFile( date );
  if( !fi) return kInitError;

  // Seek our database section
  //sprintf(keyword,"[%s_detmap]",GetName());
  //cout << "keyword = "<<keyword<<endl;

  Int_t n=strlen(keyword);
  if (locdebug) {
     printf("Reading database: BPM %s   desc %s \n",GetName(),GetTitle());
  }

  char chead[255], cdummy[100];
  Float_t fd1,fd2,fd3,fd4;

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
    if (strstr(buf,"posoff") != NULL) {
      sscanf(buf,"%s %f %f %f ", cdummy, &fd1, &fd2, &fd3);
      fOffset(0) = fd1;
      fOffset(1) = fd2;
      fOffset(2) = fd3;
      if (locdebug) {
	printf("   Position offset:  %f  %f  %f ",fOffset(0),fOffset(1),fOffset(2));
      }
    }
    if (strstr(buf,"calibration") != NULL) {
      sscanf(buf,"%s %f ", cdummy, &fCalibRot);
      if (locdebug) printf("  Calibration const  %f \n",fCalibRot);
    }     
    if (strstr(buf,"pedestals") != NULL) {
      sscanf(buf,"%s %f %f %f %f \n", cdummy, &fd1, &fd2, &fd3, &fd4);
      fPedestals(0)=fd1;
      fPedestals(1)=fd2;
      fPedestals(2)=fd3;
      fPedestals(3)=fd4;
      if (locdebug) printf("   Pedestals  %f %f %f %f \n",fPedestals(0),fPedestals(1),fPedestals(2),fPedestals(3));
    }
    if (strstr(buf,"rotation") != NULL) {
      sscanf(buf,"%s %f %f %f %f ", cdummy, &fd1, &fd2, &fd3, &fd4);
      fRot2HCSPos(0,0)=fd1;
      fRot2HCSPos(0,1)=fd2;
      fRot2HCSPos(1,0)=fd3;
      fRot2HCSPos(1,1)=fd4;
      if (locdebug) printf("  Rotation matrix %f %f %f %f \n",fRot2HCSPos(0,0),fRot2HCSPos(0,1),fRot2HCSPos(1,0),fRot2HCSPos(1,1));
    }
    
  }
  
  fclose(fi);
  return kOK;
}

//_____________________________________________________________________________
Int_t GenBPM::DefineVariables( EMode mode )
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
GenBPM::~GenBPM()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
}

//_____________________________________________________________________________
inline 
void GenBPM::ClearEvent()
{
  // Reset per-event data.
  fPosition.SetXYZ(0.,0.,-10000.);
  fDirection.SetXYZ(0.,0.,1.);
  fNfired=0;
  fRawSignal(0)=-1;
  fRawSignal(1)=-1;
  fRawSignal(2)=-1;
  fRawSignal(3)=-1;  
  fCorSignal(0)=-1;
  fCorSignal(1)=-1;
  fCorSignal(2)=-1;
  fCorSignal(3)=-1;


}

//_____________________________________________________________________________
Int_t GenBPM::Decode( const THaEvData& evdata )
{

  // clears the event structure
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // performs pedestal subtraction

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
	if ((k<4)&&(fRawSignal(k)==-1)) {
	  fRawSignal(k)= data;
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
    cout << "GenBPM::ERROR: Length of data from crate "<<fDaqCrate<<" too small: " <<len<<endl;
    return 0;
  }

  for (Int_t i = 0; i < len; i++) {
    if (evdata.GetRawData(fDaqCrate,i) == fDaqHead) {  // Found the header @ i
      if (locdebug) 
	{
	  cout << "BPM "<<GetName()<<"  ADC data ===> "<< hex << fDaqHead << dec << endl;
	  cout << "Event: " <<evdata.GetEvNum()<<endl;
	}
      for (Int_t j = 0; j < 4; j++) {   // 4 data words
        data = evdata.GetRawData(fDaqCrate,i+fDaqOff+j);
        fRawSignal(j) = data;
	if (locdebug) 
	  cout << "chan "<<j+1<<"  = 0x"<<hex<<data<<"   = (dec) "<<dec<<data<<endl;
      }
    }
  }

  fCorSignal = fRawSignal;
  fCorSignal -= fPedestals;


  return 0;
}

//____________________________________________________
//  
Int_t GenBPM::Process( )
{
 
  // called by the beam apparaturs 
  // uses the pedestal substracted signals from the antennas
  // to get the position in the bpm coordinate system 
  // and uses the transformation matrix defined in the database
  // to transform it into the HCS
  // directions are not calculated, they are always set parallel to z

  Double_t ap, am;

  for (Int_t k=0; k<2; k++) {
    if (fCorSignal(2*k)+fCorSignal(2*k+1)!=0.0) {
      ap=fCorSignal(2*k);
      am=fCorSignal(2*k+1);
      fRotPos(k)=fCalibRot*(ap-am)/(ap+am);
    }
    else {
      fRotPos(k)=0.0;
    }
  }

  TVector dum(fRotPos);

  dum*=fRot2HCSPos;

  fPosition.SetXYZ(
		   dum(0)+fOffset(0),
		   dum(1)+fOffset(1),
		   fOffset(2)
		   );

  return 0 ;
}



//_____________________________________________________________________________
UInt_t GenBPM::header_str_to_base16(const char* hdr) {
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


