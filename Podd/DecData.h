#ifndef Podd_DecData_h_
#define Podd_DecData_h_

//////////////////////////////////////////////////////////////////////////
//
// Podd::DecData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "THashList.h"
#include "BdataLoc.h"

class TString;

namespace Podd {

class DecData : public THaApparatus {

public:
  explicit DecData( const char* name = "D",
                    const char* description = "Raw decoder data" );
  virtual ~DecData();

  virtual EStatus Init( const TDatime& run_time );
  virtual void    Clear( Option_t* opt="" );
  virtual Int_t   Decode( const THaEvData& );
  virtual void    Print( Option_t* opt="" ) const;
  virtual void    Reset( Option_t* opt="" );

  // Disabled functions from THaApparatus
  virtual Int_t   AddDetector( THaDetector*, Bool_t, Bool_t ) { return 0; }
  virtual Int_t   Reconstruct() { return 0; }

protected:
  UInt_t          evtype;      // CODA event type
  UInt_t          evtypebits;  // Bitpattern of active trigger numbers
  THashList       fBdataLoc;   // Raw data channels

  virtual Int_t   DefineVariables( EMode mode = kDefine );
  virtual Int_t   ReadDatabase( const TDatime& date );

  Int_t           DefineLocType( const BdataLoc::BdataLocType& loctype,
				 const TString& configstr, bool re_init );

  // Expansion hooks for ReadDatabase
  virtual Int_t   SetupDBVersion( FILE* file, Int_t db_version );
  virtual Int_t   GetConfigstr( FILE* file, const TDatime& date,
				Int_t db_version,
				const BdataLoc::BdataLocType& loctype,
				TString& configstr );

  ClassDef(DecData,0)
};

} // namespace Podd

#endif
