#ifndef Podd_THaTextvars_h_
#define Podd_THaTextvars_h_

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaTextvars - generic string substitution facility                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <map>
#include <string>
#include <vector>

class THaTextvars {

public:
  THaTextvars() {}
  virtual ~THaTextvars() {}

  Int_t    Add( const std::string& name, const std::string& value );
  Int_t    AddVerbatim( const std::string& name, const std::string& value );
  void     Clear() { fVars.clear(); }
  void     Print( Option_t* opt="" ) const;
  void     Remove( const std::string& name ) { fVars.erase(name); }
  UInt_t   Size() const { return fVars.size(); }

  const char*               Get( const std::string& name, Int_t idx=0 ) const;
  std::vector<std::string>  GetArray( const std::string& name );
  UInt_t                    GetArray( const std::string& name,
				      std::vector<std::string>& array );
  UInt_t                    GetNvalues( const std::string& name ) const;

  Int_t    Set( const std::string& name, const std::string& value ) {
    return Add(name,value);
  }
  Int_t    Substitute( std::string& line ) const;
  Int_t    Substitute( std::vector<std::string>& lines ) const {
    return Substitute( lines, true );
  }

private:
  typedef std::map< std::string, std::vector<std::string> > Textvars_t;

  Int_t Substitute( std::vector<std::string>& lines, bool do_multi ) const;
  
  Textvars_t fVars;

  ClassDef(THaTextvars,0)
};

#endif
