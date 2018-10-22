//*-- Author :    Ole Hansen    August 2006
// Extracted from Bob Michaels' THaHelicity CVS 1.19
////////////////////////////////////////////////////////////////////////
//
// THaG0HelicityReader
//
////////////////////////////////////////////////////////////////////////

#include "THaG0HelicityReader.h"
#include "THaEvData.h"
#include "TMath.h"
#include "TError.h"
#include "VarDef.h"
#include "THaAnalysisObject.h"   // For LoadDB
#include <iostream>
#include <vector>

using namespace std;

//____________________________________________________________________
THaG0HelicityReader::THaG0HelicityReader() :
  fPresentReading(0), fQrt(0), fGate(0), fTimestamp(0),
  fOldT1(-1.0), fOldT2(-1.0), fOldT3(-1.0), fValidTime(false),
  fG0Debug(0), fHaveROCs(false), fNegGate(false)
{
  // Default constructor

  memset( fROCinfo, 0, sizeof(fROCinfo) );
}

//____________________________________________________________________
THaG0HelicityReader::~THaG0HelicityReader()
{
  // Destructor
}

//____________________________________________________________________
void THaG0HelicityReader::Clear( Option_t* ) 
{
  fPresentReading = fQrt = fGate = 0;
  fTimestamp = 0.0;
  fValidTime = false;
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
    for( i=0; i<len && evdata.GetRawData(info.roc, i) != info.header; 
	 ++i) {}
    i += info.index;
  }
  return (i < len) ? i : -1;
}

//_____________________________________________________________________________
Int_t THaG0HelicityReader::ReadDatabase( const char* dbfilename,
					 const char* prefix,
					 const TDatime& date,
					 int debug_flag )
{
  // Read parameters from database:  ROC info (detector map), G0 delay value

  static const char* const here = "THaG0HelicityReader::ReadDatabase";

  FILE* file =
    THaAnalysisObject::OpenFile( dbfilename, date, here, "r", debug_flag );
  if( !file ) return THaAnalysisObject::kFileError;

  EROC rocname[4] = { kHel, kTime, kROC2, kROC3 };
  vector< vector<Int_t> > rocaddr(4); // helroc, timeroc, time2roc, time3roc
  Int_t invert_gate = 0;
  DBRequest req[] = {
    { "helroc",      &rocaddr[0],  kIntV,   0, 0, -2 },
    { "timeroc",     &rocaddr[1],  kIntV,   0, 0, -2 },
    { "time2roc",    &rocaddr[2],  kIntV,   0, 1, -2 },
    { "time3roc",    &rocaddr[3],  kIntV,   0, 1, -2 },
    { "neg_g0_gate", &invert_gate, kInt,    0, 1, -2 },
    { 0 }
  };
  Int_t st = THaAnalysisObject::LoadDB( file, date, req, prefix );
  fclose(file);
  if( st )
    return THaAnalysisObject::kInitError;

  // TODO: implement reading into C-style arrays in LoadDB,
  // then read fROCinfo directly
  
  for( vector<Int_t>::size_type i = 0; i < rocaddr.size() && !st; ++i ) {
    vector<Int_t>& r = rocaddr[i];
    if( r.size() >= 3 )
      st = SetROCinfo( rocname[i], r[0], r[1], r[2] );
  }
  if( !fHaveROCs || st ) {
    ::Error( here, "Invalid ROC data. Fix database." );
    return THaAnalysisObject::kInitError;
  }

  fNegGate = (invert_gate != 0);

  return THaAnalysisObject::kOK;
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
  fPresentReading = ((data & 0x10) != 0);
  fQrt            = ((data & 0x20) != 0);
  fGate           = ((data & 0x40) != 0);
  fTimestamp = static_cast<Double_t>( evdata.GetRawData( hroc, itime) );

  // Invert the gate polarity if requested
  if( fNegGate )
    fGate = !fGate;

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
       
  fValidTime = true;

  return 0;
}

//TODO: this should not be needed once LoadDB can fill fROCinfo directly
//____________________________________________________________________
Int_t THaG0HelicityReader::SetROCinfo( EROC which, Int_t roc,
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
  fROCinfo[which].header = header;
  fROCinfo[which].index  = index;

  fHaveROCs = ( fROCinfo[kHel].roc > 0 && fROCinfo[kTime].roc > 0 );
    
  return 0;
}

//____________________________________________________________________
ClassImp(THaG0HelicityReader)
