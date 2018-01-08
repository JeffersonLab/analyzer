//*-- Author :    Ole Hansen     08-Jan-18

//////////////////////////////////////////////////////////////////////////
//
// Podd::FileInclude
//
// Helper functions for supporting '#include' directives in
// configuration files
//
//////////////////////////////////////////////////////////////////////////

#include "FileInclude.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TSystem.h"

#include <vector>
#include <sstream>
#include <cassert>
#include <memory>  // for auto_ptr/unique_ptr

using namespace std;

#if __cplusplus >= 201103L
# define SMART_PTR unique_ptr
#else
# define SMART_PTR auto_ptr
#endif

namespace Podd {

const string kIncTag     = "#include";
const string kWhiteSpace = " \t";

//_____________________________________________________________________________
Int_t GetIncludeFileName( const string& line, string& incfile )
{
  // Extract file name from #include statement

  if( line.substr(0,kIncTag.length()) != kIncTag ||
      line.length() <= kIncTag.length() )
    return -1; // Not an #include statement (should never happen)
  string::size_type pos = line.find_first_not_of(kWhiteSpace,kIncTag.length()+1);
  if( pos == string::npos || (line[pos] != '<' && line[pos] != '\"') )
    return -2; // No proper file name delimiter
  const char end = (line[pos] == '<') ? '>' : '\"';
  string::size_type pos2 = line.find(end,pos+1);
  if( pos2 == string::npos || pos2 == pos+1 )
    return -3; // Unbalanced delimiters or zero length filename
  if( line.length() > pos2 &&
      line.find_first_not_of(kWhiteSpace,pos2+1) != string::npos )
    return -4; // Trailing garbage after filename spec

  incfile = line.substr(pos+1,pos2-pos-1);
  return 0;
}

//_____________________________________________________________________________
Int_t CheckIncludeFilePath( string& incfile )
{
  // Check if 'incfile' can be opened in the current directory or
  // any of the directories in the include path

  vector<TString> paths;
  paths.push_back( incfile.c_str() );

  TString incp = gSystem->Getenv("ANALYZER_CONFIGPATH");
  if( !incp.IsNull() ) {
    SMART_PTR<TObjArray> incdirs( incp.Tokenize(":") );
    if( !incdirs->IsEmpty() ) {
      Int_t ndirs = incdirs->GetLast()+1;
      assert( ndirs>0 );
      for( Int_t i = 0; i < ndirs; ++i ) {
	TString path = (static_cast<TObjString*>(incdirs->At(i)))->String();
	if( path.IsNull() )
	  continue;
	if( !path.EndsWith("/") )
	  path.Append("/");
	path.Append( incfile.c_str() );
	paths.push_back( path.Data() );
      }
    }
  }

  for( vector<TString>::size_type i = 0; i<paths.size(); ++i ) {
    TString& path = paths[i];
    if( !gSystem->ExpandPathName(path) &&
	!gSystem->AccessPathName(path,kReadPermission) ) {
      incfile = path.Data();
      return 0;
    }
  }
  return -1;
}

//_____________________________________________________________________________

} // namespace Podd
