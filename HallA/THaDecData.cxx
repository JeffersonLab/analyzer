//*-- Author:   Ole Hansen, July 2018

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
// Hall A version of Podd::DecData with support for reading the Hall A
// legacy database file format and for decoding trigger bits.
//
// In addition to the channel contents that Podd::DecData supports
// (see comments in DecData.cxx), this class adds support for
// "bitNN" variables, where NN is a number 0-31. They
// report the value of a trigger bit read via a multihit TDC.
// Database entries for such variables would be as follows:
//
//  D.bit =
//  # trigger bit          crate   slot   channel  cutlo  cuthi
//           bit1            4      11      64       0    2000
//           bit2            4      11      65       0    2000
// etc.
// Ask the DAQ expert for more information.
//
//////////////////////////////////////////////////////////////////////////

// If defined, add support for reading legacy database format
#define DECDATA_LEGACY_DB

#include "THaDecData.h"
#include "TDatime.h"
#include "TClass.h"
#include "BdataLoc.h"
#include "TrigBitLoc.h"

#include <cstdio>

#ifdef DECDATA_LEGACY_DB
# include "TObjArray.h"
# include "TObjString.h"
# include <memory>  // for unique_ptr
#endif

using namespace std;
using namespace Podd;

//_____________________________________________________________________________
THaDecData::THaDecData( const char* name, const char* descript )
  : Podd::DecData(name,descript)
{
}

//_____________________________________________________________________________
FILE* THaDecData::OpenFile( const TDatime& date )
{
  // Open THaDecData database file. First look for standard file name,
  // e.g. "db_D.dat". If not found and if legacy file format support is
  // configured, look for legacy file name "decdata.map"

  FILE* fi = THaApparatus::OpenFile( date );
#ifdef DECDATA_LEGACY_DB
  if( fi )
    return fi;
  fi = OpenDBFile("decdata.dat", date,
                  Here("OpenDBFile()"), "r", fDebug);
#endif
  return fi;
}

//_____________________________________________________________________________
#ifdef DECDATA_LEGACY_DB

inline
static TString& GetString( const TObjArray* params, Int_t pos )
{
  return GetObjArrayString(params,pos);
}

//_____________________________________________________________________________
static Int_t ReadOldFormatDB( FILE* file, map<TString,TString>& configstr_map )
{
  // Read old-style THaDecData database file and put results into a map from
  // database key to value (simulating the new-style key/value database info).
  // Old-style "crate" objects are all assumed to be multihit channels, even
  // though they usually are not.

  const Int_t bufsiz = 256;
  char* buf = new char[bufsiz];
  string dbline;
  const int nkeys = 3;
  TString confkey[nkeys] = { "multi", "word", "bit" };
  TString confval[nkeys];
  // Read all non-comment lines
  rewind(file);
  while( ReadDBline(file, buf, bufsiz, dbline) != EOF ) {
    if( dbline.empty() ) continue;
    // Tokenize each line read
    TString line( dbline.c_str() );
    unique_ptr<TObjArray> tokens( line.Tokenize(" \t") );
    TObjArray* params = tokens.get();
    if( params->IsEmpty() || params->GetLast() < 4 ) continue;
    // Determine data type
    bool is_slot = ( GetString(params,1) == "crate" );
    int idx = is_slot ? 0 : 1;
    TString name = GetString(params,0);
    // TrigBits are crate types with special names
    bool is_bit = ( is_slot && name.BeginsWith("bit") && name.Length() > 3  );
    if( is_bit ) {
      TString name2 = name(3,name.Length());
      if( name2.IsDigit() && name2.Atoi() < 32 )
	idx = 2;
    }
    confval[idx] += name;
    confval[idx] += " ";
    for( int i = 2; i < 5; ++ i ) {
      confval[idx] += GetString(params,i).Data();
      confval[idx] += " ";
    }
    if( is_bit ) {
      // Simulate the hardcoded cut range for event type bit TDC channels
      // from the old THaDecData
      confval[idx] += "0 2000 ";
    }
  }
  delete [] buf;
  // Put the retrieved strings into the key/value map
  for( int i = 0; i < nkeys; ++ i ) {
    if( !confval[i].IsNull() )
      configstr_map[confkey[i]] = confval[i];
  }
  return 0;
}
#endif

//_____________________________________________________________________________
Int_t THaDecData::SetupDBVersion( FILE* file, Int_t db_version )
{
  // If configured, check database format and, if detected, read legacy
  // format database. Otherwise do what the base class does.
#ifdef DECDATA_LEGACY_DB
  fConfigstrMap.clear();
  if( db_version == 1 )
    return ReadOldFormatDB( file, fConfigstrMap );
  else
#endif
  return Podd::DecData::SetupDBVersion(file,db_version);
}

//_____________________________________________________________________________
Int_t THaDecData::GetConfigstr( FILE* file, const TDatime& date,
				Int_t db_version,
				const BdataLoc::BdataLocType& loctype,
				TString& configstr )
{
#ifdef DECDATA_LEGACY_DB
  // Retrieve old-format database parameters read above for this type
  if( db_version == 1 ) {
    auto found = fConfigstrMap.find(loctype.fDBkey);
    if( found == fConfigstrMap.end() )
      return -1;
    configstr = found->second;
    return 0;
  } else
#endif
  return Podd::DecData::GetConfigstr(file,date,db_version,loctype,configstr);
}

//_____________________________________________________________________________
Int_t THaDecData::ReadDatabase( const TDatime& date )
{
  // Read THaDecData database

  Int_t err = Podd::DecData::ReadDatabase(date);
#ifdef DECDATA_LEGACY_DB
  fConfigstrMap.clear();
#endif
  if( err )
    return err;

  // Configure the trigger bits with a pointer to our evtypebits
  TIter next( &fBdataLoc );
  while( auto* dataloc = static_cast<BdataLoc*>( next() ) ) {
    if( dataloc->IsA() == TrigBitLoc::Class() )
      dataloc->OptionPtr( &evtypebits );
  }

  fIsInit = true;
  return kOK;
}


////////////////////////////////////////////////////////////////////////////////

ClassImp(THaDecData)
