#ifndef ROOT_THaRaster
#define ROOT_THaRaster

//////////////////////////////////////////////////////////////////////////
//
// THaRaster
//
// A basic class giving Raster data.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "TString.h"

class THaRaster : public THaDetector {
  
public:
  THaRaster( const char* name, const char* description = "", 
	     const char* device = "", THaApparatus* a = NULL );
  virtual ~THaRaster();

  virtual Int_t        Decode( const THaEvData& );
  virtual EStatus      Init( const TDatime& run_time );

protected:

  Float_t    fXcur;      // Raster X-axis Current
  Float_t    fYcur;      // Raster Y-axis Current
  Float_t    fXder;      // Raster X-axis Derivative
  Float_t    fYder;      // Raster Y-axis Derivative

  TString    fDevice;    // Device name ("Struck", "VMIC", "LeCroy")
  Int_t      fDeviceNo;  // Device number (0-2)

  static const char NCHAN = 24;
  Float_t* fChanMap[NCHAN];   //Decoder lookup table

  THaRaster() {}
  THaRaster( const THaRaster& ) {}
  THaRaster& operator=( const THaRaster& ) { return *this; }
  
  void   ClearEvent() 
    { fXcur = 0.0; fYcur = 0.0; fXder = 0.0; fYder = 0.0; }

  ClassDef(THaRaster,0)   //Generic Raster class
};

#endif 







