//*-- Author :    Ole Hansen   20-Aug-2001


//////////////////////////////////////////////////////////////////////////
//
// THaArrayString
//
// This class parses the array subscript(s) of variable.
// In the trivial case of a scalar, it simply stores the variable name.
// Otherwise, it extracts the individual subscript(s) and number of
// dimensions.
//
// Example:
//
//   THaArrayString txt("x[7,4]");
//
// gives
//    fName   = "x"
//    fNdim   = 2
//    fDim    = [ 7, 4 ]
//    fLen    = 7*4 = 28
//    fStatus = 0
//
// THaArrayStrings are used in particular by THaFormula and THaVar.
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include <cstdlib>
#include "THaArrayString.h"

using namespace std;

//_____________________________________________________________________________
THaArrayString::THaArrayString( const THaArrayString& rhs )
  : fName(rhs.fName), fNdim(rhs.fNdim), fLen(rhs.fLen), fStatus(rhs.fStatus)
{
  // Copy constructor

  if( fNdim > kMaxA ) {
    fDim = new Int_t[fNdim];
    for( Int_t i = 0; i<fNdim; i++ )
      fDim[i] = rhs.fDim[i];
  } else {
    for( Int_t i = 0; i<fNdim; i++ )
      fDimA[i] = rhs.fDimA[i];
  }
}

//_____________________________________________________________________________
THaArrayString& THaArrayString::operator=( const THaArrayString& rhs )
{
  // Assignment operator

  if( this != &rhs ) {
    if( fNdim>kMaxA )
      delete [] fDim;
    fName    = rhs.fName;
    fNdim    = rhs.fNdim;
    fLen     = rhs.fLen;
    fStatus  = rhs.fStatus;
    if( fNdim>kMaxA ) {
      fDim = new Int_t[fNdim];
      for( Int_t i = 0; i<fNdim; i++ )
	fDim[i] = rhs.fDim[i];
    } else {
      for( Int_t i = 0; i<fNdim; i++ )
	fDimA[i] = rhs.fDimA[i];
    }
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaArrayString::Parse( const char* string )
{
  // Parse the given string for array syntax.
  // For multidimensional arrays, both C-style and comma-separated subscripts
  // are supported.
  //
  // Examples of legal expressions:
  //
  //    "x"           scalar "x"
  //    "x[2]"        1-d array "x" of size 2
  //    "x[2][3]"     2-d array "x" of size 2x3=6
  //    "x[2,3]"      same 2-d array
  //    "x[2][3][4]"  3-d array "x" of size 2x3x4=24
  //
  // Results are stored in this object and can be retrieved using
  // GetName(), GetNdim(), GetDim(), and GetLen().
  //
  // The maximum length of the input string is 255 characters.
  //
  // Returns 0 if ok, or EStatus error code otherwise (see header).

  static const size_t MAXLEN = 255;
  bool changed = false, dyn = false;
  char *str = 0, *s, *t;
  const char *cs;
  size_t len;
  Int_t ncomma = 0, nl = 0, nr = 0;
  Int_t j;
  Long64_t llen;;

  if( !string || !*string ) {
    // No string or empty string?
    // If already initialized, just do nothing. Otherwise, we are
    // being called from the constructor, so parse fName.
    if( fLen != -1 ) goto ok;
    string = fName.Data();
    len = fName.Length();
  } else {
    len = strlen(string);
    changed = true;
  }
  if( len > MAXLEN ) goto toolong;

  // Copy string to local buffer and get rid of all whitespace

  str = new char[ len+1 ];
  t = str;
  cs = string;
  while( *cs ) {
    if( *cs != ' ' && *cs != '\t' )
      *(t++) = *cs;
    else
      changed = true;
    cs++;
  }
  *t = 0;
  if( t == str ) goto notinit;

  // Extract name, i.e. everything up to the first bracket.
  // Check for illegal characters in the name.

  t = str;
  while( *t && *t != '[' ) {
    if( *t != ',' && *t != ']' && *t != '(' && *t != ')' )
      t++;
    else
      goto illegalchars;
  }

  // No name? Error.
  if( t == str ) goto badsyntax;

  if( fNdim>kMaxA )
    delete [] fDim;
  fNdim = 0; fLen = 1;

  // No bracket found? No array, we're done.
  if( !*t ) {
    if( changed )
      fName = str;
    goto ok;
  }

  // We have a bracket! Save its position and start parsing the subscripts.

  s = t;

  // From here on, we may only have digits, commas, and brackets.
  // While we check, we count how many separators there are.

  while( *t ) {
    switch( *t ) {
    case ',':
      ncomma++; break;
    case '[':
      nl++; break;
    case ']':
      nr++; break;
    default:
      if ( *t < '0' || *t > '9' )
	goto illegalchars;
      break;
    }
    t++;
  }

  // Number of brackets must match, only one bracket pair for comma
  // syntax, and last character must be a closing bracket.
  if( nl != nr || (ncomma && nl>1) || *(t-1) != ']' )
    goto badsyntax;

  *s = 0;  // Terminate name string
  t = s+1;
  if( ncomma )
    fNdim = ncomma+1;
  else
    fNdim = nl;
  if( fNdim>kMaxA ) {
    dyn = true;
    fDim = new Int_t[ fNdim ];
  }

  for( int i=0; i<fNdim; i++ ) {
    s = t;
    while ( *t && *t != ',' && *t != ']' ) {
      // Two left brackets in a row? Oops.
      if( *t == '[' )  goto badsyntax;
      t++;
    }

    // Unexpected end of string or no string between commas?
    if( !*t || t == s ) goto badsyntax;

    // Terminate subscript string
    *t = 0;

    // Ok, we got a number.
    j = atoi( s );
    llen = static_cast<Long64_t>(fLen) * static_cast<Long64_t>(j);
    if( llen > kMaxInt ) goto toolarge;
    if( dyn ) fDim[i]  = j;
    else      fDimA[i] = j;
    fLen = llen;

    t++;

    // Is there anything between a right and a left bracket?
    if( !ncomma && *t ) {
      if( *t == '[' )
	t++;
      else
	goto badsyntax;
    }
  }

  fName = str;
  goto ok;

 badsyntax:
  fStatus = kBadsyntax;
  goto cleanup;

 illegalchars:
  fStatus = kIllegalchars;
  goto cleanup;

 toolarge:
  fStatus = kToolarge;
  goto cleanup;

 toolong:
  fStatus = kToolong;
  goto cleanup;

 notinit:
  fStatus = kNotinit;
  goto cleanup;

 ok:
  fStatus = kOK;

 cleanup:
  delete [] str;

  return fStatus;
}

//_____________________________________________________________________________
static void WriteDims( Int_t ndim, const Int_t* dims )
{
  for( Int_t i = 0; i<ndim; i++ ) {
    cout << dims[i];
    if( i+1<ndim )
      cout << ",";
  }
}

//_____________________________________________________________________________
void THaArrayString::Print( Option_t* option ) const
{
  // Print the name and array dimension(s), if any.
  // If option == "dimonly", only print the dimensions, without square brackets.

  TString opt(option);
  if( !opt.Contains("dimonly") ) {
    cout << fName ;
    if( fNdim > 0 ) {
      cout << "[";
      if( fNdim > kMaxA ) WriteDims(fNdim,fDim);
      else                WriteDims(fNdim,fDimA);
      cout << "]";
    }
    cout << endl;
  }
  else {
    if( fNdim > kMaxA ) WriteDims(fNdim,fDim);
    else                WriteDims(fNdim,fDimA);
  }
}
//_____________________________________________________________________________
ClassImp(THaArrayString);
