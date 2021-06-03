#ifndef Podd_CodaRawDecoder_h_
#define Podd_CodaRawDecoder_h_

/////////////////////////////////////////////////////////////////////
//
//   Podd::CodaRawDecoder
//
//   Version of Decoder::CodaDecoder raw event data decoder that
//   exports Podd global variables for basic event data
//
//   Ole Hansen, August 2018
//
/////////////////////////////////////////////////////////////////////

#include "CodaDecoder.h"

namespace Podd {

class CodaRawDecoder : public Decoder::CodaDecoder {
public:
  CodaRawDecoder();
  virtual ~CodaRawDecoder();

  ClassDef(CodaRawDecoder,0) // CODA event data decoder exporting global vars
};

} // namespace Podd

#endif
