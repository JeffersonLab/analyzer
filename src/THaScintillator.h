#ifndef ROOT_THaScintillator
#define ROOT_THaScintillator

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Names, associated with Decoded parameters of Scintillator 1 detector.     //
// Variables are set in THaScintillator1::Decode()                           //
//                                                                           //
//   Name      InVariable   OutVariable     Description                      //
// - ~~~~- - - ~~~~~~~~~~- -~~~~~~~~~~~- - -~~~~~~~~~~~- - - - - - - - - - - //
//"e.s1l.nthit" fS1LNTHIT   fE_S1L_nthit    Number of Left paddles TDC times;//
//"e.s1l.t1"    fS1LT[1]    fE_S1L_tdc[1]   TDC time of Left Paddle 1;       //
// ...          ...         ...             ...                              //
//"e.s1l.t6"    fS1LT[6]    fE_S1L_tdc[6]   TDC time of Left Paddle 6;       //
//"e.s1l.t_c1"  fS1LT_c[1]  fE_S1L_tdc_c[1] Corrected TDC of Left Paddle 1;  //
// ...          ...         ...             ...                              //
//"e.s1l.t_c6"  fS1LT_c[6]  fE_S1L_tdc_c[6] Corrected TDC of Left Paddle 6;  //
//"e.s1r.nthit" fS1RNTHIT   fE_S1R_nthit    Number of Right paddles TDC times//
//"e.s1r.t1"    fS1RT[1]    fE_S1R_tdc[1]   TDC time of Right Paddle 1;      //
// ...          ...         ...             ...                              //
//"e.s1r.t6"    fS1RT[6]    fE_S1R_tdc[6]   TDC time of Right Paddle 6;      //
//"e.s1r.t_c1"  fS1RT_c[1]  fE_S1R_tdc_c[1] Corrected TDC of Right Paddle 1; //
// ...          ...         ...             ...                              //
//"e.s1r.t_c6"  fS1RT_c[6]  fE_S1R_tdc_c[6] Corrected TDC of Right Paddle 6; //
//"e.s1l.nahit" fS1LNAHIT   fE_S1L_nahit    Number of Left paddles ADC amps; //
//"e.s1l.a1"    fS1LA[1]    fE_S1L_adc[1]   ADC value of Left Paddle 1;      //
// ...          ...         ...             ...                              //
//"e.s1l.a6"    fS1LA[6]    fE_S1L_adc[6]   ADC value of Left Paddle 6;      //
//"e.s1l.a_p1"  fS1LA_p[1]  fE_S1L_adc_p[1] ADC minus ped of Left Paddle 1;  //
// ...          ...         ...             ...                              //
//"e.s1l.a_p6"  fS1LA_p[6]  fE_S1L_adc_p[6] ADC minus ped of Left Paddle 6;  //
//"e.s1l.a_c1"  fS1LA_c[1]  fE_S1L_adc_c[1] Corrected ADC of Left Paddle 1;  //
// ...          ...         ...             ...                              //
//"e.s1l.a_c6"  fS1LA_c[6]  fE_S1L_adc_c[6] Corrected ADC of Left Paddle 6;  //
//"e.s1r.nahit" fS1RNAHIT   fE_S1R_nahit    Number of Right paddles ADC amps;//
//"e.s1r.a1"    fS1RA[1]    fE_S1R_adc[1]   ADC value of Right Paddle 1;     //
// ...          ...         ...             ...                              //
//"e.s1r.a6"    fS1RA[6]    fE_S1R_adc[6]   ADC value of Right Paddle 6;     //
//"e.s1r.a_p1"  fS1RA_p[1]  fE_S1R_adc_p[1] ADC minus ped of Right Paddle 1; //
// ...          ...         ...             ...                              //
//"e.s1r.a_p6"  fS1RA_p[6]  fE_S1R_adc_p[6] ADC minus ped of Right Paddle 6; //
//"e.s1r.a_c1"  fS1RA_c[1]  fE_S1R_adc_c[1] Corrected ADC of Right Paddle 1; //
// ...          ...         ...             ...                              //
//"e.s1r.a_c6"  fS1RA_c[6]  fE_S1R_adc_c[6] Corrected ADC of Right Paddle 6; //
//                                                                           //
// ------------------------------------------------------------------------- //
//                                                                           //
// Names associated with parameters calculated in                            //
// THaScintillator1::CrudeProcess()                                          //
// Units of measurements are centimeters for coordinates.                    //
//                                                                           //
//   Name      InVariable   OutVariable     Description                      //
// - ~~~~- - - ~~~~~~~~~~- -~~~~~~~~~~~- - -~~~~~~~~~~~- - - - - - - - - - - //
//"e.s1.trx"    fS1TRX      fE_S1_trx       X coord of track on S1 plane;    //
//"e.s1.try"    fS1TRY      fE_S1_try       Y coord of track on S1 plane.    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"

class THaScintillator : public THaNonTrackingDetector {

public:
  THaScintillator( const char* name, const char* description = "",
		   THaApparatus* a = NULL );
  virtual ~THaScintillator();

  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );

protected:

  // Mapping
  UShort_t*  fFirstChan;  // Beginning channels for each detmap module

  // Calibration
  Float_t*   fLOff;       // [fNelem] TDC offsets for left paddles
  Float_t*   fROff;       // [fNelem] TDC offsets for right paddles
  Float_t*   fLPed;       // [fNelem] ADC pedestals for left paddles
  Float_t*   fRPed;       // [fNelem] ADC pedestals for right paddles
  Float_t*   fLGain;      // [fNelem] ADC gains for left paddles
  Float_t*   fRGain;      // [fNelem] ADC gains for right paddles
  
  // Per-event data
  Int_t      fLTNhit;     // Number of Left paddles TDC times
  Float_t*   fLT;         // [fNelem] Array of Left paddles TDC times
  Float_t*   fLT_c;       // [fNelem] Array of Left paddles corrected TDC times
  Int_t      fRTNhit;     // Number of Right paddles TDC times
  Float_t*   fRT;         // [fNelem] Array of Right paddles TDC times
  Float_t*   fRT_c;       // [fNelem] Array of Right paddles corrected TDC times
  Int_t      fLANhit;     // Number of Left paddles ADC amplitudes
  Float_t*   fLA;         // [fNelem] Array of Left paddles ADC amplitudes
  Float_t*   fLA_p;       // [fNelem] Array of Left paddles ADC minus ped values
  Float_t*   fLA_c;       // [fNelem] Array of Left paddles corrected ADC ampl-s
  Int_t      fRANhit;     // Number of Right paddles ADC amplitudes
  Float_t*   fRA;         // [fNelem] Array of Right paddles ADC amplitudes
  Float_t*   fRA_p;       // [fNelem] Array of Right paddles ADC minus ped values
  Float_t*   fRA_c;       // [fNelem] Array of Right paddles corrected ADC ampl-s
  Float_t    fTRX;        // x position of track cross point (cm)
  Float_t    fTRY;        // y position of track cross point (cm)

  // Useful derived quantities
  double tan_angle, sin_angle, cos_angle;
  
  static const char NDEST = 2;
  struct DataDest {
    Int_t*    nthit;
    Int_t*    nahit;
    Float_t*  tdc;
    Float_t*  tdc_c;
    Float_t*  adc;
    Float_t*  adc_p;
    Float_t*  adc_c;
    Float_t*  offset;
    Float_t*  ped;
    Float_t*  gain;
  } fDataDest[NDEST];     // Lookup table for decoder

  void           ClearEvent();
  void           DeleteArrays();
  virtual Int_t  ReadDatabase( FILE* file, const TDatime& date );
  virtual Int_t  SetupDetector( const TDatime& date );

  THaScintillator() {}
  THaScintillator( const THaScintillator& ) {}
  THaScintillator& operator=( const THaScintillator& ) { return *this; }

  ClassDef(THaScintillator,0)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
