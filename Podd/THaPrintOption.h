#ifndef Podd_THaPrintOption_h_
#define Podd_THaPrintOption_h_

//////////////////////////////////////////////////////////////////////////
//
// THaPrintOption
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"

Option_t* const kPRINTLINE  = "LINE";
Option_t* const kPRINTSTATS = "STATS";

class THaPrintOption {
  
public:
  THaPrintOption();
  THaPrintOption( const char* string );
  THaPrintOption( const THaPrintOption& opt );
  THaPrintOption& operator=( const THaPrintOption& rhs );
  THaPrintOption& operator=( const char* rhs );
  virtual ~THaPrintOption();

  Int_t        GetNOptions()          const { return fNTokens; }
  const char*  GetOption( Int_t i=0 ) const;
  Int_t        GetValue( Int_t i=0 )  const;
  Bool_t       IsLine()               const;
  const char*  Data()                 const { return fString; }
  operator const char*()              const { return Data(); }
  const char* operator[]( Int_t i )   const { return GetOption(i); }
  const char* operator()( Int_t i )   const { return GetOption(i); }

protected:
  char*       fString;      //Pointer to local copy of string
  char*       fTokenStr;    //Copy of string parsed by strtok()
  Int_t       fNTokens;     //Number of tokens
  char**      fTokens;      //Array of pointers to the tokens in the string
  Int_t*      fParam;       //Array of the parameter values in the string
  char*       fEmpty;       //Pointer to \0, returned by GetOption() if error

  virtual void  Parse();

  ClassDef(THaPrintOption,0)   //Utility class to handle option strings
};

//__________ inline functions _________________________________________________
inline
const char* THaPrintOption::GetOption( Int_t i ) const
{
  // Get i-th token from string. Tokens are delimited by blanks or commas.

  return ( i>=0 && i<fNTokens && fTokens ) ? fTokens[i] : fEmpty;
}

//_____________________________________________________________________________
inline
Int_t THaPrintOption::GetValue( Int_t i ) const
{
  // Get integer value of the i-th token from string. 
  // Example:  
  //
  // With "OPT,10,20,30" or "OPT 10 20 30", GetValue(2) returns 20.
  //

  return ( i>=0 && i<fNTokens && fParam ) ? fParam[i] : 0;
}

//_____________________________________________________________________________
inline
Bool_t THaPrintOption::IsLine() const
{
  // True if "opt" is a request for printing on a single line

  const char* opt = GetOption();
  return ( !strcmp( opt, kPRINTLINE ) || !strcmp( opt, kPRINTSTATS ));
}

#endif

