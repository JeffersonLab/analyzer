///////////////////////////////////////////////////////////////////////////////
//   Macro for demonstration of Scintillator 2 detector parameters.          //
//   This version for Analyzer v0.60.                                        //
///////////////////////////////////////////////////////////////////////////////
{
// Demonstration distributions:

  cs2r1 = new TCanvas("cs2r1", "Scintillator 2 Right Demo Canvas 1");
  cs2r1->Divide(4,2);                // Divide the Canvas to 8 regions (4x2).
  cs2r1->cd(1);
  T->Draw("fR_S2L_nthit");          // Number of Left paddles TDCs.
  cs2r1->cd(2);
  T->Draw("fR_S2R_nthit");          // Number of Right paddles TDCs.
  cs2r1->cd(3);
  T->Draw("fR_S2L_nahit");          // Number of Left paddles ADCs.
  cs2r1->cd(4);
  T->Draw("fR_S2R_nahit");          // Number of Right paddles ADCs.
  TCut ctr1 = "fR_TR_n==1";
  TCut cs2rx = "-90<fR_S2_trx && fR_S2_trx<90";
  TCut cs2ry = "-20<fR_S2_try && fR_S2_try<20";
  TCut cp4f = "250<fR_S2L_adc_c[3] && 300<fR_S2R_adc_c[3]";
  cs2r1->cd(5);
  T->Draw("fR_S2_trx", ctr1&&cs2rx); // X coord of track on Scintillator 2 plane.
  cs2r1->cd(6);
  T->Draw("fR_S2_try", ctr1&&cs2ry); // Y coord of track on Scintillator 2 plane.
  cs2r1->cd(7);
  T->Draw("fR_S2_trx:fR_S2_try", ctr1&&cs2rx&&cs2ry);       // X vs Y
  cs2r1->cd(8);
  T->Draw("fR_S2_trx:fR_S2_try", ctr1&&cs2rx&&cs2ry&&cp4f); // X vs Y when Pad.4
  cs2r1->Update();                   // Draw the Canvas.

  cs2r2 = new TCanvas("cs2r2", "Scintillator 2 Right Demo Canvas 2");
  cs2r2->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2r2->cd(1);
  T->Draw("fR_S2L_tdc[0]","0<fR_S2L_tdc[0] && fR_S2L_tdc[0]<2000");   //P1_L_TDC
  cs2r2->cd(2);
  T->Draw("fR_S2L_tdc[1]","0<fR_S2L_tdc[1] && fR_S2L_tdc[1]<2000");   //P2_L_TDC
  cs2r2->cd(3);
  T->Draw("fR_S2L_tdc[2]","0<fR_S2L_tdc[2] && fR_S2L_tdc[2]<2000");   //P3_L_TDC
  cs2r2->cd(4);
  T->Draw("fR_S2L_tdc[3]","0<fR_S2L_tdc[3] && fR_S2L_tdc[3]<2000");   //P4_L_TDC
  cs2r2->cd(5);
  T->Draw("fR_S2L_tdc[4]","0<fR_S2L_tdc[4] && fR_S2L_tdc[4]<2000");   //P5_L_TDC
  cs2r2->cd(6);
  T->Draw("fR_S2L_tdc[5]","0<fR_S2L_tdc[5] && fR_S2L_tdc[5]<2000");   //P6_L_TDC
  cs2r2->cd(7);
  T->Draw("fR_S2R_tdc[0]","0<fR_S2R_tdc[0] && fR_S2R_tdc[0]<2000");   //P1_R_TDC
  cs2r2->cd(8);
  T->Draw("fR_S2R_tdc[1]","0<fR_S2R_tdc[1] && fR_S2R_tdc[1]<2000");   //P2_R_TDC
  cs2r2->cd(9);
  T->Draw("fR_S2R_tdc[2]","0<fR_S2R_tdc[2] && fR_S2R_tdc[2]<2000");   //P3_R_TDC
  cs2r2->cd(10);
  T->Draw("fR_S2R_tdc[3]","0<fR_S2R_tdc[3] && fR_S2R_tdc[3]<2000");   //P4_R_TDC
  cs2r2->cd(11);
  T->Draw("fR_S2R_tdc[4]","0<fR_S2R_tdc[4] && fR_S2R_tdc[4]<2000");   //P5_R_TDC
  cs2r2->cd(12);
  T->Draw("fR_S2R_tdc[5]","0<fR_S2R_tdc[5] && fR_S2R_tdc[5]<2000");   //P6_R_TDC
  cs2r2->Update();                   // Draw the Canvas.

  cs2r3 = new TCanvas("cs2r3", "Scintillator 2 Right Demo Canvas 3");
  cs2r3->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2r3->cd(1);
  T->Draw("fR_S2L_tdc_c[0]","0<fR_S2L_tdc_c[0] && fR_S2L_tdc_c[0]<3000");
  cs2r3->cd(2);
  T->Draw("fR_S2L_tdc_c[1]","0<fR_S2L_tdc_c[1] && fR_S2L_tdc_c[1]<3000");
  cs2r3->cd(3);
  T->Draw("fR_S2L_tdc_c[2]","0<fR_S2L_tdc_c[2] && fR_S2L_tdc_c[2]<3000");
  cs2r3->cd(4);
  T->Draw("fR_S2L_tdc_c[3]","0<fR_S2L_tdc_c[3] && fR_S2L_tdc_c[3]<3000");
  cs2r3->cd(5);
  T->Draw("fR_S2L_tdc_c[4]","0<fR_S2L_tdc_c[4] && fR_S2L_tdc_c[4]<3000");
  cs2r3->cd(6);
  T->Draw("fR_S2L_tdc_c[5]","0<fR_S2L_tdc_c[5] && fR_S2L_tdc_c[5]<3000");
  cs2r3->cd(7);
  T->Draw("fR_S2R_tdc_c[0]","0<fR_S2R_tdc_c[0] && fR_S2R_tdc_c[0]<3000");
  cs2r3->cd(8);
  T->Draw("fR_S2R_tdc_c[1]","0<fR_S2R_tdc_c[1] && fR_S2R_tdc_c[1]<3000");
  cs2r3->cd(9);
  T->Draw("fR_S2R_tdc_c[2]","0<fR_S2R_tdc_c[2] && fR_S2R_tdc_c[2]<3000");
  cs2r3->cd(10);
  T->Draw("fR_S2R_tdc_c[3]","0<fR_S2R_tdc_c[3] && fR_S2R_tdc_c[3]<3000");
  cs2r3->cd(11);
  T->Draw("fR_S2R_tdc_c[4]","0<fR_S2R_tdc_c[4] && fR_S2R_tdc_c[4]<3000");
  cs2r3->cd(12);
  T->Draw("fR_S2R_tdc_c[5]","0<fR_S2R_tdc_c[5] && fR_S2R_tdc_c[5]<3000");
  cs2r3->Update();                   // Draw the Canvas.

  cs2r4 = new TCanvas("cs2r4", "Scintillator 2 Right Demo Canvas 4");
  cs2r4->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2r4->cd(1);
  T->Draw("fR_S2L_adc[0]","0<fR_S2L_adc[0] && fR_S2L_adc[0]<4000");   //P1_L_ADC
  cs2r4->cd(2);
  T->Draw("fR_S2L_adc[1]","0<fR_S2L_adc[1] && fR_S2L_adc[1]<4000");   //P2_L_ADC
  cs2r4->cd(3);
  T->Draw("fR_S2L_adc[2]","0<fR_S2L_adc[2] && fR_S2L_adc[2]<4000");   //P3_L_ADC
  cs2r4->cd(4);
  T->Draw("fR_S2L_adc[3]","0<fR_S2L_adc[3] && fR_S2L_adc[3]<4000");   //P4_L_ADC
  cs2r4->cd(5);
  T->Draw("fR_S2L_adc[4]","0<fR_S2L_adc[4] && fR_S2L_adc[4]<4000");   //P5_L_ADC
  cs2r4->cd(6);
  T->Draw("fR_S2L_adc[5]","0<fR_S2L_adc[5] && fR_S2L_adc[5]<4000");   //P6_L_ADC
  cs2r4->cd(7);
  T->Draw("fR_S2R_adc[0]","0<fR_S2R_adc[0] && fR_S2R_adc[0]<4000");   //P1_R_ADC
  cs2r4->cd(8);
  T->Draw("fR_S2R_adc[1]","0<fR_S2R_adc[1] && fR_S2R_adc[1]<4000");   //P2_R_ADC
  cs2r4->cd(9);
  T->Draw("fR_S2R_adc[2]","0<fR_S2R_adc[2] && fR_S2R_adc[2]<4000");   //P3_R_ADC
  cs2r4->cd(10);
  T->Draw("fR_S2R_adc[3]","0<fR_S2R_adc[3] && fR_S2R_adc[3]<4000");   //P4_R_ADC
  cs2r4->cd(11);
  T->Draw("fR_S2R_adc[4]","0<fR_S2R_adc[4] && fR_S2R_adc[4]<4000");   //P5_R_ADC
  cs2r4->cd(12);
  T->Draw("fR_S2R_adc[5]","0<fR_S2R_adc[5] && fR_S2R_adc[5]<4000");   //P6_R_ADC
  cs2r4->Update();                   // Draw the Canvas.

  cs2r5 = new TCanvas("cs2r5", "Scintillator 2 Right Demo Canvas 5");
  cs2r5->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2r5->cd(1);
  T->Draw("fR_S2L_adc_c[0]","0<fR_S2L_adc_c[0] && fR_S2L_adc_c[0]<4000");
  cs2r5->cd(2);
  T->Draw("fR_S2L_adc_c[1]","0<fR_S2L_adc_c[1] && fR_S2L_adc_c[1]<4000");
  cs2r5->cd(3);
  T->Draw("fR_S2L_adc_c[2]","0<fR_S2L_adc_c[2] && fR_S2L_adc_c[2]<4000");
  cs2r5->cd(4);
  T->Draw("fR_S2L_adc_c[3]","0<fR_S2L_adc_c[3] && fR_S2L_adc_c[3]<4000");
  cs2r5->cd(5);
  T->Draw("fR_S2L_adc_c[4]","0<fR_S2L_adc_c[4] && fR_S2L_adc_c[4]<4000");
  cs2r5->cd(6);
  T->Draw("fR_S2L_adc_c[5]","0<fR_S2L_adc_c[5] && fR_S2L_adc_c[5]<4000");
  cs2r5->cd(7);
  T->Draw("fR_S2R_adc_c[0]","0<fR_S2R_adc_c[0] && fR_S2R_adc_c[0]<4000");
  cs2r5->cd(8);
  T->Draw("fR_S2R_adc_c[1]","0<fR_S2R_adc_c[1] && fR_S2R_adc_c[1]<4000");
  cs2r5->cd(9);
  T->Draw("fR_S2R_adc_c[2]","0<fR_S2R_adc_c[2] && fR_S2R_adc_c[2]<4000");
  cs2r5->cd(10);
  T->Draw("fR_S2R_adc_c[3]","0<fR_S2R_adc_c[3] && fR_S2R_adc_c[3]<4000");
  cs2r5->cd(11);
  T->Draw("fR_S2R_adc_c[4]","0<fR_S2R_adc_c[4] && fR_S2R_adc_c[4]<4000");
  cs2r5->cd(12);
  T->Draw("fR_S2R_adc_c[5]","0<fR_S2R_adc_c[5] && fR_S2R_adc_c[5]<4000");
  cs2r5->Update();                   // Draw the Canvas.

  // Repeat for left arm

  cs2l1 = new TCanvas("cs2l1", "Scintillator 2 Left Demo Canvas 1");
  cs2l1->Divide(4,2);                // Divide the Canvas to 8 regions (4x2).
  cs2l1->cd(1);
  T->Draw("fL_S2L_nthit");          // Number of Left paddles TDCs.
  cs2l1->cd(2);
  T->Draw("fL_S2R_nthit");          // Number of Right paddles TDCs.
  cs2l1->cd(3);
  T->Draw("fL_S2L_nahit");          // Number of Left paddles ADCs.
  cs2l1->cd(4);
  T->Draw("fL_S2R_nahit");          // Number of Right paddles ADCs.
  TCut ctr1l = "fL_TR_n==1";
  TCut cs2lx = "-90<fL_S2_trx && fL_S2_trx<90";
  TCut cs2ly = "-20<fL_S2_try && fL_S2_try<20";
  TCut cp4lf = "800<fL_S2L_adc_c[3] && 500<fL_S2R_adc_c[3]";
  cs2l1->cd(5);
  T->Draw("fL_S2_trx", ctr1l&&cs2lx); // X coord of track on Scintillator 2 plane.
  cs2l1->cd(6);
  T->Draw("fL_S2_try", ctr1l&&cs2ly); // Y coord of track on Scintillator 2 plane.
  cs2l1->cd(7);
  T->Draw("fL_S2_trx:fL_S2_try", ctr1l&&cs2lx&&cs2ly);       // X vs Y
  cs2l1->cd(8);
  T->Draw("fL_S2_trx:fL_S2_try", ctr1l&&cs2lx&&cs2ly&&cp4lf); // X vs Y when Pad.4
  cs2l1->Update();                   // Draw the Canvas.

  cs2l2 = new TCanvas("cs2l2", "Scintillator 2 Left Demo Canvas 2");
  cs2l2->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2l2->cd(1);
  T->Draw("fL_S2L_tdc[0]","0<fL_S2L_tdc[0] && fL_S2L_tdc[0]<2000");   //P1_L_TDC
  cs2l2->cd(2);
  T->Draw("fL_S2L_tdc[1]","0<fL_S2L_tdc[1] && fL_S2L_tdc[1]<2000");   //P2_L_TDC
  cs2l2->cd(3);
  T->Draw("fL_S2L_tdc[2]","0<fL_S2L_tdc[2] && fL_S2L_tdc[2]<2000");   //P3_L_TDC
  cs2l2->cd(4);
  T->Draw("fL_S2L_tdc[3]","0<fL_S2L_tdc[3] && fL_S2L_tdc[3]<2000");   //P4_L_TDC
  cs2l2->cd(5);
  T->Draw("fL_S2L_tdc[4]","0<fL_S2L_tdc[4] && fL_S2L_tdc[4]<2000");   //P5_L_TDC
  cs2l2->cd(6);
  T->Draw("fL_S2L_tdc[5]","0<fL_S2L_tdc[5] && fL_S2L_tdc[5]<2000");   //P6_L_TDC
  cs2l2->cd(7);
  T->Draw("fL_S2R_tdc[0]","0<fL_S2R_tdc[0] && fL_S2R_tdc[0]<2000");   //P1_R_TDC
  cs2l2->cd(8);
  T->Draw("fL_S2R_tdc[1]","0<fL_S2R_tdc[1] && fL_S2R_tdc[1]<2000");   //P2_R_TDC
  cs2l2->cd(9);
  T->Draw("fL_S2R_tdc[2]","0<fL_S2R_tdc[2] && fL_S2R_tdc[2]<2000");   //P3_R_TDC
  cs2l2->cd(10);
  T->Draw("fL_S2R_tdc[3]","0<fL_S2R_tdc[3] && fL_S2R_tdc[3]<2000");   //P4_R_TDC
  cs2l2->cd(11);
  T->Draw("fL_S2R_tdc[4]","0<fL_S2R_tdc[4] && fL_S2R_tdc[4]<2000");   //P5_R_TDC
  cs2l2->cd(12);
  T->Draw("fL_S2R_tdc[5]","0<fL_S2R_tdc[5] && fL_S2R_tdc[5]<2000");   //P6_R_TDC
  cs2l2->Update();                   // Draw the Canvas.

  cs2l3 = new TCanvas("cs2l3", "Scintillator 2 Left Demo Canvas 3");
  cs2l3->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2l3->cd(1);
  T->Draw("fL_S2L_tdc_c[0]","0<fL_S2L_tdc_c[0] && fL_S2L_tdc_c[0]<3000");
  cs2l3->cd(2);
  T->Draw("fL_S2L_tdc_c[1]","0<fL_S2L_tdc_c[1] && fL_S2L_tdc_c[1]<3000");
  cs2l3->cd(3);
  T->Draw("fL_S2L_tdc_c[2]","0<fL_S2L_tdc_c[2] && fL_S2L_tdc_c[2]<3000");
  cs2l3->cd(4);
  T->Draw("fL_S2L_tdc_c[3]","0<fL_S2L_tdc_c[3] && fL_S2L_tdc_c[3]<3000");
  cs2l3->cd(5);
  T->Draw("fL_S2L_tdc_c[4]","0<fL_S2L_tdc_c[4] && fL_S2L_tdc_c[4]<3000");
  cs2l3->cd(6);
  T->Draw("fL_S2L_tdc_c[5]","0<fL_S2L_tdc_c[5] && fL_S2L_tdc_c[5]<3000");
  cs2l3->cd(7);
  T->Draw("fL_S2R_tdc_c[0]","0<fL_S2R_tdc_c[0] && fL_S2R_tdc_c[0]<3000");
  cs2l3->cd(8);
  T->Draw("fL_S2R_tdc_c[1]","0<fL_S2R_tdc_c[1] && fL_S2R_tdc_c[1]<3000");
  cs2l3->cd(9);
  T->Draw("fL_S2R_tdc_c[2]","0<fL_S2R_tdc_c[2] && fL_S2R_tdc_c[2]<3000");
  cs2l3->cd(10);
  T->Draw("fL_S2R_tdc_c[3]","0<fL_S2R_tdc_c[3] && fL_S2R_tdc_c[3]<3000");
  cs2l3->cd(11);
  T->Draw("fL_S2R_tdc_c[4]","0<fL_S2R_tdc_c[4] && fL_S2R_tdc_c[4]<3000");
  cs2l3->cd(12);
  T->Draw("fL_S2R_tdc_c[5]","0<fL_S2R_tdc_c[5] && fL_S2R_tdc_c[5]<3000");
  cs2l3->Update();                   // Draw the Canvas.

  cs2l4 = new TCanvas("cs2l4", "Scintillator 2 Left Demo Canvas 4");
  cs2l4->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2l4->cd(1);
  T->Draw("fL_S2L_adc[0]","0<fL_S2L_adc[0] && fL_S2L_adc[0]<4000");   //P1_L_ADC
  cs2l4->cd(2);
  T->Draw("fL_S2L_adc[1]","0<fL_S2L_adc[1] && fL_S2L_adc[1]<4000");   //P2_L_ADC
  cs2l4->cd(3);
  T->Draw("fL_S2L_adc[2]","0<fL_S2L_adc[2] && fL_S2L_adc[2]<4000");   //P3_L_ADC
  cs2l4->cd(4);
  T->Draw("fL_S2L_adc[3]","0<fL_S2L_adc[3] && fL_S2L_adc[3]<4000");   //P4_L_ADC
  cs2l4->cd(5);
  T->Draw("fL_S2L_adc[4]","0<fL_S2L_adc[4] && fL_S2L_adc[4]<4000");   //P5_L_ADC
  cs2l4->cd(6);
  T->Draw("fL_S2L_adc[5]","0<fL_S2L_adc[5] && fL_S2L_adc[5]<4000");   //P6_L_ADC
  cs2l4->cd(7);
  T->Draw("fL_S2R_adc[0]","0<fL_S2R_adc[0] && fL_S2R_adc[0]<4000");   //P1_R_ADC
  cs2l4->cd(8);
  T->Draw("fL_S2R_adc[1]","0<fL_S2R_adc[1] && fL_S2R_adc[1]<4000");   //P2_R_ADC
  cs2l4->cd(9);
  T->Draw("fL_S2R_adc[2]","0<fL_S2R_adc[2] && fL_S2R_adc[2]<4000");   //P3_R_ADC
  cs2l4->cd(10);
  T->Draw("fL_S2R_adc[3]","0<fL_S2R_adc[3] && fL_S2R_adc[3]<4000");   //P4_R_ADC
  cs2l4->cd(11);
  T->Draw("fL_S2R_adc[4]","0<fL_S2R_adc[4] && fL_S2R_adc[4]<4000");   //P5_R_ADC
  cs2l4->cd(12);
  T->Draw("fL_S2R_adc[5]","0<fL_S2R_adc[5] && fL_S2R_adc[5]<4000");   //P6_R_ADC
  cs2l4->Update();                   // Draw the Canvas.

  cs2l5 = new TCanvas("cs2l5", "Scintillator 2 Left Demo Canvas 5");
  cs2l5->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs2l5->cd(1);
  T->Draw("fL_S2L_adc_c[0]","0<fL_S2L_adc_c[0] && fL_S2L_adc_c[0]<4000");
  cs2l5->cd(2);
  T->Draw("fL_S2L_adc_c[1]","0<fL_S2L_adc_c[1] && fL_S2L_adc_c[1]<4000");
  cs2l5->cd(3);
  T->Draw("fL_S2L_adc_c[2]","0<fL_S2L_adc_c[2] && fL_S2L_adc_c[2]<4000");
  cs2l5->cd(4);
  T->Draw("fL_S2L_adc_c[3]","0<fL_S2L_adc_c[3] && fL_S2L_adc_c[3]<4000");
  cs2l5->cd(5);
  T->Draw("fL_S2L_adc_c[4]","0<fL_S2L_adc_c[4] && fL_S2L_adc_c[4]<4000");
  cs2l5->cd(6);
  T->Draw("fL_S2L_adc_c[5]","0<fL_S2L_adc_c[5] && fL_S2L_adc_c[5]<4000");
  cs2l5->cd(7);
  T->Draw("fL_S2R_adc_c[0]","0<fL_S2R_adc_c[0] && fL_S2R_adc_c[0]<4000");
  cs2l5->cd(8);
  T->Draw("fL_S2R_adc_c[1]","0<fL_S2R_adc_c[1] && fL_S2R_adc_c[1]<4000");
  cs2l5->cd(9);
  T->Draw("fL_S2R_adc_c[2]","0<fL_S2R_adc_c[2] && fL_S2R_adc_c[2]<4000");
  cs2l5->cd(10);
  T->Draw("fL_S2R_adc_c[3]","0<fL_S2R_adc_c[3] && fL_S2R_adc_c[3]<4000");
  cs2l5->cd(11);
  T->Draw("fL_S2R_adc_c[4]","0<fL_S2R_adc_c[4] && fL_S2R_adc_c[4]<4000");
  cs2l5->cd(12);
  T->Draw("fL_S2R_adc_c[5]","0<fL_S2R_adc_c[5] && fL_S2R_adc_c[5]<4000");
  cs2l5->Update();                   // Draw the Canvas.

}
