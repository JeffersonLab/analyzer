///////////////////////////////////////////////////////////////////////////////
//   Macro for demonstration of Scintillator 1 detector parameters.          //
//   This version for Analyzer v0.60.                                        //
///////////////////////////////////////////////////////////////////////////////
{
// Demonstration distributions:

  cs1r1 = new TCanvas("cs1r1", "Scintillator 1 Right Demo Canvas 1");
  cs1r1->Divide(4,2);                // Divide the Canvas to 8 regions (4x2).
  cs1r1->cd(1);
  T->Draw("fR_S1L_nthit");          // Number of Left paddles TDCs.
  cs1r1->cd(2);
  T->Draw("fR_S1R_nthit");          // Number of Right paddles TDCs.
  cs1r1->cd(3);
  T->Draw("fR_S1L_nahit");          // Number of Left paddles ADCs.
  cs1r1->cd(4);
  T->Draw("fR_S1R_nahit");          // Number of Right paddles ADCs.
  TCut ctr1 = "fR_TR_n==1";
  TCut cs1rx = "-90<fR_S1_trx && fR_S1_trx<90";
  TCut cs1ry = "-20<fR_S1_try && fR_S1_try<20";
  TCut cp4f = "800<fR_S1L_adc_c[3] && 500<fR_S1R_adc_c[3]";
  cs1r1->cd(5);
  T->Draw("fR_S1_trx", ctr1&&cs1rx); // X coord of track on Scintillator 1 plane.
  cs1r1->cd(6);
  T->Draw("fR_S1_try", ctr1&&cs1ry); // Y coord of track on Scintillator 1 plane.
  cs1r1->cd(7);
  T->Draw("fR_S1_trx:fR_S1_try", ctr1&&cs1rx&&cs1ry);       // X vs Y
  cs1r1->cd(8);
  T->Draw("fR_S1_trx:fR_S1_try", ctr1&&cs1rx&&cs1ry&&cp4f); // X vs Y when Pad.4
  cs1r1->Update();                   // Draw the Canvas.

  cs1r2 = new TCanvas("cs1r2", "Scintillator 1 Right Demo Canvas 2");
  cs1r2->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1r2->cd(1);
  T->Draw("fR_S1L_tdc[0]","0<fR_S1L_tdc[0] && fR_S1L_tdc[0]<2000");   //P1_L_TDC
  cs1r2->cd(2);
  T->Draw("fR_S1L_tdc[1]","0<fR_S1L_tdc[1] && fR_S1L_tdc[1]<2000");   //P2_L_TDC
  cs1r2->cd(3);
  T->Draw("fR_S1L_tdc[2]","0<fR_S1L_tdc[2] && fR_S1L_tdc[2]<2000");   //P3_L_TDC
  cs1r2->cd(4);
  T->Draw("fR_S1L_tdc[3]","0<fR_S1L_tdc[3] && fR_S1L_tdc[3]<2000");   //P4_L_TDC
  cs1r2->cd(5);
  T->Draw("fR_S1L_tdc[4]","0<fR_S1L_tdc[4] && fR_S1L_tdc[4]<2000");   //P5_L_TDC
  cs1r2->cd(6);
  T->Draw("fR_S1L_tdc[5]","0<fR_S1L_tdc[5] && fR_S1L_tdc[5]<2000");   //P6_L_TDC
  cs1r2->cd(7);
  T->Draw("fR_S1R_tdc[0]","0<fR_S1R_tdc[0] && fR_S1R_tdc[0]<2000");   //P1_R_TDC
  cs1r2->cd(8);
  T->Draw("fR_S1R_tdc[1]","0<fR_S1R_tdc[1] && fR_S1R_tdc[1]<2000");   //P2_R_TDC
  cs1r2->cd(9);
  T->Draw("fR_S1R_tdc[2]","0<fR_S1R_tdc[2] && fR_S1R_tdc[2]<2000");   //P3_R_TDC
  cs1r2->cd(10);
  T->Draw("fR_S1R_tdc[3]","0<fR_S1R_tdc[3] && fR_S1R_tdc[3]<2000");   //P4_R_TDC
  cs1r2->cd(11);
  T->Draw("fR_S1R_tdc[4]","0<fR_S1R_tdc[4] && fR_S1R_tdc[4]<2000");   //P5_R_TDC
  cs1r2->cd(12);
  T->Draw("fR_S1R_tdc[5]","0<fR_S1R_tdc[5] && fR_S1R_tdc[5]<2000");   //P6_R_TDC
  cs1r2->Update();                   // Draw the Canvas.

  cs1r3 = new TCanvas("cs1r3", "Scintillator 1 Right Demo Canvas 3");
  cs1r3->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1r3->cd(1);
  T->Draw("fR_S1L_tdc_c[0]","0<fR_S1L_tdc_c[0] && fR_S1L_tdc_c[0]<3000");
  cs1r3->cd(2);
  T->Draw("fR_S1L_tdc_c[1]","0<fR_S1L_tdc_c[1] && fR_S1L_tdc_c[1]<3000");
  cs1r3->cd(3);
  T->Draw("fR_S1L_tdc_c[2]","0<fR_S1L_tdc_c[2] && fR_S1L_tdc_c[2]<3000");
  cs1r3->cd(4);
  T->Draw("fR_S1L_tdc_c[3]","0<fR_S1L_tdc_c[3] && fR_S1L_tdc_c[3]<3000");
  cs1r3->cd(5);
  T->Draw("fR_S1L_tdc_c[4]","0<fR_S1L_tdc_c[4] && fR_S1L_tdc_c[4]<3000");
  cs1r3->cd(6);
  T->Draw("fR_S1L_tdc_c[5]","0<fR_S1L_tdc_c[5] && fR_S1L_tdc_c[5]<3000");
  cs1r3->cd(7);
  T->Draw("fR_S1R_tdc_c[0]","0<fR_S1R_tdc_c[0] && fR_S1R_tdc_c[0]<3000");
  cs1r3->cd(8);
  T->Draw("fR_S1R_tdc_c[1]","0<fR_S1R_tdc_c[1] && fR_S1R_tdc_c[1]<3000");
  cs1r3->cd(9);
  T->Draw("fR_S1R_tdc_c[2]","0<fR_S1R_tdc_c[2] && fR_S1R_tdc_c[2]<3000");
  cs1r3->cd(10);
  T->Draw("fR_S1R_tdc_c[3]","0<fR_S1R_tdc_c[3] && fR_S1R_tdc_c[3]<3000");
  cs1r3->cd(11);
  T->Draw("fR_S1R_tdc_c[4]","0<fR_S1R_tdc_c[4] && fR_S1R_tdc_c[4]<3000");
  cs1r3->cd(12);
  T->Draw("fR_S1R_tdc_c[5]","0<fR_S1R_tdc_c[5] && fR_S1R_tdc_c[5]<3000");
  cs1r3->Update();                   // Draw the Canvas.

  cs1r4 = new TCanvas("cs1r4", "Scintillator 1 Right Demo Canvas 4");
  cs1r4->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1r4->cd(1);
  T->Draw("fR_S1L_adc[0]","0<fR_S1L_adc[0] && fR_S1L_adc[0]<4000");   //P1_L_ADC
  cs1r4->cd(2);
  T->Draw("fR_S1L_adc[1]","0<fR_S1L_adc[1] && fR_S1L_adc[1]<4000");   //P2_L_ADC
  cs1r4->cd(3);
  T->Draw("fR_S1L_adc[2]","0<fR_S1L_adc[2] && fR_S1L_adc[2]<4000");   //P3_L_ADC
  cs1r4->cd(4);
  T->Draw("fR_S1L_adc[3]","0<fR_S1L_adc[3] && fR_S1L_adc[3]<4000");   //P4_L_ADC
  cs1r4->cd(5);
  T->Draw("fR_S1L_adc[4]","0<fR_S1L_adc[4] && fR_S1L_adc[4]<4000");   //P5_L_ADC
  cs1r4->cd(6);
  T->Draw("fR_S1L_adc[5]","0<fR_S1L_adc[5] && fR_S1L_adc[5]<4000");   //P6_L_ADC
  cs1r4->cd(7);
  T->Draw("fR_S1R_adc[0]","0<fR_S1R_adc[0] && fR_S1R_adc[0]<4000");   //P1_R_ADC
  cs1r4->cd(8);
  T->Draw("fR_S1R_adc[1]","0<fR_S1R_adc[1] && fR_S1R_adc[1]<4000");   //P2_R_ADC
  cs1r4->cd(9);
  T->Draw("fR_S1R_adc[2]","0<fR_S1R_adc[2] && fR_S1R_adc[2]<4000");   //P3_R_ADC
  cs1r4->cd(10);
  T->Draw("fR_S1R_adc[3]","0<fR_S1R_adc[3] && fR_S1R_adc[3]<4000");   //P4_R_ADC
  cs1r4->cd(11);
  T->Draw("fR_S1R_adc[4]","0<fR_S1R_adc[4] && fR_S1R_adc[4]<4000");   //P5_R_ADC
  cs1r4->cd(12);
  T->Draw("fR_S1R_adc[5]","0<fR_S1R_adc[5] && fR_S1R_adc[5]<4000");   //P6_R_ADC
  cs1r4->Update();                   // Draw the Canvas.

  cs1r5 = new TCanvas("cs1r5", "Scintillator 1 Right Demo Canvas 5");
  cs1r5->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1r5->cd(1);
  T->Draw("fR_S1L_adc_c[0]","0<fR_S1L_adc_c[0] && fR_S1L_adc_c[0]<4000");
  cs1r5->cd(2);
  T->Draw("fR_S1L_adc_c[1]","0<fR_S1L_adc_c[1] && fR_S1L_adc_c[1]<4000");
  cs1r5->cd(3);
  T->Draw("fR_S1L_adc_c[2]","0<fR_S1L_adc_c[2] && fR_S1L_adc_c[2]<4000");
  cs1r5->cd(4);
  T->Draw("fR_S1L_adc_c[3]","0<fR_S1L_adc_c[3] && fR_S1L_adc_c[3]<4000");
  cs1r5->cd(5);
  T->Draw("fR_S1L_adc_c[4]","0<fR_S1L_adc_c[4] && fR_S1L_adc_c[4]<4000");
  cs1r5->cd(6);
  T->Draw("fR_S1L_adc_c[5]","0<fR_S1L_adc_c[5] && fR_S1L_adc_c[5]<4000");
  cs1r5->cd(7);
  T->Draw("fR_S1R_adc_c[0]","0<fR_S1R_adc_c[0] && fR_S1R_adc_c[0]<4000");
  cs1r5->cd(8);
  T->Draw("fR_S1R_adc_c[1]","0<fR_S1R_adc_c[1] && fR_S1R_adc_c[1]<4000");
  cs1r5->cd(9);
  T->Draw("fR_S1R_adc_c[2]","0<fR_S1R_adc_c[2] && fR_S1R_adc_c[2]<4000");
  cs1r5->cd(10);
  T->Draw("fR_S1R_adc_c[3]","0<fR_S1R_adc_c[3] && fR_S1R_adc_c[3]<4000");
  cs1r5->cd(11);
  T->Draw("fR_S1R_adc_c[4]","0<fR_S1R_adc_c[4] && fR_S1R_adc_c[4]<4000");
  cs1r5->cd(12);
  T->Draw("fR_S1R_adc_c[5]","0<fR_S1R_adc_c[5] && fR_S1R_adc_c[5]<4000");
  cs1r5->Update();                   // Draw the Canvas.

  // Repeat for left arm

  cs1l1 = new TCanvas("cs1l1", "Scintillator 1 Left Demo Canvas 1");
  cs1l1->Divide(4,2);                // Divide the Canvas to 8 regions (4x2).
  cs1l1->cd(1);
  T->Draw("fL_S1L_nthit");          // Number of Left paddles TDCs.
  cs1l1->cd(2);
  T->Draw("fL_S1R_nthit");          // Number of Right paddles TDCs.
  cs1l1->cd(3);
  T->Draw("fL_S1L_nahit");          // Number of Left paddles ADCs.
  cs1l1->cd(4);
  T->Draw("fL_S1R_nahit");          // Number of Right paddles ADCs.
  TCut ctr1l = "fL_TR_n==1";
  TCut cs1lx = "-90<fL_S1_trx && fL_S1_trx<90";
  TCut cs1ly = "-20<fL_S1_try && fL_S1_try<20";
  TCut cp4lf = "800<fL_S1L_adc_c[3] && 500<fL_S1R_adc_c[3]";
  cs1l1->cd(5);
  T->Draw("fL_S1_trx", ctr1l&&cs1lx); // X coord of track on Scintillator 1 plane.
  cs1l1->cd(6);
  T->Draw("fL_S1_try", ctr1l&&cs1ly); // Y coord of track on Scintillator 1 plane.
  cs1l1->cd(7);
  T->Draw("fL_S1_trx:fL_S1_try", ctr1l&&cs1lx&&cs1ly);       // X vs Y
  cs1l1->cd(8);
  T->Draw("fL_S1_trx:fL_S1_try", ctr1l&&cs1lx&&cs1ly&&cp4lf); // X vs Y when Pad.4
  cs1l1->Update();                   // Draw the Canvas.

  cs1l2 = new TCanvas("cs1l2", "Scintillator 1 Left Demo Canvas 2");
  cs1l2->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1l2->cd(1);
  T->Draw("fL_S1L_tdc[0]","0<fL_S1L_tdc[0] && fL_S1L_tdc[0]<2000");   //P1_L_TDC
  cs1l2->cd(2);
  T->Draw("fL_S1L_tdc[1]","0<fL_S1L_tdc[1] && fL_S1L_tdc[1]<2000");   //P2_L_TDC
  cs1l2->cd(3);
  T->Draw("fL_S1L_tdc[2]","0<fL_S1L_tdc[2] && fL_S1L_tdc[2]<2000");   //P3_L_TDC
  cs1l2->cd(4);
  T->Draw("fL_S1L_tdc[3]","0<fL_S1L_tdc[3] && fL_S1L_tdc[3]<2000");   //P4_L_TDC
  cs1l2->cd(5);
  T->Draw("fL_S1L_tdc[4]","0<fL_S1L_tdc[4] && fL_S1L_tdc[4]<2000");   //P5_L_TDC
  cs1l2->cd(6);
  T->Draw("fL_S1L_tdc[5]","0<fL_S1L_tdc[5] && fL_S1L_tdc[5]<2000");   //P6_L_TDC
  cs1l2->cd(7);
  T->Draw("fL_S1R_tdc[0]","0<fL_S1R_tdc[0] && fL_S1R_tdc[0]<2000");   //P1_R_TDC
  cs1l2->cd(8);
  T->Draw("fL_S1R_tdc[1]","0<fL_S1R_tdc[1] && fL_S1R_tdc[1]<2000");   //P2_R_TDC
  cs1l2->cd(9);
  T->Draw("fL_S1R_tdc[2]","0<fL_S1R_tdc[2] && fL_S1R_tdc[2]<2000");   //P3_R_TDC
  cs1l2->cd(10);
  T->Draw("fL_S1R_tdc[3]","0<fL_S1R_tdc[3] && fL_S1R_tdc[3]<2000");   //P4_R_TDC
  cs1l2->cd(11);
  T->Draw("fL_S1R_tdc[4]","0<fL_S1R_tdc[4] && fL_S1R_tdc[4]<2000");   //P5_R_TDC
  cs1l2->cd(12);
  T->Draw("fL_S1R_tdc[5]","0<fL_S1R_tdc[5] && fL_S1R_tdc[5]<2000");   //P6_R_TDC
  cs1l2->Update();                   // Draw the Canvas.

  cs1l3 = new TCanvas("cs1l3", "Scintillator 1 Left Demo Canvas 3");
  cs1l3->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1l3->cd(1);
  T->Draw("fL_S1L_tdc_c[0]","0<fL_S1L_tdc_c[0] && fL_S1L_tdc_c[0]<3000");
  cs1l3->cd(2);
  T->Draw("fL_S1L_tdc_c[1]","0<fL_S1L_tdc_c[1] && fL_S1L_tdc_c[1]<3000");
  cs1l3->cd(3);
  T->Draw("fL_S1L_tdc_c[2]","0<fL_S1L_tdc_c[2] && fL_S1L_tdc_c[2]<3000");
  cs1l3->cd(4);
  T->Draw("fL_S1L_tdc_c[3]","0<fL_S1L_tdc_c[3] && fL_S1L_tdc_c[3]<3000");
  cs1l3->cd(5);
  T->Draw("fL_S1L_tdc_c[4]","0<fL_S1L_tdc_c[4] && fL_S1L_tdc_c[4]<3000");
  cs1l3->cd(6);
  T->Draw("fL_S1L_tdc_c[5]","0<fL_S1L_tdc_c[5] && fL_S1L_tdc_c[5]<3000");
  cs1l3->cd(7);
  T->Draw("fL_S1R_tdc_c[0]","0<fL_S1R_tdc_c[0] && fL_S1R_tdc_c[0]<3000");
  cs1l3->cd(8);
  T->Draw("fL_S1R_tdc_c[1]","0<fL_S1R_tdc_c[1] && fL_S1R_tdc_c[1]<3000");
  cs1l3->cd(9);
  T->Draw("fL_S1R_tdc_c[2]","0<fL_S1R_tdc_c[2] && fL_S1R_tdc_c[2]<3000");
  cs1l3->cd(10);
  T->Draw("fL_S1R_tdc_c[3]","0<fL_S1R_tdc_c[3] && fL_S1R_tdc_c[3]<3000");
  cs1l3->cd(11);
  T->Draw("fL_S1R_tdc_c[4]","0<fL_S1R_tdc_c[4] && fL_S1R_tdc_c[4]<3000");
  cs1l3->cd(12);
  T->Draw("fL_S1R_tdc_c[5]","0<fL_S1R_tdc_c[5] && fL_S1R_tdc_c[5]<3000");
  cs1l3->Update();                   // Draw the Canvas.

  cs1l4 = new TCanvas("cs1l4", "Scintillator 1 Left Demo Canvas 4");
  cs1l4->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1l4->cd(1);
  T->Draw("fL_S1L_adc[0]","0<fL_S1L_adc[0] && fL_S1L_adc[0]<4000");   //P1_L_ADC
  cs1l4->cd(2);
  T->Draw("fL_S1L_adc[1]","0<fL_S1L_adc[1] && fL_S1L_adc[1]<4000");   //P2_L_ADC
  cs1l4->cd(3);
  T->Draw("fL_S1L_adc[2]","0<fL_S1L_adc[2] && fL_S1L_adc[2]<4000");   //P3_L_ADC
  cs1l4->cd(4);
  T->Draw("fL_S1L_adc[3]","0<fL_S1L_adc[3] && fL_S1L_adc[3]<4000");   //P4_L_ADC
  cs1l4->cd(5);
  T->Draw("fL_S1L_adc[4]","0<fL_S1L_adc[4] && fL_S1L_adc[4]<4000");   //P5_L_ADC
  cs1l4->cd(6);
  T->Draw("fL_S1L_adc[5]","0<fL_S1L_adc[5] && fL_S1L_adc[5]<4000");   //P6_L_ADC
  cs1l4->cd(7);
  T->Draw("fL_S1R_adc[0]","0<fL_S1R_adc[0] && fL_S1R_adc[0]<4000");   //P1_R_ADC
  cs1l4->cd(8);
  T->Draw("fL_S1R_adc[1]","0<fL_S1R_adc[1] && fL_S1R_adc[1]<4000");   //P2_R_ADC
  cs1l4->cd(9);
  T->Draw("fL_S1R_adc[2]","0<fL_S1R_adc[2] && fL_S1R_adc[2]<4000");   //P3_R_ADC
  cs1l4->cd(10);
  T->Draw("fL_S1R_adc[3]","0<fL_S1R_adc[3] && fL_S1R_adc[3]<4000");   //P4_R_ADC
  cs1l4->cd(11);
  T->Draw("fL_S1R_adc[4]","0<fL_S1R_adc[4] && fL_S1R_adc[4]<4000");   //P5_R_ADC
  cs1l4->cd(12);
  T->Draw("fL_S1R_adc[5]","0<fL_S1R_adc[5] && fL_S1R_adc[5]<4000");   //P6_R_ADC
  cs1l4->Update();                   // Draw the Canvas.

  cs1l5 = new TCanvas("cs1l5", "Scintillator 1 Left Demo Canvas 5");
  cs1l5->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  cs1l5->cd(1);
  T->Draw("fL_S1L_adc_c[0]","0<fL_S1L_adc_c[0] && fL_S1L_adc_c[0]<4000");
  cs1l5->cd(2);
  T->Draw("fL_S1L_adc_c[1]","0<fL_S1L_adc_c[1] && fL_S1L_adc_c[1]<4000");
  cs1l5->cd(3);
  T->Draw("fL_S1L_adc_c[2]","0<fL_S1L_adc_c[2] && fL_S1L_adc_c[2]<4000");
  cs1l5->cd(4);
  T->Draw("fL_S1L_adc_c[3]","0<fL_S1L_adc_c[3] && fL_S1L_adc_c[3]<4000");
  cs1l5->cd(5);
  T->Draw("fL_S1L_adc_c[4]","0<fL_S1L_adc_c[4] && fL_S1L_adc_c[4]<4000");
  cs1l5->cd(6);
  T->Draw("fL_S1L_adc_c[5]","0<fL_S1L_adc_c[5] && fL_S1L_adc_c[5]<4000");
  cs1l5->cd(7);
  T->Draw("fL_S1R_adc_c[0]","0<fL_S1R_adc_c[0] && fL_S1R_adc_c[0]<4000");
  cs1l5->cd(8);
  T->Draw("fL_S1R_adc_c[1]","0<fL_S1R_adc_c[1] && fL_S1R_adc_c[1]<4000");
  cs1l5->cd(9);
  T->Draw("fL_S1R_adc_c[2]","0<fL_S1R_adc_c[2] && fL_S1R_adc_c[2]<4000");
  cs1l5->cd(10);
  T->Draw("fL_S1R_adc_c[3]","0<fL_S1R_adc_c[3] && fL_S1R_adc_c[3]<4000");
  cs1l5->cd(11);
  T->Draw("fL_S1R_adc_c[4]","0<fL_S1R_adc_c[4] && fL_S1R_adc_c[4]<4000");
  cs1l5->cd(12);
  T->Draw("fL_S1R_adc_c[5]","0<fL_S1R_adc_c[5] && fL_S1R_adc_c[5]<4000");
  cs1l5->Update();                   // Draw the Canvas.

}
