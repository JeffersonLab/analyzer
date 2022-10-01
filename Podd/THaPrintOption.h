#ifndef Podd_THaPrintOption_h_
#define Podd_THaPrintOption_h_

//////////////////////////////////////////////////////////////////////////
//
// THaPrintOption
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <string>
#include <vector>
#include <ostream>

Option_t* const kPRINTLINE  = "LINE";
Option_t* const kPRINTSTATS = "STATS";

class THaPrintOption {

public:
  THaPrintOption() = default;
  THaPrintOption( std::string str ); // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
  THaPrintOption( const char* str ); // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
  THaPrintOption( const THaPrintOption& opt ) = default;
  THaPrintOption( THaPrintOption&& opt ) = default;
  THaPrintOption& operator=( const THaPrintOption& rhs ) = default;
  THaPrintOption& operator=( THaPrintOption&& rhs ) = default;
  THaPrintOption& operator=( std::string rhs );
  THaPrintOption& operator=( const char* rhs );
  virtual ~THaPrintOption() = default;

  Bool_t             Contains( const std::string& token ) const;
  Int_t              GetNOptions()    const;
  const char*        GetOption( Int_t i=0 )    const;
  const std::string& GetOptionStr( Int_t i=0 ) const;
  Int_t              GetValue( Int_t i=0 )     const;
  Bool_t             IsLine()         const;
  const char*        Data()           const { return fString.c_str(); }
  virtual void       Print()          const;
  void               ToLower();
  void               ToUpper();
  explicit operator const char*()     const { return Data(); }
  const char* operator[]( Int_t i )   const { return GetOption(i); }
  const char* operator()( Int_t i )   const { return GetOption(i); }

  friend std::ostream& operator<<( std::ostream& os, const THaPrintOption& opt );

protected:
  std::string                fString;    //Local copy of option string
  std::vector<std::string>   fTokens;    //Parsed tokens
  std::vector<Int_t>         fParam;     //Parsed token values
  std::string                fEmpty;     //Empty string  FIXME make static

  virtual void  Parse();

  ClassDef(THaPrintOption,0)   //Utility class to handle option strings
};

//__________ inline functions _________________________________________________
inline
Int_t THaPrintOption::GetNOptions() const
{
  return static_cast<Int_t>(fTokens.size());
}

//_____________________________________________________________________________
inline
const char* THaPrintOption::GetOption( Int_t i ) const
{
  // Get i-th token from string. Tokens are delimited by blanks or commas.

  return ( i>=0 && i<GetNOptions() ) ? fTokens[i].c_str() : fEmpty.c_str();
}

//_____________________________________________________________________________
inline
const std::string& THaPrintOption::GetOptionStr( Int_t i ) const
{
  // Get i-th token from string as a std::string

  return ( i>=0 && i<GetNOptions() ) ? fTokens[i] : fEmpty;
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

  return ( i>=0 && i<GetNOptions() ) ? fParam[i] : 0;
}

//_____________________________________________________________________________
inline
Bool_t THaPrintOption::IsLine() const
{
  // True if "opt" is a request for printing on a single line

  return (GetOptionStr() == kPRINTLINE || GetOptionStr() == kPRINTSTATS);
}

//_____________________________________________________________________________
inline
std::ostream& operator<<( std::ostream& os, const THaPrintOption& opt )
{
  return os << opt.fString;
}

#endif
