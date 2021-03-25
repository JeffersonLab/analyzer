#include "THaVDCSimRun.h"
#include "THaVDCSim.h"

#include "TFile.h"
#include "TTree.h"
#include "TError.h"

using namespace std;

//-----------------------------------------------------------------------------
THaVDCSimRun::THaVDCSimRun(const char* filename, const char* description) :
  THaRunBase(description), rootFileName(filename), rootFile(nullptr),
  tree(nullptr), branch(nullptr), event(nullptr), nentries(0), entry(0)
{
  // Constructor

  // Default file name if none given
  if( rootFileName.IsNull() )
    rootFileName = "vdctracks.root";
}

//-----------------------------------------------------------------------------
THaVDCSimRun::THaVDCSimRun(const THaVDCSimRun &run)
  : THaRunBase(run), rootFileName(run.rootFileName), rootFile(nullptr),
    tree(nullptr), branch(nullptr), event(nullptr), nentries(0), entry(0)
{
}

//-----------------------------------------------------------------------------
THaVDCSimRun& THaVDCSimRun::operator=(const THaRunBase& rhs)
{
  if (this != &rhs) {
    THaRunBase::operator=(rhs);
    if( rhs.InheritsFrom("THaVDCSimRun") )
      rootFileName = static_cast<const THaVDCSimRun&>(rhs).rootFileName;
    rootFile = nullptr;
    tree = nullptr;
    event = nullptr;
  }
  return *this;
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::Init()
{
  // Use the date we're familiar with, so we can use those channel mappings
  fDate.Set(2009,12,22,12,0,0);
  fAssumeDate = true;
  fDataSet |= kDate;

  return THaRunBase::Init();
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::Open()
{
  rootFile = new TFile(rootFileName, "READ", "VDC Tracks");
  if (!rootFile || rootFile->IsZombie()) {
    if (rootFile->IsOpen()) Close();
    return READ_FATAL;
  }

  event = nullptr;

  tree = static_cast<TTree*>(rootFile->Get("tree"));
  if( !tree ) {
    Error( "THaVDCSimRun:Open", 
	   "Tree 'tree' does not exist in the input file. Have a nice day." );
    return READ_FATAL;
  }
  branch = tree->GetBranch("event");
  if( !branch ) {
    Error( "THaVDCSimRun:Open", 
	   "Branch 'event' does not exist in the input tree. Have a nice day." );
    return READ_FATAL;
  }
  branch->SetAddress(&event);

  nentries = static_cast<Int_t>(tree->GetEntries());
  entry = 0;

  fOpened = true;
  return 0;
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::Close() {
  if (rootFile) {
    rootFile->Close();
    delete rootFile;
    rootFile = nullptr;
  }
  fOpened = false;
  return READ_OK;
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimRun::ReadEvent() {
  if (!IsOpen()) {
    Int_t ret = Open();
    if (ret) return ret;
  }

  // Clear the event to get rid of anything still hanging around
  event->Clear();
  Int_t ret = branch->GetEntry(entry++);
  if( ret > 0 )
    return READ_OK;
  else if ( ret == 0 )
    return READ_EOF;
  return READ_ERROR;
}

//-----------------------------------------------------------------------------
const UInt_t* THaVDCSimRun::GetEvBuffer() const {
  if (!IsOpen()) return nullptr;

  return reinterpret_cast<const UInt_t*>(event);
}

//-----------------------------------------------------------------------------
THaVDCSimRun::~THaVDCSimRun() {
  if (IsOpen())
    Close();
}

//-----------------------------------------------------------------------------
ClassImp(THaVDCSimRun)
