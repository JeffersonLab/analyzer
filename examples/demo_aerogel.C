///////////////////////////////////////////////////////////////////////////////
//   Macro for demonstration of Aerogel detector parameters.                 //
//   This version for Analyzer v0.60.                                        //
///////////////////////////////////////////////////////////////////////////////
{
// Demonstration distributions:

  car1 = new TCanvas("car1", "Aerogel Demo Canvas 1");
  car1->Divide(3,3);                // Divide the Canvas to 6 regions (3x2).
  car1->cd(1);
  T->Draw("fR_AR_nthit");           // Number of Left paddles TDCs.
  car1->cd(2);
  T->Draw("fR_AR_nahit");           // Number of Left paddles ADCs.
  car1->cd(3);
  TCut ctr1 = "fR_TR_n==1";
  TCut carx = "-110<fR_AR_trx && fR_AR_trx<110";
  TCut cary =  "-30<fR_AR_try && fR_AR_try<30";
  TCut cm19 = "0<fR_AR_adc_c[18]";
  T->Draw("fR_AR_trx:fR_AR_try", ctr1&&carx&&cary);       // X vs Y
  car1->cd(4);
  T->Draw("fR_AR_trx", ctr1&&carx); // X coord of track
  car1->cd(5);
  T->Draw("fR_AR_try", ctr1&&cary); // Y coord of track
  car1->cd(6);
  T->Draw("fR_AR_trx:fR_AR_try", ctr1&&carx&&cary&&cm19); // X vs Y when Pad.19
  car1->cd(7);
  T->Draw("fR_AR_asum_p","0<fR_AR_asum_p");   // ADC - ped sum.
  car1->cd(8);
  T->Draw("fR_AR_asum_c","0<fR_AR_asum_c");  // Corrected ADC sum.
  car1->Update();                   // Draw the Canvas.

  car2 = new TCanvas("car2", "Aerogel Demo Canvas 2");
  car2->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car2->cd(1);
  T->Draw("fR_AR_tdc[1]","0<fR_AR_tdc[1]");       // TDC times of channel 2.
  car2->cd(2);
  T->Draw("fR_AR_tdc[2]","0<fR_AR_tdc[2]");       // TDC times of channel 3.
  car2->cd(3);
  T->Draw("fR_AR_tdc[3]","0<fR_AR_tdc[3]");       // TDC times of channel 4.
  car2->cd(4);
  T->Draw("fR_AR_tdc[4]","0<fR_AR_tdc[4]");       // TDC times of channel 5.
  car2->cd(5);
  T->Draw("fR_AR_tdc[5]","0<fR_AR_tdc[5]");       // TDC times of channel 6.
  car2->cd(6);
  T->Draw("fR_AR_tdc[6]","0<fR_AR_tdc[6]");       // TDC times of channel 7.
  car2->cd(7);
  T->Draw("fR_AR_tdc[7]","0<fR_AR_tdc[7]");       // TDC times of channel 8.
  car2->cd(8);
  T->Draw("fR_AR_tdc[8]","0<fR_AR_tdc[8]");       // TDC times of channel 9.
  car2->cd(9);
  T->Draw("fR_AR_tdc[9]","0<fR_AR_tdc[9]");       // TDC times of channel 10.
  car2->cd(10);
  T->Draw("fR_AR_tdc[10]","0<fR_AR_tdc[10]");     // TDC times of channel 11.
  car2->cd(11);
  T->Draw("fR_AR_tdc[11]","0<fR_AR_tdc[11]");     // TDC times of channel 12.
  car2->cd(12);
  T->Draw("fR_AR_tdc[12]","0<fR_AR_tdc[12]");     // TDC times of channel 13.

  car3 = new TCanvas("car3", "Aerogel Demo Canvas 3");
  car3->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car3->cd(1);
  T->Draw("fR_AR_tdc[14]","0<fR_AR_tdc[14]");     // TDC times of channel 15.
  car3->cd(2);
  T->Draw("fR_AR_tdc[15]","0<fR_AR_tdc[15]");     // TDC times of channel 16.
  car3->cd(3);
  T->Draw("fR_AR_tdc[16]","0<fR_AR_tdc[16]");     // TDC times of channel 17.
  car3->cd(4);
  T->Draw("fR_AR_tdc[17]","0<fR_AR_tdc[17]");     // TDC times of channel 18.
  car3->cd(5);
  T->Draw("fR_AR_tdc[18]","0<fR_AR_tdc[18]");     // TDC times of channel 19.
  car3->cd(6);
  T->Draw("fR_AR_tdc[19]","0<fR_AR_tdc[19]");     // TDC times of channel 20.
  car3->cd(7);
  T->Draw("fR_AR_tdc[20]","0<fR_AR_tdc[20]");     // TDC times of channel 21.
  car3->cd(8);
  T->Draw("fR_AR_tdc[21]","0<fR_AR_tdc[21]");     // TDC times of channel 22.
  car3->cd(9);
  T->Draw("fR_AR_tdc[22]","0<fR_AR_tdc[22]");     // TDC times of channel 23.
  car3->cd(10);
  T->Draw("fR_AR_tdc[23]","0<fR_AR_tdc[23]");     // TDC times of channel 24.
  car3->cd(11);
  T->Draw("fR_AR_tdc[24]","0<fR_AR_tdc[24]");     // TDC times of channel 25.
  car3->cd(12);
  T->Draw("fR_AR_tdc[25]","0<fR_AR_tdc[25]");     // TDC times of channel 26.

  car4 = new TCanvas("car4", "Aerogel Demo Canvas 4");
  car4->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car4->cd(1);
  T->Draw("fR_AR_tdc_c[1]","0<fR_AR_tdc_c[1]");   // TDC corrected of chan. 2.
  car4->cd(2);
  T->Draw("fR_AR_tdc_c[2]","0<fR_AR_tdc_c[2]");   // TDC corrected of chan. 3.
  car4->cd(3);
  T->Draw("fR_AR_tdc_c[3]","0<fR_AR_tdc_c[3]");   // TDC corrected of chan. 4.
  car4->cd(4);
  T->Draw("fR_AR_tdc_c[4]","0<fR_AR_tdc_c[4]");   // TDC corrected of chan. 5.
  car4->cd(5);
  T->Draw("fR_AR_tdc_c[5]","0<fR_AR_tdc_c[5]");   // TDC corrected of chan. 6.
  car4->cd(6);
  T->Draw("fR_AR_tdc_c[6]","0<fR_AR_tdc_c[6]");   // TDC corrected of chan. 7.
  car4->cd(7);
  T->Draw("fR_AR_tdc_c[7]","0<fR_AR_tdc_c[7]");   // TDC corrected of chan. 8.
  car4->cd(8);
  T->Draw("fR_AR_tdc_c[8]","0<fR_AR_tdc_c[8]");   // TDC corrected of chan. 9.
  car4->cd(9);
  T->Draw("fR_AR_tdc_c[9]","0<fR_AR_tdc_c[9]");   // TDC corrected of chan. 10.
  car4->cd(10);
  T->Draw("fR_AR_tdc_c[10]","0<fR_AR_tdc_c[10]"); // TDC corrected of chan. 11.
  car4->cd(11);
  T->Draw("fR_AR_tdc_c[11]","0<fR_AR_tdc_c[11]"); // TDC corrected of chan. 12.
  car4->cd(12);
  T->Draw("fR_AR_tdc_c[12]","0<fR_AR_tdc_c[12]"); // TDC corrected of chan. 13.

  car5 = new TCanvas("car5", "Aerogel Demo Canvas 5");
  car5->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car5->cd(1);
  T->Draw("fR_AR_tdc_c[14]","0<fR_AR_tdc_c[14]"); // TDC corrected of chan. 15.
  car5->cd(2);
  T->Draw("fR_AR_tdc_c[15]","0<fR_AR_tdc_c[15]"); // TDC corrected of chan. 16.
  car5->cd(3);
  T->Draw("fR_AR_tdc_c[16]","0<fR_AR_tdc_c[16]"); // TDC corrected of chan. 17.
  car5->cd(4);
  T->Draw("fR_AR_tdc_c[17]","0<fR_AR_tdc_c[17]"); // TDC corrected of chan. 18.
  car5->cd(5);
  T->Draw("fR_AR_tdc_c[18]","0<fR_AR_tdc_c[18]"); // TDC corrected of chan. 19.
  car5->cd(6);
  T->Draw("fR_AR_tdc_c[19]","0<fR_AR_tdc_c[19]"); // TDC corrected of chan. 20.
  car5->cd(7);
  T->Draw("fR_AR_tdc_c[20]","0<fR_AR_tdc_c[20]"); // TDC corrected of chan. 21.
  car5->cd(8);
  T->Draw("fR_AR_tdc_c[21]","0<fR_AR_tdc_c[21]"); // TDC corrected of chan. 22.
  car5->cd(9);
  T->Draw("fR_AR_tdc_c[22]","0<fR_AR_tdc_c[22]"); // TDC corrected of chan. 23.
  car5->cd(10);
  T->Draw("fR_AR_tdc_c[23]","0<fR_AR_tdc_c[23]"); // TDC corrected of chan. 24.
  car5->cd(11);
  T->Draw("fR_AR_tdc_c[24]","0<fR_AR_tdc_c[24]"); // TDC corrected of chan. 25.
  car5->cd(12);
  T->Draw("fR_AR_tdc_c[25]","0<fR_AR_tdc_c[25]"); // TDC corrected of chan. 26.

  car6 = new TCanvas("car6", "Aerogel Demo Canvas 6");
  car6->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car6->cd(1);
  T->Draw("fR_AR_adc[1]","0<fR_AR_adc[1]");       // ADC values of channel 2.
  car6->cd(2);
  T->Draw("fR_AR_adc[2]","0<fR_AR_adc[2]");       // ADC values of channel 3.
  car6->cd(3);
  T->Draw("fR_AR_adc[3]","0<fR_AR_adc[3]");       // ADC values of channel 4.
  car6->cd(4);
  T->Draw("fR_AR_adc[4]","0<fR_AR_adc[4]");       // ADC values of channel 5.
  car6->cd(5);
  T->Draw("fR_AR_adc[5]","0<fR_AR_adc[5]");       // ADC values of channel 6.
  car6->cd(6);
  T->Draw("fR_AR_adc[6]","0<fR_AR_adc[6]");       // ADC values of channel 7.
  car6->cd(7);
  T->Draw("fR_AR_adc[7]","0<fR_AR_adc[7]");       // ADC values of channel 8.
  car6->cd(8);
  T->Draw("fR_AR_adc[8]","0<fR_AR_adc[8]");       // ADC values of channel 9.
  car6->cd(9);
  T->Draw("fR_AR_adc[9]","0<fR_AR_adc[9]");       // ADC values of channel 10.
  car6->cd(10);
  T->Draw("fR_AR_adc[10]","0<fR_AR_adc[10]");     // ADC values of channel 11.
  car6->cd(11);
  T->Draw("fR_AR_adc[11]","0<fR_AR_adc[11]");     // ADC values of channel 12.
  car6->cd(12);
  T->Draw("fR_AR_adc[12]","0<fR_AR_adc[12]");     // ADC values of channel 13.

  car7 = new TCanvas("car7", "Aerogel Demo Canvas 7");
  car7->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car7->cd(1);
  T->Draw("fR_AR_adc[14]","0<fR_AR_adc[14]");     // ADC values of channel 15.
  car7->cd(2);
  T->Draw("fR_AR_adc[15]","0<fR_AR_adc[15]");     // ADC values of channel 16.
  car7->cd(3);
  T->Draw("fR_AR_adc[16]","0<fR_AR_adc[16]");     // ADC values of channel 17.
  car7->cd(4);
  T->Draw("fR_AR_adc[17]","0<fR_AR_adc[17]");     // ADC values of channel 18.
  car7->cd(5);
  T->Draw("fR_AR_adc[18]","0<fR_AR_adc[18]");     // ADC values of channel 19.
  car7->cd(6);
  T->Draw("fR_AR_adc[19]","0<fR_AR_adc[19]");     // ADC values of channel 20.
  car7->cd(7);
  T->Draw("fR_AR_adc[20]","0<fR_AR_adc[20]");     // ADC values of channel 21.
  car7->cd(8);
  T->Draw("fR_AR_adc[21]","0<fR_AR_adc[21]");     // ADC values of channel 22.
  car7->cd(9);
  T->Draw("fR_AR_adc[22]","0<fR_AR_adc[22]");     // ADC values of channel 23.
  car7->cd(10);
  T->Draw("fR_AR_adc[23]","0<fR_AR_adc[23]");     // ADC values of channel 24.
  car7->cd(11);
  T->Draw("fR_AR_adc[24]","0<fR_AR_adc[24]");     // ADC values of channel 25.
  car7->cd(12);
  T->Draw("fR_AR_adc[25]","0<fR_AR_adc[25]");     // ADC values of channel 26.

  car8 = new TCanvas("car8", "Aerogel Demo Canvas 8");
  car8->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car8->cd(1);
  T->Draw("fR_AR_adc_c[1]","0<fR_AR_adc_c[1]");   // ADC corrected of chan. 2.
  car8->cd(2);
  T->Draw("fR_AR_adc_c[2]","0<fR_AR_adc_c[2]");   // ADC corrected of chan. 3.
  car8->cd(3);
  T->Draw("fR_AR_adc_c[3]","0<fR_AR_adc_c[3]");   // ADC corrected of chan. 4.
  car8->cd(4);
  T->Draw("fR_AR_adc_c[4]","0<fR_AR_adc_c[4]");   // ADC corrected of chan. 5.
  car8->cd(5);
  T->Draw("fR_AR_adc_c[5]","0<fR_AR_adc_c[5]");   // ADC corrected of chan. 6.
  car8->cd(6);
  T->Draw("fR_AR_adc_c[6]","0<fR_AR_adc_c[6]");   // ADC corrected of chan. 7.
  car8->cd(7);
  T->Draw("fR_AR_adc_c[7]","0<fR_AR_adc_c[7]");   // ADC corrected of chan. 8.
  car8->cd(8);
  T->Draw("fR_AR_adc_c[8]","0<fR_AR_adc_c[8]");   // ADC corrected of chan. 9.
  car8->cd(9);
  T->Draw("fR_AR_adc_c[9]","0<fR_AR_adc_c[9]");   // ADC corrected of chan. 10.
  car8->cd(10);
  T->Draw("fR_AR_adc_c[10]","0<fR_AR_adc_c[10]"); // ADC corrected of chan. 11.
  car8->cd(11);
  T->Draw("fR_AR_adc_c[11]","0<fR_AR_adc_c[11]"); // ADC corrected of chan. 12.
  car8->cd(12);
  T->Draw("fR_AR_adc_c[12]","0<fR_AR_adc_c[12]"); // ADC corrected of chan. 13.

  car9 = new TCanvas("car9", "Aerogel Demo Canvas 9");
  car9->Divide(3,4);                // Divide the Canvas to 12 regions (3x4).
  car9->cd(1);
  T->Draw("fR_AR_adc_c[14]","0<fR_AR_adc_c[14]"); // ADC corrected of chan. 15.
  car9->cd(2);
  T->Draw("fR_AR_adc_c[15]","0<fR_AR_adc_c[15]"); // ADC corrected of chan. 16.
  car9->cd(3);
  T->Draw("fR_AR_adc_c[16]","0<fR_AR_adc_c[16]"); // ADC corrected of chan. 17.
  car9->cd(4);
  T->Draw("fR_AR_adc_c[17]","0<fR_AR_adc_c[17]"); // ADC corrected of chan. 18.
  car9->cd(5);
  T->Draw("fR_AR_adc_c[18]","0<fR_AR_adc_c[18]"); // ADC corrected of chan. 19.
  car9->cd(6);
  T->Draw("fR_AR_adc_c[19]","0<fR_AR_adc_c[19]"); // ADC corrected of chan. 20.
  car9->cd(7);
  T->Draw("fR_AR_adc_c[20]","0<fR_AR_adc_c[20]"); // ADC corrected of chan. 21.
  car9->cd(8);
  T->Draw("fR_AR_adc_c[21]","0<fR_AR_adc_c[21]"); // ADC corrected of chan. 22.
  car9->cd(9);
  T->Draw("fR_AR_adc_c[22]","0<fR_AR_adc_c[22]"); // ADC corrected of chan. 23.
  car9->cd(10);
  T->Draw("fR_AR_adc_c[23]","0<fR_AR_adc_c[23]"); // ADC corrected of chan. 24.
  car9->cd(11);
  T->Draw("fR_AR_adc_c[24]","0<fR_AR_adc_c[24]"); // ADC corrected of chan. 25.
  car9->cd(12);
  T->Draw("fR_AR_adc_c[25]","0<fR_AR_adc_c[25]"); // ADC corrected of chan. 26.
}
