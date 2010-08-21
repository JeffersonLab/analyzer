//*-- Author :    Ole Hansen   26-Jun-2010

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaTextvars                                                          //
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

#include "THaTextvars.h"
#include "TError.h"
#include <utility>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <list>
#include <algorithm>

using namespace std;

typedef string::size_type ssiz_t;
typedef vector<string>::size_type vssiz_t;

#define ALL(c) (c).begin(), (c).end()

//_____________________________________________________________________________
static void Tokenize( const string& s, const string& delim,
		      vector<string>& tokens )
{
  // Tokenize string 's' on delimiter(s) in 'delim'. Put results in 'tokens'

  if( s.empty() ) {
    tokens.push_back(s);
    return;
  }
  ssiz_t start = s.find_first_not_of(delim);
  ssiz_t pos   = s.find_first_of(delim,start);

  while( pos != string::npos || start != string::npos ) {
    tokens.push_back( s.substr(start,pos-start));
    start = s.find_first_not_of(delim,pos);
    pos = s.find_first_of(delim,start);
  } 
  if( start != string::npos )
    tokens.push_back( s.substr(start) );
}

//_____________________________________________________________________________
static string ValStr( const vector<string>& s )
{
  // Assemble vector of strings into a comma-separated single string.
  // Used by print functions.

  string val;
  if( s.size() > 0 ) {
    val = "\"" + s[0] + "\"";
    for( vssiz_t i = 1; i < s.size(); ++i )
      val.append(",\"" + s[i] + "\"");
  }
  return val;
}

//_____________________________________________________________________________
static ssiz_t Index( const string& s, ssiz_t& ext, ssiz_t start )
{
  // Find index of text variable pattern in string 's'. 'ext' is the length
  // of the pattern. Search starts at 'start'.
  // This is equivalent to searching for regexp "\$\{[^\{\}]*\}"

  static const string pat = "${";

  if( s.empty() || start == string::npos || start > s.length() )
    return string::npos;

  ssiz_t beg = s.find(pat, start), beg2, end;
  if( beg == string::npos ) return string::npos;
  do {
    end = s.find('}', beg);
    if( end == string::npos) return string::npos;
    beg2 = s.find(pat, beg+1);
  }
  while( beg2 != string::npos && beg2 < end && (beg = beg2) );

  assert( end>beg );
  ext = end-beg+1;
  return beg;
}

//_____________________________________________________________________________
Int_t THaTextvars::Add( const string& name, const string& value )
{
  // Add or Set/Replace text variable with given name and value.
  // Parse value for multiple comma-separated tokens and store them as
  // individual values.

  if( name.empty() )
    return 0;

  static const string delim(",");
  vector<string> tokens;
  Tokenize( value, delim, tokens );

  Textvars_t::iterator it = fVars.find(name);

  if( it != fVars.end() ) {  // already exists?
    (*it).second.swap(tokens);
  } else {                   // if not, add a new one
    pair<Textvars_t::iterator,bool> ret =
      fVars.insert( make_pair(name,tokens) );
    assert( ret.second );
  }
  return 1;
}

//_____________________________________________________________________________
Int_t THaTextvars::AddVerbatim( const string& name, const string& value )
{
  // Add or Set/Replace text variable with given name and value.
  // Does not scan value for tokens; hence allows values that contain a ","

  if( name.empty() )
    return 0;

  vector<string> values( 1, value );;

  Textvars_t::iterator it = fVars.find(name);

  if( it != fVars.end() ) {  // already exists?
    (*it).second.swap(values);
  } else {                   // if not, add a new one
    pair<Textvars_t::iterator,bool> ret =
      fVars.insert( make_pair(name,values) );
    assert( ret.second );
  }
  return 1;
}

//_____________________________________________________________________________
const char* THaTextvars::Get( const string& name, Int_t idx ) const
{
  // Get the i-th value of the text variable with the given name.
  // Returns either the value or NULL if not found.

  if( name.empty() || idx < 0 )
    return 0;

  Textvars_t::const_iterator it = fVars.find(name);
  if( it == fVars.end() )
    return 0;
  if( (ssiz_t)idx >= (*it).second.size() )
    return 0;

  return (*it).second[idx].c_str();
}

//_____________________________________________________________________________
vector<string>& THaTextvars::GetArray( const string& name )
{
  // Get the values array for the text variable with the given name.
  // If name is not found, a zero-size dummy array is returned, which 
  // one should not use.

  static vector<string> dummy;

  if( name.empty() )
    return dummy;

  Textvars_t::iterator it = fVars.find(name);
  if( it == fVars.end() )
    return dummy;

  return (*it).second;
}

//_____________________________________________________________________________
UInt_t THaTextvars::GetNvalues( const string& name ) const
{
  // Get the number of values for the text variable with the given name.

  if( name.empty() )
    return 0;

  Textvars_t::const_iterator it = fVars.find(name);
  if( it == fVars.end() )
    return 0;

  return (*it).second.size();
}

//_____________________________________________________________________________
void THaTextvars::Print( Option_t* /*opt*/ ) const
{
  // Print all text variables

  Ssiz_t maxw = 0;
  for( Textvars_t::const_iterator it = fVars.begin();
       it != fVars.end(); ++it ) {
    Ssiz_t len = ((*it).first).length();
    if( len > maxw )
      maxw = len;
  }
  for( Textvars_t::const_iterator it = fVars.begin();
       it != fVars.end(); ++it ) {
    cout << "Textvar:  " << setw(maxw) << (*it).first << " = " 
	 << ValStr((*it).second) << endl;
  }
}

//_____________________________________________________________________________
Int_t THaTextvars::Substitute( string& line ) const
{
  // Substitute text variables in a single string.
  // If any multi-valued variables are encountered, treat it as an error

  vector<string> lines( 1, line );
  Int_t ret = Substitute( lines, false );
  if( ret )
    return ret;
  assert( lines.size() == 1 );
  line = lines[0];
  return 0;
}

//_____________________________________________________________________________
Int_t THaTextvars::Substitute( vector<string>& lines, bool do_multi ) const
{
  // Substitute text variables in the given array of strings. 
  // Supports multi-valued text variables, where each instance is
  // separated by commas; e.g. arm = "L,R".
  //
  // Text Variables have the form ${name}
  //
  // Returns 0 if success, 1 on error (name not found)

  static const char* const here = "THaTextvars::Substitute";

  bool good = true;

  list<string> newlines( ALL(lines) );
  for( list<string>::iterator li = newlines.begin(); li != newlines.end()
	 && good; ++li ) {
    ssiz_t pos = 0, ext = 0;
    while( (pos = Index((*li), ext, pos+ext)) != string::npos ) {
      assert( ext >= 3 );
      Textvars_t::const_iterator it;
      if( ext > 3 && 
	  (it = fVars.find((*li).substr(pos+2,ext-3))) != fVars.end() ) {
	const vector<string>& repl = (*it).second;
	assert( !repl.empty() );
	if( !do_multi && repl.size() > 1 ) {
	  ::Error( here, "multivalue variable %s = %s not supported in this "
		   "context", (*li).substr(pos,ext).c_str(),
		   ValStr(repl).c_str());
	  good = false;
	}
	if( good ) { // don't replace and rescan if error already occurred
	  string s = *li;  // save copy of unmodified line
	  list<string>::iterator lp = li;
	  assert( lp != newlines.end() );
	  ++lp; // new elements will be inserted before here
	  for( vector<string>::const_iterator jt = repl.begin();
	       jt != repl.end(); ++jt ) {
	    if( jt == repl.begin() ) {
	      // Replace first instance in place
	      (*li).replace( pos, ext, *jt );
	    } else {
	      // Any additional instances are appended to the results array
	      // and scanned further in subsequent iterations
	      list<string>::iterator np = newlines.insert( lp, s );
	      (*np).replace( pos, ext, *jt );
	    }
	  }
	  pos = ext = 0;  // Rescan current line *li
	}
      } else {
	::Error( here, "unknown text variable %s",
		 (*li).substr(pos,ext).c_str() );
	good = false;
      }
    }
  }
  if( good ) {
    // Change input array only if there were no errors
    lines.clear();
    copy( ALL(newlines), back_inserter(lines) );
  }
  return (good ? 0 : 1);
}

//_____________________________________________________________________________

ClassImp(THaTextvars)
