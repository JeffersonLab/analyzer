#ifndef THaScalerGui_
#define THaScalerGui_


/////////////////////////////////////////////////////////////////////
//
//   THaScalerGui
//
//   GUI to display scaler data in Hall A
//   (Status :  May 2001, not finished).
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
#include "TRootHelpDialog.h"
#include "THaScaler.h"

class THaScaler;
class THaScalerBank;

class THaScalerGui : public TGMainFrame {

public:

   THaScalerGui(const TGWindow *p, UInt_t width, UInt_t height, std::string bankgr);
   virtual ~THaScalerGui();
   Int_t InitPlots();    // Initialize plots (xscaler style)

private:
   Int_t iloop,showselect,lastsize;
   TGMainFrame *ScalGui;
   TGTab *fTab;
   TGCompositeFrame *tgcf;
   TGTextBuffer *fDataBuff;
   map<Int_t, TGTextEntry *> fDataEntry;
   map<Int_t, TNtuple*> fDataHistory;
   TTimer *timer;
   std::vector < THaScalerBank* > scalerbanks;
   TGCheckButton *fRateSelect, *fCountSelect;
   TRootHelpDialog *fHelpDialog;
   Bool_t ProcessMessage(Long_t , Long_t , Long_t);
   Bool_t HandleTimer(TTimer *t);
   void updateValues();
   void popPlot(int index);
   THaScaler *scaler;
   void setRcsNames(int ipage, THaScalerBank *bank);
   void clearNtuples();
   void TestInput();          // Test function
   void Help();
   std::string bankgroup;
   int *yboxsize;
   int *occupied;


#ifndef ROOTPRE3
ClassDef(THaScalerGui,0)  // GUI to display scaler data in Hall A (xscaler style)
#endif


};

#endif








