#ifndef Podd_Textvars_h_
#define Podd_Textvars_h_

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Textvars - generic string substitution facility                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <map>
#include <string>
#include <vector>
#include <cctype>   // for isspace

namespace Podd {

// Generic string utility functions
void  Tokenize( const std::string& s, const std::string& delim,
                std::vector<std::string>& tokens );
void  Trim( std::string& str );
std::vector<std::string> vsplit( const std::string& s );

//_____________________________________________________________________________
class Textvars {

public:
  using StrVec_t = std::vector<std::string>;
  using Textvars_t = std::map<std::string, StrVec_t>;

  Textvars() = default;

  Int_t    Add( const std::string& name, const std::string& value );
  Int_t    AddVerbatim( const std::string& name, const std::string& value );
  void     Clear() { fVars.clear(); }
  void     Print( Option_t* opt="" ) const;
  StrVec_t Remove( const std::string& name );
  UInt_t   Size() const { return fVars.size(); }

  const char*               Get( const std::string& name, UInt_t idx=0 ) const;
  const Textvars_t&         GetAllStringsMap() const { return fVars; }
  StrVec_t                  GetArray( const std::string& name ) const;
  StrVec_t                  GetNames() const;
  UInt_t                    GetNvalues( const std::string& name ) const;

  Int_t    Set( const std::string& name, const std::string& value ) {
    return Add(name,value);
  }
  Int_t    Substitute( std::string& line ) const;
  Int_t    Substitute( StrVec_t& lines ) const {
    return Substitute( lines, true );
  }

private:
  Int_t Substitute( StrVec_t& lines, bool do_multi ) const;
  
  Textvars_t fVars;

  ClassDefNV(Textvars, 0)  // String substitution facility
};

//_____________________________________________________________________________
inline
void Trim( std::string& str )
{
  // Trim leading and trailing whitespace from string 'str'

  if( str.empty() )
    return;
  const char *s = str.data(), *c = s;
  while( isspace(*c) )
    ++c;
  if( !*c ) {
    str.clear();
    return;
  }
  const char *e = s + str.size() - 1, *p = e;
  while( isspace(*p) )
    --p;
  if( p != e )
    str.erase(p-s+1);
  if( c != s )
    str.erase(0,c-s);
}

} // namespace Podd

//_____________________________________________________________________________
// Global instance of Podd::Textvars for easy access from scripts

#ifndef R__EXTERN
// Pick up definition of R__EXTERN
#include "DllImport.h"
#endif
R__EXTERN class Podd::Textvars* gHaTextvars;  //String substitution definitions

#endif
