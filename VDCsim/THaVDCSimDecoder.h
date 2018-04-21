#ifndef Podd_THaVDCSimDecoder_h_
#define Podd_THaVDCSimDecoder_h_

/////////////////////////////////////////////////////////////////////
//
//   THaVDCSimDecoder
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "TClonesArray.h"
#include "THaAnalysisObject.h"
#include "TList.h"

class THaCrateMap;

class THaVDCSimDecoder : public THaEvData {
 public:
  THaVDCSimDecoder();
  virtual ~THaVDCSimDecoder();

  virtual Int_t  LoadEvent( const int* evbuffer);

  void   Clear( Option_t* opt="" );
  Int_t  GetNTracks() const;
  Int_t  DefineVariables( THaAnalysisObject::EMode mode = 
			  THaAnalysisObject::kDefine );

 protected:

  TList   fTracks;    // Monte Carlo tracks

  bool    fIsSetup;

  ClassDef(THaVDCSimDecoder,0) // Decoder for simulated VDC data
};

#endif
