//*-- Author :    Ole Hansen   26-Jun-2010

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Podd::Textvars                                                       //
//                                                                      //
// Generic string substitution facility                                 //
// Contains a dictionary of text variable names and associated values.  //
//                                                                      //
// Used for preprocessing of analyzer input files: Variable references  //
// of the form ${name} in input files are replaced by their value.      //
// For example, ${spectro}.s1.adc  ==>  R.s1.adc                        //
//                                                                      //
// The analyzer makes one instance of this class available via the      //
// global gHaTextvars for easy access in scripts.                       //
//                                                                      //
// This code runs at initialization time and thus is not time-critical. //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "Textvars.h"
#include "Helper.h"
#include "TError.h"
#include <utility>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

typedef string::size_type ssiz_t;
typedef vector<string>::size_type vssiz_t;

static const string whtspc = " \t\n\v\f\r";

// Text variable definitions. To be assigned by main program.
Podd::Textvars* gHaTextvars = nullptr;

namespace Podd {

//_____________________________________________________________________________
void Tokenize( const string& s, const string& delim, vector<string>& tokens )
{
  // Tokenize string 's' on delimiter(s) in 'delim'. Put results in 'tokens'.

  tokens.clear();
  if( s.empty() ) {
    return;
  }
  auto start = s.find_first_not_of(delim);
  auto pos   = s.find_first_of(delim, start);

  while( start != string::npos ) {
    tokens.push_back( s.substr(start,pos-start));
    start = s.find_first_not_of(delim,pos);
    pos = s.find_first_of(delim,start);
  }
}

//_____________________________________________________________________________
vector<string> vsplit( const string& s )
{
  // Utility function to split a string into whitespace-separated strings
  vector<string> tokens;
  Podd::Tokenize(s, whtspc, tokens);
  return tokens;
}

//_____________________________________________________________________________
static string ValStr( const vector<string>& s )
{
  // Assemble vector of strings into a comma-separated single string.
  // Used by print functions.

  string val;
  if( !s.empty() ) {
    val = "\"" + s[0] + "\"";
    for( vssiz_t i = 1; i < s.size(); ++i )
      val.append(",\"" + s[i] + "\"");
  }
  return val;
}

//_____________________________________________________________________________
static ssiz_t Index( const string& s, ssiz_t& ext /*, ssiz_t start*/ )
{
  // Find index of text variable pattern in string 's'. 'ext' is the length
  // of the pattern. Search starts at 'start'.
  // This is equivalent to searching for regexp "\$\{[^\{\}]*\}"

  if( s.empty() || /*start == string::npos || start+*/2 >= s.length() )
    return string::npos;

  const char* c = s.c_str() /*+ start*/, *end;
  if( (c = strchr(c,'$')) && *(++c) == '{' ) {
    if( !(end = strchr(++c,'}')) ) return string::npos;
    const char *c2;
    do {
      // Find innermost bracket, allowing nesting
      if( (c2 = strchr(c,'$')) && *(++c2) != '{' ) c2 = nullptr;
    }
    while( c2 && ++c2 < end && (c = c2) );
  } else
    return string::npos;

  c -= 2;   // Point to beginning of pattern: ${
  assert( end>c );
  ext = end-c+1;
  return (c - s.c_str());
}

//_____________________________________________________________________________
Int_t Textvars::Add( const string& name, const string& value )
{
  // Add or Set/Replace text variable with given name and value.
  // Parse value for multiple comma-separated tokens and store them as
  // individual values.

  if( name.empty() )
    return 0;

  static const string delim(",");
  vector<string> tokens;
  Tokenize( value, delim, tokens );
  if( tokens.empty() )
    tokens.emplace_back("");

  auto it = fVars.find(name);

  if( it != fVars.end() ) {  // already exists?
    (*it).second.swap(tokens);
  } else {                   // if not, add a new one
#ifndef NDEBUG
    pair<Textvars_t::iterator,bool> ret =
#endif
      fVars.insert( make_pair(name,tokens) );
    assert( ret.second );
  }
  return 1;
}

//_____________________________________________________________________________
Int_t Textvars::AddVerbatim( const string& name, const string& value )
{
  // Add or Set/Replace text variable with given name and value.
  // Does not scan value for tokens; hence allows values that contain a ","

  if( name.empty() )
    return 0;

  vector<string> values( 1, value );

  auto it = fVars.find(name);

  if( it != fVars.end() ) {  // already exists?
    (*it).second.swap(values);
  } else {                   // if not, add a new one
#ifndef NDEBUG
    pair<Textvars_t::iterator,bool> ret =
#endif
      fVars.insert( make_pair(name,values) );
    assert( ret.second );
  }
  return 1;
}

//_____________________________________________________________________________
const char* Textvars::Get( const string& name, Int_t idx ) const
{
  // Get the i-th value of the text variable with the given name.
  // Returns either the value or nullptr if not found.

  if( name.empty() || idx < 0 )
    return nullptr;

  auto it = fVars.find(name);
  if( it == fVars.end() )
    return nullptr;
  if( (ssiz_t)idx >= it->second.size() )
    return nullptr;

  return it->second[idx].c_str();
}

//_____________________________________________________________________________
UInt_t Textvars::GetArray( const string& name, vector<string>& array )
{
  // Get the array of values for the text variable with the given name.
  // Returns number of values found (size of array), or zero if not found.

  array.clear();
  if( name.empty() )
    return 0;

  auto it = fVars.find(name);
  if( it == fVars.end() )
    return 0;

  array.swap( (*it).second );
  return array.size();
}

//_____________________________________________________________________________
vector<string> Textvars::GetArray( const string& name )
{
  // Get the values array for the text variable with the given name.
  // If name is not found, the returned array has size zero.

  vector<string> the_array;
  GetArray( name, the_array );
  return the_array;
}

//_____________________________________________________________________________
UInt_t Textvars::GetNvalues( const string& name ) const
{
  // Get the number of values for the text variable with the given name.

  if( name.empty() )
    return 0;

  auto it = fVars.find(name);
  if( it == fVars.end() )
    return 0;

  return (*it).second.size();
}

//_____________________________________________________________________________
void Textvars::Print( Option_t* /*opt*/ ) const
{
  // Print all text variables

  size_t maxw = 0;
  for( const auto& var : fVars ) {
    size_t len = var.first.length();
    if( len > maxw )
      maxw = len;
  }
  for( const auto& var : fVars ) {
    cout << "Textvar:  " << setw(int(maxw)) << var.first << " = "
         << ValStr(var.second) << endl;
  }
}

//_____________________________________________________________________________
Int_t Textvars::Substitute( string& line ) const
{
  // Substitute text variables in a single string.
  // If any multi-valued variables are encountered, treat it as an error

  vector<string> lines( 1, line );
  Int_t ret = Substitute( lines, false );
  if( ret )
    return ret;
  assert( lines.size() == 1 );
  line = std::move(lines[0]);
  return 0;
}

//_____________________________________________________________________________
Int_t Textvars::Substitute( vector<string>& lines, bool do_multi ) const
{
  // Substitute text variables in the given array of strings.
  // Supports multi-valued text variables, where each instance is
  // separated by commas; e.g. arm = "L,R".
  //
  // Text Variables have the form ${name}
  //
  // Returns 0 if success, 1 on error (name not found)

  static const char* const here = "Textvars::Substitute";

  bool good = true;

  vector<string> newlines;
  for( auto li = lines.begin(); li != lines.end() && good; ++li ) {
    const string& line = *li;
    ssiz_t ext = 0, pos = Index( line, ext/*, 0 */);
    if( pos != string::npos ) {
      assert( ext >= 3 );
      Textvars_t::const_iterator it;
      if( ext > 3 &&
	  (it = fVars.find(line.substr(pos+2,ext-3))) != fVars.end() ) {
	const vector<string>& repl = (*it).second;
	assert( !repl.empty() );
	if( !do_multi && repl.size() > 1 ) {
          ::Error(here, "multivalue variable %s = %s not supported in this "
                        "context: \"%s\"", line.substr(pos, ext).c_str(),
                  ValStr(repl).c_str(), line.c_str());
	  good = false;
	}
	if( good ) { // stop replacing once an error has been detected
	  // Found a valid replacement, so we actually need to do something
	  if( newlines.empty() ) {
	    // Guess that every line gets replaced similarly
	    newlines.reserve( lines.size()*repl.size() );
	    newlines.assign( lines.begin(), li );
	  }
          for( const auto& jt : repl ) {
            newlines.push_back(line);
            newlines.back().replace(pos, ext, jt);
          }
        }
      } else {
        ::Error(here, "unknown text variable %s",
                line.substr(pos, ext).c_str());
        good = false;
      }
    }
  }

  if( !newlines.empty() && good ) {
    // Rescan for multiple and/or nested replacements
    good = ( Substitute( newlines, do_multi ) == 0 );
    if( good )
      lines.swap( newlines );
  }

  return (good ? 0 : 1);
}

} // namespace Podd

//_____________________________________________________________________________

ClassImp(Podd::Textvars)
