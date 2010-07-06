#ifndef ROOT_THaTextvars
#define ROOT_THaTextvars

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaTextvars - generic string substitution facility                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TString.h"
#include <map>
#include <string>

class THaTextvars {

public:
  THaTextvars() {}
  virtual ~THaTextvars() {}

  Int_t    Add( const TString& name, const TString& value );
  void     Clear();
  const char* Get( const TString& name ) const;
  UInt_t   GetNvars() const;
  Int_t    Substitute( TString& line ) const;
  Int_t    Remove( const TString& name );
  void     Print( Option_t* opt="" ) const;

  Int_t Set( const TString& name, const TString& value ) {
    return Add(name,value);
  }
  Int_t Substitute( std::string& line ) const {
    TString s(line);
    Int_t err = Substitute(s);
    if( !err ) line = s;
    return err;
  }

private:
  typedef std::map<TString, TString> Textvars_t;

  Textvars_t fVars;

  ClassDef(THaTextvars,0)
};

#endif
