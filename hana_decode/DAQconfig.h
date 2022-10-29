#ifndef Podd_DAQconfig_h_
#define Podd_DAQconfig_h_

//////////////////////////////////////////////////////////////////////////
//
// DAQconfig, DAQInfoExtra
//
// Helper classes to support DAQ configuration info.
//
//////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <map>

//_____________________________________________________________________________
struct DAQconfig {
  std::vector<std::string> strings;
  std::map<std::string,std::string> keyval;

  void clear() { strings.clear(); keyval.clear(); }
  size_t parse( size_t i );
} __attribute__((aligned(64)));

//_____________________________________________________________________________
//FIXME: BCI. Make member variable in client
#include "TObject.h"

class DAQInfoExtra : public TObject {
public:
  DAQInfoExtra();
  virtual TObject* Clone( const char*  /*newname*/ = "" ) const {
    return new DAQInfoExtra(*this);
  }
  static void AddTo( TObject*& p, TObject* obj = nullptr );
  static DAQInfoExtra* GetExtraInfo( TObject* p );
  static DAQconfig* GetFrom( TObject* p );

  DAQconfig fDAQconfig;
  UInt_t fMinScan;

  ClassDef(DAQInfoExtra, 2)
};

//_____________________________________________________________________________

#endif //Podd_DAQconfig_h_
