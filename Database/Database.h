#ifndef Podd_Database_h_
#define Podd_Database_h_

//////////////////////////////////////////////////////////////////////////
//
// Podd
//
// Database support functions
//
//////////////////////////////////////////////////////////////////////////

#include "VarDef.h"
#include <cstdio>  // for FILE
#include <vector>
#include <string>

// For backward compatibility with existing client code
#include "Textvars.h"
#include "TDatime.h"
#include "TString.h"

class TObjArray;

// Helper function for error messages
const char* Here( const char* here, const char* prefix = nullptr );

namespace Podd {

// Generic utility functions
TString& GetObjArrayString( const TObjArray* params, Int_t pos );

// File-based database API
std::vector<std::string> GetDBFileList( const char* name, const TDatime& date,
                                        const char* here = "Podd::GetDBFileList()" );

FILE*    OpenDBFile( const char* name, const TDatime& date, const char* here = "Podd::OpenDBFile()",
                     const char* filemode = "r", int debug_flag = 0 );
FILE*    OpenDBFile( const char* name, const TDatime& date, const char* here,
                     const char* filemode, int debug_flag, std::string& openpath );
Int_t    ReadDBline( FILE* file, char* buf, Int_t bufsiz, std::string& line );
Bool_t   DBDatesDiffer( const TDatime& a, const TDatime& b );

//FIXME: BCI: To be removed in next version. Do not use.
FILE*    OpenDBFile( const char* name, const TDatime& date, const char* here,
                     const char* filemode, int debug_flag, const char*& openpath );

// Access functions for reading key/value pairs from database files
template<class T> // instantiations available for all supported types (see VarType.h)
Int_t    LoadDBvalue( FILE* file, const TDatime& date, const char* key, T& value );
Int_t    LoadDBvalue( FILE* file, const TDatime& date, const char* key, std::string& value );
Int_t    LoadDBvalue( FILE* file, const TDatime& date, const char* key, TString& value );
template <class T>
Int_t    LoadDBarray( FILE* file, const TDatime& date, const char* key, std::vector<T>& values );
template <class T>
Int_t    LoadDBmatrix( FILE* file, const TDatime& date, const char* key,
                       std::vector<std::vector<T>>& values, UInt_t ncols );
Int_t    LoadDatabase( FILE* file, const TDatime& date, const DBRequest* request, const char* prefix,
                       Int_t search = 0, const char* here = "Podd::LoadDatabase" );
Int_t    SeekDBdate( FILE* file, const TDatime& date, Bool_t end_on_tag = false );
Int_t    SeekDBconfig( FILE* file, const char* tag, const char* label = "config",
                       Bool_t end_on_tag = false );

Int_t    SeekDBdate( std::istream& istr, const TDatime& date, Bool_t end_on_tag = false );
Bool_t   IsDBtimestamp( const std::string& line, TDatime& keydate );

// Time zone to assume for legacy database time stamps without time zone offsets.
// The default is "US/Eastern".
void     SetDefaultTZ( const char* tz = nullptr );
Long64_t GetTZOffsetToLocal( UInt_t tloc );
extern TString  gDefaultTZ;
extern Bool_t   gNeedTZCorrection;

}  // namespace Podd

// Macro for running arbitrary code ("cmd") with TZ set to gDefaultTZ.
#ifdef __GLIBC__
// glibc seems to require an explicit call to tzset() to pick up a change of TZ
#include <ctime>
#define WithDefaultTZ(cmd)                   \
 TString cur_tz;                             \
 if( Podd::gNeedTZCorrection ) {             \
   cur_tz = gSystem->Getenv("TZ");           \
   gSystem->Setenv("TZ", Podd::gDefaultTZ);  \
   tzset();                                  \
 }                                           \
 cmd;                                        \
 if( Podd::gNeedTZCorrection ) {             \
   if( !cur_tz.IsNull() )                    \
     gSystem->Setenv("TZ", cur_tz );         \
   else                                      \
     gSystem->Unsetenv("TZ");                \
   tzset();                                  \
 }
#else
#define WithDefaultTZ(cmd)                   \
 TString cur_tz;                             \
 if( Podd::gNeedTZCorrection ) {             \
   cur_tz = gSystem->Getenv("TZ");           \
   gSystem->Setenv("TZ", Podd::gDefaultTZ);  \
 }                                           \
 cmd;                                        \
 if( Podd::gNeedTZCorrection ) {             \
   if( !cur_tz.IsNull() )                    \
     gSystem->Setenv("TZ", cur_tz );         \
   else                                      \
     gSystem->Unsetenv("TZ");                \
 }
#endif

#endif //Podd_Database_h_
