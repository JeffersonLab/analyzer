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
#include "THaInterface.h"
#include "TInterpreter.h"
#include "THaVarList.h"
#include "THaCutList.h"
#include "THaEvData.h"
#include "THaString.h"
#include "ha_compiledata.h"

//#include "TGXW.h"
//#include "TVirtualX.h"

using namespace std;

THaVarList* gHaVars    = NULL;  //List of symbolic analyzer variables
THaCutList* gHaCuts    = NULL;  //List of global analyzer cuts/tests
TList*      gHaApps    = NULL;  //List of Apparatuses
TList*      gHaScalers = NULL;  //List of scaler groups
TList*      gHaPhysics = NULL;  //List of physics modules
THaRun*     gHaRun     = NULL;  //The currently active run
TClass*     gHaDecoder = NULL;  //Class(!) of decoder to use

THaInterface* THaInterface::fgAint = NULL;  //Pointer to this interface

//_____________________________________________________________________________
THaInterface::THaInterface( const char* appClassName, int* argc, char** argv, 
			    void* options, int numOptions, Bool_t noLogo ) :
  TRint( appClassName, argc, argv, options, numOptions, kTRUE )
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
  gHaScalers = new TList;
  gHaPhysics = new TList;
  // Use the standard CODA file decoder by default
  gHaDecoder = THaEvData::Class();

  THaString ipath( HA_INCLUDEPATH );
  vector<THaString> ipv(ipath.Split());
  for (vector<THaString>::size_type i=0; i<ipv.size(); i++)
    gInterpreter->AddIncludePath( ipv[i].c_str() );
  
  fgAint = this;
}

//_____________________________________________________________________________
THaInterface::~THaInterface()
{
  if( fgAint == this ) {
    delete gHaPhysics;
    delete gHaScalers;
    delete gHaApps;
    delete gHaVars;
    delete gHaCuts;
    fgAint = NULL;
  }
}

//_____________________________________________________________________________
void THaInterface::PrintLogo()
{
   // Print the Hall A analyzer logo on standard output.

   Int_t iday,imonth,iyear,mille;
   static const char* months[] = {"Jan","Feb","Mar","Apr","May",
                                  "Jun","Jul","Aug","Sep","Oct",
                                  "Nov","Dec"};
   const char* root_version = gROOT->GetVersion();
   Int_t idatqq = gROOT->GetVersionDate();
   iday   = idatqq%100;
   imonth = (idatqq/100)%100;
   iyear  = (idatqq/10000);
   if ( iyear < 90 ) 
     mille = 2000 + iyear;
   else if ( iyear < 1900 )
     mille = 1900 + iyear;
   else
     mille = iyear;
   char* root_date = Form("%s %d %4d",months[imonth-1],iday,mille);

   const char* halla_version = "1.1-cvs";
   //   const char* halla_date = Form("%d %s %4d",24,months[2-1],2003);

   Printf("  ************************************************");
   Printf("  *                                              *");
   Printf("  *            W E L C O M E  to  the            *");
   Printf("  *       H A L L A   C++  A N A L Y Z E R       *");
   Printf("  *                                              *");
   Printf("  *        Release %10s %18s *",halla_version,__DATE__);
   Printf("  *  Based on ROOT %8s %20s *",root_version,root_date);
   Printf("  *             Development version              *");
   Printf("  *                                              *");
   Printf("  *            For information visit             *");
   Printf("  *        http://hallaweb.jlab.org/root/        *");
   Printf("  *                                              *");
   Printf("  ************************************************");

#ifdef R__UNIX
   //   if (!strcmp(gGXW->GetName(), "X11TTF"))
   //   Printf("\nFreeType Engine v1.1 used to render TrueType fonts.");
#endif

   gInterpreter->PrintIntro();

}

//_____________________________________________________________________________
ClassImp(THaInterface)

