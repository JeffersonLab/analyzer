//*-- Author:   Ole Hansen, October 2013

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
// Hall A miscellaneous decoder data, which typically do not belong to a
// detector class. Provides a place to rapidly add new channels and to
// make their data available as analyzer global variables.
//
// To use this class, add something like the following to your analysis
// script:
//
// gHaApps->Add( new THaDecData("D","Decoder raw data") );
// 
// This will give your class the name "D", which will be the prefix for
// all its database keys and global variables.
//
// To define channels, corresponding database entries have to be
// set up. Either use the old-style flat file database format
// (see examples/decdata.map) or the new key/value format.
// For each channel, first decide how to identify the data:
//
// 1. "crate" variables define single 32-bit data words by crate/slot/channel
// 2. "multi" is the same as "crate", except it allows multihit channels
//    with an arbitrary number of hits.
// 3. "word" variables define single data words found in a given crate's
//    event buffer by a 32-bit header word and an offset. Offset = 1
//    means the data word following the header, etc.
// 4. "roclen" variables contain the event length of a given crate.
// 5. Hall A-specific: "bitNN" variables, where NN is a number 0-31,
//    report the value of a trigger bit read via a multihit TDC.
//    Ask the DAQ expert for details.
//
// In the key/value database format, define one database key for ALL
// the variables of a given type. The key's value is then a long string
// of n-tuples of configuration parameters, variable name first. For
// example:
//
/*
// D.crate = syncadc1, 1, 25, 16,  \
//           syncadc2, 2, 24, 48
*/
//
// will set up two "crate" variables, for crate/slot/chan = (1/25/16) and
// (2/24/48), respectively, that will appear as global variables
// "D.syncadc1" and "D.syncadc2". Note the continuation mark "\".
//
// "word" variables are defined similarly, but take header (in hex) and
// offset as the 3rd and 4th parameters.
//
// Alternatively, use the "decdata.map" example file to define channels
// line by line. (This file format will be phased out in the future.)
// To add a new variable, if it is on a single-hit channel, you may
// imitate 'synchadc1' if you know the (crate,slot,chan), and 
// imitate 'timeroc2' if you know the (crate,header,offset).
//
// If your variable is more complicated and relies on several 
// channels, you may easily write you own plug-in class derived
// from BdataLoc (see BdataLoc.[Ch]). Decoding is done in Load().
// You'll have to define a new, unique type name (like "crate") and
// initialize the class with a static initialization call to DoRegister
// (see top of BdataLoc.C). Also, you'll probably need to override
// Configure() to get additional or different parameters from the
// database.
//
// Originally written by R. Michaels, March 2002
// Modified by R.J. Feuerbach, April 2004
// Rewritten by O. Hansen, October 2013
//
//////////////////////////////////////////////////////////////////////////

#include "THaDecData.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TDatime.h"
#include "TClass.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "BdataLoc.h"
#include "THaEvData.h"

#include <iostream>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <memory>

#define DECDATA_LEGACY_DB

using namespace std;

static Int_t kInitHashCapacity = 100;
static Int_t kRehashLevel = 3;

//_____________________________________________________________________________
THaDecData::THaDecData( const char* name, const char* descript )
  : THaApparatus( name, descript ), evtype(0), evtypebits(0),
    fBdataLoc( kInitHashCapacity, kRehashLevel )
{
  fProperties &= ~kNeedsRunDB;
  fBdataLoc.SetOwner(kTRUE);
}

//_____________________________________________________________________________
THaDecData::~THaDecData()
{
  // Destructor. Delete data location objects and global variables.

  fBdataLoc.Clear();
  RemoveVariables();
}

//_____________________________________________________________________________
void THaDecData::Clear( Option_t* )
{
  // Reset event-by-event data

  TIter next( &fBdataLoc );
  while( TObject* obj = next() ) {
    obj->Clear();
  }
  evtype     = 0;
  evtypebits = 0;

}

//_____________________________________________________________________________
Int_t THaDecData::DefineVariables( EMode mode )
{
  // Register global variables, open decdata map file, and parse it.
  // If mode == kDelete, remove global variables.

  // Each defined decoder data location defines its own global variable(s)
  // The BdataLoc have their own equivalent of fIsSetup, so we can
  // unconditionally call their DefineVariables() here (similar to detetcor
  // initialization in THaAppratus::Init).
  Int_t retval = kOK;
  TIter next( &fBdataLoc );
  while( BdataLoc* dataloc = static_cast<BdataLoc*>( next() ) ) {
    if( dataloc->DefineVariables( mode ) != kOK )
      retval = kInitError;
  }
  if( retval != kOK )
    return retval;

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "evtype",     "CODA event type",             "evtype" },  
    { "evtypebits", "event type bit pattern",      "evtypebits" },  
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t THaDecData::DefineLocType( const BdataLoc::BdataLocType& loctype,
				 const TString& configstr, bool re_init )
{
  // Define variables for given loctype using parameters in configstr

  const char* const here = "DefineLocType";

  Int_t err = 0;

  // Split the string from the database into values separated by commas,
  // spaces, and/or tabs
  auto_ptr<TObjArray> config( configstr.Tokenize(", \t") );
  if( !config->IsEmpty() ) {
    Int_t nparams = config->GetLast()+1;
    assert( nparams > 0 );   // else bug in IsEmpty() or GetLast()
      
    if( nparams % loctype.fNparams != 0 ) {
      Error( Here(here), "Incorrect number of parameters in database key "
	     "%s%s. Have %d, but must be a multiple of %d. Fix database.",
	     GetPrefix(), loctype.fDBkey, nparams, loctype.fNparams );
      return -1;
    }

    TObjArray* params = config.get();
    for( Int_t ip = 0; ip < nparams; ip += loctype.fNparams ) {
      // Prepend prefix to name in parameter array
      TString& bname = GetObjArrayString( params, ip );
      bname.Prepend(GetPrefix());
      BdataLoc* item = static_cast<BdataLoc*>(fBdataLoc.FindObject(bname) );
      Bool_t already_defined = ( item != 0 );
      if( already_defined ) {
	// Name already exists
	if( re_init ) {
	  // Changing the variable type during a run makes no sense
	  if( loctype.fTClass != item->IsA() ) {
	    Error( Here(here), "Attempt to redefine existing variable %s "
		   "with different type.\nOld = %s, new = %s. Fix database.",
		   item->GetName(), item->IsA()->GetName(),
		   loctype.fTClass->GetName() );
	    return -1;
	  }
	  if( fDebug>2 ) 
	    Info( Here(here), "Updating variable %s", bname.Data() );
	} else {
	  // Duplicate variable name (i.e. duplicate names in database)
	  Error( Here(here), "Duplicate variable name %s. Fix database.",
		 item->GetName() );
	  return -1;
	}
      } else {
	// Make a new BdataLoc
	if( fDebug>2 )
	  Info( Here(here), "Defining new variable %s",  bname.Data() );
	item = static_cast<BdataLoc*>( loctype.fTClass->New() );
	if( !item ) {
	  Error( Here(here), "Failed to create variable of type %s. Should "
		 "never happen. Call expert.", loctype.fTClass->GetName() );
	  return -1;
	}
      }
      // Configure the new or existing BdataLoc with current database parameters. 
      // The first parameter is always the name. Note that this object's prefix
      // was already prepended above.
      err = item->Configure( params, ip );
      if( !err && loctype.fOptptr != 0 ) {
	// Optional pointer to some type-specific data
	err = item->OptionPtr( loctype.fOptptr );
      }
      if( err ) {
	Int_t in = ip - (ip % loctype.fNparams); // index of name
	Error( Here(here), "Illegal parameter for variable %s, "
	       "index = %d, value = %s. Fix database.",
	       GetObjArrayString(params,in).Data(), ip,
	       GetObjArrayString(params,ip).Data() );
	if( !already_defined )
	  delete item;
	break;
      } else if( !already_defined ) {
	// Add this BdataLoc to the list to be processed
	fBdataLoc.Add(item);
      }
    }
  } else {
    Warning( Here(here), "Empty database key %s%s.",
	     GetPrefix(), loctype.fDBkey );
  }

  return err;
}

//_____________________________________________________________________________
FILE* THaDecData::OpenFile( const TDatime& date )
{ 
  // Open DecData database file. First look for standard file name,
  // e.g. "db_D.dat", then for legacy file name "decdata.map"

  FILE* fi = THaApparatus::OpenFile( date );
  if( fi )
    return fi;
  return THaAnalysisObject::OpenFile("decdata.dat", date,
				     Here("OpenFile()"), "r", fDebug);
}

//_____________________________________________________________________________
#ifdef DECDATA_LEGACY_DB
#include <map>

static Int_t CheckDBVersion( FILE* file )
{
  // Check database version. Similar to emacs mode specs, versions are
  // determined by an identifier comment "# Version: 2" on the first line
  // (within first 80 chars). If no such tag is found, version 1 is assumed.

  const TString identifier("Version:");

  const size_t bufsiz = 82;
  char* buf = new char[bufsiz];
  rewind(file);
  const char* s = fgets(buf,bufsiz,file);
  if( !s ) { // No first line? Not our problem...
    delete [] buf;
    return 1;
  }
  TString line(buf);
  delete [] buf;
  Ssiz_t pos = line.Index(identifier,0,TString::kIgnoreCase);
  if( pos == kNPOS )
    return 1;
  pos += identifier.Length();
  while( pos < line.Length() && isspace(line(pos)) ) pos++;
  if( pos >= line.Length() )
    return 1;
  TString line2 = line(pos,line.Length() );
  Int_t vers = line2.Atoi();
  if( vers == 0 )
    vers = 1;
  return vers;
}

//_____________________________________________________________________________
inline
static TString& GetString( const TObjArray* params, Int_t pos )
{
  return THaAnalysisObject::GetObjArrayString(params,pos);
}

//_____________________________________________________________________________
static Int_t ReadOldFormatDB( FILE* file, map<TString,TString>& configstr_map )
{
  // Read old-style THaDecData database file and put results into a map from
  // database key to value (simulating the new-style key/value database info).
  // Old-style "crate" objects are all assumed to be multihit channels, even
  // though they usually are not.

  const size_t bufsiz = 256;
  char* buf = new char[bufsiz];
  string dbline;
  const int nkeys = 3;
  TString confkey[nkeys] = { "multi", "word", "bit" };
  TString confval[nkeys];
  // Read all non-comment lines
  rewind(file);
  while( THaAnalysisObject::ReadDBline(file, buf, bufsiz, dbline) != EOF ) {
    if( dbline.empty() ) continue;
    // Tokenize each line read
    TString line( dbline.c_str() );
    auto_ptr<TObjArray> tokens( line.Tokenize(" \t") );
    TObjArray* params = tokens.get();
    if( params->IsEmpty() || params->GetLast() < 4 ) continue;
    // Determine data type
    bool is_slot = ( GetString(params,1) == "crate" );
    int idx = is_slot ? 0 : 1;
    TString name = GetString(params,0);
    // TrigBits are crate types with special names
    if( is_slot && name.BeginsWith("bit") ) {
      if( name.Length() > 3 ) {
	TString name2 = name(3,name.Length());
	if( name2.IsDigit() && name2.Atoi() < 32 )
	  idx = 2;
      }
    }
    confval[idx] += name;
    confval[idx] += " ";
    for( int i = 2; i < 5; ++ i ) {
      confval[idx] += GetString(params,i).Data();
      confval[idx] += " ";
    }
  }
  delete [] buf;
  // Put the retrieved strings into the key/value map
  for( int i = 0; i < nkeys; ++ i ) {
    if( !confval[i].IsNull() )
      configstr_map[confkey[i]] = confval[i];
  }
  return 0;
}
#endif

//_____________________________________________________________________________
Int_t THaDecData::ReadDatabase( const TDatime& date )
{
  // Read THaDecData database

  static const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  Bool_t re_init = fIsInit;
  fIsInit = kFALSE;
  if( !re_init ) {
    fBdataLoc.Clear();
  }

#ifdef DECDATA_LEGACY_DB
  bool old_format = (CheckDBVersion(file) == 1);
  map<TString,TString> configstr_map;
  if( old_format )
    ReadOldFormatDB( file, configstr_map );
#endif

  Int_t err = 0;
  for( BdataLoc::TypeIter_t it = BdataLoc::fgBdataLocTypes().begin();
       !err && it != BdataLoc::fgBdataLocTypes().end(); ++it ) {
    const BdataLoc::BdataLocType& loctype = *it;

    // Get the ROOT class for this type
    assert( loctype.fClassName && *loctype.fClassName );
    if( !loctype.fTClass ) {
      loctype.fTClass = TClass::GetClass( loctype.fClassName );
      if( !loctype.fTClass ) {
	// Probably typo in the call to BdataLoc::DoRegister
	Error( Here(here), "No class defined for data type \"%s\". Programming "
	       "error. Call expert.", loctype.fClassName );
	err = -1;
	break;
      }
    }
    if( !loctype.fTClass->InheritsFrom( BdataLoc::Class() )) {
      Error( Here(here), "Class %s is not a BdataLoc. Programming error. "
	     "Call expert.", loctype.fTClass->GetName() );
      err = -1;
      break;
    }

    TString configstr;
#ifdef DECDATA_LEGACY_DB
    // Retrieve old-format database parameters read above for this type
    if( old_format ) {
      map<TString,TString>::const_iterator found =
	configstr_map.find(loctype.fDBkey);
      if( found == configstr_map.end() )
	continue;
      configstr = found->second;
    } else
#endif
    {
      // Read key/value database format
      TString dbkey = loctype.fDBkey;
      dbkey.Prepend( GetPrefix() );
      if( LoadDBvalue( file, date, dbkey, configstr ) != 0 )
	continue;  // No definitions in database for this BdataLoc type
    }
    err = DefineLocType( loctype, configstr, re_init );
  }

  fclose(file);
  if( err )
    return kInitError;

// ======= FIXME: Hall A lib ================================================
  // Configure the trigger bits with a pointer to our evtypebits
  TIter next( &fBdataLoc );
  while( BdataLoc* dataloc = static_cast<BdataLoc*>( next() ) ) {
    if( dataloc->Class() == TrigBitLoc::Class() )
      dataloc->OptionPtr( &evtypebits );
  }
// ======= END FIXME: Hall A lib ============================================

  fIsInit = kTRUE;
  return kOK;
}


//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDecData::Init( const TDatime& run_time ) 
{
  // Custom Init() method. Since this apparatus has no traditional "detectors",
  // we skip the detector initialization.

  // Standard analysis object init, calls MakePrefix(), ReadDatabase()
  // and DefineVariables(), and Clear("I")
  return THaAnalysisObject::Init( run_time );
}

//_____________________________________________________________________________
Int_t THaDecData::Decode(const THaEvData& evdata)
{
  // Extract the requested variables from the event data

  if( !IsOK() )
    return -1;

  Clear();

  evtype = evdata.GetEvType();   // CODA event type 

  // For each raw data source registered in fBdataLoc, get the data 

  //TODO: accelerate search for multiple header words in same crate
  //- group WordLoc objects by crate
  //- for each crate with >1 WordLoc defs, make a MultiWordLoc
  //- MultiWordLoc uses faster search algo to scan crate buffer

  TIter next( &fBdataLoc );
  while( BdataLoc *dataloc = static_cast<BdataLoc*>( next() ) ) {
    dataloc->Load( evdata );
  }
  
  if( fDebug>1 )
    Print();

  return 0;
}

//_____________________________________________________________________________
void THaDecData::Print( Option_t* opt ) const
{
  // Print current status of all THaDecData variables

  THaAnalysisObject::Print(opt);

  cout << " event types,  CODA = " << evtype
       << "   bit pattern = 0x"      << hex << evtypebits << dec
       << endl;

  if( evtypebits != 0 ) {
    cout << " trigger bits set = ";
    bool cont = false;
    for( UInt_t i = 0; i < sizeof(evtypebits)*kBitsPerByte-1; ++i ) {
      if( TESTBIT(evtypebits,i) ) {
	if( cont ) cout << ", ";
	else       cont = true;
	cout << i;
      }
    }
    cout << endl;
  }

  // Print variables in the order they were defined
  cout << " number of user variables: " << fBdataLoc.GetSize() << endl;
  TIter next( &fBdataLoc );
  while( TObject* obj = next() ) {
    obj->Print(opt);
  }
}

//_____________________________________________________________________________
ClassImp(THaDecData)

