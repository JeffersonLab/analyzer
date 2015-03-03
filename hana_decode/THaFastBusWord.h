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


#include "TString.h"

namespace Decoder {

class THaFastBusWord {

public:

  THaFastBusWord() { init(); }
  virtual ~THaFastBusWord() {}
  UChar_t  Slot(UInt_t word);                    // returns the slot
  UShort_t Chan(UShort_t model, UInt_t word);    // returns fastbus channel
  UShort_t Data(UShort_t model, UInt_t word);    // returns fastbus data
  UShort_t Wdcnt(UShort_t model, UInt_t word);   // returns word count
  UChar_t  Opt(UShort_t model, UInt_t word);     // returns fastbus opt
  const char* devType(UShort_t model);
  bool HasHeader(UShort_t model);    // true if header exists for this model
  static const UShort_t FB_ERR;

private:

  static const UChar_t  MAXSLOT = 26;  // There are no more than this #slots
  static const UInt_t   slotmask = 0xf8000000;
  static const UChar_t  slotshift = 27;
  static const UShort_t modoff = 1874;
  // note to myself: all lower bits here 1.
  static const UInt_t   MAXIDX = 0xf;
  static const UInt_t   MAXMODULE = 0x3;

  UChar_t  modindex[MAXIDX];
  UShort_t module_type[MAXMODULE];
  struct module_information {
    UInt_t  datamask,wdcntmask,chanmask,optmask;
    UChar_t chanshift,optshift;
    bool    headexists;
    TString devtype;
  } module_info[MAXMODULE];
  void init();
  UChar_t idx(UShort_t model);

  ClassDef(THaFastBusWord,0)  // Definitions for fastbus data standard
};

//=============== inline functions ================================
// returns fastbus slot
inline
UChar_t THaFastBusWord::Slot(UInt_t word)
{
  UChar_t slot = word>>slotshift;
  return (slot < MAXSLOT) ? slot : 0;
}

inline
UChar_t THaFastBusWord::idx(UShort_t model)
{
  return modindex[(model-modoff)&MAXIDX]&MAXMODULE;
}

// returns fastbus channel
inline
UShort_t THaFastBusWord::Chan(UShort_t model, UInt_t word)
{
  UChar_t imod = idx(model);
  return ( ( word & module_info[imod].chanmask )
	   >> module_info[imod].chanshift );
}

// returns word count of header
inline
UShort_t THaFastBusWord::Wdcnt(UShort_t model, UInt_t word)
{
  if (!HasHeader(model)) return FB_ERR;
  return ( word & module_info[idx(model)].wdcntmask );
}


// returns fastbus data
inline
UShort_t THaFastBusWord::Data(UShort_t model, UInt_t word)
{
  return ( word & module_info[idx(model)].datamask);
}

// returns fastbus opt;
inline
UChar_t THaFastBusWord::Opt(UShort_t model, UInt_t word)
{
  UChar_t imod = idx(model);
  return ( ( word & module_info[imod].optmask )
	   >> module_info[imod].optshift);
}

// answers if the model has a header
inline
bool THaFastBusWord::HasHeader(UShort_t model)
{
  return module_info[idx(model)].headexists;
}


// returns device type
inline
const char* THaFastBusWord::devType(UShort_t model)
{
  return module_info[idx(model)].devtype.Data();
}

}

#endif





