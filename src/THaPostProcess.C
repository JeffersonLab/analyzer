#include "THaPostProcess.h"

#include "TList.h"

TList* gHaPostProcess=0;
THaPostProcess::THaPostProcess() : fIsInit(0) {
  if (!gHaPostProcess) gHaPostProcess = new TList;
  if (gHaPostProcess) gHaPostProcess->Add(this);
}

THaPostProcess::~THaPostProcess() {
  if (gHaPostProcess) gHaPostProcess->Remove(this);
}

ClassImp(THaPostProcess)
