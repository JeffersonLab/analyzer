//*-- Author :    Ole Hansen   12/05/2000

//////////////////////////////////////////////////////////////////////////
//
// THaInterface
//
// THaInterface is the interactive interface to the Hall A Analyzer.
// It allows interactive access to all analyzer classes as well as
// all of standard ROOT.
//
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "TClass.h"
#include "TError.h"
#include "TSystem.h"
#include "TString.h"
#include "TRegexp.h"
#include "TTree.h"
#include "THaInterface.h"
#include "TInterpreter.h"
#include "THaVarList.h"
#include "THaCutList.h"
#include "CodaRawDecoder.h"
#include "THaGlobals.h"
#include "THaAnalyzer.h"
//#include "THaFileDB.h"
#include "Textvars.h"   // for gHaTextars
#include "ha_compiledata.h"
#include <cstring>
#include <sstream>

//#include "TGXW.h"
//#include "TVirtualX.h"

using namespace std;

THaVarList*  gHaVars     = nullptr;  // List of symbolic analyzer variables
THaCutList*  gHaCuts     = nullptr;  // List of global analyzer cuts/tests
TList*       gHaApps     = nullptr;  // List of Apparatuses
TList*       gHaPhysics  = nullptr;  // List of physics modules
TList*       gHaEvtHandlers  = nullptr;  // List of event handlers
THaRunBase*  gHaRun      = nullptr;  // The currently active run
TClass*      gHaDecoder  = nullptr;  // Class(!) of decoder to use
THaDB*       gHaDB       = nullptr;  // Database system to use

THaInterface* THaInterface::fgAint = nullptr;  // Pointer to this interface

static TString fgTZ;

//_____________________________________________________________________________
THaInterface::THaInterface( const char* appClassName, int* argc, char** argv,
			    void* options, int numOptions, Bool_t noLogo ) :
  TRint( appClassName, argc, argv, options, numOptions, true )
{
  // Create the Hall A analyzer application environment. The THaInterface
  // environment provides an interface to the the interactive ROOT system
  // via inheritance of TRint as well as access to the Hall A analyzer classes.

  if( fgAint ) {
    Error("THaInterface", "only one instance of THaInterface allowed");
    MakeZombie();
    return;
  }

  if( !noLogo )
    PrintLogo();

  SetPrompt("analyzer [%d] ");
  gHaVars    = new THaVarList;
  gHaCuts    = new THaCutList( gHaVars );
  gHaApps    = new TList;
  gHaPhysics = new TList;
  gHaEvtHandlers = new TList;
  // Use the standard CODA file decoder by default
  gHaDecoder = Podd::CodaRawDecoder::Class();
  // File-based database by default
  //  gHaDB      = new THaFileDB();
  gHaTextvars = new Podd::Textvars;

  // Set the maximum size for a file written by Podd contained by the TTree
  //  putting it to 1.5 GB, down from the default 1.9 GB since something odd
  //  happens for larger files
  //FIXME: investigate
  TTree::SetMaxTreeSize(1500000000);

  // Make the Podd header directory(s) available so scripts don't have to
  // specify an explicit path.
  // If $ANALYZER defined, we take our includes from there, otherwise we fall
  // back to the compile-time directories (which may have moved!)
  TString s = gSystem->Getenv("ANALYZER");
  if( s.IsNull() ) {
    s = HA_INCLUDEPATH;
  } else {
    // Give preference to $ANALYZER/include
    TString p = s+"/include";
    void* dp = gSystem->OpenDirectory(p);
    if( dp ) {
      gSystem->FreeDirectory(dp);
      s = p;
    } else
      s = s+"/src " + s+"/hana_decode ";
  }
  // Directories names separated by blanks.
  // FIXME: allow blanks
  TRegexp re("[^ ]+");
  TString ss = s(re);
  while( !ss.IsNull() ) {
    // Only add dirs that exist
    void* dp = gSystem->OpenDirectory(ss);
    if( dp ) {
      gInterpreter->AddIncludePath(ss);
      gSystem->FreeDirectory(dp);
    }
    s.Remove(0,s.Index(re)+ss.Length());
    ss = s(re);
  }

  // Because of lack of foresight, the analyzer uses TDatime objects,
  // which are kept in localtime() and hence are not portable, and also
  // uses localtime() directly in several places. As a result, database
  // lookups depend on the timezone of the machine that the replay is done on!
  // If this timezone is different from the one in which the data were taken,
  // mismatches may occur. This is bad.
  // FIXME: Use TTimeStamp to keep time in UTC internally.
  // To be done in version 1.6
  //
  // As a temporary workaround, we assume that all data were taken in
  // US/Eastern time, and that the database has US/Eastern timestamps.
  // This is certainly true for all JLab production data..
  fgTZ = gSystem->Getenv("TZ");
  gSystem->Setenv("TZ","US/Eastern");


  fgAint = this;
}

//_____________________________________________________________________________
THaInterface::~THaInterface()
{
  // Destructor

  if( fgAint == this ) {
    // Restore the user's original TZ
    gSystem->Setenv("TZ",fgTZ.Data());
    // Clean up the analyzer object if defined
    delete THaAnalyzer::GetInstance();
    // Delete all global lists and objects contained in them
    delete gHaTextvars; gHaTextvars=nullptr;
    //    delete gHaDB;           gHaDB = nullptr;
    delete gHaPhysics;   gHaPhysics=nullptr;
    delete gHaEvtHandlers;  gHaEvtHandlers=nullptr;
    delete gHaApps;         gHaApps=nullptr;
    delete gHaVars;         gHaVars=nullptr;
    delete gHaCuts;         gHaCuts=nullptr;
    fgAint = nullptr;
  }
}

//_____________________________________________________________________________
void THaInterface::PrintLogo( Bool_t lite )
{
   // Print the Hall A analyzer logo on standard output.

   static const char* months[] = {"???","Jan","Feb","Mar","Apr","May",
                                  "Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
   const char* root_version = gROOT->GetVersion();
   Int_t idatqq = gROOT->GetVersionDate();
   Int_t iday   = idatqq%100;
   Int_t imonth = (idatqq/100)%100;
   if( imonth < 1 || imonth > 12 ) // should never happen, but to be safe,
     imonth = 0;                   // print "???"
   Int_t iyear  = (idatqq/10000);
   Int_t mille = iyear;
   if ( iyear < 90 )
     mille = 2000 + iyear;
   else if ( iyear < 1900 )
     mille = 1900 + iyear;
   ostringstream ostr;
   ostr << iday << " " << months[imonth] << " " << mille;

   if( !lite ) {
     Printf("  ************************************************");
     Printf("  *                                              *");
     Printf("  *            W E L C O M E  to  the            *");
     Printf("  *       H A L L A   C++  A N A L Y Z E R       *");
     Printf("  *                                              *");
     Printf("  *  Release %16s %18s *",HA_VERSION,HA_DATE);
     Printf("  *  Based on ROOT %10s %18s *",root_version,ostr.str().c_str());
//     Printf("  *             Development version              *");
     Printf("  *                                              *");
     Printf("  *            For information visit             *");
     Printf("  * https://redmine.jlab.org/projects/podd/wiki/ *");
     Printf("  *                                              *");
     Printf("  ************************************************");
   }

#ifdef R__UNIX
   //   if (!strcmp(gGXW->GetName(), "X11TTF"))
   //   Printf("\nFreeType Engine v1.1 used to render TrueType fonts.");
#endif

   gInterpreter->PrintIntro();

}

//_____________________________________________________________________________
TClass* THaInterface::GetDecoder()
{
  // Get class of the current decoder
  return gHaDecoder;
}

//_____________________________________________________________________________
const char* THaInterface::GetVersion()
{
  // Get software version
  return HA_VERSION;
}

//_____________________________________________________________________________
const char* THaInterface::GetVersionString()
{
  // Get software version string (printed by analyzer -v)

  static TString version_string;

  //Podd 1.7.1.0-dev git@Release-170-10-gf6087c2f-dirty 29 Mar 2022 macOS 11.6.4 clang++ 13.0.0 ROOT 6.24/06
  if( version_string.IsNull() ) {
    ostringstream ostr;
    ostr << "Podd " << HA_VERSION;
    if( strlen(HA_GITREV) > 0 )
      ostr << " git@" << HA_GITREV;
    ostr << " " << HA_DATE << endl;
    ostr << "Built for " << HA_OSVERS;
    ostr << " using " << HA_CXXSHORTVERS;
    if( strlen(HA_ROOTVERS) )
      ostr << ", ROOT " << HA_ROOTVERS;
    version_string = ostr.str().c_str();
  }
  return version_string.Data();
}

//_____________________________________________________________________________
TClass* THaInterface::SetDecoder( TClass* c )
{
  // Set the type of decoder to be used. Make sure the specified class
  // actually inherits from the standard THaEvData decoder.
  // Returns the decoder class (i.e. its argument) or nullptr if error.

  if( !c ) {
    ::Error("THaInterface::SetDecoder", "argument is nullptr");
    return nullptr;
  }
  if( !c->InheritsFrom("THaEvData")) {
    ::Error("THaInterface::SetDecoder",
	    "decoder class must inherit from THaEvData");
    return nullptr;
  }

  gHaDecoder = c;
  return gHaDecoder;
}

//_____________________________________________________________________________
const char* THaInterface::SetPrompt( const char* newPrompt )
{
  // Make sure the prompt is and stays "analyzer [%d]". ROOT 6 resets the
  // interpreter prompt for every line without respect to any user-set
  // default prompt.

  TString s;
  if( newPrompt ) {
    s = newPrompt;
    if( s.Index("root") == 0 )
      s.Replace(0,4,"analyzer");
  }
  return TRint::SetPrompt(s.Data());
}

//_____________________________________________________________________________
ClassImp(THaInterface)
