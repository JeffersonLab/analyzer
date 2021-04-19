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
#include <iostream>

ClassImp(THaPrintOption)

using namespace std;

//_____________________________________________________________________________
THaPrintOption::THaPrintOption( string str ) : fString{std::move(str)}
{
  // Normal constructor

  Parse();
}

//_____________________________________________________________________________
THaPrintOption::THaPrintOption( const char* str ) : fString{str}
{
  // Normal constructor

  Parse();
}

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
    string tok = fString.substr(start, stop-start);
    if( !tok.empty() ) {
      fTokens.push_back(tok);
      fParam.push_back(atoi(tok.c_str()));
    }
    start = fString.find_first_not_of(delim, stop);
  };
}

//_____________________________________________________________________________
void THaPrintOption::Print() const
{
  for( Int_t i=0; i<GetNOptions(); i++ )
    cout << "\"" << GetOptionStr(i) << "\" => " << GetValue(i) << endl;
}

