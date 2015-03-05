////////////////////////////////////////////////////////////////////
//
//   THaEvtTypeHandler
//   handles events of a particular type
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

THaEvtTypeHandler::THaEvtTypeHandler(const char* name, const char* description)
  : THaAnalysisObject(name, description), fDebugFile(0)
{
}

THaEvtTypeHandler::~THaEvtTypeHandler()
{
  if (fDebugFile) {
    fDebugFile->close();
    delete fDebugFile;
  }
}

Int_t THaEvtTypeHandler::Analyze(THaEvData *evdata)
{
  return 1;
}

void THaEvtTypeHandler::EvPrint() const
{
  cout << "Hello !  THaEvtTypeHandler name =  "<<GetName()<<endl;
  cout << "    description "<<GetTitle()<<endl;
  cout << "    event types handled are "<<endl;
  for (UInt_t i=0; i < eventtypes.size(); i++) {
    cout << "    event type "<<eventtypes[i]<<endl;
  }
  cout << "----------------- good bye ----------------- "<<endl;
}

void THaEvtTypeHandler::EvDump(THaEvData *evdata) const
{
  // Dumps data to file, if fDebugFile was set.
  if (!fDebugFile) return;
  if (!evdata) return;  // get what you deserve
  int evnum = evdata->GetRawData(4);
  int len = evdata->GetRawData(0) + 1;
  int evtype = evdata->GetRawData(1)>>16;
  *fDebugFile << "\n ------ Raw Data Dump ------ "<<endl;
  *fDebugFile << "\n\n Event number " << dec << evnum << endl;
  *fDebugFile << " length " << len << " type " << evtype << endl;
  int ipt = 0;
  for (int j=0; j<(len/5); j++) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for (int k=j; k<j+5; k++) {
      *fDebugFile << hex << evdata->GetRawData(ipt++) << " ";
    }
    *fDebugFile << endl;
  }
  if (ipt < len) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for (int k=ipt; k<len; k++) {
      *fDebugFile << hex << evdata->GetRawData(ipt++) << " ";
    }
    *fDebugFile << endl;
  }
}

THaAnalysisObject::EStatus THaEvtTypeHandler::Init(const TDatime& dt)
{
  return kOK;
}

Bool_t THaEvtTypeHandler::IsMyEvent(Int_t evnum) const
{
  for (UInt_t i=0; i < eventtypes.size(); i++) {
    if (evnum == eventtypes[i]) return kTRUE;
  }

  return kFALSE;
}

//_____________________________________________________________________________
void THaEvtTypeHandler::MakePrefix()
{
  THaAnalysisObject::MakePrefix( NULL );
}

ClassImp(THaEvtTypeHandler)
