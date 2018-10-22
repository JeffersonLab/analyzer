#include "THaVDCSimRun.h"
#include "THaVDCSim.h"

#include "TFile.h"
#include "TTree.h"
#include "TError.h"
#include <cstdio>
#include <evio.h>

using namespace std;

//-----------------------------------------------------------------------------
THaVDCSimRun::THaVDCSimRun(const char* filename, const char* description) :
  THaRunBase(description), rootFileName(filename), rootFile(0), tree(0), 
  branch(0), event(0), nentries(0), entry(0)
{
  // Constructor

  // Default file name if none given
  if( rootFileName.IsNull() )
    rootFileName = "vdctracks.root";
}

//-----------------------------------------------------------------------------
THaVDCSimRun::THaVDCSimRun(const THaVDCSimRun &run)
  : THaRunBase(run), nentries(0), entry(0)
{
  rootFileName = run.rootFileName;
  rootFile = NULL;
  tree = NULL;
  event = NULL;
  branch = NULL;
}

//-----------------------------------------------------------------------------
THaVDCSimRun& THaVDCSimRun::operator=(const THaRunBase& rhs)
{
  if (this != &rhs) {
    THaRunBase::operator=(rhs);
    if( rhs.InheritsFrom("THaVDCSimRun") )
      rootFileName = static_cast<const THaVDCSimRun&>(rhs).rootFileName;
    rootFile = NULL;
    tree = NULL;
    event = NULL;
  }
  return *this;
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::Init()
{
  // Use the date we're familiar with, so we can use those channel mappings
  fDate.Set(2009,12,22,12,0,0);
  fAssumeDate = kTRUE;
  fDataSet |= kDate;

  return THaRunBase::Init();
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::Open()
{
  rootFile = new TFile(rootFileName, "READ", "VDC Tracks");
  if (!rootFile || rootFile->IsZombie()) {
    if (rootFile->IsOpen()) Close();
    return -1;
  }

  event = 0;

  tree = static_cast<TTree*>(rootFile->Get("tree"));
  if( !tree ) {
    Error( "THaVDCSimRun:Open", 
	   "Tree 'tree' does not exist in the input file. Have a nice day." );
    return -2;
  }
  branch = tree->GetBranch("event");
  if( !branch ) {
    Error( "THaVDCSimRun:Open", 
	   "Branch 'event' does not exist in the input tree. Have a nice day." );
    return -3;
  }
  branch->SetAddress(&event);

  nentries = static_cast<Int_t>(tree->GetEntries());
  entry = 0;

  fOpened = kTRUE;
  return 0;
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::Close() {
  if (rootFile) {
    rootFile->Close();
    delete rootFile;
    rootFile = 0;
  }
  fOpened = kFALSE;
  return 0;
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::ReadEvent() {
  Int_t ret;
  if (!IsOpen()) {
    ret = Open();
    if (ret) return ret;
  }

  // Clear the event to get rid of anything still hanging around
  event->Clear();
  ret = branch->GetEntry(entry++);
  if( ret > 0 )
    return S_SUCCESS;
  else if ( ret == 0 )
    return EOF;
  return -128;  // CODA_ERR
}

//-----------------------------------------------------------------------------
const Int_t *THaVDCSimRun::GetEvBuffer() const {
  if (!IsOpen()) return NULL;

  return reinterpret_cast<Int_t*>(event);
}

//-----------------------------------------------------------------------------
THaVDCSimRun::~THaVDCSimRun() {
  if (IsOpen())
    Close();
}

//-----------------------------------------------------------------------------
ClassImp(THaVDCSimRun)
