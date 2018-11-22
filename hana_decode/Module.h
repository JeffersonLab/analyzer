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

namespace Decoder {

  class Module : public TNamed  {

  public:

    Module();   // for ROOT TClass & I/O
    Module(Int_t crate, Int_t slot);
    virtual ~Module();

    struct ModuleType {
    public:
      ModuleType ( const char *c1, Int_t i1 )
	: fClassName(c1), fMapNum(i1), fTClass(0) {}
      ModuleType() : fClassName(0), fMapNum(0), fTClass(0) {} // For ROOT RTTI
      bool operator<( const ModuleType& rhs ) const { return fMapNum < rhs.fMapNum; }
      const char*      fClassName;
      Int_t            fMapNum;
      mutable TClass*  fTClass;
    };
    typedef std::set<ModuleType> TypeSet_t;
    typedef TypeSet_t::iterator TypeIter_t;
    static TypeSet_t& fgModuleTypes();
    static TypeIter_t DoRegister( const ModuleType& registration_info );

    Bool_t IsMultiBlockMode() {return fMultiBlockMode; };
    Bool_t BlockIsDone() { return fBlockIsDone; };
    virtual void SetFirmware(Int_t fw) {fFirmwareVers=fw;};

    Int_t GetBlockSize() { return block_size; };

    // inheriting classes need to implement one or more of these
    virtual UInt_t GetData(Int_t) const { return 0; };
    virtual Int_t GetData(Int_t, Int_t) const { return 0; };
    virtual Int_t GetData(Int_t, Int_t, Int_t) const { return 0; };
    virtual Int_t GetData(Decoder::EModuleType,
			  Int_t, Int_t ) const { return 0; };
    virtual Int_t GetData(Decoder::EModuleType,
			  Int_t, Int_t, Int_t) const {return 0;};
    virtual Int_t GetOpt(UInt_t /* rdata */) { return 0; };

    virtual Int_t Decode(const UInt_t *p) = 0; // implement in derived class
    // Loads slot data
    virtual Int_t LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer,
			   const UInt_t *pstop ) = 0;
    // Loads slot data from pos to pos+len
    virtual Int_t LoadSlot(THaSlotData* sldat, const UInt_t *evbuffer,
			   Int_t pos, Int_t len);
    virtual Int_t LoadNextEvBuffer(THaSlotData* /* sldat */) { return 0; };

    virtual Int_t GetNumChan()         const { return fNumChan; };

    virtual Int_t GetNumEvents(Decoder::EModuleType, Int_t) const { return 0; };
    virtual Int_t GetNumEvents(Int_t)  const { return 0; };
    virtual Int_t GetNumEvents()       const { return 0; };
    virtual Int_t GetNumSamples(Int_t) const { return 0; };
    virtual Int_t GetMode()            const { return fMode; };

    virtual void SetSlot(Int_t crate, Int_t slot, UInt_t header=0,
			 UInt_t mask=0, Int_t modelnum=0)
    {
      fCrate      = crate;
      fSlot       = slot;
      fHeader     = header;
      fHeaderMask = mask;
      fModelNum   = modelnum;
    }

    virtual void SetBank(Int_t bank) { fBank = bank; };
    virtual void SetMode(Int_t mode) { fMode = mode; };

    virtual void Init();

    virtual void Clear(Option_t* = "") { fWordsSeen = 0; };

    virtual Bool_t IsSlot(UInt_t rdata);

    virtual Int_t GetCrate() const { return fCrate; };
    virtual Int_t GetSlot()  const { return fSlot; };

    virtual void SetDebugFile(std::ofstream *file)
    {
      if (file!=0) fDebugFile = file;
    }

    virtual void SetHeader(UInt_t header, UInt_t mask) {
      fHeader = header;
      fHeaderMask = mask;
    }

    virtual void DoPrint() const;

    virtual Bool_t IsMultiFunction() { return kFALSE; };
    virtual Bool_t HasCapability(Decoder::EModuleType) { return kFALSE; };

  protected:

    std::vector<Int_t> fData;  // Raw data

    Int_t fCrate, fSlot;
    UInt_t fHeader, fHeaderMask;
    Int_t fBank;
    Int_t fWordsExpect, fWordsSeen;
    Int_t fWdcntMask, fWdcntShift;
    Int_t fModelNum, fNumChan, fMode;
    Int_t block_size;
    Bool_t IsInit;
    Bool_t fMultiBlockMode, fBlockIsDone;
    Int_t fFirmwareVers;

    Int_t fDebug;
    std::ofstream *fDebugFile;

    TObject* fExtra;  // additional member data, for binary compatibility

  private:

    ClassDef(Module,0)  // A module in a crate and slot

  };

}

#endif
