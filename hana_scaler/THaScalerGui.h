#ifndef THaScalerGui_
#define THaScalerGui_

/////////////////////////////////////////////////////////////////////
//
//   THaScalerGui
//
//   xscaler GUI to display scaler data in Hall A.  
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TGCanvas.h"
#include "TPad.h"
#include "TGTab.h"
#include "TGFrame.h"
#include "TGMenu.h"
#include "TSystem.h"
#include "TGLayout.h"
#include "TGLabel.h"
#include "TGDimension.h"
#include "TGButton.h"
#include "TGTextBuffer.h"
#include "TGTextEntry.h"
#include "TFile.h"
#include "TTimer.h"
#include "TNtuple.h"
#include "TCanvas.h"
#include "TCut.h"
#include "TRootHelpDialog.h"
#include "THaScaler.h"
#include <map>

class THaScaler;

class THaScalerGui : public TGMainFrame {

public:

   THaScalerGui(const TGWindow *p, UInt_t width, UInt_t height, std::string bankgr);
   virtual ~THaScalerGui();
   Int_t InitPlots();    

private:
   Int_t npages,iloop,showselect,lastsize;
   TGMainFrame *ScalGui;
   TGTab *fTab;
   TGCompositeFrame *tgcf;
   TGTextBuffer *fDataBuff;
   std::map<Int_t, TGTextEntry *> fDataEntry;
   std::map<Int_t, TNtuple*> fDataHistory;
   std::map<Int_t, Int_t> slotmap;
   TTimer *timer;
   TGCheckButton *fRateSelect, *fCountSelect;
   TRootHelpDialog *fHelpDialog;
   Int_t InitFromDB();
   Bool_t ProcessMessage(Long_t , Long_t , Long_t);
   Bool_t HandleTimer(TTimer *t);
   void updateValues();
   void InitPages();
   void popPlot(int index);
   THaScaler *scaler;
   void clearNtuples();
   void TestInput();          
   void Help();
   Int_t crate;
   std::string bankgroup;
   int *yboxsize;
   int *occupied;

};

#endif








