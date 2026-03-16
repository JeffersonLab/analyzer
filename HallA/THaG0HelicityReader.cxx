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
#include "Database.h"
#include "THaAnalysisObject.h"   // For LoadDB
#include <iostream>
#include <vector>

using namespace std;

//____________________________________________________________________
Bool_t THaG0HelicityReader::ROCinfo::valid() const
{
  return roc < Decoder::MAXROC;
}

//____________________________________________________________________
THaG0HelicityReader::THaG0HelicityReader()
  : fPresentReading(0)
  , fQrt(0)
  , fGate(0)
  , fTimestamp(0)
  , fOldT1(-1.0)
  , fOldT2(-1.0)
  , fOldT3(-1.0)
  , fValidTime(false)
  , fG0Debug(0)
  , fHaveROCs(false)
  , fNegGate(false)
{
  // Default constructor
}

//____________________________________________________________________
void THaG0HelicityReader::Clear( Option_t* ) 
{
  fPresentReading = fQrt = fGate = 0;
  fTimestamp = 0.0;
  fValidTime = false;
}

//____________________________________________________________________
UInt_t THaG0HelicityReader::FindWord( const THaEvData& evdata,
                                      const ROCinfo& info )
{
  UInt_t len = evdata.GetRocLength(info.roc);
  if (len <= 4) 
    return -1;

  UInt_t i = 0;
  if( info.header == 0 )
    i = info.index;
  else {
    for( ; i < len && evdata.GetRawData(info.roc, i) != info.header; ++i ) {}
    i += info.index;
  }
  return (i < len) ? i : kMaxUInt;
}

//_____________________________________________________________________________
Int_t THaG0HelicityReader::ReadDatabase( const char* dbfilename,
					 const char* prefix,
					 const TDatime& date,
					 int debug_flag )
{
  // Read parameters from database:  ROC info (detector map), G0 delay value

  static const char* const here = "THaG0HelicityReader::ReadDatabase";

  FILE* file = Podd::OpenDBFile(dbfilename, date, here, "r", debug_flag);
  if( !file ) return THaAnalysisObject::kFileError;

  Int_t invert_gate = 0;
  fROCinfo[kROC2].roc = fROCinfo[kROC3].roc = kMaxUInt;
  const vector<DBRequest> req = {
    { .name = "helroc",      .var = &fROCinfo[kHel],  .type = kInt, .nelem = 3, .optional = false, .search = -2 },
    { .name = "timeroc",     .var = &fROCinfo[kTime], .type = kInt, .nelem = 3, .optional = false, .search = -2 },
    { .name = "time2roc",    .var = &fROCinfo[kROC2], .type = kInt, .nelem = 3, .optional = true,  .search = -2 },
    { .name = "time3roc",    .var = &fROCinfo[kROC3], .type = kInt, .nelem = 3, .optional = true,  .search = -2 },
    { .name = "neg_g0_gate", .var = &invert_gate,     .type = kInt, .nelem = 0, .optional = true,  .search = -2 },
  };
  Int_t st = Podd::LoadDatabase( file, date, req, prefix );
  (void)fclose(file);
  if( st )
    return THaAnalysisObject::kInitError;

  // Require the helicity and time ROCs to be defined
  fHaveROCs = ( fROCinfo[kHel].valid() && fROCinfo[kTime].valid() );
  // If ROC2 or ROC3 are defined in tha database, they must be valid
  st = any_of(fROCinfo+kROC2, fROCinfo + kROC3+1, []( const ROCinfo& R ) {
    return R.roc != kMaxUInt && !R.valid();
  });

  if( st || !fHaveROCs ) {
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
  UInt_t hroc = fROCinfo[kHel].roc;
  UInt_t len = evdata.GetRocLength(hroc);
  if (len <= 4) 
    return -1;

  UInt_t ihel = FindWord( evdata, fROCinfo[kHel] );
  if (ihel == kMaxUInt) {
    ::Error( here , "Cannot find helicity" );
    return -1;
  }
  UInt_t itime = FindWord( evdata, fROCinfo[kTime] );
  if (itime == kMaxUInt) {
    ::Error( here, "Cannot find timestamp" );
    return -1;
  }

  UInt_t data = evdata.GetRawData( hroc, ihel );
  fPresentReading = ((data & 0x10) != 0);
  fQrt            = ((data & 0x20) != 0);
  fGate           = ((data & 0x40) != 0);
  fTimestamp = static_cast<Double_t>( evdata.GetRawData( hroc, itime) );

  // Invert the gate polarity if requested
  if( fNegGate )
    fGate = !fGate;

  // Look for redundant clock info and patch up the time if it appears wrong

  if( fROCinfo[kROC2].valid() && fROCinfo[kROC3].valid() ) {
    UInt_t itime2 = FindWord( evdata, fROCinfo[kROC2] );
    UInt_t itime3 = FindWord( evdata, fROCinfo[kROC3] );
    if (itime2 != kMaxUInt && itime3 != kMaxUInt ) {
      Double_t t1 = fTimestamp;
      Double_t t2 = evdata.GetRawData(fROCinfo[kROC2].roc, itime2);
      Double_t t3 = evdata.GetRawData(fROCinfo[kROC3].roc, itime3);
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

//____________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaG0HelicityReader)
#endif
