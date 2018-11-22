//*-- Author :    Bob Michaels,  March 2002

//////////////////////////////////////////////////////////////////////////
//
// BdataLoc, CrateLoc, WordLoc
//
// Utility classes for THaDecData generic raw data decoder
//
//////////////////////////////////////////////////////////////////////////

#include "BdataLoc.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "THaVarList.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TError.h"
#include "TClass.h"

#include <cstring>   // for memchr
#include <cstdlib>   // for strtoul
#include <errno.h>
#include <utility>
#include <iostream>

using namespace std;

typedef BdataLoc::TypeSet_t  TypeSet_t;
typedef BdataLoc::TypeIter_t TypeIter_t;

TypeIter_t CrateLoc::fgThisType      = DoRegister( BdataLocType( "CrateLoc",      "crate",  4 ));
TypeIter_t CrateLocMulti::fgThisType = DoRegister( BdataLocType( "CrateLocMulti", "multi",  4 ));
TypeIter_t WordLoc::fgThisType       = DoRegister( BdataLocType( "WordLoc",       "word",   4 ));
TypeIter_t RoclenLoc::fgThisType     = DoRegister( BdataLocType( "RoclenLoc",     "roclen", 2 ));

// Shorthands
#define kDefine    THaAnalysisObject::kDefine
#define kDelete    THaAnalysisObject::kDelete
#define kOK        THaAnalysisObject::kOK
#define kInitError THaAnalysisObject::kInitError

//_____________________________________________________________________________
BdataLoc::~BdataLoc()
{
  // Destructor - clean up global variable(s)

  // The following call will always invoke the base class instance 
  // BdataLoc::DefineVariables, even when destroying derived class
  // objects. But that's OK since the kDelete mode is the same for 
  // the entire class hierarchy; it simply removes the name.

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
Int_t BdataLoc::DefineVariables( EMode mode )
{
  // Export this object's data as a global variable

  // Note that the apparatus prefix is already part of this object's name,
  // e.g. "D.syncroc1", where "D" is the name of the parent THaDecData object

  if( mode == kDefine && TestBit(kIsSetup) ) return kOK;
  SetBit( kIsSetup, mode == kDefine );

  Int_t ret = kOK;
  if( mode == kDefine ) {
    if( !gHaVars->Define( GetName(), data ) )
      ret = kInitError;
  } else 
    gHaVars->RemoveName( GetName() );

  return ret;
}

//_____________________________________________________________________________
Int_t BdataLoc::CheckConfigureParams( const TObjArray* params, Int_t start )
{
  // Check given parameters of call to Configure for obvious errors.
  // Internal helper function.

  // Invalid call parameter?
  if( !params || start < 0 || start + GetNparams() > params->GetLast()+1 )
    return 1;

  // Invalid parameter array data?
  for( Int_t ip = start; ip < start + GetNparams(); ++ip ) {
    if( params->At(ip)->IsA() != TObjString::Class() )
      return 2;
    TObjString* os = static_cast<TObjString*>(params->At(ip));
    if( !os )
      return 3;
    if( os->String().IsNull() )
      return 4;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t BdataLoc::Configure( const TObjArray* params, Int_t start )
{
  // Initialize this object from the TObjString parameters given in the params
  // array, starting at index 'start'

  Int_t ret = CheckConfigureParams( params, start );
  if( ret )
    return ret;

  SetName( GetString( params, start ) );
  SetTitle( GetName() );
  
  crate = GetString( params, start+1 ).Atoi();
  
  return 0;
}

//_____________________________________________________________________________
TypeSet_t& BdataLoc::fgBdataLocTypes()
{
  // Local storage for all defined BdataLoc types. Initialize here on first use
  // (cf. http://www.parashift.com/c++-faq/static-init-order-on-first-use-members.html)

  static TypeSet_t* fgBdataLocTypes = new TypeSet_t;

  return *fgBdataLocTypes;
}

//_____________________________________________________________________________
TypeIter_t BdataLoc::DoRegister( const BdataLocType& info )
{
  // Add given info in fgBdataLocTypes

  if( !info.fClassName ||!*info.fClassName ) {
    ::Error( "BdataLoc::DoRegister", "Attempt to register empty class name. "
	     "Coding error. Call expert." );
    return fgBdataLocTypes().end();
  }

  pair< TypeIter_t, bool > ins = fgBdataLocTypes().insert(info);

  if( !ins.second ) {
    ::Error( "BdataLoc::DoRegister", "Attempt to register duplicate database "
	     "key \"%s\". Coding error. Call expert.", info.fDBkey );
    return fgBdataLocTypes().end();
  }
  // NB: std::set guarantees that iterators remain valid on further insertions,
  // so this return value will remain good, unlike, e.g., std::vector iterators.
  return ins.first;
}

//_____________________________________________________________________________
void BdataLoc::PrintNameType( Option_t* opt ) const
{
  // Print name (and type for "FULL")

  cout << " " << fName << "\t";
  if( fName.Length() < 7 ) cout << "\t";
  TString option(opt);
  if( option.Contains("FULL") ) {
    cout << "type = " << GetTypeKey();
  }
}

//_____________________________________________________________________________
void BdataLoc::Print( Option_t* opt ) const
{
  // Print name and data value

  PrintNameType(opt);
  TString option(opt);
  if( option.Contains("FULL") ) {
    cout << "\t crate = " << crate;
  }
  cout << "\t data = " << data << endl;
}

//_____________________________________________________________________________
Int_t CrateLoc::Configure( const TObjArray* params, Int_t start )
{
  // Initialize CrateLoc from given parmeters

  Int_t ret = BdataLoc::Configure( params, start );
  if( ret )
    return ret;

  slot = GetString( params, start+2 ).Atoi();
  chan = GetString( params, start+3 ).Atoi();

  return 0;
}

//_____________________________________________________________________________
void CrateLoc::Load( const THaEvData& evdata )
{
  // Load one data word from crate/slot/chan address

  if( evdata.GetNumHits(crate,slot,chan) > 0 ) {
    data = evdata.GetData(crate,slot,chan,0);
  }
}

//_____________________________________________________________________________
void CrateLoc::PrintCrateLocHeader( Option_t* opt ) const
{
  // Print name and data

  PrintNameType(opt);
  TString option(opt);
  if( option.Contains("FULL") ) {
    cout << "\t cr/sl/ch = " << crate << "/" << slot << "/" << chan;
  }
}

//_____________________________________________________________________________
void CrateLoc::Print( Option_t* opt ) const
{
  // Print name and data

  PrintCrateLocHeader(opt);
  cout << "\t data = " << data << endl;
}

//_____________________________________________________________________________
Int_t CrateLocMulti::DefineVariables( EMode mode )
{
  // For multivalued data (multihit modules), define a variable-sized global
  // variable on the vector<UInt_t> rdata member.

  if( mode == kDefine && TestBit(kIsSetup) ) return kOK;
  SetBit( kIsSetup, mode == kDefine );

  Int_t ret = kOK;

  if( mode == kDefine ) {
    TString comment = GetName();
    comment.Append(" multihit data");
    if( !gHaVars->Define( GetName(), comment, rdata ) )
      ret = kInitError;
  } else
    ret = CrateLoc::DefineVariables( mode );

  return ret;
}

//_____________________________________________________________________________
void CrateLocMulti::Load( const THaEvData& evdata )
{
  // Load all decoded hits from crate/slot/chan address

  data = 0;
  for (Int_t i = 0; i < evdata.GetNumHits(crate,slot,chan); ++i) {
    rdata.push_back( evdata.GetData(crate,slot,chan,i) );
  }
}

//_____________________________________________________________________________
void CrateLocMulti::PrintMultiData( Option_t* ) const
{
  // Print the data array

  cout << "\t data = ";
  if( rdata.empty() )
    cout << "(no hits)";
  else {
    for( vector<UInt_t>::const_iterator it = rdata.begin();
	 it != rdata.end(); ++it ) {
      cout << " " << *it;
    }
  }
}

//_____________________________________________________________________________
void CrateLocMulti::Print( Option_t* opt ) const
{
  // Print name and all data values

  PrintCrateLocHeader(opt);
  PrintMultiData(opt);
  cout << endl;
}

//_____________________________________________________________________________
Int_t WordLoc::Configure( const TObjArray* params, Int_t start )
{
  // Initialize WordLoc from given parmeters

  Int_t ret = BdataLoc::Configure( params, start );
  if( ret )
    return ret;

  // Convert header word, given as a hex string, to unsigned int32
  const char* hs = GetString( params, start+2 ).Data();
  char* p = 0;
  errno = 0;
  unsigned long li = strtoul( hs, &p, 16 );
  if( errno || *p )   return 10;
  if( li > kMaxUInt ) return 11;
  header = static_cast<UInt_t>(li);

  ntoskip = GetString( params, start+3 ).Atoi();

  return 0;
}

//_____________________________________________________________________________
void WordLoc::Load( const THaEvData& evdata )
{
  // Load data at header/notoskip position from crate data buffer 

  typedef const UInt_t rawdata_t;

  Int_t roclen = evdata.GetRocLength(crate);
  if( roclen < ntoskip+1 ) return;

  rawdata_t* cratebuf = evdata.GetRawDataBuffer(crate), *endp = cratebuf+roclen+1;
  assert(cratebuf);  // Must exist if roclen > 0

  // Accelerated search for the header word. Coded explicitly because there
  // is no "memstr" in the standard library.

  //FIXME: Can this be made faster because each header is followed by the offset
  // to the next header?
  //FIXME: What about byte order? Are the raw data always a certain endianness?

  // Get the first byte of the header, regardless of byte order
  int h = ((UChar_t*)&header)[0];
  rawdata_t* p = cratebuf+2;
  while( p < endp &&
	 (p = (rawdata_t*)memchr(p,h,sizeof(rawdata_t)*(endp-p-1)+1)) &&
	 p <= endp-ntoskip-1 ) {
    // The header must be aligned to a word boundary
    int off = ((char*)p-(char*)cratebuf) & (sizeof(rawdata_t)-1);  // same as % sizeof()
    if( off != 0 ) {
      p = (rawdata_t*)((char*)p + sizeof(rawdata_t)-off);
      continue;
    }
    // Compare all 4 bytes of the header word
    if( memcmp(p,&header,sizeof(rawdata_t)) == 0 ) {
      // Fetch the requested word (as UInt_t, i.e. 4 bytes)
      // BTW, notoskip == 0 makes no sense since it would fetch the header itself
      data = *(p+ntoskip);
      break;
    }
    ++p;  // Next rawdata_t word
  }
}

//_____________________________________________________________________________
void WordLoc::Print( Option_t* opt ) const
{
  // Print name and data

  PrintNameType(opt);
  TString option(opt);
  if( option.Contains("FULL") ) {
    cout << "\t cr/hdr/off = " << crate << "/0x" 
	 << hex << header << dec << "/" << ntoskip;
  }
  cout << "\t data = " << data << endl;
}

//_____________________________________________________________________________
void RoclenLoc::Load( const THaEvData& evdata )
{
  // Get event length for this crate

  data = evdata.GetRocLength(crate);
}

//_____________________________________________________________________________
ClassImp(BdataLoc)
ClassImp(CrateLoc)
ClassImp(CrateLocMulti)
ClassImp(WordLoc)
ClassImp(RoclenLoc)
