#ifndef ToyModuleCollect_
#define ToyModuleCollect_

/////////////////////////////////////////////////////////////////////
//
//   ToyModuleCollect
//   Test code
//   Collection of module types.  Something like this would be a helper
//   class for the decoder.
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "THashList.h"
#include "Rtypes.h"

using namespace std;

class ToyModuleCollect  {

public:

  ToyModuleCollect() {};  
  virtual ~ToyModuleCollect() {};  

   THashList fModuleList;

   Int_t Init();

private:

   Int_t ProcessCrateMap(void);

   ClassDef(ToyModuleCollect,0)  // A set of modules

};

#endif
