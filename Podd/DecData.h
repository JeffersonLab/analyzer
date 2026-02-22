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
  ~DecData() override;

  EStatus         Init( const TDatime& run_time ) override;
  void            Clear( Option_t* opt = "" ) override;
  Int_t           Decode( const THaEvData& ) override;
  void            Print( Option_t* opt = "" ) const override;
  virtual void    Reset( Option_t* opt = "" );

  // Disabled functions from THaApparatus
  Int_t           AddDetector( THaDetector*, Bool_t, Bool_t ) override { return 0; }
  Int_t           Reconstruct() override { return 0; }

protected:
  UInt_t          evtype;      // CODA event type
  UInt_t          evtypebits;  // Bitpattern of active trigger numbers
  THashList       fBdataLoc;   // Raw data channels

  Int_t           DefineVariables( EMode mode = kDefine ) override;
  Int_t           ReadDatabase( const TDatime& date ) override;

  Int_t           DefineLocType( const BdataLoc::BdataLocType& loctype,
                                 const TString& configstr, bool re_init );

  // Expansion hooks for ReadDatabase
  virtual Int_t   SetupDBVersion( FILE* file, Int_t db_version );
  virtual Int_t   GetConfigstr( FILE* file, const TDatime& date,
                                Int_t db_version,
                                const BdataLoc::BdataLocType& loctype,
                                TString& configstr );

  ClassDefOverride(DecData,0)
};

} // namespace Podd

#endif
