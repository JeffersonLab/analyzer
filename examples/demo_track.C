///////////////////////////////////////////////////////////////////////////////
// Macro for demonstration of results of track reconstruction by using the   //
// unique clusters on each of VDC planes U1, V1, U2, V2.                     //
// This version for Analyzer v0.60.                                          //
///////////////////////////////////////////////////////////////////////////////
{
// Demonstration distributions:

// Reconstructed track parameters on the VDC U1 plane: -------------------------
  ctrpar = new TCanvas("ctrpar","Reconstructed track parameters");
  ctrpar->Divide(3,2);
  ctrpar->cd(1);
  T->Draw("fR_TR_n");
  ctrpar->cd(2);
  T->Draw("fR_TR_x","fR_TR_n == 1 && -80<fR_TR_x && fR_TR_x<80");
  ctrpar->cd(3);
  T->Draw("fR_TR_y","fR_TR_n == 1 && -10<fR_TR_y && fR_TR_y<10");
  ctrpar->cd(4);
  T->Draw("fR_TR_z","fR_TR_n == 1");
  ctrpar->cd(5);
  T->Draw("fR_TR_th","fR_TR_n == 1 && -0.25<fR_TR_th && fR_TR_th<0.25");
  ctrpar->cd(6);
  T->Draw("fR_TR_ph","fR_TR_n == 1 && -0.25<fR_TR_ph && fR_TR_ph<0.25");
  ctrpar->Update();

// Track points coordinates on Shower and Preshower planes: --------------------
  ctrsh=new TCanvas("ctrsh","Track and (Pre)Shower plane cross points");
  ctrsh->Divide(2,2);
  ctrsh->cd(1);
  T->Draw("fR_PSH_trx","fR_TR_n == 1 && -120<fR_PSH_trx && fR_PSH_trx<120");
  ctrsh->cd(2);
  T->Draw("fR_PSH_try","fR_TR_n == 1 && -45<fR_PSH_try && fR_PSH_try<45");
  ctrsh->cd(3);
  T->Draw("fR_SHR_trx","fR_TR_n == 1 && -120<fR_SHR_trx && fR_SHR_trx<120");
  ctrsh->cd(4);
  T->Draw("fR_SHR_try","fR_TR_n == 1 && -45<fR_SHR_try && fR_SHR_try<45");
  ctrsh->Update();

  ctrshc=new TCanvas("ctrshc","Track and (Pre)Shower plane cross points");
  TCut ctr1 = "fR_TR_n==1";
  TCut cpsx = "-40<fR_PSH_trx && fR_PSH_trx<40";
  TCut cpsy = "-40<fR_PSH_try && fR_PSH_try<40";
  TCut cshx = "-40<fR_SHR_trx && fR_SHR_trx<40";
  TCut cshy = "-40<fR_SHR_try && fR_SHR_try<40";
  ctrshc->Divide(3,2);
  ctrshc->cd(1);
  TH2F *h1 = new TH2F("h1","PSH: X vs Y, when Block 23",100,-30,30,100,-30,30);
  T->Draw("fR_PSH_trx:fR_PSH_try>>h1",ctr1&&cpsx&&cpsy&&"fR_PSH_adc_c[22]>300");
  ctrshc->cd(2);
  TH2F *h2 = new TH2F("h2","PSH: X vs Y, when Block 25",100,-30,30,100,-30,30);
  T->Draw("fR_PSH_trx:fR_PSH_try>>h2",ctr1&&cpsx&&cpsy&&"fR_PSH_adc_c[24]>300");
  ctrshc->cd(3);
  TH2F *h3 = new TH2F("h3","PSH: X vs Y, when Block 27",100,-30,30,100,-30,30);
  T->Draw("fR_PSH_trx:fR_PSH_try>>h3",ctr1&&cpsx&&cpsy&&"fR_PSH_adc_c[26]>300");

  ctrshc->cd(4);
  TH2F *h4 = new TH2F("h4","SHR: X vs Y, when Block 39",100,-30,30,100,-30,30);
  T->Draw("fR_SHR_trx:fR_SHR_try>>h4",ctr1&&cshx&&cshy&&"fR_SHR_adc_c[38]>300");
  ctrshc->cd(5);
  TH2F *h5 = new TH2F("h5","SHR: X vs Y, when Block 45",100,-30,30,100,-30,30);
  T->Draw("fR_SHR_trx:fR_SHR_try>>h5",ctr1&&cshx&&cshy&&"fR_SHR_adc_c[44]>200");
  ctrshc->cd(6);
  TH2F *h6 = new TH2F("h6","SHR: X vs Y, when Block 51",100,-30,30,100,-30,30);
  T->Draw("fR_SHR_trx:fR_SHR_try>>h6",ctr1&&cshx&&cshy&&"fR_SHR_adc_c[50]>300");

// Track points coordinates on Scintillator 1 plane: ---------------------------
  cs1xvy = new TCanvas("cs1xvy","Track points on the S1 plane");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.03,0.25,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.27,0.03,0.49,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.51,0.03,0.73,0.97);
  TPad *pad4 = new TPad("pad4","This is pad4",0.75,0.03,0.97,0.97);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  TCut c1tr = "fR_TR_n==1";
  TCut cxlm = "-100<fR_S1_trx && fR_S1_trx<100";
  TCut cylm = "-10<fR_S1_try && fR_S1_try<10";
  TCut cp3f = "500<fR_S1L_adc_c[2] || 500<fR_S1R_adc_c[2]";
  TCut cp4f = "600<fR_S1L_adc_c[3] || 400<fR_S1R_adc_c[3]";
  TCut cp5f = "500<fR_S1L_adc_c[4] || 500<fR_S1R_adc_c[4]";
  pad1->cd();
  pad1->SetGrid();
  TH2F *h7 = new TH2F("h7","S1: X vs Y", 100,-9,9,100,-70,70);
  T->Draw("fR_S1_trx:fR_S1_try>>h7", c1tr && cxlm && cylm );
  pad2->cd();
  pad2->SetGrid();
  TH2F *h8 = new TH2F("h8","S1: X vs Y, when Pad 3", 100,-9,9,100,-70,70);
  T->Draw("fR_S1_trx:fR_S1_try>>h8", c1tr && cxlm && cylm && cp3f );
  pad3->cd();
  pad3->SetGrid();
  TH2F *h9 = new TH2F("h9","S1: X vs Y, when Pad 4", 100,-9,9,100,-70,70);
  T->Draw("fR_S1_trx:fR_S1_try>>h9", c1tr && cxlm && cylm && cp4f );
  pad4->cd();
  pad4->SetGrid();
  TH2F *h10 = new TH2F("h10","S1: X vs Y, when Pad 5", 100,-9,9,100,-70,70);
  T->Draw("fR_S1_trx:fR_S1_try>>h10", c1tr && cxlm && cylm && cp5f );

  cs1x = new TCanvas("cs1x","Coordinates of track point on the S1 plane");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  pad1->SetGrid();
  T->Draw("fR_S1_trx", c1tr && cxlm );
  pad2->cd();
  pad2->SetGrid();
  T->Draw("fR_S1_try", c1tr && cylm );
  pad3->cd();
  pad3->SetGrid();
  T->Draw("fR_S1_trx", c1tr && cp4f && "-40<fR_S1_trx && fR_S1_trx<40");
  pad4->cd();
  pad4->SetGrid();
  T->Draw("fR_S1_trx", c1tr && cp3f && cp4f && "-40<fR_S1_trx && fR_S1_trx<40");
}
