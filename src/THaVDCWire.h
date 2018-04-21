#ifndef Podd_THaVDCWire_h_
#define Podd_THaVDCWire_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCWire                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"

namespace VDC {
  class TimeToDistConv;
}

class THaVDCWire : public TObject {

public:

  THaVDCWire( Int_t num=0, Double_t pos=0.0, Double_t offset=0.0,
	      VDC::TimeToDistConv* ttd=NULL ) :
    fNum(num), fFlag(0), fPos(pos), fTOffset(offset), fTTDConv(ttd) {}
  virtual ~THaVDCWire() {}

  // Get and Set Functions
  Int_t    GetNum()     const { return fNum;  }
  Int_t    GetFlag()    const { return fFlag; }
  Double_t GetPos()     const { return fPos; }
  Double_t GetTOffset() const { return fTOffset; }
  VDC::TimeToDistConv* GetTTDConv() { return fTTDConv; }

  void SetNum  (Int_t num)  {fNum = num;}
  void SetFlag (Int_t flag) {fFlag = flag;}
  void SetPos  (Double_t pos)       { fPos = pos; }
  void SetTOffset (Double_t tOffset){ fTOffset = tOffset; }
  void SetTTDConv (VDC::TimeToDistConv * ttdConv){ fTTDConv = ttdConv;}

protected:
  Int_t    fNum;                       //Wire Number
  Int_t    fFlag;                      //Flag for errors (e.g. Bad wire)
  Double_t fPos;                       //Position within the plane
  Double_t fTOffset;                   //Timing Offset
  VDC::TimeToDistConv* fTTDConv;       //!Time to Distance Converter

private:
  THaVDCWire( const THaVDCWire& );
  THaVDCWire& operator=( const THaVDCWire& );

  ClassDef(THaVDCWire,1)             // VDCWire class
};

////////////////////////////////////////////////////////////////////////////////

#endif
