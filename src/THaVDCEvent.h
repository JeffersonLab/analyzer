#ifndef ROOT_THaVDCEvent
#define ROOT_THaVDCEvent

//////////////////////////////////////////////////////////////////////////
//
// THaRawEvent
//
//////////////////////////////////////////////////////////////////////////

#include "THaEvent.h"

class THaVar;

class THaVDCEvent : public THaEvent {

public:
  THaVDCEvent();
  virtual ~THaVDCEvent();

  virtual void      Clear( Option_t *opt = "" );
  virtual Int_t     Fill();

protected:

  static const int NVDC = 8;     // Number of global variables per VDC

  enum EBlock { kAll, kHits, kClusters, kTracks };

  // made data members public, since they need to be accessed
  // for data analysis
public:
  Int_t fMaxhit;                 //! Size of variable VDC hit arrays
  Int_t fMaxclu;                 //! Size of variable VDC cluster arrays
  Int_t fMaxtrk;                 //! Size of variable VDC track arrays
  THaVar* fNhitVar[NVDC];        //! Global variable holding VDC Nhits
  THaVar* fNcluVar[NVDC];        //! Global variable holding VDC Nclusters
  THaVar* fNtrkVar[2];           //! Global variables holding number of VDC tracks

  //=== Right HRS raw data

  // VDC (except variable size arrays)
  Int_t    fR_U1_nhit;           // VDC plane U1:  Number of hits
  Int_t    fR_U1_nclust;         // Number of clusters
  Int_t    fR_V1_nhit;           // VDC plane V1:  Number of hits
  Int_t    fR_V1_nclust;         // Number of clusters
  Int_t    fR_U2_nhit;           // VDC plane U2:  Number of hits
  Int_t    fR_U2_nclust;         // Number of clusters
  Int_t    fR_V2_nhit;           // VDC plane V2:  Number of hits
  Int_t    fR_V2_nclust;         // Number of clusters
  Int_t    fR_TR_n;              // Number of reconstructed VDC tracks

  //=== Left HRS raw data

  // VDC (except variable size arrays)
  Int_t    fL_U1_nhit;           // VDC plane U1:  Number of hits
  Int_t    fL_U1_nclust;         // Number of clusters
  Int_t    fL_V1_nhit;           // VDC plane V1:  Number of hits
  Int_t    fL_V1_nclust;         // Number of clusters
  Int_t    fL_U2_nhit;           // VDC plane U2:  Number of hits
  Int_t    fL_U2_nclust;         // Number of clusters
  Int_t    fL_V2_nhit;           // VDC plane V2:  Number of hits
  Int_t    fL_V2_nclust;         // Number of clusters
  Int_t    fL_TR_n;              // Number of reconstructed VDC tracks

  // Right HRS VDC variable size arrays 
  Int_t*    fR_U1_wire;           //[fR_U1_nhit] Hit wires numbers
  Int_t*    fR_U1_rawtime;        //[fR_U1_nhit] Raw TDC times
  Double_t* fR_U1_time;           //[fR_U1_nhit] Corresponding TDC times
  Double_t* fR_U1_dist;           //[fR_U1_nhit] Drift distances
  Double_t* fR_U1_clpos;          //[fR_U1_nclust] Centers of clusters (m)
  Double_t* fR_U1_slope;          //[fR_U1_nclust] Cluster slopes
  Int_t*    fR_U1_clpiv;          //[fR_U1_nclust] Pivot wire number
  Int_t*    fR_U1_clsiz;          //[fR_U1_nclust]  Sizes of clusters (in wires)
  Int_t*    fR_V1_wire;           //[fR_V1_nhit] Hit wires numbers
  Int_t*    fR_V1_rawtime;        //[fR_V1_nhit] Raw TDC times
  Double_t* fR_V1_time;           //[fR_V1_nhit] Corresponding TDC times
  Double_t* fR_V1_dist;           //[fR_V1_nhit] Drift distances
  Double_t* fR_V1_clpos;          //[fR_V1_nclust]  Centers of clusters (m)
  Double_t* fR_V1_slope;          //[fR_V1_nclust] Cluster slopes
  Int_t*    fR_V1_clpiv;          //[fR_V1_nclust]  Pivot wire number
  Int_t*    fR_V1_clsiz;          //[fR_V1_nclust]  Sizes of clusters (in wires)
  Int_t*    fR_U2_wire;           //[fR_U2_nhit] Hit wires numbers
  Int_t*    fR_U2_rawtime;        //[fR_U2_nhit] Raw TDC times
  Double_t* fR_U2_time;           //[fR_U2_nhit] Corresponding TDC times
  Double_t* fR_U2_dist;           //[fR_U2_nhit] Drift distances
  Double_t* fR_U2_clpos;          //[fR_U2_nclust]  Centers of clusters (m)
  Double_t* fR_U2_slope;          //[fR_U2_nclust] Cluster slopes
  Int_t*    fR_U2_clpiv;          //[fR_U2_nclust]  Pivot wire number
  Int_t*    fR_U2_clsiz;          //[fR_U2_nclust]  Sizes of clusters (in wires)
  Int_t*    fR_V2_wire;           //[fR_V2_nhit] Hit wires numbers
  Int_t*    fR_V2_rawtime;        //[fR_V2_nhit] Raw TDC times
  Double_t* fR_V2_time;           //[fR_V2_nhit] Corresponding TDC times
  Double_t* fR_V2_dist;           //[fR_V2_nhit] Drift distances
  Double_t* fR_V2_clpos;          //[fR_V2_nclust]  Centers of clusters (m)
  Double_t* fR_V2_slope;          //[fR_V2_nclust] Cluster slopes
  Int_t*    fR_V2_clpiv;          //[fR_V2_nclust]  Pivot wire number
  Int_t*    fR_V2_clsiz;          //[fR_V2_nclust]  Sizes of clusters (in wires)
  Double_t* fR_TR_x;              //[fR_TR_n] X coordinate (in cm) of track in E-arm cs
  Double_t* fR_TR_y;              //[fR_TR_n] Y coordinate (in cm) of track in E-arm cs
  Double_t* fR_TR_th;             //[fR_TR_n] Tangent of Theta angle of track in E-arm cs
  Double_t* fR_TR_ph;             //[fR_TR_n] Tangent of Phi angle of track in E-arm cs
  Double_t* fR_TR_p;              //[fR_TR_n] Track momentum (GeV)
  UInt_t*   fR_TR_flag;           //[fR_TR_n] Track status flag

  Double_t* fR_TD_x;              //[fR_TR_n] X coordinate (in cm) of track in detector cs
  Double_t* fR_TD_y;              //[fR_TR_n] Y coordinate (in cm) of track in detector cs
  Double_t* fR_TD_th;             //[fR_TR_n] Tangent of Theta angle of track in detector cs
  Double_t* fR_TD_ph;             //[fR_TR_n] Tangent of Phi angle of track in detector cs

  Double_t* fR_TR_rx;             //[fR_TR_n] X coordinate (in cm) of track in rotating cs
  Double_t* fR_TR_ry;             //[fR_TR_n] Y coordinate (in cm) of track in rotating cs
  Double_t* fR_TR_rth;            //[fR_TR_n] Tangent of Theta angle of track in rotating cs
  Double_t* fR_TR_rph;            //[fR_TR_n] Tangent of Phi angle of track in rotating cs

  Double_t* fR_TG_y;              //[fR_TR_n] Y coordinate (in cm) of track in E-arm cs
  Double_t* fR_TG_th;             //[fR_TR_n] Tangent of Thtta angle of track in E-arm cs
  Double_t* fR_TG_ph;             //[fR_TR_n] Tangent of Phi angle of track in E-arm cs
  Double_t* fR_TG_dp;             //[fR_TR_n] Deviation of momentum from central mmmentum

  // Left HRS VDC variable size arrays 
  Int_t*    fL_U1_wire;           //[fL_U1_nhit] Hit wires numbers
  Int_t*    fL_U1_rawtime;        //[fL_U1_nhit] Raw TDC times
  Double_t* fL_U1_time;           //[fL_U1_nhit] Corresponding TDC times
  Double_t* fL_U1_dist;           //[fL_U1_nhit] Drift distances
  Double_t* fL_U1_clpos;          //[fL_U1_nclust] Centers of clusters (m)
  Double_t* fL_U1_slope;          //[fL_U1_nclust] Cluster slopes
  Int_t*    fL_U1_clpiv;          //[fL_U1_nclust] Pivot wire of cluster
  Int_t*    fL_U1_clsiz;          //[fL_U1_nclust]  Sizes of clusters (in wires)
  Int_t*    fL_V1_wire;           //[fL_V1_nhit] Hit wires numbers
  Int_t*    fL_V1_rawtime;        //[fL_V1_nhit] Raw TDC times
  Double_t* fL_V1_time;           //[fL_V1_nhit] Corresponding TDC times
  Double_t* fL_V1_dist;           //[fL_V1_nhit] Drift distances
  Double_t* fL_V1_clpos;          //[fL_V1_nclust]  Centers of clusters (m)
  Double_t* fL_V1_slope;          //[fL_V1_nclust] Cluster slopes
  Int_t*    fL_V1_clpiv;          //[fL_V1_nclust]  Pivot wire of cluster
  Int_t*    fL_V1_clsiz;          //[fL_V1_nclust]  Sizes of clusters (in wires)
  Int_t*    fL_U2_wire;           //[fL_U2_nhit] Hit wires numbers
  Int_t*    fL_U2_rawtime;        //[fL_U2_nhit] Raw TDC times
  Double_t* fL_U2_time;           //[fL_U2_nhit] Corresponding TDC times
  Double_t* fL_U2_dist;           //[fL_U2_nhit] Drift distances
  Double_t* fL_U2_clpos;          //[fL_U2_nclust]  Centers of clusters (m)
  Double_t* fL_U2_slope;          //[fL_U2_nclust] Cluster slopes
  Int_t*    fL_U2_clpiv;          //[fL_U2_nclust]  Pivot wire of cluster
  Int_t*    fL_U2_clsiz;          //[fL_U2_nclust]  Sizes of clusters (in wires)
  Int_t*    fL_V2_wire;           //[fL_V2_nhit] Hit wires numbers
  Int_t*    fL_V2_rawtime;        //[fL_V2_nhit] Raw TDC times
  Double_t* fL_V2_time;           //[fL_V2_nhit] Corresponding TDC times
  Double_t* fL_V2_dist;           //[fL_V2_nhit] Drift distances
  Double_t* fL_V2_clpos;          //[fL_V2_nclust]  Centers of clusters (m)
  Double_t* fL_V2_slope;          //[fL_V2_nclust] Cluster slopes
  Int_t*    fL_V2_clpiv;          //[fL_V2_nclust]  Pivot wire of cluster
  Int_t*    fL_V2_clsiz;          //[fL_V2_nclust]  Sizes of clusters (in wires)
  Double_t* fL_TR_x;              //[fL_TR_n] X coordinate (in m) of track in E-arm cs
  Double_t* fL_TR_y;              //[fL_TR_n] Y coordinate (in m) of track in E-arm cs
  Double_t* fL_TR_th;             //[fL_TR_n] Tangent of Theta angle of track in E-arm cs
  Double_t* fL_TR_ph;             //[fL_TR_n] Tangent of Phi angle of track in E-arm cs
  Double_t* fL_TR_p;              //[fL_TR_n] Track momentum (GeV)
  UInt_t*   fL_TR_flag;           //[fL_TR_n] Track status flag

  Double_t* fL_TD_x;              //[fL_TR_n] X coordinate (in m) of track in detetcor cs
  Double_t* fL_TD_y;              //[fL_TR_n] Y coordinate (in m) of track in detector cs
  Double_t* fL_TD_th;             //[fL_TR_n] Tangent of Theta angle of track in detector cs
  Double_t* fL_TD_ph;             //[fL_TR_n] Tangent of Phi angle of track in detector cs

  Double_t* fL_TR_rx;             //[fL_TR_n] X coordinate (in m) of track in rotating cs
  Double_t* fL_TR_ry;             //[fL_TR_n] Y coordinate (in m) of track in rotating cs
  Double_t* fL_TR_rth;            //[fL_TR_n] Tangent of Theta angle of track in rotating cs
  Double_t* fL_TR_rph;            //[fL_TR_n] Tangent of Phi angle of track in rotating cs

  Double_t* fL_TG_y;              //[fL_TR_n] Y coordinate (in m) of track in E-arm cs
  Double_t* fL_TG_th;             //[fL_TR_n] Tangent of Theta angle of track in E-arm cs
  Double_t* fL_TG_ph;             //[fL_TR_n] Tangent of Phi angle of track in E-arm cs
  Double_t* fL_TG_dp;             //[fL_TR_n] Deviation of momentum from center

protected:
  void CreateVariableArrays( EBlock which = kAll );
  void DeleteVariableArrays( EBlock which = kAll );
  void SetupDatamap( EBlock which = kAll );
  void InitCounters();

  ClassDef(THaVDCEvent,1)  //THaVDCEvent structure
};


#endif

