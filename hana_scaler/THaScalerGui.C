/////////////////////////////////////////////////////////////////////
//
//   THaScalerGui
//
//   "xscaler" GUI to display scaler data in Hall A
//   version Jan 2005
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

// Test mode (generate fake data)
// #define TESTONLY    1
// Some GUI geometry sizes (16 vs 32 channel)
#define YBOXSMALL    460
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

#include <vector>
#include <string>
#include "THaScalerGui.h"
#include "THaScaler.h"
#include "THaScalerDB.h"

using namespace std;

THaScalerGui::THaScalerGui(const TGWindow *p, UInt_t w, UInt_t h, string b) : TGMainFrame(p, w, h) { 
  bankgroup = b;
  scaler = 0;
  timer = 0;
  crate = 0;
  scaler = new THaScaler(bankgroup.c_str());
  if (scaler->Init() == -1) {
    cout << "ERROR: Cannot initialize THaScaler member"<<endl;
  }
  crate = scaler->GetCrate();
  InitFromDB();
  if (InitPlots() == -1) {
    cout << "ERROR: Cannot initialize a THaScalerGui"<<endl;
    exit(0);
  }
  timer = new TTimer(this, UPDATE_TIME);
  timer->TurnOn();
};

THaScalerGui::~THaScalerGui() {
  delete [] yboxsize;
  if (scaler) delete scaler;
  if (timer) delete timer;
};


Int_t THaScalerGui::InitFromDB() {
// Adjust some parameters of xscaler from DB if they were set there.
// Otherwise the parameters are defaults which are probably ok.
  if (!scaler) return -1;
  if ( !scaler->GetDataBase()) return -1;
  std::string server,port,clkchan,clkslot,clkrate;
  server = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-server", "IP"); 
  if (server != "none") {
    scaler->SetIpAddress(server);
    cout << "Setting server IP = "<<server<<endl;
  } 
  port = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-server", "port"); 
  if (port != "none") {
    scaler->SetPort(atoi(port.c_str()));
    cout << "Setting server port = "<<port<<endl;
  } 
  clkslot = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-clock", "slot"); 
  clkchan = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-clock", "chan"); 
  clkrate = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-clock", "rate"); 
  if (clkchan != "none") {
    if (clkslot != "none") {
      cout << "Setting clock location, slot = "<<clkslot;
      cout << "  chan = "<<clkchan<<endl;
      scaler->SetClockLoc(atoi(clkslot.c_str()), atoi(clkchan.c_str()));
    } else {
      cout << "Setting clock location, chan = "<<clkchan<<endl;
      scaler->SetClockLoc(-1, atoi(clkchan.c_str()));
    }
  }
  if (clkrate != "none") {
      cout << "Setting clock rate "<<clkrate<<endl;
      scaler->SetClockRate((Double_t)atoi(clkrate.c_str()));
  }
  return 0;
}

Int_t THaScalerGui::InitPlots() {
// Initialize plots (xscaler style)
  if (!scaler) {
    cout << "THaScalerGui::ERROR: no scaler defined... cannot init."<<endl;
    return -1;
  }
  InitPages();
  yboxsize = new Int_t[SCAL_NUMBANK];
  occupied = new Int_t[SCAL_NUMBANK];
  memset(occupied, 0, SCAL_NUMBANK*sizeof(Int_t));
  fDataBuff  = new TGTextBuffer[SCAL_NUMBANK*SCAL_NUMCHAN];
  pair<Int_t, TGTextEntry *> txtpair;
  pair<Int_t, TNtuple *> ntupair;
  char string_ntup[]="UpdateNum:Count:Rate";
  iloop = 0;  lastsize = YBOXSMALL;
  showselect = SHOWRATE;
  TGTab *fTab = new TGTab(this, 600, 800);
  TGLayoutHints *fLayout = new TGLayoutHints(kLHintsNormal | kLHintsExpandX, 10, 10, 10, 10);
  TGLayoutHints *fLayout2 = new TGLayoutHints(kLHintsNormal ,10, 10, 10, 10);
  if (!scaler->GetDataBase()) {
    cout << "THaScalerGui::WARNING: no database.  Will use defaults..."<<endl;
  }
  Int_t crate = scaler->GetCrate();
  std::string sdirect;
  for (Int_t ipage = 0; ipage < npages; ipage++) {
    Int_t slot = slotmap[ipage];
    char cpage[100]; sprintf(cpage,"%d",ipage);
    std::string spage = cpage;
    sdirect = "none";
    if (scaler->GetDataBase()) 
      sdirect = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-tabs", spage.c_str());
    if (sdirect != "none") spage = sdirect;
    TGCompositeFrame *tgcf = fTab->AddTab(spage.c_str());
    std::string slayout = "none";
    if (scaler->GetDataBase()) 
      slayout = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-layout", (std::string)cpage);
    UInt_t pos = slayout.find("x");
    int nrow = 8;  int ncol = 4;  // defaults
    if (pos != std::string::npos) {
      nrow = atoi(slayout.substr(0,pos).c_str());
      ncol = atoi(slayout.substr(pos+1,slayout.length()).c_str());
    }
    yboxsize[ipage] = YBOXSMALL;
    if (nrow * ncol > 16) yboxsize[ipage] = YBOXBIG;
    GCValues_t gval;
    gval.fMask = kGCForeground;
    gClient->GetColorByName("black", gval.fForeground);
    GContext_t labelgc = gVirtualX->CreateGC(gClient->GetRoot()->GetId(), &gval);
    TGLabel *fLpage;
    std::string pagename = "none";
    if (scaler->GetDataBase()) 
      pagename = scaler->GetDataBase()->GetStringDirectives(crate, "xscaler-pagename", (std::string)cpage);
    if (pagename == "none") pagename = spage;
    fLpage = new TGLabel(tgcf, new TGString(pagename.c_str()),labelgc);
    tgcf->AddFrame(fLpage,fLayout);
    occupied[ipage] = nrow;
    Int_t chan = 0;
    for (int row = 0; row < nrow; row++) { // rows 
       TGCompositeFrame *fr; 
       fr = new TGCompositeFrame(tgcf, 50, 0, kHorizontalFrame);
       TGHorizontalLayout *fLhorz;
       fLhorz =  new TGHorizontalLayout(fr);
       fr->SetLayoutManager(fLhorz);
       for (int col = 0; col < ncol; col++) {  // columns
          int index = ipage*SCAL_NUMCHAN + ncol*row+col;
          ntupair.first = index;
          char nname[10];  sprintf(nname,"%d",index);
          ntupair.second = new TNtuple(nname,"scaler",string_ntup); 
          fDataHistory.insert(ntupair);
          TGTextButton *fButton1;
          char cbutton[100];
	  std::string buttonname = "none";
          if (scaler->GetDataBase()) { 
            std::vector<std::string> strb = 
               scaler->GetDataBase()->GetShortNames(crate,slot,chan);
  	    buttonname = strb[0];
	  }
          if (buttonname == "none") {
            sprintf(cbutton,"%d ==>",ncol*row+col+1);
          } else {
            sprintf(cbutton,buttonname.c_str());
	  }
          fButton1 = new TGTextButton(fr,new TGHotString(cbutton),
                   SCAL_NUMBANK*SCAL_NUMCHAN + OFFSET_HIST + index);
          fr->AddFrame(fButton1, fLayout);
          fButton1->Associate(this);
          txtpair.first = index;
          txtpair.second = new TGTextEntry(fr,&fDataBuff[index]);
          fDataEntry.insert(txtpair);
	  fr->AddFrame(fDataEntry[index],fLayout);             
 	  chan++;
       }
       tgcf->AddFrame(fr,fLayout);
     }       
  }
  AddFrame(fTab,new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,0,0,0,0));
  TGGroupFrame *fGb = 
     new TGGroupFrame(this, new TGString("Click channel button for history plot.  Click ``Show Rates'' or ``Show Counts''"));
  TGCompositeFrame *fGCb = new TGCompositeFrame(fGb, 10, 0, kHorizontalFrame);
  TGHorizontalLayout *fLhorzb =  new TGHorizontalLayout(fGCb);
  fGCb->SetLayoutManager(fLhorzb);
  TGHotString *fLabel = new TGHotString("HELP");
  TGTextButton *fHelp = new TGTextButton(fGCb,fLabel,SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_HELP);
  fGCb->AddFrame(fHelp, fLayout2);
  fHelp->Associate(this);
  fLabel = new TGHotString("QUIT");
  TGTextButton *fQuit = new TGTextButton(fGCb,fLabel,SCAL_NUMBANK*SCAL_NUMCHAN+OFFSET_QUIT);
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
  MapWindow(); 
  Resize(900,yboxsize[0]);
  lastsize = yboxsize[0];
  return 0;
};

void THaScalerGui::InitPages() {
  Int_t ipage, slot;
  static char value[50];
  slotmap.clear();
  if (!scaler) return;
  if (scaler->GetDataBase())  
    npages = scaler->GetDataBase()->GetNumDirectives(crate, "xscaler-layout"); 
  if (npages == 0) npages = 10;  // reasonable default
  for (ipage = 0; ipage < npages; ipage++) {
    slot = ipage;
    if ( scaler->GetDataBase()) {
      sprintf(value, "%d", ipage);
      string spage = value;
      string detname = scaler->GetDataBase()->GetStringDirectives(crate,"xscaler-pageslot",spage);
      if (detname.find("slot") != std::string::npos) {
	sscanf(detname.c_str(), "slot%d", &slot); 
      } else {
        if (detname != "none") {
          slot = scaler->GetSlot(detname);
	}
      }
    }
    slotmap.insert(make_pair(ipage, slot));
  }
}

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
  fHelpDialog->SetText("xscaler++ treats the data from one bank\nof scalers (e.g. L-arm, R-arm, etc).\nUsage:\nxscaler [bankgroup]\nTo view a recent history of updates, press on\nthe button corresponding to the channel and\na canvas will pop up.\nClick ``Show Rates'' or ``Show Counts'' to switch\nbetween rate display and accumulated counts.\n\nSupport: Robert Michaels, JLab Hall A\nDocumentation:\nhallaweb.jlab.org/equipment/daq/THaScaler.html");
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
  static int ipage,slot,chan,index;
  static char value[50];
  static Float_t farray_ntup[3];
  static float rate,count;
  static map< Int_t, TGTextEntry* >::iterator txt;
  static map< Int_t, TNtuple* >::iterator ntu;
#ifndef TESTONLY
  if (scaler->LoadDataOnline() == SCAL_ERROR) {
      cout << "Error loading data online"<<endl;
      return;
  }
#endif
  for (ipage = 0; ipage < npages; ipage++) {
    slot = slotmap[ipage];
    for (chan = 0; chan < SCAL_NUMCHAN; chan++) {
       index = ipage*SCAL_NUMCHAN + chan;
#ifdef TESTONLY
       rate = 1000*(slot+1) + 100*(chan+1) + iloop; 
       count = rate + 100000;
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
  return;
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









