#ifndef ToyModuleCollect_
#define ToyModuleCollect_

/////////////////////////////////////////////////////////////////////
//
//   ToyModuleCollect
//   Collection of module types.  Something like this would be a helper
//   class for the decoder.
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"

using namespace std;

class ToyModule;

class ToyModuleCollect  {

public:

   ToyModuleCollect();  
   virtual ~ToyModuleCollect();  

   map<string, ToyModule*> fModuleList;

   Int_t Init();


private:

   Int_t ProcessCrateMap();

   ClassDef(ToyModuleCollect,0)  // A set of modules



};

#endif
