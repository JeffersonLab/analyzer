#ifndef ROOT_THaVDCT0CalTable
#define ROOT_THaVDCT0CalTable

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCT0CalTable                                                        //
//                                                                           //
// Class to calculate t-offset for a wire in the VDC                         //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"

class THaVDCT0CalTable : public TObject {

public:
  THaVDCT0CalTable( );
  virtual ~THaVDCT0CalTable();
  
  void AddToCalibration(Int_t time);
  Double_t CalcTOffset();

  // Get and Set Functions
  Double_t GetTOffset() {return fTOffset;}
  Int_t * GetTable() {return fCalibrationTable; }
  Int_t GetBinSize() {return fBinSize; }
  Int_t GetMinTime() {return fMinTime; }
  Int_t GetMaxTime() {return fMaxTime; }

  void SetBinSize(Int_t binSize) {fBinSize = binSize; }
  void SetMinTime(Int_t minTime) {fMinTime = minTime; }
  void SetMaxTime(Int_t maxTime) {fMaxTime = maxTime; }
  
protected:
  Double_t fTOffset;
  Int_t fCalibrationTable[100];
  Int_t fBinSize;
  Int_t fMinTime;
  Int_t fMaxTime;
  
  THaVDCT0CalTable( const THaVDCT0CalTable& ) {}
  THaVDCT0CalTable& operator=( const THaVDCT0CalTable& ) { return *this; }

  
  ClassDef(THaVDCT0CalTable,0)             // VDCT0CalTable class
};

////////////////////////////////////////////////////////////////////////////////

#endif
