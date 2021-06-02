#ifndef Podd_BdataLoc_h_
#define Podd_BdataLoc_h_

//////////////////////////////////////////////////////////////////////////
//
// BdataLoc
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "TString.h"
#include <vector>
#include <cassert>
#include <set>

class THaEvData;
class TObjArray;

//___________________________________________________________________________
class BdataLoc : public TNamed {
  // Utility class used by THaDecData.
  // Data location, either in (crates, slots, channel), or
  // relative to a unique header in a crate or in an event.
public:
  // Helper class for holding info on BdataLoc classes
  class BdataLocType {
  public:
    BdataLocType( const char* cl, const char* key, Int_t np, void* ptr = nullptr )
      : fClassName(cl), fDBkey(key), fNparams(np), fOptptr(ptr), fTClass(nullptr) {}
    // The database keys have to be unique, so use them for sorting
    bool operator<( const BdataLocType& rhs ) const { return fDBkey < rhs.fDBkey; }

    const char*      fClassName; // Name of class to use for this data type
    const char*      fDBkey;     // Database key name to search for definitions
    Int_t            fNparams;   // Number of database parameters for this type
    void*            fOptptr;    // Optional pointer to arbitrary data
    mutable TClass*  fTClass;    // Pointer to ROOT class representing the type
  };
  typedef std::set<BdataLocType> TypeSet_t;
  typedef TypeSet_t::iterator TypeIter_t;
  // Returns set of all defined (i.e. compiled & loaded) BdataLoc classes
  static TypeSet_t& fgBdataLocTypes();

  BdataLoc() : crate(0), data(0) {}   // For ROOT TClass & I/O
  virtual ~BdataLoc();

  // Main function: extract the defined data from the event
  virtual void    Load( const THaEvData& evt ) = 0;

  // Initialization from TObjString parameters in TObjArray
  virtual Int_t   Configure( const TObjArray* params, Int_t start = 0 );
  // Type-specific data
  virtual Int_t   GetNparams() const = 0;
  virtual const char* GetTypeKey() const = 0;
  // Optional data passed in via generic pointer
  virtual Int_t   OptionPtr( void* ) { return 0; }

  virtual void    Clear( const Option_t* ="" )  { data = kMaxUInt; }
  virtual Bool_t  DidLoad() const               { return (data != kMaxUInt); }
  virtual UInt_t  NumHits() const               { return DidLoad() ? 1 : 0; }
  virtual UInt_t  Get( UInt_t i = 0 ) const     { assert(DidLoad() && i == 0); return data; }
  virtual void    Print( Option_t* opt="" ) const;
  //TODO: Needed?
  Bool_t operator==( const char* aname ) const  { return fName == aname; }
  // operator== and != compare the hardware definitions of two BdataLoc's
  // virtual Bool_t operator==( const BdataLoc& rhs ) const
  // { return (crate == rhs.crate); }
  // Bool_t operator!=( const BdataLoc& rhs ) const { return !(*this==rhs); }

  typedef THaAnalysisObject::EMode EMode;
  virtual Int_t   DefineVariables( EMode mode = THaAnalysisObject::kDefine );
   
  // Helper function for parameter parsing
  static TString& GetString( const TObjArray* params, Int_t pos )
  { return Podd::GetObjArrayString(params, pos); }

protected:
  // Abstract base class constructor
  BdataLoc( const char* name, UInt_t cra )
    : TNamed(name,name), crate(cra), data(kMaxUInt) { }

  UInt_t  crate;   // Data location: crate number
  UInt_t  data;    // Raw data word

  Int_t    CheckConfigureParams( const TObjArray* params, Int_t start ) const;
  void     PrintNameType( Option_t* opt="" ) const;

  static TypeIter_t DoRegister( const BdataLocType& registration_info );

  // Bit used by DefineVariables
  enum { kIsSetup = BIT(14) };

  ClassDef(BdataLoc,0)  
};

//___________________________________________________________________________
class CrateLoc : public BdataLoc {
public:
  // c'tor for (crate,slot,channel) selection
  CrateLoc( const char* nm, UInt_t cra, UInt_t slo, UInt_t cha )
    : BdataLoc(nm,cra), slot(slo), chan(cha) { ResetBit(kIsSetup); }
  CrateLoc() : slot(0), chan(0) {}
  virtual ~CrateLoc() = default;

  virtual void   Load( const THaEvData& evt );
  virtual Int_t  Configure( const TObjArray* params, Int_t start = 0 );
  virtual Int_t  GetNparams() const       { return fgThisType->fNparams; }
  virtual const char* GetTypeKey() const  { return fgThisType->fDBkey; };
  virtual void    Print( Option_t* opt="" ) const;

  // virtual Bool_t operator==( const BdataLoc& rhs ) const
  // { return (crate == rhs.crate && slot == rhs.slot && chan == rhs.chan); }

protected:
  UInt_t slot, chan;    // Data location: slot and channel

  void  PrintCrateLocHeader( Option_t* opt="" ) const;

private:
  static TypeIter_t fgThisType;

  ClassDef(CrateLoc,0)  
};

//___________________________________________________________________________
class CrateLocMulti : public CrateLoc {
public:
  // (crate,slot,channel) allowing for multiple hits per channel
  CrateLocMulti( const char* nm, UInt_t cra, UInt_t slo, UInt_t cha )
    : CrateLoc(nm,cra,slo,cha) { }
  CrateLocMulti() = default;
  virtual ~CrateLocMulti() = default;

  virtual void    Load( const THaEvData& evt );

  virtual void    Clear( const Option_t* ="" ) { CrateLoc::Clear(); rdata.clear(); }
  virtual UInt_t  NumHits() const              { return rdata.size(); }
  virtual UInt_t  Get( UInt_t i = 0 ) const    { return rdata.at(i); }
  virtual Int_t   GetNparams() const           { return fgThisType->fNparams; }
  virtual const char* GetTypeKey() const       { return fgThisType->fDBkey; };
  virtual void    Print( Option_t* opt="" ) const;

  virtual Int_t   DefineVariables( EMode mode = THaAnalysisObject::kDefine );

protected:
  std::vector<UInt_t> rdata;     // raw data

  void  PrintMultiData( Option_t* opt="" ) const;

private:
  static TypeIter_t fgThisType;

  ClassDef(CrateLocMulti,0)  
};

//___________________________________________________________________________
class WordLoc : public BdataLoc {
public:
  // c'tor for header search 
  WordLoc( const char* nm, UInt_t cra, UInt_t head, UInt_t skip )
    : BdataLoc(nm,cra), header(head), ntoskip(skip) { }
  WordLoc() : header(0), ntoskip(1) {}
  virtual ~WordLoc() = default;

  virtual void   Load( const THaEvData& evt );
  virtual Int_t  Configure( const TObjArray* params, Int_t start = 0 );
  virtual Int_t  GetNparams() const       { return fgThisType->fNparams; }
  virtual const char* GetTypeKey() const  { return fgThisType->fDBkey; };
  virtual void    Print( Option_t* opt="" ) const;

  // virtual Bool_t operator==( const BdataLoc& rhs ) const
  // { return (crate == rhs.crate &&
  // 	    header == rhs.header && ntoskip == rhs.ntoskip); }

protected:
  UInt_t header;              // header (unique either in data or in crate)
  UInt_t ntoskip;             // how far to skip beyond header
   
private:
  static TypeIter_t fgThisType;

  ClassDef(WordLoc,0)  
};

//___________________________________________________________________________
class RoclenLoc : public BdataLoc {
public:
  // Event length of a crate
  RoclenLoc( const char* nm, UInt_t cra ) : BdataLoc(nm, cra) { }
  RoclenLoc() = default;
  virtual ~RoclenLoc() = default;

  virtual void   Load( const THaEvData& evt );
  virtual Int_t  GetNparams() const       { return fgThisType->fNparams; }
  virtual const char* GetTypeKey() const  { return fgThisType->fDBkey; };

private:
  static TypeIter_t fgThisType;

  ClassDef(RoclenLoc,0)  
};

///////////////////////////////////////////////////////////////////////////////

#endif
