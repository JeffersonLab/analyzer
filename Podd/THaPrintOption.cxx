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
#include <cstdlib>
#include <cstring>

ClassImp(THaPrintOption)

//_____________________________________________________________________________
THaPrintOption::THaPrintOption() : 
  fString(nullptr), fTokenStr(nullptr), fNTokens(0), fTokens(nullptr),
  fParam(nullptr), fEmpty("")
{
  // Default constructor
}

//_____________________________________________________________________________
THaPrintOption::THaPrintOption( const char* str ) :
  fString(nullptr), fTokenStr(nullptr), fNTokens(0), fTokens(nullptr),
  fParam(nullptr), fEmpty("")
{
  // Normal constructor
  
  if( str ) {
    fString = new char[ strlen(str) + 1 ];
    strcpy( fString, str );
    Parse();
  } 
}

//_____________________________________________________________________________
THaPrintOption::THaPrintOption( const THaPrintOption& rhs ) :
  fString(nullptr), fTokenStr(nullptr), fNTokens(0), fTokens(nullptr),
  fParam(nullptr), fEmpty("")
{
  // Copy constructor 

  if( rhs.fString ) {
    fString = new char[ strlen(rhs.fString)+1 ];
    strcpy( fString, rhs.fString );
    Parse();
  }
}

//_____________________________________________________________________________
THaPrintOption& THaPrintOption::operator=(const THaPrintOption& rhs)
{
  // Assignment operator

  if (this != &rhs) {
    *this = rhs.fString;
  }
  return *this;
}

//_____________________________________________________________________________
THaPrintOption& THaPrintOption::operator=(const char* rhs)
{
  // Initialize object with a string

  delete [] fString; fString = nullptr;
  fEmpty = "";
  if( rhs ) {
    fString = new char[ strlen(rhs)+1 ];
    strcpy( fString, rhs );
  }
  Parse();
  return *this;
}

//_____________________________________________________________________________
THaPrintOption::~THaPrintOption()
{
  // Destructor

  delete [] fString;
  delete [] fTokenStr;
  delete [] fTokens;
  delete [] fParam;
}

//_____________________________________________________________________________
void THaPrintOption::Parse()
{
  // Parse the string and put the results in the internal arrays.
  // This function should never generate an error since the input
  // cannot be invalid.

  const char* const delim = ", ";       // token delimiters

  delete [] fTokenStr; fTokenStr = nullptr;
  delete [] fTokens; fTokens = nullptr;
  delete [] fParam; fParam = nullptr;
  fNTokens = 0;
  if( !fString || !*fString ) return;

  fTokenStr = new char[ strlen(fString)+1 ];
  strcpy( fTokenStr, fString );

  // How many tokens?

  char* t = strtok( fTokenStr, delim );
  if( t ) fNTokens = 1;
  else    return;                           //Nothing to do
  while( strtok( nullptr, delim )) fNTokens++;

  // Set up arrays to hold token data

  fTokens = new char* [fNTokens];
  fParam  = new Int_t [fNTokens];
  fTokens[0] = t;
  fParam[0]  = atoi(t);
  if( fNTokens == 1 ) return;

  // Parse the tokens

  strcpy( fTokenStr, fString );
  strtok( fTokenStr, delim );
  int i = 1;
  while( (t = strtok( nullptr, delim )) ) {
    fTokens[i]  = t;
    fParam[i++] = atoi(t);
  }

}

//_____________________________________________________________________________



