#ifndef ROOT_THaVDCPlane
#define ROOT_THaVDCPlane

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPlane                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "TClonesArray.h"

class THaEvData;
class THaVDCWire;
class THaVDCUVPlane;
class THaVDCCluster;
class THaVDCHit;
class THaVDCTimeToDistConv;

class THaVDCPlane : public THaSubDetector {

public:

  THaVDCPlane( const char* name="", const char* description="",
	       THaDetectorBase* parent = NULL );
  virtual ~THaVDCPlane();

  virtual Int_t   Decode( const THaEvData& ); // Raw data -> hits
  virtual Int_t   FindClusters();             // Hits -> clusters  
  virtual Int_t   FitTracks();                // Clusters -> tracks

  //Get and Set functions
  Int_t          GetNClusters() const { return fClusters->GetLast()+1; }
  TClonesArray*  GetClusters()  const { return fClusters; }
  THaVDCCluster* GetCluster(Int_t i) const
  { return (THaVDCCluster*)fClusters->At(i); } 

  Int_t          GetNWires()    const { return fWires->GetLast()+1; }
  TClonesArray*  GetWires()     const { return fWires; }
  THaVDCWire*    GetWire(Int_t i) const
  { return (THaVDCWire*)fWires->At(i);}

  Int_t          GetNHits()     const { return fHits->GetLast()+1; }
  TClonesArray*  GetHits()      const { return fHits; }
  THaVDCHit*     GetHit(Int_t i) const
  { return (THaVDCHit*)fHits->At(i); }

  Int_t    GetNWiresHit() const  { return fNWiresHit; } 

  Double_t GetZ()        const   { return fZ; }
  Double_t GetWBeg()     const   { return fWBeg; }
  Double_t GetWSpac()    const   { return fWSpac; }
  Double_t GetWAngle()   const   { return fWAngle; }
  Double_t GetTDCRes()   const   { return fTDCRes; }
  Double_t GetDriftVel() const   { return fDriftVel; }

  Double_t GetT0() const { return fT0; }
  Int_t GetNumBins() const { return fNumBins; }
  Float_t *GetTable() const { return fTable; }

/*    void SetZ(Double_t z)          {fZ     = z;    } */
/*    void SetWBeg(Double_t wbeg)    {fWBeg  = wbeg; } */
/*    void SetWSpac(Double_t wspac)  {fWSpac = wspac;} */
/*    void SetWAngle(Double_t wangle){fWAngle = wangle;} */
  //  void SetUVPlane(THaVDCUVPlane * plane) {fUVPlane = plane;}

protected:

  //Use TClonesArray::GetLast()+1 to get the number of wires, hits, & clusters 
  TClonesArray*  fWires;     // Wires
  TClonesArray*  fHits;      // Fired wires 
  TClonesArray*  fClusters;  // Clusters
  
  Int_t fNWiresHit;  // Number of wires that were hit

  // The following parameters are read from database.
 
  Int_t fNMaxGap;            // Max gap in a cluster
  Int_t fMinTime, fMaxTime;  // Min and Max limits of TDC times for clusters
  Int_t fFlags;              // Analysis control flags

  Double_t fZ;            // Z coordinate of planes in U1 coord sys (m)
  Double_t fWBeg;         // Position of 1-st wire in E-arm coord sys (m)
  Double_t fWSpac;        // Wires spacing and direction (m)
  Double_t fWAngle;       // Angle between dispersive direction and direction of 
                          // decreasing wire number (rad)
  Double_t fTDCRes;       // TDC Resolution ( s / channel)
  Double_t fDriftVel;     // Drift velocity in the wire plane (m/s)

  // Lookup table parameters
  Double_t fT0;     // calculated zero time 
  Int_t fNumBins;   // size of lookup table
  Float_t *fTable;  // time-to-distance lookup table

  THaVDCTimeToDistConv* fTTDConv;  // Time-to-distance converter for this plane's wires

  THaDetectorBase* fVDC;  // VDC detector to which this plane belongs

  virtual void  Clear( Option_t* opt="" );

  virtual void  MakePrefix();
  virtual Int_t ReadDatabase( const TDatime& date );
  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaVDCPlane,0)             // VDCPlane class
};

///////////////////////////////////////////////////////////////////////////////

#endif
