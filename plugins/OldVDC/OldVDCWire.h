#ifndef Podd_OldVDCWire_h_
#define Podd_OldVDCWire_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCWire                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"

class OldVDCTimeToDistConv;

class OldVDCWire : public TObject {

public:

  OldVDCWire( Int_t num=0, Double_t pos=0.0, Double_t offset=0.0,
	      OldVDCTimeToDistConv* ttd=NULL ) :
    fNum(num), fFlag(0), fPos(pos), fTOffset(offset), fTTDConv(ttd) {}
  virtual ~OldVDCWire() {}

  // Get and Set Functions
  Int_t    GetNum()     const { return fNum;  }
  Int_t    GetFlag()    const { return fFlag; }
  Double_t GetPos()     const { return fPos; }
  Double_t GetTOffset() const { return fTOffset; }
  OldVDCTimeToDistConv * GetTTDConv() { return fTTDConv; }

  void SetNum  (Int_t num)  {fNum = num;}
  void SetFlag (Int_t flag) {fFlag = flag;}
  void SetPos  (Double_t pos)       { fPos = pos; }
  void SetTOffset (Double_t tOffset){ fTOffset = tOffset; } 
  void SetTTDConv (OldVDCTimeToDistConv * ttdConv){ fTTDConv = ttdConv;}

protected:
  Int_t    fNum;                       //Wire Number
  Int_t    fFlag;                      //Flag for errors (e.g. Bad wire)
  Double_t fPos;                       //Position within the plane
  Double_t fTOffset;                      //Timing Offset
  OldVDCTimeToDistConv* fTTDConv;     //!Time to Distance Converter

private:
  OldVDCWire( const OldVDCWire& );
  OldVDCWire& operator=( const OldVDCWire& );
 
  ClassDef(OldVDCWire,1)             // VDCWire class
};

////////////////////////////////////////////////////////////////////////////////

#endif
