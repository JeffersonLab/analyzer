#ifndef ROOT_THaCherenkov
#define ROOT_THaCherenkov

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCherenkov                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Names, associated with Decoded parameters of Cherenkov detector.          //
// Variables are set in THaCherenkov::Decode()                               //
//                                                                           //
//   Name      InVariable OutVariable     Description                        //
// - ~~~~- - - ~~~~~~~~~~-~~~~~~~~~~~- - -~~~~~~~~~~~- - - - - - - - - - - - //
//"e.ar.nthit"  fNTHIT    fE_AR_nthit     Number of mirrors with TDC > 0;    //
//"e.ar.t1"     fT[0]     fE_AR_tdc[0]    TDC time of channel 1;             //
// ...          ...       ...             ...                                //
//"e.ar.t26"    fT[25]    fE_AR_tdc[25]   TDC time of channel 26;            //
//"e.ar.t_c1"   fT_c[0]   fE_AR_tdc_c[0]  Corrected TDC time of chan 1;      //
// ...          ...       ...             ...                                //
//"e.ar.t_c26"  fT_c[25]  fE_AR_tdc_c[25] Corrected TDC time of chan 26;     //
//"e.ar.nahit"  fNAHIT    fE_AR_nahit     Number of mirrors with ADC > 0;    //
//"e.ar.a1"     fA[0]     fE_AR_adc[0]    ADC value of channel 1;            //
// ...          ...       ...             ...                                //
//"e.ar.a26"    fA[25]    fE_AR_adc[25]   ADC value of channel 26;           //
//"e.ar.a_p1"   fA_p[0]   fE_AR_adc_p[0]  ADC minus ped value of chan 1;     //
// ...          ...       ...             ...                                //
//"e.ar.a_p26"  fA_p[25]  fE_AR_adc_p[25] ADC minus ped value of chan 26;    //
//"e.ar.a_c1"   fA_c[0]   fE_AR_adc_c[0]  Corrected ADC value of chan 1;     //
// ...          ...       ...             ...                                //
//"e.ar.a_c26"  fA_c[25]  fE_AR_adc_c[25] Corrected ADC value of chan 26;    //
//"e.ar.asum_p" fASUM_p   fE_AR_asum_p    Sum of chs ADC minus ped values;   //
//"e.ar.asum_c" fASUM_c   fE_AR_asum_c    Sum of channels corrected ADCs.    //
//                                                                           //
// ------------------------------------------------------------------------- //
//                                                                           //
// Names associated with parameters calculated in                            //
// THaCherenkov::CoarseProcess().                                            //
// Units of measurements are centimeters for coordinates.                    //
//                                                                           //
//   Name      InVariable   OutVariable     Description                      //
// - ~~~~- - - ~~~~~~~~~~- -~~~~~~~~~~~- - -~~~~~~~~~~~- - - - - - - - - -   //
//"e.ar.trx"    fTRX      fE_AR_trx       X coord of track in det plane      //
//"e.ar.try"    fTRY      fE_AR_try       Y coord of track in det plane      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"

class THaCherenkov : public THaPidDetector {

public:
  THaCherenkov( const char* name, const char* description = "",
	      THaApparatus* a = NULL );
  virtual ~THaCherenkov();

  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          Float_t    GetAsum() const { return fASUM_c; }

protected:
  // Mapping
  UShort_t*  fFirstChan;  // Beginning channels for each detmap module

  // Calibration
  Float_t*   fOff;        // [fNelem] TDC offsets (chan)
  Float_t*   fPed;        // [fNelem] ADC pedestals (chan)
  Float_t*   fGain;       // [fNelem] ADC gains

  // Per-event data
  Int_t      fNThit;      // Number of mirrors with non zero TDC times
  Float_t*   fT;          // [fNelem] Array of TDC times of channels
  Float_t*   fT_c;        // [fNelem] Array of corrected TDC times of channels
  Int_t      fNAhit;      // Number of mirrors with non zero ADC amplitudes
  Float_t*   fA;          // [fNelem] Array of ADC amplitudes of channels
  Float_t*   fA_p;        // [fNelem] Array of ADC minus pedestal values of chans
  Float_t*   fA_c;        // [fNelem] Array of corrected ADC amplitudes of chans
  Float_t    fASUM_p;     // Sum of ADC minus pedestal values of channels
  Float_t    fASUM_c;     // Sum of corrected ADC amplitudes of channels
  Float_t    fTRX;        // Xcoord(cm) of track cross point with Aero pl
  Float_t    fTRY;        // Ycoord(cm) of track cross point with Aero pl

  // Useful derived quantities
  double tan_angle, sin_angle, cos_angle; // Rotation angle of the detector plane

  void           ClearEvent();
  void           DeleteArrays();
  virtual Int_t  ReadDatabase( FILE* file, const TDatime& date );
  virtual Int_t  SetupDetector( const TDatime& date );

  THaCherenkov() {}
  THaCherenkov( const THaCherenkov& ) {}
  THaCherenkov& operator=( const THaCherenkov& ) { return *this; }

  ClassDef(THaCherenkov,0)    //Generic Cherenkov class
};

////////////////////////////////////////////////////////////////////////////////

#endif
