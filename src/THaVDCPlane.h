#ifndef ROOT_THaVDCPlane
#define ROOT_THaVDCPlane

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPlane                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "THaEvData.h"
#include "TClonesArray.h"

class THaVDCWire;
class THaVDCUVPlane;
class THaVDCCluster;
class THaVDCHit;

class THaVDCPlane : public THaSubDetector {

public:

  THaVDCPlane( const char* name="", const char* description="",
	       THaDetectorBase* parent = NULL );
  virtual ~THaVDCPlane();

  virtual Int_t   Decode( const THaEvData& ); // Raw data -> hits
  virtual Int_t   FindClusters();             // Hits -> clusters  
  virtual Int_t   FitTracks();                // Clusters -> tracks

  //sort hits?
  
  //Get and Set functions
  Int_t GetNClusters()         { return fClusters->GetLast()+1;  }
  TClonesArray * GetClusters() { return fClusters; }
  THaVDCCluster* GetCluster(Int_t i) 
    { return (THaVDCCluster*)(*fClusters)[i]; } 

  Int_t GetNWires()            { return fWires->GetLast()+1;  }
  TClonesArray * GetWires()    { return fWires; }
  THaVDCWire * GetWire(Int_t i){ return (THaVDCWire *)(*fWires)[i];}

  Int_t GetNHits()             { return fHits->GetLast()+1; }
  TClonesArray * GetHits()     { return fHits; }
  THaVDCHit * GetHit(Int_t i)  { return (THaVDCHit *)(*fHits)[i];}

  Int_t GetNWiresHit()         { return fNWiresHit; } 

  Double_t GetZ()              { return fZ;       }
  Double_t GetWBeg()           { return fWBeg;    }
  Double_t GetWSpac()          { return fWSpac;   }
  Double_t GetWAngle()         { return fWAngle;  }
  Double_t GetTDCRes()         { return fTDCRes;  }
  Double_t GetDriftVel()       { return fDriftVel;}

/*    void SetZ(Double_t z)          {fZ     = z;    } */
/*    void SetWBeg(Double_t wbeg)    {fWBeg  = wbeg; } */
/*    void SetWSpac(Double_t wspac)  {fWSpac = wspac;} */
/*    void SetWAngle(Double_t wangle){fWAngle = wangle;} */
  void SetUVPlane(THaVDCUVPlane * plane) {fUVPlane = plane;}

protected:

  //Use TClonesArray::GetLast()+1 to get the number of wires, hits, & clusters 
  TClonesArray * fWires;     // Wires
  TClonesArray * fHits;      // Fired wires 
  TClonesArray * fClusters;  // Clusters

  Int_t fNWiresHit;  // Number of wires that were hit

  THaVDCUVPlane * fUVPlane;// Ptr to UV plane owning this plane

// The following data are read from database in function THaVDCPlane::Init(),
// if personal database file "db_VDC.dat" exists in current directory
 
  Int_t fNMaxGap;            // Max gap in a cluster
  Int_t fMinTime, fMaxTime;  // Min and Max limits of TDC times for clusters

  Double_t fZ;        // Z coordinate of planes in U1 coord sys (m)
  Double_t fWBeg;     // Position of 1-st wire in E-arm coord sys (m)
  Double_t fWSpac;    // Wires spacing and direction (m)
  Double_t fWAngle;   // Angle between dispersive direction and direction of 
                      // decreasing wire number (rad)
  Double_t fTDCRes;   // TDC Resolution ( s / channel)
  Double_t fDriftVel; // Drift velocity in the wire plane (m/s)

  void Clear();
 
  THaVDCPlane( const THaVDCPlane& ) {}
  THaVDCPlane& operator=( const THaVDCPlane& ) { return *this; }
 
  virtual void  MakePrefix();
  virtual Int_t ReadDatabase( FILE* file, const TDatime& date );
  virtual Int_t SetupDetector( const TDatime& date );

  ClassDef(THaVDCPlane,0)             // VDCPlane class
};

///////////////////////////////////////////////////////////////////////////////

#endif
