//*-- Author :    Ole Hansen   30/06/2000


//////////////////////////////////////////////////////////////////////////
//
// THaPrintOption
//
// Utility class to deal with print option strings. Supports blank- and
// comma-separated lists of options and/or numerical arguments that are
// contained in a single option string.
//
//////////////////////////////////////////////////////////////////////////

#include "THaPrintOption.h"
#include "Helper.h"
#include <iostream>
#include <algorithm>
#include <cctype>   // ::tolower, ::toupper
#include <cstdlib>  // strtol
#include <limits>

ClassImp(THaPrintOption)

using namespace std;

//_____________________________________________________________________________
THaPrintOption::THaPrintOption( string str ) : fString{std::move(str)}
{
  // Normal constructor

  Parse();
}

//_____________________________________________________________________________
THaPrintOption::THaPrintOption( const char* str )
  : THaPrintOption(string(str ? str : ""))
{}

//_____________________________________________________________________________
THaPrintOption& THaPrintOption::operator=( string rhs )
{
  fString = std::move(rhs);
  Parse();
  return *this;
}

//_____________________________________________________________________________
THaPrintOption& THaPrintOption::operator=(const char* rhs)
{
  // Initialize object with a string

  if( rhs )
    fString = rhs;
  else
    fString.clear();
  Parse();
  return *this;
}

//_____________________________________________________________________________
Bool_t THaPrintOption::Contains( const string& token ) const
{
  // Test if option contains the given token 'tok'

  return any_of(ALL(fTokens), [&token]( const string& tok ) {
    return tok == token;
  });
}

//_____________________________________________________________________________
void THaPrintOption::Parse()
{
  // Parse the string and put the results in the internal arrays.
  // This function should never generate an error.

  const char* const delim = ", ";       // token delimiters

  fTokens.clear();
  fParam.clear();

  string::size_type start = 0;
  string::size_type stop = 0;
  while( start != string::npos && stop != string::npos ) {
    stop = fString.find_first_of(delim, start);
    string tok = fString.substr(start, stop - start);
    if( !tok.empty() ) {
      Int_t val = 0;
      const char* str = tok.c_str();
      char* str_end = nullptr;
      auto conv = strtol(str, &str_end, 10);
      if( str + tok.length() == str_end
          && conv <= numeric_limits<Int_t>::max()
          && conv >= numeric_limits<Int_t>::min() )
        val = static_cast<Int_t>(conv);
      fTokens.push_back(std::move(tok));
      fParam.push_back(val);
    }
    start = fString.find_first_not_of(delim, stop);
  }
}

//_____________________________________________________________________________
void THaPrintOption::Print() const
{
  for( Int_t i = 0; i < GetNOptions(); i++ )
    cout << "\"" << GetOptionStr(i) << "\" => " << GetValue(i) << endl;
}

//_____________________________________________________________________________
void THaPrintOption::ToLower()
{
  transform(ALL(fString), fString.begin(), ::tolower);
  for( auto& tok: fTokens )
    transform(ALL(tok), tok.begin(), ::tolower);
}

//_____________________________________________________________________________
void THaPrintOption::ToUpper()
{
  transform(ALL(fString), fString.begin(), ::toupper);
  for( auto& tok: fTokens )
    transform(ALL(tok), tok.begin(), ::toupper);
}

//_____________________________________________________________________________
