#ifndef ROOT_THaRawEvent
#define ROOT_THaRawEvent

//////////////////////////////////////////////////////////////////////////
//
// THaRawEvent
//
//////////////////////////////////////////////////////////////////////////

#include "THaEvent.h"

class THaRawEvent : public THaEvent {

public:
  THaRawEvent();
  virtual ~THaRawEvent();

  virtual void      Clear( Option_t *opt = "" );

protected:
  static const int NBPM = 6;     // BPM channels
  static const int NPAD = 6;     // # scint paddles
  static const int RA1M = 26;    // # right aerogel 1 mirrors
  static const int RCMR = 10;    // # right Cherenkov mirrors
  static const int PSBL = 48;    // # preshower blocks
  static const int SHBL = 96;    // # shower blocks

  //=== Beamline

  Float_t   fB_X4a[NBPM];        // BPM 4a X-axis channel. 
  Float_t   fB_Y4a[NBPM];        // BPM 4a Y-axis channel.
  Float_t   fB_X4b[NBPM];        // BPM 4b X-axis channel.
  Float_t   fB_Y4b[NBPM];        // BPM 4b Y-axis channel.

  Float_t   fB_Xcur;             // Raster X-axis current.
  Float_t   fB_Ycur;             // Raster Y-axis current.
  Float_t   fB_Xder;             // Raster X-axis derivative.
  Float_t   fB_Yder;             // Raster Y-axis derivative.

  //=== Right HRS raw data

#if 0
  // VDC (except variable size arrays)
  Int_t    fR_U1_nhit;           // VDC plane U1:  Number of hits
  Int_t    fR_U1_nclust;         // Number of clusters
  Int_t    fR_V1_nhit;           // VDC plane V1:  Number of hits
  Int_t    fR_V1_nclust;         // Number of clusters
  Int_t    fR_U2_nhit;           // VDC plane U2:  Number of hits
  Int_t    fR_U2_nclust;         // Number of clusters
  Int_t    fR_V2_nhit;           // VDC plane V2:  Number of hits
  Int_t    fR_V2_nclust;         // Number of clusters
  Int_t    fR_TR_n;              // Number of reconstructed VDC tracks (0 or 1)
  Float_t  fR_TR_x;              // X coordinate (in cm) of track in E-arm cs
  Float_t  fR_TR_y;              // Y coordinate (in cm) of track in E-arm cs
  Float_t  fR_TR_z;              // Z coordinate (in cm) of track in E-arm cs
  Float_t  fR_TR_th;             // Tangent of Thetta angle of track in E-arm cs
  Float_t  fR_TR_ph;             // Tangent of Phi angle of track in E-arm cs
#endif

  // S1
  Int_t    fR_S1L_nthit;         // Scint 1: Number of Left pad-s TDC times
  Float_t  fR_S1L_tdc[NPAD];     // Array of Left paddles TDC times
  Float_t  fR_S1L_tdc_c[NPAD];   // Array of Left paddles corrected TDC times
  Int_t    fR_S1R_nthit;         // Number of Right paddles TDC times
  Float_t  fR_S1R_tdc[NPAD];     // Array of Right paddles TDC times
  Float_t  fR_S1R_tdc_c[NPAD];   // Array of Right paddles corrected TDC times
  Int_t    fR_S1L_nahit;         // Number of Left paddles ADC amplitudes
  Float_t  fR_S1L_adc[NPAD];     // Array of Left paddles ADC amplitudes
  Float_t  fR_S1L_adc_p[NPAD];   // Array of Left paddles ADC minus ped values
  Float_t  fR_S1L_adc_c[NPAD];   // Array of Left paddles corrected ADC ampls
  Int_t    fR_S1R_nahit;         // Number of Right paddles ADC amplitudes
  Float_t  fR_S1R_adc[NPAD];     // Array of Right paddles ADC amplitudes
  Float_t  fR_S1R_adc_p[NPAD];   // Array of Right paddles ADC minus ped values
  Float_t  fR_S1R_adc_c[NPAD];   // Array of Right paddles corrected ADC ampls
  Float_t  fR_S1_trx;            // X coord of track cross point with S1 plane
  Float_t  fR_S1_try;            // Y coord of track cross point with S1 plane

  // S2
  Int_t    fR_S2L_nthit;         // Scint 2: Number of Left pad-s TDC times
  Float_t  fR_S2L_tdc[NPAD];     // Array of Left paddles TDC times
  Float_t  fR_S2L_tdc_c[NPAD];   // Array of Left paddles corrected TDC times
  Int_t    fR_S2R_nthit;         // Number of Right paddles TDC times
  Float_t  fR_S2R_tdc[NPAD];     // Array of Right paddles TDC times
  Float_t  fR_S2R_tdc_c[NPAD];   // Array of Right paddles corrected TDC times
  Int_t    fR_S2L_nahit;         // Number of Left paddles ADC amplitudes
  Float_t  fR_S2L_adc[NPAD];     // Array of Left paddles ADC amplitudes
  Float_t  fR_S2L_adc_p[NPAD];   // Array of Left paddles ADC minus ped values
  Float_t  fR_S2L_adc_c[NPAD];   // Array of Left paddles corrected ADC ampls
  Int_t    fR_S2R_nahit;         // Number of Right paddles ADC amplitudes
  Float_t  fR_S2R_adc[NPAD];     // Array of Right paddles ADC amplitudes
  Float_t  fR_S2R_adc_p[NPAD];   // Array of Right paddles ADC minus ped values
  Float_t  fR_S2R_adc_c[NPAD];   // Array of Right paddles corrected ADC ampls
  Float_t  fR_S2_trx;            // X coord of track cross point with S2 plane
  Float_t  fR_S2_try;            // Y coord of track cross point with S2 plane

  // Aerogel
  Int_t    fR_AR_nthit;          // Aerogel: Number of TDC times
  Float_t  fR_AR_tdc[RA1M];      // Array of TDC times of mirrors
  Float_t  fR_AR_tdc_c[RA1M];    // Array of corrected TDC times of mirrors
  Int_t    fR_AR_nahit;          // Number of ADC amplitudes
  Float_t  fR_AR_adc[RA1M];      // Array of ADC amplitudes of mirrors
  Float_t  fR_AR_adc_p[RA1M];    // Array of ADC minus ped values of mirrors
  Float_t  fR_AR_adc_c[RA1M];    // Array of corrected ADC amplitudes of mirrors
  Float_t  fR_AR_asum_p;         // Sum of mirrors ADC minus pedestal values
  Float_t  fR_AR_asum_c;         // Sum of mirrors corrected ADC amplitudes
  Float_t  fR_AR_trx;            // X coord of track cross point with Aero plane
  Float_t  fR_AR_try;            // Y coord of track cross point with Aero plane

  // Cherenkov
  Int_t    fR_CH_nthit;          // Gas Cherenkov: Number of TDC times
  Float_t  fR_CH_tdc[RCMR];      // Array of TDC times of mirrors
  Float_t  fR_CH_tdc_c[RCMR];    // Array of corrected TDC times of mirrors
  Int_t    fR_CH_nahit;          // Number of ADC amplitudes
  Float_t  fR_CH_adc[RCMR];      // Array of ADC amplitudes of mirrors
  Float_t  fR_CH_adc_p[RCMR];    // Array of ADC minus ped values of mirrors
  Float_t  fR_CH_adc_c[RCMR];    // Array of corrected ADC amplitudes of mirrors
  Float_t  fR_CH_asum_p;         // Sum of mirrors ADC minus pedestal values
  Float_t  fR_CH_asum_c;         // Sum of mirrors corrected ADC amplitudes
  Float_t  fR_CH_trx;            // X coord of track cross point with Cher plane
  Float_t  fR_CH_try;            // Y coord of track cross point with Cher plane

  // Preshower
  Int_t    fR_PSH_nhit;          // Preshower: Number of ADC hits
  Float_t  fR_PSH_adc[PSBL];     // Array of ADC amplitudes of blocks
  Float_t  fR_PSH_adc_p[PSBL];   // Array of ADC minus peds values of blocks
  Float_t  fR_PSH_adc_c[PSBL];   // Array of corrected ADC ampls of blocks
  Float_t  fR_PSH_asum_p;        // Sum of blocks ADC minus pedestal values
  Float_t  fR_PSH_asum_c;        // Sum of blocks corrected ADC amplitudes
  Int_t    fR_PSH_nclust;        // Number of clusters in Preshower
  Float_t  fR_PSH_e;             // Energy of the largest cluster
  Float_t  fR_PSH_x;             // X-coord of the cluster
  Float_t  fR_PSH_y;             // Y-coord of the cluster
  Int_t    fR_PSH_mult;          // Number of blocks in the cluster
  Int_t    fR_PSH_nblk[6];       // Array of num of blocks composing the cluster
  Float_t  fR_PSH_eblk[6];       // Array of Eners in blks composing the cluster
  Float_t  fR_PSH_trx;           // X coord of track cross point with Psh plane
  Float_t  fR_PSH_try;           // Y coord of track cross point with Psh plane

  // Shower
  Int_t    fR_SHR_nhit;          // E-arm Shower: Number of ADC hits
  Float_t  fR_SHR_adc[SHBL];     // Array of ADC amplitudes of blocks
  Float_t  fR_SHR_adc_p[SHBL];   // Array of ADC minus peds values of blocks
  Float_t  fR_SHR_adc_c[SHBL];   // Array of corrected ADC ampls of blocks
  Float_t  fR_SHR_asum_p;        // Sum of blocks ADC minus pedestal values
  Float_t  fR_SHR_asum_c;        // Sum of blocks corrected ADC amplitudes
  Int_t    fR_SHR_nclust;        // Number of clusters in Shower
  Float_t  fR_SHR_e;             // Energy of the largest cluster
  Float_t  fR_SHR_x;             // X-coord of the cluster
  Float_t  fR_SHR_y;             // Y-coord of the cluster
  Int_t    fR_SHR_mult;          // Number of blocks in the cluster
  Int_t    fR_SHR_nblk[9];       // Array of num of blocks composing the cluster
  Float_t  fR_SHR_eblk[9];       // Array of Eners in blks composing the cluster
  Float_t  fR_SHR_trx;           // X coord of track cross point with Shwr plane
  Float_t  fR_SHR_try;           // Y coord of track cross point with Shwr plane

  // Total shower
  Float_t  fR_TSH_e;             // Total shower energy in Preshower and Shower
  Int_t    fR_TSH_id;            // ID of Presh and Shower clusters coincidence

  //=== Left HRS raw data

#if 0
  // VDC (except variable size arrays)
  Int_t    fL_U1_nhit;           // VDC plane U1:  Number of hits
  Int_t    fL_U1_nclust;         // Number of clusters
  Int_t    fL_V1_nhit;           // VDC plane V1:  Number of hits
  Int_t    fL_V1_nclust;         // Number of clusters
  Int_t    fL_U2_nhit;           // VDC plane U2:  Number of hits
  Int_t    fL_U2_nclust;         // Number of clusters
  Int_t    fL_V2_nhit;           // VDC plane V2:  Number of hits
  Int_t    fL_V2_nclust;         // Number of clusters
  Int_t    fL_TR_n;              // Number of reconstructed VDC tracks (0 or 1)
  Float_t  fL_TR_x;              // X coordinate (in cm) of track in E-arm cs
  Float_t  fL_TR_y;              // Y coordinate (in cm) of track in E-arm cs
  Float_t  fL_TR_z;              // Z coordinate (in cm) of track in E-arm cs
  Float_t  fL_TR_th;             // Tangent of Thetta angle of track in E-arm cs
  Float_t  fL_TR_ph;             // Tangent of Phi angle of track in E-arm cs
#endif

  // S1
  Int_t    fL_S1L_nthit;         // Scint 1: Number of Left pad-s TDC times
  Float_t  fL_S1L_tdc[NPAD];     // Array of Left paddles TDC times
  Float_t  fL_S1L_tdc_c[NPAD];   // Array of Left paddles corrected TDC times
  Int_t    fL_S1R_nthit;         // Number of Right paddles TDC times
  Float_t  fL_S1R_tdc[NPAD];     // Array of Right paddles TDC times
  Float_t  fL_S1R_tdc_c[NPAD];   // Array of Right paddles corrected TDC times
  Int_t    fL_S1L_nahit;         // Number of Left paddles ADC amplitudes
  Float_t  fL_S1L_adc[NPAD];     // Array of Left paddles ADC amplitudes
  Float_t  fL_S1L_adc_p[NPAD];   // Array of Left paddles ADC minus ped values
  Float_t  fL_S1L_adc_c[NPAD];   // Array of Left paddles corrected ADC ampls
  Int_t    fL_S1R_nahit;         // Number of Right paddles ADC amplitudes
  Float_t  fL_S1R_adc[NPAD];     // Array of Right paddles ADC amplitudes
  Float_t  fL_S1R_adc_p[NPAD];   // Array of Right paddles ADC minus ped values
  Float_t  fL_S1R_adc_c[NPAD];   // Array of Right paddles corrected ADC ampls
  Float_t  fL_S1_trx;            // X coord of track cross point with S1 plane
  Float_t  fL_S1_try;            // Y coord of track cross point with S1 plane

  // S2
  Int_t    fL_S2L_nthit;         // Scint 2: Number of Left pad-s TDC times
  Float_t  fL_S2L_tdc[NPAD];     // Array of Left paddles TDC times
  Float_t  fL_S2L_tdc_c[NPAD];   // Array of Left paddles corrected TDC times
  Int_t    fL_S2R_nthit;         // Number of Right paddles TDC times
  Float_t  fL_S2R_tdc[NPAD];     // Array of Right paddles TDC times
  Float_t  fL_S2R_tdc_c[NPAD];   // Array of Right paddles corrected TDC times
  Int_t    fL_S2L_nahit;         // Number of Left paddles ADC amplitudes
  Float_t  fL_S2L_adc[NPAD];     // Array of Left paddles ADC amplitudes
  Float_t  fL_S2L_adc_p[NPAD];   // Array of Left paddles ADC minus ped values
  Float_t  fL_S2L_adc_c[NPAD];   // Array of Left paddles corrected ADC ampls
  Int_t    fL_S2R_nahit;         // Number of Right paddles ADC amplitudes
  Float_t  fL_S2R_adc[NPAD];     // Array of Right paddles ADC amplitudes
  Float_t  fL_S2R_adc_p[NPAD];   // Array of Right paddles ADC minus ped values
  Float_t  fL_S2R_adc_c[NPAD];   // Array of Right paddles corrected ADC ampls
  Float_t  fL_S2_trx;            // X coord of track cross point with S2 plane
  Float_t  fL_S2_try;            // Y coord of track cross point with S2 plane

#if 0
  // Right HRS VDC variable size arrays 
  Int_t*   fR_U1_wire;           //[fR_U1_nhit] Hit wires numbers
  Float_t* fR_U1_time;           //[fR_U1_nhit] Corresponding TDC times
  Float_t* fR_U1_clpos;          //[fR_U1_nclust] Centers of clusters (in wires)
  Int_t*   fR_U1_clsiz;          //[fR_U1_nclust]  Sizes of clusters (in wires)
  Int_t*   fR_V1_wire;           //[fR_V1_nhit] Hit wires numbers
  Float_t* fR_V1_time;           //[fR_V1_nhit] Corresponding TDC times
  Float_t* fR_V1_clpos;          //[fR_V1_nclust]  Centers of clusters (in wires)
  Int_t*   fR_V1_clsiz;          //[fR_V1_nclust]  Sizes of clusters (in wires)
  Int_t*   fR_U2_wire;           //[fR_U2_nhit] Hit wires numbers
  Float_t* fR_U2_time;           //[fR_U2_nhit] Corresponding TDC times
  Float_t* fR_U2_clpos;          //[fR_U2_nclust]  Centers of clusters (in wires)
  Int_t*   fR_U2_clsiz;          //[fR_U2_nclust]  Sizes of clusters (in wires)
  Int_t*   fR_V2_wire;           //[fR_V2_nhit] Hit wires numbers
  Float_t* fR_V2_time;           //[fR_V2_nhit] Corresponding TDC times
  Float_t* fR_V2_clpos;          //[fR_V2_nclust]  Centers of clusters (in wires)
  Int_t*   fR_V2_clsiz;          //[fR_V2_nclust]  Sizes of clusters (in wires)

  // Left HRS VDC variable size arrays 
  Int_t*   fL_U1_wire;           //[fL_U1_nhit] Hit wires numbers
  Float_t* fL_U1_time;           //[fL_U1_nhit] Corresponding TDC times
  Float_t* fL_U1_clpos;          //[fL_U1_nclust] Centers of clusters (in wires)
  Int_t*   fL_U1_clsiz;          //[fL_U1_nclust]  Sizes of clusters (in wires)
  Int_t*   fL_V1_wire;           //[fL_V1_nhit] Hit wires numbers
  Float_t* fL_V1_time;           //[fL_V1_nhit] Corresponding TDC times
  Float_t* fL_V1_clpos;          //[fL_V1_nclust]  Centers of clusters (in wires)
  Int_t*   fL_V1_clsiz;          //[fL_V1_nclust]  Sizes of clusters (in wires)
  Int_t*   fL_U2_wire;           //[fL_U2_nhit] Hit wires numbers
  Float_t* fL_U2_time;           //[fL_U2_nhit] Corresponding TDC times
  Float_t* fL_U2_clpos;          //[fL_U2_nclust]  Centers of clusters (in wires)
  Int_t*   fL_U2_clsiz;          //[fL_U2_nclust]  Sizes of clusters (in wires)
  Int_t*   fL_V2_wire;           //[fL_V2_nhit] Hit wires numbers
  Float_t* fL_V2_time;           //[fL_V2_nhit] Corresponding TDC times
  Float_t* fL_V2_clpos;          //[fL_V2_nclust]  Centers of clusters (in wires)
  Int_t*   fL_V2_clsiz;          //[fL_V2_nclust]  Sizes of clusters (in wires)
#endif

  ClassDef(THaRawEvent,2)  //THaRawEvent structure
};


#endif

