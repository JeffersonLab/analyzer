#ifndef ROOT_THaBpm
#define ROOT_THaBpm

//////////////////////////////////////////////////////////////////////////
//
// THaBpm
//
// A basic class giving BPM data.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "TString.h"

class THaBpm : public THaDetector {
  
public:

  THaBpm( const char* name, const char* description = "", 
	  const char* device = "", THaApparatus* a = NULL );
  virtual ~THaBpm();

  virtual Int_t        Decode( const THaEvData& );
  virtual EStatus      Init( const TDatime& run_time );

protected:

  // Per-event user data
  Float_t     fX[6];       // X-axis, corrected
  Float_t     fY[6];       // Y-Axis, corrected

  Float_t     fYx[3];      // Curve Parameter [0] Offset 
                           //                 [1] Apmlitude
  Float_t     fYy[3];      //                 [2] Phase Shift   

  // Parameters, calibration data, etc.
  TString     fDevice;     // Device name ("LeCroy", "VMIC", "Struck")

  Int_t       kxp_ped;    // Pedestal for X-axis Plus ant. on BPM 4a 
  Int_t       kxm_ped;    // Pedastal for X-axis Minus ant. on BPM 4a
  Int_t       kyp_ped;    // Pedastal for Y-axis Plus ant. on BPM 4a
  Int_t       kym_ped;    // Pedastal for Y-axis Minus ant. on BPM 4a
  Float_t     kalpha_x;   // Gain Factor for X-axis set by Espace to 1.0
  Float_t     kalpha_y;   // Gain Factor for y-axis set by Espase to 1.0
  Float_t     kkappa;     // Conversion Factor in millimeters 
  Float_t     kx_att;     // Attenuation Factors X-axis, BPM 4a
  Float_t     ky_att;     // Attenuation Factors Y-axis, BPM 4a
  Float_t     kx_off;     // Offset to X-axis im millimeters, BPM 4a 
  Float_t     ky_off;     // Offset to Y-axis im millimeters, BPM 4a

  Float_t     fwx;        // Raster Freq. in radians, X-axis
  Int_t       kFxrast;    // Raster Frequency, X-axis
  Float_t     kDx;         // Determinant, X-axis 
  Float_t     kDxsub1;     // Subset of Determinant, X-axis
  Float_t     kDxsub2;     // Subset of Determinant, X-axis
  Float_t     kDxsub3;     // Subset of Determinant, X-axis

  Float_t     fDxsub2Xo;   // Second SubDeterminant, Xo parameter, X-axis
  Float_t     fDxsub3Xo;   // Third SubDeterminant, Xo parameter, X-axis
  Float_t     fDxXo;       // Determinant of the Xo parameter, X-axis
  Float_t     fDxsub1AC;   // First subDeterminant, Ao cos(phi) parameter, X-axis
  Float_t     fDxsub3AC;   // Third subDeterminant, Ao cos(phi) parameter, X-axis
  Float_t     fDxAC;       // Determinant of the Ao cos(phi) parameter, X-axis  
  Float_t     fDxsub1AS;   // First subDeterminant, Ao sin(phi) parameter, X-axis
  Float_t     fDxsub2AS;   // Second SubDeterminant, Ao sin(phi) parameter, X-axis
  Float_t     fDxAS;       // Determinant of the Ao sin(phi) parameter, X-axis

  Float_t     fwy;         // Raster Freq. in radians, Y-axis
  Int_t       kFyrast;     // Raster Frequency, Y-axis
  Float_t     kDy;         // Determinant, Y-axis
  Float_t     kDysub1;     // Subset of Determinant, Y-axis
  Float_t     kDysub2;     // Subset of Determinan, Y-axist
  Float_t     kDysub3;     // Subset of Determinant, Y-axis

  Float_t     fDysub2Xo;   // Second SubDeterminant, Xo paramete, Y-axisr
  Float_t     fDysub3Xo;   // Third SubDeterminant, Xo parameter, Y-axis
  Float_t     fDyXo;       // Determinant of the Xo parameter, Y-axis
  Float_t     fDysub1AC;   // First subDeterminant, Ao cos(phi) parameter, Y-axis
  Float_t     fDysub3AC;   // Third subDeterminant, Ao cos(phi) parameter, Y-axis
  Float_t     fDyAC;       // Determinant of the Ao cos(phi) parameter , Y-axis 
  Float_t     fDysub1AS;   // First subDeterminant, Ao sin(phi) parameter, Y-axis
  Float_t     fDysub2AS;   // Second SubDeterminant, Ao sin(phi) parameter, Y-axis
  Float_t     fDyAS;       // Determinant of the Ao sin(phi) parameter, Y-axis

  Float_t     fXsum;       // Sum of the X-axis positions
  Float_t     fXswtsum;    // Sum of (X-axis position * sin(wx*t))
  Float_t     fXcwtsum;    // Sum of (X-axis position * cos(wx*t))

  Float_t     fYsum;       // Sum of the Y-axis positions 
  Float_t     fYswtsum;    // Sum of (Y-axis position * sin(wx*t))
  Float_t     fYcwtsum;    // Sum of (Y-axis position * cos(wx*t))

  Float_t     fcwxt;       // Sum(cos(w*time*index)), X-axis
  Float_t     fswxt;       // Sum(sin(w*time*index)), X-axis
  Float_t     fccwxt;      // Sum(cos(cos(w*time*index)), X-axis)
  Float_t     fsswxt;      // Sum(sin(sin(w*time*index))), X-axis
  Float_t     fs2wxt;      // Sum(sin(2*w*time*index)), X-axis

  Float_t     fcwyt;       // Sum(cos(w*time*index)), Y-axis
  Float_t     fswyt;       // Sum(sin(w*time*index), Y-axis)
  Float_t     fccwyt;      // Sum(cos(cos(w*time*index))), Y-axis
  Float_t     fsswyt;      // Sum(sin(sin(w*time*index))), Y-axis
  Float_t     fs2wyt;      // Sum(sin(2*w*time*index)), Y-axis

  Float_t     kDelT;       // Time difference between hits, seconds 
  
  void   BurstFit( Int_t numhits );
  void   ClearEvent()
    { memset( fX, 0, 12*sizeof(Float_t)); }
   
  // Prevent default construction, copying, assignment
  THaBpm() {}
  THaBpm( const THaBpm& ) {}
  THaBpm& operator=( const THaBpm& ) { return *this; }
  
  ClassDef(THaBpm,0)   //Generic BPM class
};

#endif 












