//*-- Author :    Ole Hansen    August 2002
////////////////////////////////////////////////////////////////////////
//
// THaG0HelicityReader
//
////////////////////////////////////////////////////////////////////////

#include "THaG0HelicityReader.h"
#include "THaEvData.h"
#include "TMath.h"
#include "TError.h"
#include <iostream>

using namespace std;

//____________________________________________________________________
THaG0HelicityReader::THaG0HelicityReader() :
  fPresentReading(0), fQrt(0), fGate(0), fTimestamp(0),
  fOldT1(-1.0), fOldT2(-1.0), fOldT3(-1.0), fValidTime(kFALSE),
  fHaveROCs(kFALSE), fG0Debug(0)
{
  // Default constructor

  memset( fROCinfo, 0, sizeof(fROCinfo) );
}

//____________________________________________________________________
THaG0HelicityReader::~THaG0HelicityReader() 
{

}

//____________________________________________________________________
Int_t THaG0HelicityReader::SetROCinfo( Int_t which, Int_t roc,
				       Int_t header, Int_t index )
{
  // Define source and offset of data.  Normally called by ReadDatabase
  // of the detector that is a THaG0HelicityReader.
  //
  // "which" is one of  { kHel, kTime, kROC2, kROC3 }.
  // You must define at least the kHel and kTime ROCs.
  // Returns <0 if parameter error, 0 if success

  if( which<kHel || which>kROC3 )
    return -1;
  if( roc <= 0 || roc > 255 )
    return -2;

  fROCinfo[which].roc    = roc;
  fROCinfo[which].header = roc;
  fROCinfo[which].index  = roc;

  fHaveROCs = ( fROCinfo[kHel].roc > 0 && fROCinfo[kTime].roc > 0 );
    
  return 0;
}

//____________________________________________________________________
void THaG0HelicityReader::ClearG0() 
{
  fPresentReading = fQrt = fGate = 0;
  fTimestamp = 0.0;
  fValidTime = kFALSE;
}

//____________________________________________________________________
Int_t THaG0HelicityReader::ReadData( const THaEvData& evdata ) 
{
  
  // Obtain the present data from the event for G0 helicity mode.

  static const char* here = "THaG0HelicityReader::ReadData";

  if( !fHaveROCs ) {
    ::Error( here, "ROC data (detector map) not properly set up." );
    return -1;
  }
  Int_t hroc = fROCinfo[kHel].roc;
  Int_t len = evdata.GetRocLength(hroc);
  if (len <= 4) 
    return -1;

  Int_t ihel = FindWord( evdata, fROCinfo[kHel] );
  if (ihel < 0) {
    ::Error( here , "Cannot find helicity" );
    return -1;
  }
  Int_t itime = FindWord (evdata, fROCinfo[kTime] );
  if (itime < 0) {
    ::Error( here, "Cannot find timestamp" );
    return -1;
  }

  Int_t data = evdata.GetRawData( hroc, ihel );   
  fPresentReading = (data & 0x10) >> 4;
  fQrt = (data & 0x20) >> 5;
  fGate = (data & 0x40) >> 6;
  fTimestamp = evdata.GetRawData( hroc, itime);

  // Look for redundant clock info and patch up time if it appears wrong

  if (fROCinfo[kROC2].roc > 0 && fROCinfo[kROC3].roc > 0) {
    Int_t itime2 = FindWord( evdata, fROCinfo[kROC2] );
    Int_t itime3 = FindWord( evdata, fROCinfo[kROC3] );
    if (itime2 >= 0 && itime3 >= 0) {
      Double_t t1 = fTimestamp;
      Double_t t2 = evdata.GetRawData (fROCinfo[kROC2].roc, itime2);
      Double_t t3 = evdata.GetRawData (fROCinfo[kROC3].roc, itime3);
      Double_t t2t1 = fOldT1 + t2 - fOldT2;
      Double_t t3t1 = fOldT1 + t3 - fOldT3;
      // We believe t1 unless it disagrees with t2 and t2 agrees with t3
      if (fOldT1 >= 0.0 && TMath::Abs(t1-t2t1) > 3.0 
	  && TMath::Abs(t2t1-t3t1) <= 3.0 ) {
	if (fG0Debug >= 1)
	  ::Warning( here, "Clock 1 disagrees with 2, 3; using 2: %lf %lf %lf",
		     t1, t2t1, t3t1 );
	fTimestamp = t2t1;
      }
      if (fG0Debug >= 3) {
	cout << "Clocks: " << t1 
	     << " " << t2 << "->" << t2t1 
	     << " " << t3 << "->" << t3t1 
	     << endl;
      }
      fOldT1 = fTimestamp;
      fOldT2 = t2;
      fOldT3 = t3;
    }
  }
       
  fValidTime = kTRUE;

  if (fG0Debug >= 3) {
    cout << dec << "-->  evtype " << evdata.GetEvType()
	 << "  helicity " << fPresentReading
	 << "  qrt " << fQrt
	 << " gate " << fGate
	 << "   time stamp " << fTimestamp 
	 << endl;
  }

  return 0;
}

//____________________________________________________________________
Int_t THaG0HelicityReader::FindWord( const THaEvData& evdata, 
				     const ROCinfo& info )
{
  Int_t len = evdata.GetRocLength(info.roc);
  if (len <= 4) 
    return -1;

  Int_t i;
  if( info.header == 0 )
    i = info.index;
  else {
    for( i=0; i<len && evdata.GetRawData(info.roc, i) != info.header; ++i);
    i += info.index;
  }
  return (i < len) ? i : -1;
}

ClassImp(THaG0HelicityReader)
