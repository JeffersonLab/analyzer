////////////////////////////////////////////////////////////////////
//
//   THaEpicsEvtHandler
//
//   Event handler for Hall A EPICS data events
//   R. Michaels,  April 2015
//
//   This class does the following
//      For a particular set of event types (here, event type 131)
//      use the THaEpics class to decode the data.  This class interacts
//      with THaOutput much the same way the old (now obsolete) THaCodaDecoder
//      did, providing EPICS data to the output.
//
//   At the moment, I foresee this as a member of THaAnalyzer.
//   To use as a plugin with your own modifications, you can do this:
//       gHaEvtHandlers->Add (new THaEpicsEvtHandler("epics","HA EPICS event type 131"));
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "THaEpicsEvtHandler.h"
#include "THaCodaData.h"
#include "THaEvData.h"
#include "THaEpics.h"
#include "TNamed.h"
#include "TMath.h"
#include "TString.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "THaVarList.h"
#include "VarDef.h"

using namespace std;
using namespace Decoder;

THaEpicsEvtHandler::THaEpicsEvtHandler(const char *name, const char* description)
  : THaEvtTypeHandler(name,description)
{
  fEpics = new Decoder::THaEpics();
}

THaEpicsEvtHandler::~THaEpicsEvtHandler()
{
  delete fEpics;
}

Int_t THaEpicsEvtHandler::End( THaRunBase* )
{
  return 0;
}

Bool_t THaEpicsEvtHandler::IsLoaded(const char* tag) const {
  if ( !fEpics ) return kFALSE;
  return fEpics->IsLoaded(tag);
}

Double_t THaEpicsEvtHandler::GetData(const char* tag, Int_t event) const {
  assert( IsLoaded(tag) ); // Should never ask for non-existent data
  if ( !fEpics ) return 0;
  return fEpics->GetData(tag, event);
}

Double_t THaEpicsEvtHandler::GetTime(const char* tag, Int_t event) const {
  assert( IsLoaded(tag) );
  if ( !fEpics ) return 0;
  return fEpics->GetTimeStamp(tag, event);
}

TString THaEpicsEvtHandler::GetString(const char* tag, Int_t event) const {
  assert( IsLoaded(tag) );
  if ( !fEpics ) return TString("nothing");
  return TString(fEpics->GetString(tag, event).c_str());
}

Int_t THaEpicsEvtHandler::Analyze(THaEvData *evdata)
{

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  UInt_t evbuffer[MAXDATA];
  Int_t recent_event = evdata->GetEvNum();  

  if (evdata->GetEvLength() >= MAXDATA) 
      cerr << "EpicsHandler:  need a bigger buffer ! "<<endl;

// Copy the buffer.  EPICS events are infrequent, so no harm.
  for (Int_t i = 0; i < evdata->GetEvLength(); i++) 
         evbuffer[i] = evdata->GetRawData(i);

  if (fDebugFile) EvDump(evdata);

  fEpics->LoadData(evbuffer, recent_event);

  return 1;
}

THaAnalysisObject::EStatus THaEpicsEvtHandler::Init(const TDatime&)
{

  cout << "Howdy !  We are initializing THaEpicsEvtHandler !!   name =   "<<fName<<endl;

// Set the event type to the default unless the client has already defined it.
  if (GetNumTypes()==0) SetEvtType(Decoder::EPICS_EVTYPE); 

  fStatus = kOK;
  return kOK;
}

ClassImp(THaEpicsEvtHandler)
