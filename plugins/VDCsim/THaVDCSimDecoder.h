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
#include "THaVDCSim.h"
#include <vector>

class THaVDCSimDecoder : public THaEvData {
 public:
  THaVDCSimDecoder();
  ~THaVDCSimDecoder() override;

  Int_t  LoadEvent( const UInt_t* evbuffer) override;

  void   Clear( Option_t* opt = "" ) override;
  Int_t  GetNTracks() const;
  Int_t  DefineVariables( THaAnalysisObject::EMode mode =
			  THaAnalysisObject::kDefine );

 protected:

  std::vector<THaVDCSimTrack> fTracks;  // Monte Carlo tracks

  bool    fIsSetup;

  ClassDefOverride(THaVDCSimDecoder,0) // Decoder for simulated VDC data
};

#endif
