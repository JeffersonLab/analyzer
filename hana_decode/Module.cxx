////////////////////////////////////////////////////////////////////
//
//   Module
//
//   Abstract class to help decode the module.
//   No module data is stored here; instead it is in THaSlotData
//   arrays, as it was traditionally.
//
/////////////////////////////////////////////////////////////////////

#include "Module.h"
#include "Textvars.h"  // for Podd::Tokenize
#include "Helper.h"
#include "TError.h"
#include "TString.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cassert>

using namespace std;

namespace Decoder {

//_____________________________________________________________________________
Module::Module(UInt_t crate, UInt_t slot)
  : fCrate{crate}
  , fSlot{slot}
  , fHeader{0}
  , fHeaderMask{0xffffffff}
  , fBank{-1}
  , fWordsExpect{0}
  , fWordsSeen{0}
  , fWdcntMask{0}
  , fWdcntShift{0}
  , fModelNum{-1}
  , fNumChan{0}
  , fMode{0}
  , block_size{1}
  , IsInit{false}
  , fMultiBlockMode{false}
  , fBlockIsDone{false}
  , fFirmwareVers{0}
  , fDebug{0}
  , fDebugFile{nullptr}
  , fExtra{nullptr}
{
  // Warning: see comments at Init()
}

//_____________________________________________________________________________
Module::~Module()
{
  delete fExtra; fExtra = nullptr;
}

//_____________________________________________________________________________
void Module::Init()
{
// Suggestion: call this Init() before calling the inheriting class's Init.
// Otherwise, some variables may be undefined.  The "factory" method
// using TClass::New does NOT call the c'tor of this base class !!
// Make sure all your variables are defined.
  fHeader=0;
  fHeaderMask=0xffffffff;
  fBank=-1;
  fWordsSeen = 0;
  fWordsExpect = 0;
  fWdcntMask=0;
  fWdcntShift=0;
  fDebug = 0;
  fDebugFile=nullptr;
  fModelNum = -1;
  fName = "";
  fNumChan = 0;
}

//_____________________________________________________________________________
void Module::Clear( Option_t* opt )
{
  // Clear event-by-event data.
  // This is called for each event block read from file, but not for each
  // individual event in a multi-event block.

  if( TString sopt(opt); !sopt.Contains("E")) {
    fWordsSeen = 0;
    block_size = 1;
    fMultiBlockMode = false;
    fBlockIsDone = false;
  }
}

//_____________________________________________________________________________
namespace {
void StoreValue( const string& item, UInt_t& data )
{
  // Convert 'item' to unsigned int and store the result in 'data'
  size_t len = 0;
  unsigned long val = stoul(item, &len, 0);
  if( val > kMaxUInt ) {
    ostringstream ostr;
    ostr << "Value " << val << " out of range. Must be <= " << kMaxUInt << ". "
         << "Fix the cratemap.";
    throw std::out_of_range(ostr.str());
  }
  data = val;
}
} // namespace

//_____________________________________________________________________________
void Module::ParseConfigStr( const char* configstr,
                             const vector<ConfigStrReq>& req )
{
  // Helper function for parsing a module configuration string

  if( !configstr || !*configstr || req.empty() )
    // Nothing to do
    return;

  size_t idx = 0;
  vector<string> items;
  // Use commas and/or whitespace to separate fields in the configuration string
  Podd::Tokenize(configstr, ", \t\n\v\f\r", items);
  for( auto item : items ) {
    // Too many parameters?
    if( idx >= req.size() ) {
      ostringstream ostr;
      ostr << "Too many configuration parameters in string \"" << configstr
           << "\". Maximum number is " << req.size() << ". Fix the cratemap.";
      throw invalid_argument(ostr.str());
    }
    // Do we have a named parameter?
    string name;
    if( auto pos = item.find('='); pos != string::npos ) {
      if( pos > 0 )
        name = item.substr(0,pos);
      item.erase(0,pos+1);
    }
    if( name.empty() ) {
      // Unnamed: use item position to assign to corresponding destination
      StoreValue( item, req[idx].data );
    } else {
      // Named: find and assign to corresponding destination
      auto it = ranges::find_if(
        req, [&name]( const ConfigStrReq& r ) { return name == r.name; });
      if( it != req.end() ) {
        StoreValue( item, it->data );
      } else {
        // Unknown parameter name in configstr. Probably some confusion.
        // Make the user think about it.
        ostringstream ostr;
        ostr << "Unsupported parameter name \"" << name << "\". "
             << "Fix the cratemap.";
        throw invalid_argument(ostr.str());
      }
    }
    ++idx;
  }
}

//_____________________________________________________________________________
void Module::Init( const char* /*configstr*/ )
{
  // Initialize using optional configuration string defined in the crate map.
  // This is the method actually called from THaSlotData::loadModule. By
  // default, it simply calls the standard Init() method, which is kept around
  // for backward compatibility. Derived classes wanting to use configuration
  // strings to set extra parameters must override this method.

  Init();
}


//_____________________________________________________________________________
Bool_t Module::IsSlot( UInt_t rdata )
{
  // Simplest version of IsSlot relies on a unique header.
  if( (rdata & fHeaderMask) == fHeader ) {
    fWordsExpect = (rdata & fWdcntMask) >> fWdcntShift;
    return true;
  }
  return false;
}

//_____________________________________________________________________________
void Module::DoPrint() const
{
  if( fDebugFile ) {
    *fDebugFile << "Module   name = " << fName << endl;
    *fDebugFile << "Module::    Crate  " << fCrate << "     slot " << fSlot << endl;
    *fDebugFile << "Module::    fWdcntMask " << hex << fWdcntMask << dec << endl;
    *fDebugFile << "Module::    fHeader " << hex << fHeader << endl;
    *fDebugFile << "Module::    fHeaderMask " << hex << fHeaderMask << dec << endl;
    *fDebugFile << "Module::    num chan " << fNumChan << endl;
  }
}

//_____________________________________________________________________________
Module::TypeSet_t& Module::fgModuleTypes()
{
  // Local storage for all defined Module types. Initialize here on first use
  // (cf. http://www.parashift.com/c++-faq/static-init-order-on-first-use-members.html)

  static auto *fgModuleTypes = new TypeSet_t;

  return *fgModuleTypes;
}

//_____________________________________________________________________________
Module::TypeIter_t Module::DoRegister( const ModuleType& registration_info )
{
  // Add given info in fgModuleTypes

  if( !registration_info.fClassName || !*registration_info.fClassName ) {
    ::Error( "Module::DoRegister", "Attempt to register empty class name. "
	     "Coding error. Call expert." );
    return fgModuleTypes().end();
  }

  auto [elem, success] = fgModuleTypes().insert(registration_info);

  if( !success ) {
    ::Error( "Module::DoRegister", "Attempt to register duplicate decoder module "
	     "class \"%s\". Coding error. Call expert.", registration_info.fClassName );
    return fgModuleTypes().end();
  }
  // NB: std::set guarantees that iterators remain valid on further insertions,
  // so this return value will remain good, unlike, e.g., std::vector iterators.
  return elem;
}

//_____________________________________________________________________________
UInt_t Module::LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer,
                         UInt_t pos, UInt_t len)
{
  // Load slot from [pos, pos+len). pos+len-1 is the last word of data

  // Basic example of how this could be done
  // if (fDebugFile) {
  //      *fDebugFile << "Module:: Loadslot "<<endl;
  //      *fDebugFile << "pos"<<dec<<pos<<"   len "<<len<<endl;
  // }
  // fWordsSeen=0;
  // while ( fWordsSeen<len ) {
  //   if (fDebugFile) *fDebugFile <<endl;
  //   for (size_t ichan = 0, nchan = GetNumChan(); ichan < nchan; ichan++) {
  //     Int_t mdata,rdata;
  //     rdata = evbuffer[pos+fWordsSeen];
  //     mdata = rdata;
  //     sldat->loadData(ichan, mdata, rdata);
  //     if (ichan < fData.size()) fData[ichan]=rdata;
  //     fWordsSeen++;
  //   }
  // }
  // return fWordsSeen;

  // By default, this is just a wrapper for the 3-parameter version of LoadSlot,
  // which every module is required to implement

  assert(len > 0);
#ifdef NDEBUG
  if( len == 0 ) return 0;
#endif
  return LoadSlot(sldat, evbuffer+pos, evbuffer+pos+len-1);
}

//_____________________________________________________________________________
// Lookup table of DAQ frontend module parameters. For backwards compatibility
// only. Modules should set these values when adding themselves to the module
// registry (call to DoRegister).

class ModuleParam_t {
public:
  Int_t       model; // Model identifier
  ECrateCode  bus;   // Bus type (Fastbus, VME), for crosscheck with crate map
  EModuleType type;  // Module type
  UShort_t    nchan; // Number of input channels
};

using enum EModuleType;
using enum ECrateCode;

static constexpr array<ModuleParam_t, 26> module_list = {{
  { .model=250,  .bus=kVME,     .type=kMultiFunctionADC,  .nchan=16   }, // FADC 250
  { .model=514,  .bus=kVME                                            }, // VTP
  { .model=526,  .bus=kVME,     .type=kMultiFunctionTDC,  .nchan=128  }, // VETROC
  { .model=527,  .bus=kVME,     .type=kADC,               .nchan=256  }, // vfTDC
  { .model=550,  .bus=kVME,     .type=kADC,               .nchan=2    }, // CAEN 550 ADC sequencer readout (RICH)
  { .model=560,  .bus=kVME,     .type=kScaler,            .nchan=16   }, // CAEN 560 scaler
  { .model=767,  .bus=kVME,     .type=kCommonStartTDC,    .nchan=128  }, // CAEN 767 multihit TDC
  { .model=775,  .bus=kVME,     .type=kCommonStopTDC,     .nchan=32   }, // CAEN V775 TDC
  { .model=792,  .bus=kVME,     .type=kADC,               .nchan=32   }, // CAEN V792 QDC
  { .model=1151, .bus=kVME,     .type=kScaler,            .nchan=16   }, // LeCroy 1151 scaler
  { .model=1182, .bus=kVME,     .type=kADC,               .nchan=8    }, // LeCroy 1182 QDC
  { .model=1190, .bus=kVME,     .type=kCommonStopTDC,     .nchan=128  }, // CAEN 1190A
  { .model=1872, .bus=kFastbus, .type=kCommonStartTDC,    .nchan=64   }, // High-res ToF TDC
  { .model=1875, .bus=kFastbus, .type=kCommonStartTDC,    .nchan=64   }, // High-res Detector TDC
  { .model=1877, .bus=kFastbus, .type=kCommonStopTDC,     .nchan=96   }, // Multi-hit wire-chamber TDC
  { .model=1881, .bus=kFastbus, .type=kADC,               .nchan=64   }, // Detector ADC
  { .model=3123, .bus=kVME,     .type=kADC,               .nchan=16   }, // VMIC 3123 ADC
  { .model=3201, .bus=kVME,     .type=kMultiFunctionTDC,  .nchan=32   }, // JLab F1 TDC high resolution
  { .model=3204, .bus=kVME,     .type=kMultiFunctionTDC,  .nchan=32   }, // F1TDC high res
  { .model=3560, .bus=kVME                                            }, // MPD module VMEv4
  { .model=3561, .bus=kVME                                            }, // MPD module
  { .model=3800, .bus=kVME,     .type=kScaler,            .nchan=32   }, // Struck 3800 scaler
  { .model=3801, .bus=kVME,     .type=kScaler,            .nchan=32   }, // Struck 3801 scaler
  { .model=4450, .bus=kVME                                            }, // HCAL LED module
  { .model=6401, .bus=kVME,     .type=kMultiFunctionTDC,  .nchan=64   }, // JLab F1 TDC normal resolution
  { .model=7510, .bus=kVME,     .type=kADC,               .nchan=8    }  // Struck 7510 (33xx?) Flash ADC (multihit)
}};

//_____________________________________________________________________________
#ifdef PODD_ENABLE_TESTS
Module::ModuleType Module::GetModuleType( Int_t ID )
{
  ModuleType mtyp;
  const auto* item =
    ranges::find_if(module_list,
      [ID]( const auto& mod ) { return ID == mod.model; });
  if( item != module_list.end() ) {
    mtyp.fModel = item->model;
    mtyp.fBus   = item->bus;
    mtyp.fType  = item->type;
    mtyp.fNchan = item->nchan;
  }
  return mtyp;
}
#endif

// struct ModelPar_t { UInt_t model, nchan, ndata; };
// static constexpr array<ModelPar_t, 18> modelpar = {{
//   { 250,  16,  20000 },
//   { 550,  512, 1024 },
//   { 560,  16,   16 },
//   { 767,  128, 1024 },
//   { 775,  32,  32 },
//   { 792,  32,  32 },
//   { 1151, 16,  16 },
//   { 1182, 8,   128 },
//   { 1190, 128, 1024 },
//   { 1875, 64,  512 },
//   { 1877, 96,  672 },
//   { 1881, 64,  64 },
//   { 3123, 16,  16 },
//   { 3201, 32,  512 },
//   { 3800, 32,  32 },
//   { 3801, 32,  32 },
//   { 6401, 64,  1024 },
//   { 7510, 8,   1024 },
// }};

//_____________________________________________________________________________
Module::ModuleType::ModuleType( const char* class_name, Int_t ID )
  : ModuleType(class_name, ID, kUnknown, kUndefined, 0)
{}

//_____________________________________________________________________________
Module::ModuleType::ModuleType( const char* class_name, Int_t ID,
  ECrateCode bus, EModuleType type, UShort_t nchan )
  : fClassName(class_name)
  , fModel(ID)
  , fBus(bus)
  , fType(type)
  , fNchan(nchan)
  , fTClass(nullptr)
{
  if( fType == kUndefined || fBus == kUnknown || fNchan == 0 ) {
    const auto* item =
      ranges::find_if(module_list,
        [ID]( const auto& mod ) { return ID == mod.model; });
    if( item != module_list.end() ) {
      if( fType == kUndefined ) fType = item->type;
      if( fBus == kUnknown ) fBus = item->bus;
      if( fNchan == 0 ) fNchan = item->nchan;
    }
  }
}

//_____________________________________________________________________________
void Module::ModuleType::Print() const
{
  // This must match the order of EModuleType in Decoder.h
  constexpr const char* kModuleTypeName[] =
  { "undefined", "ADC", "CommonStopTDC", "CommonStartTDC",
    "MultiFunctionADC", "MultiFunctionTDC" };
  cout << fClassName
    << "\t" << fModel
    << "\t" << kModuleTypeName[static_cast<int>(fType)]
    << "\t" << fNchan
    << endl;
}

} // namespace Decoder

ClassImp(Decoder::Module)
