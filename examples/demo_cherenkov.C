///////////////////////////////////////////////////////////////////////////////
//   Macro for demonstration of Gas Cherenkov detector parameters.           //
//   This version for Analyzer v0.60.                                        //
///////////////////////////////////////////////////////////////////////////////
{
// Demonstration distributions:

  cch1 = new TCanvas("cch1", "Gas Cherenkov Demo Canvas 1");
  cch1->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  cch1->cd(1);
  T->Draw("fR_CH_nthit");           // Number of TDC times.
  cch1->cd(2);
  T->Draw("fR_CH_nahit");           // Number of ADC times.
  cch1->cd(3);
  T->Draw("fR_CH_asum_p","0<fR_CH_asum_p");   // ADC - ped sum.
  cch1->cd(4);
  T->Draw("fR_CH_asum_c","0<fR_CH_asum_c");  // Corrected ADC sum.
  cch1->Update();                   // Draw the Canvas.

  TCut ctr1 = "fR_TR_n==1";
  TCut cchx = "-120<fR_CH_trx && fR_CH_trx<120";
  TCut cchy =  "-50<fR_CH_try && fR_CH_try<50";
  cch2 = new TCanvas("cch2", "Gas Cherenkov Demo Canvas 2");
  cch2->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  cch2->cd(1);
  T->Draw("fR_CH_trx", ctr1&&cchx); // X coord of track on Cherenkov plane.
  cch2->cd(2);
  T->Draw("fR_CH_try", ctr1&&cchy); // Y coord of track on Cherenkov plane.
  cch2->cd(3);
  T->Draw("fR_CH_trx:fR_CH_try", ctr1&&cchx&&cchy); // X vs Y
  cch2->cd(4);
  T->Draw("fR_CH_trx:fR_CH_try", ctr1&&cchx&&cchy&&"0<fR_CH_adc_c[4]");
  cch2->Update();                   // Draw the Canvas.

  cch3 = new TCanvas("cch3", "Gas Cherenkov Demo Canvas 3");
  cch3->Divide(3,3);                // Divide the Canvas to 9 regions (3x3).
  cch3->cd(1);
  T->Draw("fR_CH_tdc[1]","0<fR_CH_tdc[1]");       // TDC times of channel 2.
  cch3->cd(2);
  T->Draw("fR_CH_tdc[2]","0<fR_CH_tdc[2]");       // TDC times of channel 3.
  cch3->cd(3);
  T->Draw("fR_CH_tdc[3]","0<fR_CH_tdc[3]");       // TDC times of channel 4.
  cch3->cd(4);
  T->Draw("fR_CH_tdc[4]","0<fR_CH_tdc[4]");       // TDC times of channel 5.
  cch3->cd(5);
  T->Draw("fR_CH_tdc[5]","0<fR_CH_tdc[5]");       // TDC times of channel 6.
  cch3->cd(6);
  T->Draw("fR_CH_tdc[6]","0<fR_CH_tdc[6]");       // TDC times of channel 7.
  cch3->cd(7);
  T->Draw("fR_CH_tdc[7]","0<fR_CH_tdc[7]");       // TDC times of channel 8.
  cch3->cd(8);
  T->Draw("fR_CH_tdc[8]","0<fR_CH_tdc[8]");       // TDC times of channel 9.
  cch3->cd(9);
  T->Draw("fR_CH_tdc[9]","0<fR_CH_tdc[9]");       // TDC times of channels 10.
  cch3->Update();                   // Draw the Canvas.

  cch4 = new TCanvas("cch4", "Gas Cherenkov Demo Canvas 4");
  cch4->Divide(3,3);                // Divide the Canvas to 9 regions (3x3).
  cch4->cd(1);
  T->Draw("fR_CH_tdc_c[1]","0<fR_CH_tdc_c[1]");   // Corrected TDC of chan 2.
  cch4->cd(2);
  T->Draw("fR_CH_tdc_c[2]","0<fR_CH_tdc_c[2]");   // Corrected TDC of chan 3.
  cch4->cd(3);
  T->Draw("fR_CH_tdc_c[3]","0<fR_CH_tdc_c[3]");   // Corrected TDC of chan 4.
  cch4->cd(4);
  T->Draw("fR_CH_tdc_c[4]","0<fR_CH_tdc_c[4]");   // Corrected TDC of chan 5.
  cch4->cd(5);
  T->Draw("fR_CH_tdc_c[5]","0<fR_CH_tdc_c[5]");   // Corrected TDC of chan 6.
  cch4->cd(6);
  T->Draw("fR_CH_tdc_c[6]","0<fR_CH_tdc_c[6]");   // Corrected TDC of chan 7.
  cch4->cd(7);
  T->Draw("fR_CH_tdc_c[7]","0<fR_CH_tdc_c[7]");   // Corrected TDC of chan 8.
  cch4->cd(8);
  T->Draw("fR_CH_tdc_c[8]","0<fR_CH_tdc_c[8]");   // Corrected TDC of chan 9.
  cch4->cd(9);
  T->Draw("fR_CH_tdc_c[9]","0<fR_CH_tdc_c[9]");   // Corrected TDC of chan 10.
  cch4->Update();                   // Draw the Canvas.

  cch5 = new TCanvas("cch5", "Gas Cherenkov Demo Canvas 5");
  cch5->Divide(3,3);                // Divide the Canvas to 9 regions (3x3).
  cch5->cd(1);
  T->Draw("fR_CH_adc[1]","0<fR_CH_adc[1]");       // ADC of channel 2.
  cch5->cd(2);
  T->Draw("fR_CH_adc[2]","0<fR_CH_adc[2]");       // ADC of channel 3.
  cch5->cd(3);
  T->Draw("fR_CH_adc[3]","0<fR_CH_adc[3]");       // ADC of channel 4.
  cch5->cd(4);
  T->Draw("fR_CH_adc[4]","0<fR_CH_adc[4]");       // ADC of channel 5.
  cch5->cd(5);
  T->Draw("fR_CH_adc[5]","0<fR_CH_adc[5]");       // ADC of channel 6.
  cch5->cd(6);
  T->Draw("fR_CH_adc[6]","0<fR_CH_adc[6]");       // ADC of channel 7.
  cch5->cd(7);
  T->Draw("fR_CH_adc[7]","0<fR_CH_adc[7]");       // ADC of channel 8.
  cch5->cd(8);
  T->Draw("fR_CH_adc[8]","0<fR_CH_adc[8]");       // ADC of channel 9.
  cch5->cd(9);
  T->Draw("fR_CH_adc[9]","0<fR_CH_adc[9]");     // ADC of channel 10.
  cch5->Update();                   // Draw the Canvas.

  cch6 = new TCanvas("cch6", "Gas Cherenkov Demo Canvas 6");
  cch6->Divide(3,3);                // Divide the Canvas to 9 regions (3x3).
  cch6->cd(1);
  T->Draw("fR_CH_adc_c[1]","0<fR_CH_adc_c[1]");   // Corrected ADC of chan 2.
  cch6->cd(2);
  T->Draw("fR_CH_adc_c[2]","0<fR_CH_adc_c[2]");   // Corrected ADC of chan 3.
  cch6->cd(3);
  T->Draw("fR_CH_adc_c[3]","0<fR_CH_adc_c[3]");   // Corrected ADC of chan 4.
  cch6->cd(4);
  T->Draw("fR_CH_adc_c[4]","0<fR_CH_adc_c[4]");   // Corrected ADC of chan 5.
  cch6->cd(5);
  T->Draw("fR_CH_adc_c[5]","0<fR_CH_adc_c[5]");   // Corrected ADC of chan 6.
  cch6->cd(6);
  T->Draw("fR_CH_adc_c[6]","0<fR_CH_adc_c[6]");   // Corrected ADC of chan 7.
  cch6->cd(7);
  T->Draw("fR_CH_adc_c[7]","0<fR_CH_adc_c[7]");   // Corrected ADC of chan 8.
  cch6->cd(8);
  T->Draw("fR_CH_adc_c[8]","0<fR_CH_adc_c[8]");   // Corrected ADC of chan 9.
  cch6->cd(9);
  T->Draw("fR_CH_adc_c[9]","0<fR_CH_adc_c[9]"); // Corrected ADC of chan 10.
  cch6->Update();                   // Draw the Canvas.
}
