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
// global gHaTextvars, so variables can be set in scripts.              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "THaTextvars.h"
#include "TRegexp.h"
#include "TError.h"
#include <utility>
#include <cassert>

using namespace std;

//_____________________________________________________________________________
Int_t THaTextvars::Add( const TString& name, const TString& value )
{
  // Add or Set/Replace text variable with given name and value.

  if( name.IsNull() )
    return 0;

  Textvars_t::iterator it = fVars.find(name);

  if( it != fVars.end() ) {  // already exists?
    (*it).second = value;
  } else {                   // if not, add a new one
    pair<Textvars_t::iterator,bool> ret =
      fVars.insert( make_pair(name,value) );
    assert( ret.second );
  }
  return 1;
}

//_____________________________________________________________________________
void THaTextvars::Clear()
{
  fVars.clear();
}

//_____________________________________________________________________________
const char* THaTextvars::Get( const TString& name ) const
{
  // Get the value of the text variable with the given name.
  // Returns either the value or NULL if not found.

  if( name.IsNull() )
    return 0;

  Textvars_t::const_iterator it = fVars.find(name);

  return ( it != fVars.end() ) ? ((*it).second).Data() : 0;
}

//_____________________________________________________________________________
UInt_t THaTextvars::GetNvars() const
{
  return fVars.size();
}

//_____________________________________________________________________________
Int_t THaTextvars::Remove( const TString& name )
{
  // Remove text variable with given name. Always returns 0.

  fVars.erase(name);

  return 0;
}

//_____________________________________________________________________________
void THaTextvars::Print( Option_t* /*opt*/ ) const
{

}

//_____________________________________________________________________________
Int_t THaTextvars::Substitute( TString& line ) const
{
  // Substitute text variables in given string s
  // Text Variables have the form ${name}
  //
  // Returns 0 if success, 1 on error (name not found)

  TRegexp re("\\$\\{[a-zA-Z0-9]*\\}");
  bool err = false;
  TString s(line);

  Ssiz_t pos = 0, ext = 0;
  while( (pos = s.Index(re, &ext, pos+ext)) != kNPOS ) {
    assert( ext > 3 );
    Textvars_t::const_iterator it = fVars.find( s(pos+2,ext-3) );
    if( it != fVars.end() ) {
      s.Replace( pos, ext, (*it).second );
      pos = ext = 0;
    } else {
      TString t(s(pos,ext));
      ::Error("THaTextvars", "unknown text variable %s", t.Data() );
      err = true;
    }
  }
  if( !err ) line = s;
  return (err ? 1 : 0);
}

//_____________________________________________________________________________

ClassImp(THaTextvars)
