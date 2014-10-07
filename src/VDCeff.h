#ifndef HALLA_VDCeff
#define HALLA_VDCeff

//////////////////////////////////////////////////////////////////////////
//
// VDCeff - VDC efficiency calculation
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include <vector>

class THaVar;
class TH1F;

class VDCeff : public THaPhysicsModule {
public:
  VDCeff( const char* name, const char* description );
  virtual ~VDCeff();

  virtual Int_t   Begin( THaRunBase* r=0 );
  virtual Int_t   End( THaRunBase* r=0 );
  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

  void            Reset( Option_t* opt="" );

protected:

  typedef std::vector<Long64_t> Vcnt_t;
  typedef const THaVar CVar_t;

  // Data needed for efficiency calculation for one VDC plane/wire spectrum
  struct VDCvar_t {
    TString  name;
    TString  histname;
    CVar_t*  pvar;
    Int_t    nwire;
    Vcnt_t   ncnt;
    Vcnt_t   nhit;
    TH1F*    hist_nhit;
    TH1F*    hist_eff;
    VDCvar_t( const char* nm, const char* hn, Int_t nw )
      : name(nm), histname(hn), pvar(0), nwire(nw), hist_nhit(0), hist_eff(0) {}
    ~VDCvar_t();
    void     Reset( Option_t* opt ="" );
  };

  typedef std::vector<Short_t>::iterator Vsiter_t;
  typedef std::vector<VDCvar_t>::iterator variter_t;

  // Internal working storage
  std::vector<VDCvar_t>  fVDCvar;
  std::vector<Short_t>   fWire;
  std::vector<bool>      fHitWire;

  Long64_t  fNevt;

  // Configuration parameters
  Int_t     fCycle;
  Double_t  fMaxOcc;

  virtual Int_t ReadDatabase( const TDatime& date );

  void WriteHist();

  ClassDef(VDCeff,0)   // VDC hit efficiency physics module
};

#endif
