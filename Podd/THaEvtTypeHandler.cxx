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
#include <algorithm>

using namespace std;

THaEvtTypeHandler::THaEvtTypeHandler(const char* name, const char* description)
  : THaAnalysisObject(name, description), fDebugFile(nullptr)
{
}

THaEvtTypeHandler::~THaEvtTypeHandler()
{
  if (fDebugFile) {
    fDebugFile->close();
    delete fDebugFile;
  }
}

void THaEvtTypeHandler::AddEvtType( UInt_t evtype) {
  eventtypes.push_back(evtype);
}

void THaEvtTypeHandler::SetEvtType( UInt_t evtype) {
  eventtypes.clear();
  AddEvtType(evtype);
}


void THaEvtTypeHandler::EvPrint() const
{
  cout << "Hello !  THaEvtTypeHandler name =  "<<GetName()<<endl;
  cout << "    description "<<GetTitle()<<endl;
  cout << "    event types handled are "<<endl;
  for( auto eventtype : eventtypes ) {
    cout << "    event type "<<eventtype<<endl;
  }
  cout << "----------------- good bye ----------------- "<<endl;
}

void THaEvtTypeHandler::EvDump(THaEvData *evdata) const
{
  // Dumps data to file, if fDebugFile was set.
  if (!fDebugFile) return;
  if (!evdata) return;  // get what you deserve
  //FIXME: update for CODA 3
  auto evnum = evdata->GetRawData(4);
  auto len = evdata->GetRawData(0) + 1;
  auto evtype = evdata->GetRawData(1)>>16;
  *fDebugFile << "\n ------ Raw Data Dump ------ "<<endl;
  *fDebugFile << "\n\n Event number " << dec << evnum << endl;
  *fDebugFile << " length " << len << " type " << evtype << endl;
  UInt_t ipt = 0;
  for (UInt_t j=0; j<(len/5); j++) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for (UInt_t k=j; k<j+5; k++) {
      *fDebugFile << hex << evdata->GetRawData(ipt++) << " ";
    }
    *fDebugFile << endl;
  }
  if (ipt < len) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for (UInt_t k=ipt; k<len; k++) {
      *fDebugFile << hex << evdata->GetRawData(ipt++) << " ";
    }
    *fDebugFile << endl;
  }
}

// By default, event type handlers ignore the run database
Int_t THaEvtTypeHandler::ReadRunDatabase( const TDatime& )
{
  return kOK;
}

void THaEvtTypeHandler::SetDebugFile(const char *filename) {
    delete fDebugFile;
    fDebugFile = new ofstream;
    fDebugFile->open(filename);
}

Bool_t THaEvtTypeHandler::IsMyEvent( UInt_t type ) const
{
  return any_of(eventtypes.begin(), eventtypes.end(),
                [type](UInt_t evtype){ return type == evtype; });
}

//_____________________________________________________________________________
void THaEvtTypeHandler::MakePrefix()
{
  THaAnalysisObject::MakePrefix( nullptr );
}

ClassImp(THaEvtTypeHandler)
