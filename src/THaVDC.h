#ifndef ROOT_THaVDC
#define ROOT_THaVDC

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTrackingDetector.h"

class THaVDC : public THaTrackingDetector {

public:
  THaVDC( const char* name, const char* description = "",
	  THaApparatus* a = NULL );
  virtual ~THaVDC();

  virtual Int_t Decode( const THaEvData& );
  virtual Int_t CoarseTrack( TClonesArray& tracks );
  virtual Int_t FineTrack( TClonesArray& tracks );

  static const int MAXHIT = 50;  // Maximum number of hits considered on a plane
  static const int MAXCLU = 10;  // Max number of clusters considered on a plane

protected:

  // Mapping
  Int_t*     fFirstWire;         // Numbers of first wires for each det map module

  // Geometry
  double u1wbeg,  v1wbeg,  u2wbeg,  v2wbeg; // Position of 1-st wire in own cs
  double u1wspac, v1wspac, u2wspac, v2wspac;// Wires spacing and direction

  // Parameters
  Int_t      fMaxgap;            // Max gap allowed in a cluster
  Int_t      fMintime, fMaxtime; // Min and Max limits of TDC times for clusters

  // Per-event data
  Int_t      fU1NHIT;            // VDC Plane U1: Number of TDC hits
  Int_t      fU1WIRE[MAXHIT];    //               Fired wires numbers
  Float_t    fU1TIME[MAXHIT];    //               Corresponding TDC times
  Int_t      fU1NCLUST;          //               Number of clusters
  Float_t    fU1CLPOS[MAXCLU];   //               Positions of clusters (wires)
  Int_t      fU1CLSIZ[MAXCLU];   //               Sizess of clusters (wires)
  Int_t      fV1NHIT;            // VDC Plane V1: Number of TDC hits
  Int_t      fV1WIRE[MAXHIT];    //               Fired wires numbers
  Float_t    fV1TIME[MAXHIT];    //               Corresponding TDC times
  Int_t      fV1NCLUST;          //               Number of clusters
  Float_t    fV1CLPOS[MAXCLU];   //               Positions of clusters (wires)
  Int_t      fV1CLSIZ[MAXCLU];   //               Sizess of clusters (wires)
  Int_t      fU2NHIT;            // VDC Plane U2: Number of TDC hits
  Int_t      fU2WIRE[MAXHIT];    //               Fired wires numbers
  Float_t    fU2TIME[MAXHIT];    //               Corresponding TDC times
  Int_t      fU2NCLUST;          //               Number of clusters
  Float_t    fU2CLPOS[MAXCLU];   //               Positions of clusters (wires)
  Int_t      fU2CLSIZ[MAXCLU];   //               Sizess of clusters (wires)
  Int_t      fV2NHIT;            // VDC Plane V2: Number of TDC hits
  Int_t      fV2WIRE[MAXHIT];    //               Fired wires numbers
  Float_t    fV2TIME[MAXHIT];    //               Corresponding TDC times
  Int_t      fV2NCLUST;          //               Number of clusters
  Float_t    fV2CLPOS[MAXCLU];   //               Positions of clusters (wires)
  Int_t      fV2CLSIZ[MAXCLU];   //               Sizess of clusters (wires)

  Int_t      fTRN;               // Number of reconstructed VDC tracks (0 or 1)
  Float_t    fTRX;               // X coordinate (cm) of track in E-arm cs
  Float_t    fTRY;               // Y coordinate (cm) of track in E-arm cs
  Float_t    fTRZ;               // Z coordinate (cm) of track in E-arm cs
  Float_t    fTRTH;              // Tangent of Theta angle of track in E-arm cs
  Float_t    fTRPH;              // Tangent of Phi angle of track in E-arm cs

  // Useful derived quantities

  Float_t du2u1, dv2v1, dv1u1;
  Float_t tan_vdc, sin_vdc, cos_vdc, sin_uw, cos_uw, sin_vw, cos_vw, sin_uvw;

  static const char NPLANES = 4;
  struct DataDest {
    Int_t*    nhit;
    Int_t*    wire;
    Float_t*  time;
  } fDataDest[NPLANES];          // Lookup table for decoder

  void           ClearEvent();
  virtual Int_t  ReadDatabase( FILE* file, const TDatime& date );
  virtual Int_t  SetupDetector( const TDatime& date );

  Int_t Detcluster( Int_t nhit, Int_t wires[], Float_t times[],
		    Int_t& nclust, Float_t clpos[], Int_t clsiz[] );

  // Prevent default construction, copying, assignment
  THaVDC() {}
  THaVDC( const THaVDC& ) {}
  THaVDC& operator=( const THaVDC& ) { return *this; }

  ClassDef(THaVDC,0)             // VDC class
};

////////////////////////////////////////////////////////////////////////////////

#endif
