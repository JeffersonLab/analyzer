#ifndef THaFastBusWord_
#define THaFastBusWord_

/////////////////////////////////////////////////////////////////////
//
//   THaFastBusWord
//   Interpretation of standard fastbus data words
//
//   Given a word of fastbus data, we use methods here
//   to pick out the slot, channel, data, and opt
//   which depends on the fastbus model (1875, 1877, 1881, etc).
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include "Rtypes.h"
#include "TString.h"
// note to myself: all lower bits here 1.
#define MAXIDX 0xf     
#define MAXMODULE 0x3  

class THaFastBusWord 
{

public:

   THaFastBusWord();
   virtual ~THaFastBusWord();
   int Slot(int word);               // returns the slot
   int Chan(int model, int word);    // returns fastbus channel
   int Data(int model, int word);    // returns fastbus data
   int Opt(int model, int word);     // returns fastbus opt
   int Wdcnt(int model, int word);   // returns word count 
   TString devType(int model);
   bool headExist(int model);        // true if header exists for this model
   int FB_ERR;

private:

   static const int MAXSLOT = 26;  // There are no more than this #slots
   static const unsigned long slotmask = 0xf8000000;
   static const int slotshift = 27;
   static const int modoff = 1874;
   int modindex[MAXIDX];
   int module_type[MAXMODULE]; 
   struct module_information {
       int datamask,chanmask,wdcntmask,optmask;
       int chanshift,optshift;
       bool headexists;
       TString devtype;
   } module_info[MAXMODULE];
   void init();

   ClassDef(THaFastBusWord,0)  // Definitions for fastbus data standard

};

#endif





