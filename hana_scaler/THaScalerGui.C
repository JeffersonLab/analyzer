/////////////////////////////////////////////////////////////////////
//
//   THaScalerGui
//
//   GUI to display scaler data in Hall A
//   version Sept 17, 2001
//   Stuff with "FIXME" needs to be improved.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

// Test mode (generate fake data)
//#define TESTONLY    1
// Some GUI geometry sizes (16 vs 32 channel)
#define YBOXSMALL    370
#define YBOXBIG      700
// Constants for the logic
#define SHOWRATE       1
#define SHOWCOUNT      2
#define OFFSET_START   1
#define OFFSET_QUIT    2
#define OFFSET_HELP    3
#define OFFSET_RATE    4
#define OFFSET_COUNT   5
#define OFFSET_HIST    6
// Show the last TIME_CUT entries into ntuple
#define TIME_CUT      50
// Update time (msec) of TTImer
#define UPDATE_TIME 4000
// Colors
//#define CHGCOLORS 1
#define FRAME1_COLOR   160
#define FRAME2_COLOR   160
#define BUTTON1_COLOR  159
#define BUTTON2_COLOR  157

#include "THaScalerGui.h"
#include "THaScaler.h"
#include "THaScalerBank.h"
#include "THaNormScaler.h"

using namespace std;

#ifndef ROOTPRE3
ClassImp(THaScalerGui)
#endif

THaScalerGui::THaScalerGui(const TGWindow *p, UInt_t w, UInt_t h, string b) : TGMainFrame(p, w, h) { 
  bankgroup = b;
  scaler = new THaScaler(bankgroup.c_str());
  if (scaler->Init() == -1) {
    cout << "ERROR: Cannot initialize THaScaler member"<<endl;
  }
  scalerbanks = scaler->GetScalerBanks();
  if (InitPlots() == -1) {
    cout << "ERROR: Cannot initialize a THaScalerGui"<<endl;
    exit(0);
  }
  timer->TurnOn();
};

THaScalerGui::~THaScalerGui() {
  delete [] yboxsize;
};

Int_t THaScalerGui::InitPlots() {
// Initialize plots (xscaler style)
  timer = new TTimer(this, UPDATE_TIME);
  yboxsize = new Int_t[SCAL_NUMBANK];
  occupied = new Int_t[SCAL_NUMBANK];
  memset(occupied, 0, SCAL_NUMBANK*sizeof(Int_t));
  fDataBuff  = new TGTextBuffer[SCAL_NUMBANK*SCAL_NUMCHAN];
  pair<Int_t, TGTextEntry *> txtpair;
  pair<Int_t, TNtuple *> ntupair;
  char string_ntup[]="UpdateNum:Count:Rate";
  int ipage = 0;  iloop = 0;  lastsize = YBOXSMALL;
  showselect = SHOWRATE;
  TGTab *fTab = new TGTab(this, 600, 800);
  TGLayoutHints *fLayout = new TGLayoutHints(kLHintsNormal | kLHintsExpandX, 10, 10, 10, 10);
  TGLayoutHints *fLayout2 = new TGLayoutHints(kLHintsNormal ,10, 10, 10, 10);
  for (vector<THaScalerBank*>::iterator p = scalerbanks.begin();
       p != scalerbanks.end(); p++) {
     THaScalerBank *scbank = *p;
     if ( !scbank->location.valid() ) continue;
     //    setRcsNames(ipage, scbank);
     setCaloNames(ipage, scbank);

// skip displaying "norm" for dvcs (it is dvcscalo1).  This is ugly.
     if(scaler->GetCrate()==9 && (strcmp(scbank->location.short_desc.c_str(),"norm") == 0)) continue;

     TGCompositeFrame *tgcf = fTab->AddTab(scbank->name.c_str());
#ifdef CHGCOLORS
     tgcf->ChangeBackground(FRAME1_COLOR);
#endif
// FIXME: Better layout decisions (e.g. no strcmp's).
     int nrow = 3;  int isnorm = 0;
     if (ipage >=0 && ipage < SCAL_NUMBANK) yboxsize[ipage] = YBOXSMALL;
     if ( strcmp(scbank->location.short_desc.c_str(),"s2") == 0) {
         nrow = 8;
         if (ipage >= 0 && ipage < SCAL_NUMBANK) yboxsize[ipage] = YBOXBIG;
     }
     if ( (strcmp(scbank->location.short_desc.c_str(),"norm") == 0) ||
          (strcmp(scbank->location.short_desc.c_str(),"nplus") == 0) ||
          (strcmp(scbank->location.short_desc.c_str(),"nminus") == 0) ) {
             nrow = 8;  isnorm = 1;
             if (ipage >=0 && ipage < SCAL_NUMBANK) yboxsize[ipage] = YBOXBIG;
     }
     if (scaler->GetCrate() == 9) {
             nrow = 8;  isnorm = 0;
             if (ipage >=0 && ipage < SCAL_NUMBANK) yboxsize[ipage] = YBOXBIG;
     }
// <-- end of FIXME
     GCValues_t gval;
     gval.fMask = kGCForeground;
     gClient->GetColorByName("black", gval.fForeground);
     GContext_t labelgc 
         = gVirtualX->CreateGC(gClient->GetRoot()->GetId(), &gval);
     TGLabel *fLpage;
     if (isnorm == 0) {
       fLpage = new TGLabel(tgcf, new TGString(scbank->location.long_desc.c_str()),labelgc);
       tgcf->AddFrame(fLpage,fLayout);
     } else {
// FIXME: Preferred that the page name come from THaScalerBank, but did not for norm scalers.
       if (strcmp(scbank->location.short_desc.c_str(),"norm") == 0) {
         fLpage = new TGLabel(tgcf, new TGString("Normalization Data   (NOT gated by helicity)"),labelgc);
         tgcf->AddFrame(fLpage,fLayout);
       }
       if (strcmp(scbank->location.short_desc.c_str(),"nplus") == 0) {
         fLpage = new TGLabel(tgcf, new TGString("Normalization Data ++ gated by helicity PLUS"),labelgc);
         tgcf->AddFrame(fLpage,fLayout);
       }
       if (strcmp(scbank->location.short_desc.c_str(),"nminus") == 0) {
         fLpage = new TGLabel(tgcf, new TGString("Normalization Data -- gated by helicity MINUS"),labelgc);
         tgcf->AddFrame(fLpage,fLayout);
       }
     }
     occupied[ipage] = nrow;
     for (int row = 0; row < nrow; row++) { // rows 
       TGCompositeFrame *fr; 
       fr = new TGCompositeFrame(tgcf, 50, 0, kHorizontalFrame);
       TGHorizontalLayout *fLhorz;
       fLhorz =  new TGHorizontalLayout(fr);
       fr->SetLayoutManager(fLhorz);
       for (int col = 0; col < 4; col++) {  // columns
          int index = ipage*SCAL_NUMCHAN + 4*row+col;
          ntupair.first = index;
          char nname[10];  sprintf(nname,"%d",index);
          ntupair.second = new TNtuple(nname,"scaler",string_ntup); 
          fDataHistory.insert(ntupair);
          TGTextButton *fButton1;
          char cnum[20];
// FIXME:  The buttons take too much space.  Treatment of isnorm is awkward.
  	  if (isnorm == 55) {
  	     if (scbank->GetChanName(4*row+col) == "NULL") {
                char cnum[10];  sprintf(cnum,"%d ==>",4*row+col+1);
                fButton1 = new TGTextButton(fr,new TGHotString(cnum),
                     SCAL_NUMBANK*SCAL_NUMCHAN + OFFSET_HIST + index);
#ifdef CHGCOLORS
                fButton1->ChangeBackground(BUTTON1_COLOR);
#endif
	     } else {   
                fr->AddFrame(fButton1, fLayout);
  	        fButton1 = new TGTextButton(fr,
	           new TGHotString((scbank->GetChanName(4*row+col)).c_str()),
	               SCAL_NUMBANK*SCAL_NUMCHAN + OFFSET_HIST + index);
                fButton1->Associate(this);
#ifdef CHGCOLORS
                fButton1->ChangeBackground(BUTTON1_COLOR);
#endif
	    }
	  } else {
	    if (scbank->GetChanName(4*row+col) == "NULL") {
              sprintf(cnum,"%d ==>",4*row+col+1);
              fButton1 = new TGTextButton(fr,new TGHotString(cnum),
                   SCAL_NUMBANK*SCAL_NUMCHAN + OFFSET_HIST + index);
#ifdef CHGCOLORS
              fButton1->ChangeBackground(BUTTON1_COLOR);
#endif
	    } else {   
              fButton1 = new TGTextButton(fr,
		   new TGHotString((scbank->GetChanName(4*row+col)).c_str()),
                          SCAL_NUMBANK*SCAL_NUMCHAN + OFFSET_HIST + index);
#ifdef CHGCOLORS
              fButton1->ChangeBackground(BUTTON1_COLOR);
#endif
	    }
	  }
          fr->AddFrame(fButton1, fLayout);
          fButton1->Associate(this);
          txtpair.first = index;
          txtpair.second = new TGTextEntry(fr,&fDataBuff[index]);
          fDataEntry.insert(txtpair);
	  fr->AddFrame(fDataEntry[index],fLayout);             
       }
       tgcf->AddFrame(fr,fLayout);
     }       
     ipage++;
  }
  AddFrame(fTab,new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,0,0,0,0));
  TGGroupFrame *fGb = 
     new TGGroupFrame(this, new TGString("Press QUIT to exit, HELP for help.  Click ``Show Rates'' or ``Show Counts''"));
  TGCompositeFrame *fGCb = new TGCompositeFrame(fGb, 10, 0, kHorizontalFrame);
#ifdef CHGCOLORS
  fGCb->ChangeBackground(FRAME2_COLOR);
#endif
  TGHorizontalLayout *fLhorzb =  new TGHorizontalLayout(fGCb);
  fGCb->SetLayoutManager(fLhorzb);
  TGHotString *fLabel = new TGHotString("HELP");
  TGTextButton *fHelp = new TGTextButton(fGCb,fLabel,SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_HELP);
#ifdef CHGCOLORS
  fHelp->ChangeBackground(BUTTON2_COLOR);
#endif
  fGCb->AddFrame(fHelp, fLayout2);
  fHelp->Associate(this);
  fLabel = new TGHotString("QUIT");
  TGTextButton *fQuit = new TGTextButton(fGCb,fLabel,SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_QUIT);
#ifdef CHGCOLORS
  fQuit->ChangeBackground(BUTTON2_COLOR);
#endif
  fGCb->AddFrame(fQuit, fLayout2);
  fQuit->Associate(this);
  TGLabel *fGlabel2 = new TGLabel(fGCb, new TGString("                              "));
  fGCb->AddFrame(fGlabel2, fLayout);
  TGHotString *fHString1 = new TGHotString("Show Rates"); 
  fRateSelect = new TGCheckButton(fGCb,fHString1,SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_RATE);
  fGCb->AddFrame(fRateSelect, fLayout);
  fRateSelect->Associate(this);
  fRateSelect->SetState(kButtonDown);
  TGHotString *fHString2 = new TGHotString("Show Counts"); 
  fCountSelect = new TGCheckButton(fGCb, fHString2,SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_COUNT);
  fGCb->AddFrame(fCountSelect, fLayout);
  fCountSelect->Associate(this);
  fCountSelect->SetState(kButtonUp);
  fGb->AddFrame(fGCb,fLayout2);
  TGLayoutHints *fL1 = new TGLayoutHints(kLHintsNormal | kLHintsExpandX , 10, 10, 10, 10);
  AddFrame(fGb, fL1);
  MapSubwindows();
  Layout();
  SetWindowName("HALL  A   SCALER   DATA");
  SetIconName("Scalers");
  if (scaler->GetCrate() != 9) {
       Resize(900,YBOXSMALL);
  } else {
       Resize(900,YBOXBIG);
  } 
  MapWindow(); 
  return 0;
};

Bool_t THaScalerGui::ProcessMessage(Long_t msg, Long_t parm1, Long_t) {
// Process events generated by the buttons in the frame.
  if (parm1 >= 0 && parm1 < SCAL_NUMBANK) {
      Resize(900,yboxsize[parm1]);
      lastsize = yboxsize[parm1];
  }
  switch (GET_MSG(msg)) {
  case kC_COMMAND:
    if (parm1 >= SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_HIST) {
      int index = parm1-(SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_HIST);
      popPlot(index);
      return kTRUE;
    }
    switch (parm1)  {
      case SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_QUIT:
        exit(0);
      case SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_HELP:
        Help();
        break;
      case SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_RATE:
        showselect = SHOWRATE;
        fCountSelect->SetState(kButtonUp);
        break;
      case SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_COUNT:
        showselect = SHOWCOUNT;
        fRateSelect->SetState(kButtonUp);
        break;
      default:
        break;
     }
  }
  return kTRUE;
};

void THaScalerGui::Help() {
  fHelpDialog = new TRootHelpDialog(this,"BRIEF  HELP  INSTRUCTIONS",350,250);
  fHelpDialog->SetText("xscaler++ treats the data from one bank\nof scalers (e.g. L-arm, R-arm, DVCS Calo bank, etc).\nUsage:\nxscaler [bankgroup]\nTo view a recent history of updates, press on\nthe button corresponding to the channel and\na canvas will pop up.\nClick ``Show Rates'' or ``Show Counts'' to switch\nbetween rate display and accumulated counts.\n\nSupport: Robert Michaels, JLab Hall A\nDocumentation:\nhallaweb.jlab.org/equipment/daq/THaScaler.html");
  fHelpDialog->Popup();
};

Bool_t THaScalerGui::HandleTimer(TTimer *t) {
  iloop++;  
#ifdef TESTONLY
  cout << "Test data (not real).   loop = "<<iloop<<endl;
#endif
  updateValues();
  if ( (iloop%20000) == 0 ) clearNtuples();
  return true;
};

void THaScalerGui::updateValues() {
  static int ipage,row,col,base,index;
  static int chanlo,chanhi,chanstart,chanstop;
  static int slot,chan;
  static char value[20];
  static Float_t farray_ntup[3];
  static float rate,count;
  static map< Int_t, TGTextEntry* >::iterator txt;
  static map< Int_t, TNtuple* >::iterator ntu;
#ifndef TESTONLY
// FIXME: A major problem with LoadDataOnline(): mapping of slots to headers was "hard coded".
  if (scaler->LoadDataOnline() == SCAL_ERROR) {
      cout << "Error loading data online"<<endl;
      return;
  }
#endif
  ipage = 0;
  for (vector<THaScalerBank*>::iterator p = scalerbanks.begin();
     p != scalerbanks.end(); p++) {   
     THaScalerBank *scbank = *p;
     if ( !scbank->location.valid() ) continue;
     int isnorm = 0;
// FIXME:  This case resolution is awkward
     if ( (strcmp(scbank->location.short_desc.c_str(),"norm") == 0) ||
          (strcmp(scbank->location.short_desc.c_str(),"nplus") == 0) ||
          (strcmp(scbank->location.short_desc.c_str(),"nminus") == 0) ) {
             isnorm = 1;
     }
// FIXME:  The Gui should not need to compute chanlo, chanhi.  Get it from THaScalerBank.
     chanlo = 9999;
     chanhi = 0;
     for (int k = 0; k < (long)scbank->location.startchan.size(); k++) {
         chanstart = scbank->location.startchan[k];
         chanstop = chanstart + scbank->location.numchan[k];
         if (chanstart <= chanlo) chanlo = chanstart;
         if (chanstop >= chanhi) chanhi = chanstop;
     }
     slot = scbank->location.slot;     
     for (row = 0; row < occupied[ipage]; row++) {  
       for (col = 0; col < 4; col++) {
          index = ipage*SCAL_NUMCHAN + 4*row+col;
          base = 0;
          chan = chanlo + 4*row + col;  
          if (chan > chanhi & !isnorm) continue;
#ifdef TESTONLY
          base = 10000*iloop;
          rate = base + 1000*row + 100*col;
          count = rate + 1e5;
#else
          count = (float)scaler->GetScaler(slot, chan);
          rate  = (float)scaler->GetScalerRate(slot, chan);
#endif
          switch (showselect) {
             case SHOWRATE:
                 sprintf(value,"%-6.3e Hz",rate);
                 break;
             case SHOWCOUNT:
                 sprintf(value,"%-6.4e",count);
                 break;
             default:
   	         cout << "WARNING: Not updating rates or counts "<<endl;
                 break;
          }      
          fDataBuff[index].Clear();
          fDataBuff[index].AddText(0,value);
          txt = fDataEntry.find(index);
          if (txt != fDataEntry.end()) {
              fClient->NeedRedraw(txt->second);
          }
          ntu = fDataHistory.find(index);
          if (ntu != fDataHistory.end()) {
	     farray_ntup[0] = (float) iloop;
             farray_ntup[1] = count;
             farray_ntup[2] = rate;                      
	     if (iloop > 1) fDataHistory[index]->Fill(farray_ntup);
          }
       }
     }
     ipage++;
  }
  return;
};

void THaScalerGui::setRcsNames(int ipage, THaScalerBank *scbank) {
  if (scaler->GetCrate() != 9) return;
  int i;
  static char cname[10];
  if (ipage == 0) {
    for (i = 0; i < 32; i++) {
      if (i+1 < 10) {
          sprintf(cname,"gsum0%d",i+1);
      } else {
          sprintf(cname,"gsum%d",i+1);
      }
      scbank->SetChanName(i,cname);
    }
  }
  if (ipage == 1) {
    for (i = 0; i < 24; i++) {
      sprintf(cname,"gsum%d",i+33);
      scbank->SetChanName(i,cname);
    }
    for (i = 24; i < 32; i++) {
      sprintf(cname,"misc%d",i-23);
      scbank->SetChanName(i,cname);
    }
  }
  if (ipage == 2) {
    for (i = 0; i < 32; i++) {
      if (i+9 < 10) {
          sprintf(cname,"gsum0%d",i+9);
      } else {
          sprintf(cname,"gsum%d",i+9);
      }
      scbank->SetChanName(i,cname);
    }
  }
};

void THaScalerGui::setCaloNames(int ipage, THaScalerBank *scbank) {
  if (scaler->GetCrate() != 9) return;
  int i;
  static char cname[10];
  if (ipage == 0) {
    for (i = 0; i < 32; i++) {
      if (i+1 < 10) {
          sprintf(cname,"calo0%d",i+1);
      } else {
          sprintf(cname,"calo%d",i+1);
      }
      scbank->SetChanName(i,cname);
    }
  }
  if (ipage == 1) {
    for (i = 0; i < 32; i++) {
      sprintf(cname,"calo%d",i+33);
      scbank->SetChanName(i,cname);
    }
  }
};

void THaScalerGui::popPlot(int index) {
  static char timecut[100],ctit[100];
  static map< Int_t, TNtuple* >::iterator ntu = fDataHistory.find(index);
  static float upd = UPDATE_TIME/1000;  
  if (ntu != fDataHistory.end()) {
      sprintf(timecut,"%6.1f-UpdateNum<%d",
         fDataHistory[index]->GetMaximum("UpdateNum"),TIME_CUT);        
      TCut tc(timecut);
      if (showselect == SHOWRATE) {
  	  sprintf(ctit,"RECENT HISTORY of RATE (Hz) updated each %3.0f sec",upd);
	  TCanvas *c1 = new TCanvas("Rate",ctit,500,400);
          fDataHistory[index]->SetMarkerColor(4);
          fDataHistory[index]->SetMarkerStyle(21);
          fDataHistory[index]->Draw("Rate:UpdateNum",tc);
      } else {
  	  sprintf(ctit,"RECENT HISTORY of COUNTS updated each %3.0f sec",upd);
	  TCanvas *c1 = new TCanvas("Rate",ctit,500,400);
          fDataHistory[index]->SetMarkerColor(2);
          fDataHistory[index]->SetMarkerStyle(22);
          fDataHistory[index]->Draw("Count:UpdateNum",tc);
      }
  }
};

void THaScalerGui::clearNtuples() {
  static int i,j,index;
  static map< Int_t, TNtuple* >::iterator ntu;
  for (i = 0; i < SCAL_NUMBANK; i++) {
     for (j = 0; j < SCAL_NUMCHAN; j++) {
       index = i*SCAL_NUMCHAN+j;
       if ( !occupied[index] ) continue;
       ntu = fDataHistory.find(index);
       if (ntu != fDataHistory.end()) {
         fDataHistory[index]->Reset(0);
       }
     }
  }
  return;
};









