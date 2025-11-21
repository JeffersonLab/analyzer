#ifndef Podd_DAQInfoExtra_h_
#define Podd_DAQInfoExtra_h_

//////////////////////////////////////////////////////////////////////////
//
// DAQInfoExtra
//
// Support for schema evolution of THaRun/THaRunParameters
// from analyzer 1.7 -> 1.8
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include <vector>
#include <string>
#include <map>

class THaRun;
class THaRunBase;
class THaRunParameters;

//_____________________________________________________________________________
struct DAQconfig {
  std::vector<std::string> strings;
  std::map<std::string,std::string> keyval;
} __attribute__((aligned(64)));

//_____________________________________________________________________________
class DAQInfoExtra : public TObject {
public:
  DAQInfoExtra();

  // Used in ROOT I/O rules in Podd_LinkDef.h
  static THaRunParameters* UpdateRunParam( THaRunBase* run );
  static unsigned int GetMinScan( THaRun* run );
  static void DeleteExtra( THaRunBase* run );

  DAQconfig fDAQconfig;
  std::vector<UInt_t> fTags;
  UInt_t fMinScan;

  ClassDef(DAQInfoExtra, 3)
};

//_____________________________________________________________________________

#endif //Podd_DAQInfoExtra_h_
