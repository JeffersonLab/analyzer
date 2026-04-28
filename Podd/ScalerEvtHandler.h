#ifndef Podd_ScalerEvtHandler_h_
#define Podd_ScalerEvtHandler_h_

/////////////////////////////////////////////////////////////////////
//
//   ScalerEvtHandler
//   Class to handle scaler data
//   author  Robert Michaels (rom@jlab.org) Sep 2014
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"  // for THaEvtTypeHandler
#include "GenScaler.h"          // for GenScaler
#include "Rtypes.h"             // for Rtypes, ClassDefOverride
#include <memory>               // for unique_ptr
#include <string>               // for string, operator+
#include <vector>               // for vector

namespace Decoder { class CodaDecoder; }
class TBranch;
class THaEvData;
class THaVar;
class TTree;

namespace Podd {

class ScalerEvtHandler : public THaEvtTypeHandler {

public:
  ScalerEvtHandler( const char* name, const char* description );
  ScalerEvtHandler( const ScalerEvtHandler& ) = delete;
  ScalerEvtHandler( const ScalerEvtHandler&& ) = delete;
  ScalerEvtHandler& operator=( const ScalerEvtHandler& ) = delete;
  ScalerEvtHandler& operator=( ScalerEvtHandler&& ) = delete;
  ~ScalerEvtHandler() override;

  Int_t   Analyze( THaEvData* evdata ) override;
  Int_t   Begin( THaRunBase* r = nullptr ) override;
  void    Clear( Option_t* = "" ) override;
  Int_t   End( THaRunBase* r = nullptr ) override;
  EStatus Init( const TDatime& run_time ) override;

  // Put array index arrays into a separate tree for smaller file size.
  // The space savings will be minor if ROOT file compression is enabled,
  // but throughput may improve due to fewer branches to be written/read.
  void    EnableIndexTree( bool set = true ) { SetBit(kDoIndexTree, set); }

  enum EKind : Byte_t { kCount, kRate };
  enum EPick : Byte_t { kSlot, kCrate, kAll };

protected:
  struct Variable {
    Variable( const std::string& nm, const std::string& desc,
               UInt_t cr, UInt_t sl, EKind kind )
      : name(nm + KindName(kind))
      , description(desc + KindDesc(kind))
      , var(nullptr)
      , icrate(cr)
      , islot(sl)
      , ikind(kind)
    {}
    std::string name;        // Full variable name in tree
    std::string description; // Global variable description
    THaVar*     var;         // Analysis variable attached to these data
    UShort_t    icrate;
    Byte_t      islot;
    EKind       ikind;       // Data type: raw count or rate
  private:
    static std::string KindName(EKind kind);
    static std::string KindDesc(EKind kind);
  };
  struct ScalarVariable : Variable {
    ScalarVariable( const std::string& nm, const std::string& desc,
                    UInt_t cr, UInt_t sl, UInt_t ch, EKind kind )
      : Variable(nm, desc, cr, sl, kind )
      , index(0)
      , ichan(ch)
      , count(0)
    {}
    void Fill( const Decoder::GenScaler* scaler );
    // Check if this variable is associated with the given scaler
    bool Match( const Decoder::GenScaler* scaler ) const
    { return icrate == scaler->GetCrate() && islot == scaler->GetSlot(); }

    UInt_t     index;  // Index of scaler module to use
    UShort_t   ichan;  // Scaler channel with the data
    union {
      UInt_t   count;  // Scaler data
      Float_t  rate;
    };
  };
  struct ArrayVariable : Variable {
    ArrayVariable( const std::string& nm, const std::string& desc,
                 UInt_t cr, UInt_t sl, EKind kind, Int_t bank, EPick pick );
    ArrayVariable( const ArrayVariable& rhs ) = delete; // not needed
    ArrayVariable( ArrayVariable&& rhs ) noexcept;
    ArrayVariable& operator=( const ArrayVariable& rhs ) = delete;
    ArrayVariable& operator=( ArrayVariable&& other ) noexcept;
    ~ArrayVariable() { Deallocate();  }
    void   Allocate( UInt_t numchan );
    UInt_t Fill( UInt_t pos, const Decoder::GenScaler* scaler ) const;
    UInt_t FillIndices( UInt_t pos, const Decoder::GenScaler* scaler ) const;
    bool   MatchesAll() const { return ipick == kAll && ibank <= 0; }
    bool   Match( const Decoder::GenScaler* scaler ) const;

    UInt_t     size;   // Number of array elements
    Int_t      ibank;  // Bank number to select (-1: ignore)
    EPick      ipick;  // What to pick (entire slot, entire crate, all crates)
    std::vector<UInt_t>  idxlist; // scaler modules to use
    std::vector<THaVar*> idxvars; // variables for chan/slot/crate indices
    union {
      UInt_t*  pCount; // [size] Data: Raw memory blocks treated as fixed-size
      Float_t* pRate;  // arrays, ROOT's favorite data structure for such things
    };
    Byte_t*    pChan;  // [size] Channel number of each data word
    Byte_t*    pSlot;  // [size] Slot number of each data word
    UShort_t*  pCrate; // [size] Crate number of each data word
  private:
    void Deallocate();
  };

  struct ClockDef {
    Int_t  iref;
    UInt_t icrate;
    UInt_t islot;
    UInt_t ichan;
    Double_t freq;
  };

  // Status bits
  enum EConfigFlags { kDoIndexTree = BIT(16) }; // NOLINT(*-enum-size)

  std::vector<std::unique_ptr<Decoder::GenScaler>> fScalers;
  std::vector<ClockDef> fClocks;
  std::vector<ScalarVariable> fScalarVars;
  std::vector<ArrayVariable> fArrayVars;
  ULong64_t fEvtCount;
  ULong64_t fEvtNum;       // last seen physics event number
  TTree*    fScalerTree;   // Scaler events. ROOT will own the tree
  TTree*    fIndexTree;    // If arrays defined, variable indices may go here
  Double_t  fDeltaT;       // approximate time between scaler readings

  virtual Int_t Decode( THaEvData* evdata );
  virtual Int_t DecodeBank( Decoder::CodaDecoder* codaevent, Decoder::GenScaler* scaler );
  virtual Int_t DecodeRoc( THaEvData* evdata, Decoder::GenScaler* scaler );

  Int_t   ReadDatabase( const TDatime& date ) override;
  Int_t   DefineVariables( EMode mode = kDefine ) override;

  EStatus AssignNormScaler() const;
  EStatus MakeGlobalVars();
  EStatus UnmakeGlobalVars();

  decltype(fScalers)::const_iterator FindScaler(UInt_t icrate, UInt_t islot) const;
  void    ParseMap( const std::vector<std::string>& words );
  void    ParseClock( const std::vector<std::string>& words );
  void    ParseVariable( const std::vector<std::string>& words );
  void    SetIndices();

  static TBranch* MakeBranch( const std::string& name, const THaVar* var,
                              TTree* tree );
  TBranch* MakeIndexBranch( const THaVar* var ) const;

  ClassDefOverride(ScalerEvtHandler,0)  // Scaler Event handler
};

} // namespace Podd

#endif
