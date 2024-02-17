#ifndef Podd_VDCeff_h_
#define Podd_VDCeff_h_

//////////////////////////////////////////////////////////////////////////
//
// VDCeff - VDC efficiency calculation
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include <vector>
#include <string_view> // for std::swap (since C++17)

class THaVar;
class TH1F;

class VDCeff : public THaPhysicsModule {
public:
  VDCeff( const char* name, const char* description );
  ~VDCeff() override;

  Int_t   Begin( THaRunBase* r=nullptr ) override;
  Int_t   End( THaRunBase* r=nullptr ) override;
  EStatus Init( const TDatime& run_time ) override;
  Int_t   Process( const THaEvData& ) override;

  void    Reset( Option_t* opt="" );

protected:

  typedef std::vector<Long64_t> Vcnt_t;
  typedef const THaVar CVar_t;

  // Data needed for efficiency calculation for one VDC plane/wire spectrum
  class VDCvar_t {
  public:
    VDCvar_t( const char* nm, const char* hn, Int_t nw )
      : name(nm), histname(hn), pvar(nullptr), nwire(nw), hist_nhit(nullptr),
        hist_eff(nullptr) {}
    VDCvar_t() : pvar{nullptr}, nwire{0}, hist_nhit{nullptr}, hist_eff{nullptr} {}
    // Copying is unwise because of the TH1F members. Don't want duplicate names.
    VDCvar_t( const VDCvar_t& src ) = delete;
    VDCvar_t& operator=( const VDCvar_t& rhs ) = delete;
    // Moving is unproblematic. Needed for storing these objects by value in a vector
    VDCvar_t( VDCvar_t&& src ) noexcept : VDCvar_t() { swap( *this, src); }
    VDCvar_t& operator=( VDCvar_t&& rhs ) noexcept { swap( *this, rhs); return *this; }
    friend void swap( VDCvar_t& a, VDCvar_t& b ) noexcept {
      using std::swap;
      swap(a.name, b.name);
      swap(a.histname, b.histname);
      swap(a.pvar, b.pvar);
      swap(a.nwire, b.nwire);
      swap(a.ncnt, b.ncnt);
      swap(a.nhit, b.nhit);
      swap(a.hist_nhit, b.hist_nhit);
      swap(a.hist_eff, b.hist_eff);
    }
    ~VDCvar_t();
    void     Reset( Option_t* opt ="" );
    TString  name;
    TString  histname;
    CVar_t*  pvar;
    Int_t    nwire;
    Vcnt_t   ncnt;
    Vcnt_t   nhit;
    TH1F*    hist_nhit;
    TH1F*    hist_eff;
  };

  // Internal working storage
  std::vector<VDCvar_t>  fVDCvar;
  std::vector<Short_t>   fWire;
  std::vector<bool>      fHitWire;

  Long64_t  fNevt;

  // Configuration parameters
  Int_t     fCycle;
  Double_t  fMaxOcc;

  Int_t ReadDatabase( const TDatime& date ) override;

  void WriteHist();

  ClassDefOverride(VDCeff,0)   // VDC hit efficiency physics module
};

#endif
