#ifndef ROOT_THaVDCWire
#define ROOT_THaVDCWire

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCWire                                                                //
//                                                                           //
// Class to represent a wire from the Hall A Vertical Drift Chambers         //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"

class THaVDCTimeToDistConv;

class THaVDCWire : public TObject {

public:

  THaVDCWire() :
    fNum(0), fFlag(0), fPos(0.0), fTOffset(0.0), fTTDConv(NULL) {}
  virtual ~THaVDCWire();

  // Get and Set Functions
  Int_t    GetNum()     const { return fNum;  }
  Int_t    GetFlag()    const { return fFlag; }
  Double_t GetPos()     const { return fPos; }
  Double_t GetTOffset() const { return fTOffset; }
  THaVDCTimeToDistConv * GetTTDConv() { return fTTDConv; }

  void SetNum  (Int_t num)  {fNum = num;}
  void SetFlag (Int_t flag) {fFlag = flag;}
  void SetPos  (Double_t pos)       { fPos = pos; }
  void SetTOffset (Double_t tOffset){ fTOffset = tOffset; } 
  void SetTTDConv (THaVDCTimeToDistConv * ttdConv){ fTTDConv = ttdConv;}

protected:
  Int_t    fNum;                       //Wire Number
  Int_t    fFlag;                      //Flag for errors (e.g. Bad wire)
  Double_t fPos;                       //Position within the plane
  Double_t fTOffset;                      //Timing Offset
  THaVDCTimeToDistConv* fTTDConv;     //Time to Distance Converter

  THaVDCWire( const THaVDCWire& ) {}
  THaVDCWire& operator=( const THaVDCWire& ) { return *this; }
 
  ClassDef(THaVDCWire,0)             // VDCWire class
};

////////////////////////////////////////////////////////////////////////////////

#endif
