#ifndef Podd_THaDecData_h_
#define Podd_THaDecData_h_

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
// Hall A version of Podd::DecData with support for legacy decdata.dat
// file format and trigger bit decoding.
//
//////////////////////////////////////////////////////////////////////////

#include "DecData.h"
#include "TString.h"
#include <map>

class THaDecData : public Podd::DecData {
public:
  explicit THaDecData( const char* name = "D",
                       const char* description = "Raw Hall A decoder data" );
  FILE*    OpenFile( const TDatime& date ) override;

protected:
  Int_t    ReadDatabase( const TDatime& date ) override;

  // Expansion hooks for DecData::ReadDatabase
  Int_t    SetupDBVersion( FILE* file, Int_t db_version ) override;
  Int_t    GetConfigstr( FILE* file, const TDatime& date, Int_t db_version,
                         const BdataLoc::BdataLocType& loctype,
                         TString& configstr ) override;

  std::map<TString,TString> fConfigstrMap;

  ClassDefOverride(THaDecData,0)
};

#endif
