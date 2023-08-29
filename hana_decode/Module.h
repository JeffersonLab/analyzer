#ifndef Podd_Module_h_
#define Podd_Module_h_

/////////////////////////////////////////////////////////////////////
//
//   Module
//   Abstract interface for a module that sits in a slot of a crate.
//
/////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "Decoder.h"
#include <set>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

namespace Decoder {

  class Module : public TNamed  {

  public:

    Module() : Module(0,0) {}   // for ROOT TClass & I/O
    Module(UInt_t crate, UInt_t slot);
    virtual ~Module() = default;

    class ModuleType {
    public:
      ModuleType ( const char *c1, UInt_t i1 )
	: fClassName(c1), fMapNum(i1), fTClass(nullptr) {}
      ModuleType() : fClassName(nullptr), fMapNum(0), fTClass(nullptr) {} // For ROOT RTTI
      bool operator<( const ModuleType& rhs ) const { return fMapNum < rhs.fMapNum; }
      const char*      fClassName;
      UInt_t           fMapNum;
      mutable TClass*  fTClass;
    };
    using TypeSet_t = std::set<ModuleType>;
    using TypeIter_t = TypeSet_t::iterator;
    static TypeSet_t& fgModuleTypes();
    static TypeIter_t DoRegister( const ModuleType& registration_info );

    Bool_t IsMultiBlockMode() const {return fMultiBlockMode; };
    Bool_t BlockIsDone() const { return fBlockIsDone; };
    virtual void SetFirmware(Int_t fw) {fFirmwareVers=fw;};

    UInt_t GetBlockSize() const { return block_size; };

    // inheriting classes need to implement one or more of these
    virtual UInt_t GetData( UInt_t /*chan*/) const { return 0; };
    virtual UInt_t GetData( UInt_t /*chan*/, UInt_t /*hit*/) const { return 0; };
    virtual UInt_t GetData( UInt_t, UInt_t, UInt_t) const { return 0; };
    virtual UInt_t GetData( Decoder::EModuleType /*emode*/,
                            UInt_t /*chan*/, UInt_t /*ievent*/) const { return 0; };
    virtual UInt_t GetData( Decoder::EModuleType,
                            UInt_t, UInt_t, UInt_t ) const { return 0; };
    virtual UInt_t GetOpt( UInt_t /* rdata */) const { return 0; };

    virtual UInt_t GetOpt( UInt_t /*chan*/, UInt_t /*hit*/) const { return 0; }; //1190

    virtual Int_t  Decode(const UInt_t *p) = 0; // FIXME: unused, remove from public interface
    // Loads slot data from [evbuffer,pstop]. pstop points to last word of data
    // This is used in decoding based on header words (roc_decode)
    virtual UInt_t LoadSlot( THaSlotData *sldat, const UInt_t *evbuffer,
                             const UInt_t *pstop ) = 0;
    // Loads slot data from [pos,pos+len). pos = start of data, len = num words
    // This is used in bank decoding (bank_decode)
    virtual UInt_t LoadSlot( THaSlotData* sldat, const UInt_t *evbuffer,
                             UInt_t pos, UInt_t len);
    virtual UInt_t LoadNextEvBuffer( THaSlotData* /* sldat */) { return 0; };

    virtual UInt_t GetNumChan()         const { return fNumChan; };

    virtual UInt_t GetNumEvents( Decoder::EModuleType /*emode*/,
                                 UInt_t /*chan*/ ) const { return 0; };
    virtual UInt_t GetNumEvents( UInt_t )  const { return 0; };
    virtual UInt_t GetNumEvents()          const { return 0; };
    virtual UInt_t GetNumSamples( UInt_t ) const { return 0; };
    virtual Int_t  GetMode()               const { return fMode; };

    virtual void   SetSlot( UInt_t crate, UInt_t slot, UInt_t header = 0,
                            UInt_t mask = 0, Int_t modelnum = 0 ) {
      fCrate      = crate;
      fSlot       = slot;
      fHeader     = header;
      fHeaderMask = mask;
      fModelNum   = modelnum;
    }

    virtual void   SetBank( Int_t bank ) { fBank = bank; };
    virtual void   SetMode( Int_t mode ) { fMode = mode; };

    virtual void   Init();
    virtual void   Init( const char* configstr );

    virtual void   Clear( Option_t* = "" );

    virtual Bool_t IsSlot( UInt_t rdata );

    virtual UInt_t GetCrate() const { return fCrate; };
    virtual UInt_t GetSlot()  const { return fSlot; };

    virtual void   SetDebugFile( std::ofstream* file )
    {
      if (file) fDebugFile = file;
    }

    virtual void SetHeader( UInt_t header, UInt_t mask ) {
      fHeader = header;
      fHeaderMask = mask;
    }

    virtual void DoPrint() const;

    virtual Bool_t IsMultiFunction() { return false; };
    virtual Bool_t HasCapability( Decoder::EModuleType ) { return false; };

    // Invalid input data exception, thrown by "Load*" functions
    class module_data_error : public std::runtime_error {
    public:
      explicit module_data_error( const std::string& what_arg )
        : std::runtime_error(what_arg) {}
      explicit module_data_error( const char* what_arg )
        : std::runtime_error(what_arg) {}
    };

    // Wrappers for LoadSlot methods to allow buffer preprocessing
    virtual UInt_t LoadBlock( THaSlotData* sldat, const UInt_t* evbuffer,
                              const UInt_t* pstop ) {
      return LoadSlot(sldat, evbuffer, pstop);
    }
    virtual UInt_t LoadBank( THaSlotData* sldat, const UInt_t* evbuffer,
                             UInt_t pos, UInt_t len ) {
      return LoadSlot(sldat, evbuffer, pos, len);
    }

  protected:

    std::vector<UInt_t> fData;  // Raw data

    UInt_t fCrate, fSlot;
    UInt_t fHeader, fHeaderMask;
    Int_t fBank;
    UInt_t fWordsExpect, fWordsSeen;
    UInt_t fWdcntMask, fWdcntShift;
    Int_t fModelNum;
    UInt_t fNumChan;
    Int_t fMode;
    UInt_t block_size;
    Bool_t IsInit;
    Bool_t fMultiBlockMode, fBlockIsDone;
    Int_t fFirmwareVers;

    Int_t fDebug;
    std::ofstream *fDebugFile;

    TObject* fExtra;  // additional member data, for binary compatibility

    struct ConfigStrReq {
      std::string name;  // (Optional) parameter name ("name=value")
      UInt_t&     data;  // Reference to variable to which to assign data
    };
    static void ParseConfigStr( const char* configstr,
                                const std::vector<ConfigStrReq>& req );

  private:

    ClassDef(Module,0)  // A module in a crate and slot

  };

}

#endif
