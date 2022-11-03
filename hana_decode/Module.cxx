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
void Module::Init()
{
// Suggestion: call this Init() before calling the inheriting class's Init.
// Otherwise some variables may be undefined.  The "factory" method
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
void Module::Clear( Option_t* )
{
  // Clear event-by-event data.
  // This is called for each event block read from file, but not for each
  // individual event in a multi-event block.

  fWordsSeen = 0;
  block_size = 1;
  fMultiBlockMode = false;
  fBlockIsDone = false;
}

//_____________________________________________________________________________
static void StoreValue( const string& item, UInt_t& data )
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
    auto pos = item.find('=');
    if( pos != string::npos ) {
      if( pos > 0 )
        name = item.substr(0,pos);
      item.erase(0,pos+1);
    }
    if( name.empty() ) {
      // Unnamed: use item position to assign to corresponding destination
      StoreValue( item, req[idx].data );
    } else {
      // Named: find and assign to corresponding destination
      auto it = find_if(ALL(req), [&name]( const ConfigStrReq& r ) { return name == r.name; });
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
Bool_t Module::IsSlot(UInt_t rdata) {
// Simplest version of IsSlot relies on a unique header.
 if ((rdata & fHeaderMask)==fHeader) {
    fWordsExpect = (rdata & fWdcntMask)>>fWdcntShift;
    return true;
  }
  return false;
}

//_____________________________________________________________________________
void Module::DoPrint() const {
  if (fDebugFile) {
    *fDebugFile << "Module   name = "<<fName<<endl;
    *fDebugFile << "Module::    Crate  "<<fCrate<<"     slot "<<fSlot<<endl;
    *fDebugFile << "Module::    fWdcntMask "<<hex<<fWdcntMask<<dec<<endl;
    *fDebugFile << "Module::    fHeader "<<hex<<fHeader<<endl;
    *fDebugFile << "Module::    fHeaderMask "<<hex<<fHeaderMask<<dec<<endl;
    *fDebugFile << "Module::    num chan "<<fNumChan<<endl;
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
Module::TypeIter_t Module::DoRegister( const ModuleType& info )
{
  // Add given info in fgModuleTypes

  if( !info.fClassName ||!*info.fClassName ) {
    ::Error( "Module::DoRegister", "Attempt to register empty class name. "
	     "Coding error. Call expert." );
    return fgModuleTypes().end();
  }

  pair< TypeIter_t, bool > ins = fgModuleTypes().insert(info);

  if( !ins.second ) {
    ::Error( "Module::DoRegister", "Attempt to register duplicate decoder module "
	     "class \"%s\". Coding error. Call expert.", info.fClassName );
    return fgModuleTypes().end();
  }
  // NB: std::set guarantees that iterators remain valid on further insertions,
  // so this return value will remain good, unlike, e.g., std::vector iterators.
  return ins.first;
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
  if( len == 0 ) return 0;
  return LoadSlot(sldat, evbuffer+pos, evbuffer+pos+len-1);
}

} // namespace Decoder

ClassImp(Decoder::Module)
