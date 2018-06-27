//*-- Author :    Ole Hansen   15-May-00

//////////////////////////////////////////////////////////////////////////
//
// THaDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "THaApparatus.h"
#include "THaDetMap.h"

#include <cassert>
#include <sstream>
#define OSSTREAM ostringstream

using namespace std;

//_____________________________________________________________________________
THaDetector::THaDetector( const char* name, const char* description,
			  THaApparatus* apparatus )
  : THaDetectorBase(name,description), fApparatus(apparatus)
{
  // Constructor

  if( !name || !*name ) {
    Error( "THaDetector()", "Must construct detector with valid name! "
	   "Object construction failed." );
    MakeZombie();
    return;
  }
}

//_____________________________________________________________________________
THaDetector::THaDetector() : fApparatus(0)
{
  // for ROOT I/O only
}

//_____________________________________________________________________________
THaDetector::~THaDetector()
{
  // Destructor
}

//_____________________________________________________________________________
THaApparatus* THaDetector::GetApparatus() const
{
  return static_cast<THaApparatus*>(fApparatus.GetObject());
}

//_____________________________________________________________________________
void THaDetector::SetApparatus( THaApparatus* apparatus )
{
  // Associate this detector with the given apparatus.
  // Only possible before initialization.

  if( IsInit() ) {
    Warning( Here("SetApparatus()"), "Cannot set apparatus. "
	     "Object already initialized.");
    return;
  }
  fApparatus = apparatus;
}

//_____________________________________________________________________________
void THaDetector::MakePrefix()
{
  // Set up name prefix for global variables. Internal function called
  // during initialization.

  THaApparatus *app = GetApparatus();
  const char* basename = (app != 0) ? app->GetName() : 0;
  THaDetectorBase::MakePrefix( basename );

}

//_____________________________________________________________________________
Int_t THaDetector::End( THaRunBase* run )
{
  // Print detector-specific warning message summary

  if( !fMessages.empty() ) {
    ULong64_t ntot = 0;
    map<string,UInt_t> chan_count;
    for( map<string,UInt_t>::const_iterator it = fMessages.begin();
	 it != fMessages.end(); ++it ) {
      ntot += it->second;
      const string& m = it->first;
      string::size_type pos = m.find("channel");
      if( pos != string::npos ) {
	string::size_type pos2 = m.find(".",pos+7), len = string::npos;
	if( pos > 3 ) pos -= 4;
	if( pos2 != string::npos ) {
	  assert( pos2 > pos );
	  len = pos2-pos;
	}
	++chan_count[m.substr(pos,len)]; // e.g. substr = "TDC channel 1/2/3"
      } else {
	++chan_count[m];
      }
    }
    OSSTREAM msg;
    msg << endl
	<< "  Encountered " << fNEventsWithWarnings << " events with "
	<< "warnings, " << ntot << " total warnings"
	<< endl
	<< "  affecting " << chan_count.size() << " out of "
	<< fDetMap->GetTotNumChan() << " channels. Check signals for noise!"
	<< endl
	<< "  Call Print(\"WARN\") for channel list. "
	<< "Re-run with fDebug>0 for per-event details.";
    Warning( Here("End"), "%s", msg.str().c_str() );
  }

  fMessages.clear();
  return THaDetectorBase::End(run);
}

//_____________________________________________________________________________
ClassImp(THaDetector)
