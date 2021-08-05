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
#include "Textvars.h"
#include <cstdio>  // for FILE
#include <vector>
#include <string>
#include <type_traits>

class TObjArray;
class TDatime;
class TString;

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
                     const char* filemode, int debug_flag, const char*& openpath );
Int_t    ReadDBline( FILE* file, char* buf, Int_t bufsiz, std::string& line );

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

}  // namespace Podd

#endif //Podd_Database_h_
