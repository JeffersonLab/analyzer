///////////////////////////////////////////////////////////////////////////////
//   Macro for demonstration of Total Absorption Shower detector             //
//   This version for Analyzer v0.60                                         //
///////////////////////////////////////////////////////////////////////////////
{

// Demonstration distributions:

  csh1 = new TCanvas("csh1", "The Shower Demo Canvas 1");
  //csh1->Clear();                  // Clear the Canvas.
  csh1->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  csh1->cd(1);                      // Go to region 1.
  T->Draw("fR_PSH_adc[24]","fR_PSH_adc[24]>0");     // Preshower 25-th channel.
  csh1->cd(2);                      // Go to region 2.
  T->Draw("fR_PSH_adc_c[24]","fR_PSH_adc_c[24]>0"); // Corrected ampl of 25-th.
  csh1->cd(3);                      // Go to region 3.
  T->Draw("fR_SHR_adc[50]","fR_SHR_adc[50]>0");     // Shower 51-th channel.
  csh1->cd(4);                      // Go to region 4.
  T->Draw("fR_SHR_adc_c[50]","fR_SHR_adc_c[50]>0"); // Corrected ampl of 51-th.
  csh1->Update();                   // Draw the Canvas.

  csh2 = new TCanvas("csh2", "The Shower Demo Canvas 2");
  csh2->Divide(2,3);                // Divide the Canvas to 6 regions (2x3).
  csh2->cd(1);                      // Go to region 1.
  T->Draw("fR_PSH_asum_p");         // Sum of Preshower channels ADC minus ped.
  csh2->cd(2);                      // Go to region 2.
  T->Draw("fR_PSH_asum_c");         // Sum of Preshower corrected amplitudes.
  csh2->cd(3);                      // Go to region 3.
  T->Draw("fR_SHR_asum_p");         // Sum of Shower channels ADC minus ped.
  csh2->cd(4);                      // Go to region 4.
  T->Draw("fR_SHR_asum_c");         // Sum of Shower corrected amplitudes.
  csh2->cd(5);                      // Go to region 5.
  T->Draw("fR_PSH_asum_p+fR_SHR_asum_p>>h1");
  gStyle->SetOptFit();              // Output Fit Statistic.
  h1->SetFillColor(5);              // Histogram Filling Color.
  h1->Fit("gaus");                  // Gaussian Fit.
  csh2->cd(6);                      // Go to region 6.
  T->Draw("fR_PSH_asum_c+fR_SHR_asum_c>>h2");
  gStyle->SetOptFit();              // Output Fit Statistic.
  h2->SetFillColor(7);              // Histogram Filling Color.
  h2->Fit("gaus");                  // Gaussian Fit.
  csh2->Update();                   // Draw the Canvas.

  csh3 = new TCanvas("csh3", "The Shower Demo Canvas 3");
  csh3->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  csh3->cd(1);                      // Go to region 1.
  T->Draw("fR_PSH_nhit>>h3");       // Number of hits on Preshower.
  h3->SetFillColor(4);              // Histogram Filling Color.
  csh3->cd(2);                      // Go to region 2.
  T->Draw("fR_SHR_nhit>>h4");       // Number of hits on Shower.
  h4->SetFillColor(6);              // Histogram Filling Color.
  csh3->cd(3);                      // Go to region 3.
  T->Draw("fR_PSH_nclust");         // Number of clusters in Preshower.
  csh3->cd(4);                      // Go to region 4.
  T->Draw("fR_SHR_nclust");         // Number of clusters in Shower.
  csh3->Update();                   // Draw the Canvas.

  csh4 = new TCanvas("csh4", "The Shower Demo Canvas 4");
  csh4->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  csh4->cd(1);                      // Go to region 1.
  T->Draw("fR_PSH_e","fR_PSH_nclust>0");  // Energy of shower cluster in Presh.
  csh4->cd(2);                      // Go to region 2.
  T->Draw("fR_PSH_x","fR_PSH_nclust>0");  // X-coord of shower clust. in Presh.
  csh4->cd(3);                      // Go to region 4.
  T->Draw("fR_PSH_y","fR_PSH_nclust>0");  // Y-coord of shower clust. in Presh.
  csh4->cd(4);                      // Go to region 4.
  T->Draw("fR_PSH_mult","fR_PSH_nclust>0");  // Multiplicity of shower clusters
  csh4->Update();                   // Draw the Canvas.

  csh5 = new TCanvas("csh5", "The Shower Demo Canvas 5");
  csh5->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  csh5->cd(1);                      // Go to region 1.
  T->Draw("fR_SHR_e","fR_SHR_nclust>0");  // Energy of shower cluster in Shower.
  csh5->cd(2);                      // Go to region 2.
  T->Draw("fR_SHR_x","fR_SHR_nclust>0");  // X-coord of shower clust. in Shower.
  csh5->cd(3);                      // Go to region 3.
  T->Draw("fR_SHR_y","fR_SHR_nclust>0");  // Y-coord of shower clust. in Shower.
  csh5->cd(4);                      // Go to region 4.
  T->Draw("fR_SHR_mult","fR_SHR_nclust>0");  // Multiplicity of shower clusters
  csh5->Update();                   // Draw the Canvas.

  csh6 = new TCanvas("csh6", "The Shower Demo Canvas 6");
  csh6->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  csh6->cd(1);                      // Go to region 1.
  T->Draw("fR_PSH_nblk[0]","fR_PSH_nclust>0"); // No of centr block of Psh clust
  csh6->cd(2);                      // Go to region 2.
  T->Draw("fR_PSH_eblk[0]","fR_PSH_nclust>0"); // E in centr block of Psh clust
  csh6->cd(3);                      // Go to region 3.
  T->Draw("fR_SHR_nblk[0]","fR_SHR_nclust>0"); // No of centr block of Shr clust
  csh6->cd(4);                      // Go to region 4.
  T->Draw("fR_SHR_eblk[0]","fR_SHR_nclust>0"); // E in centr block of Shr clust
  csh6->Update();                   // Draw the Canvas.

  csh7 = new TCanvas("csh7", "The Shower Demo Canvas 7");
  csh7->Divide(2,2);                // Divide the Canvas to 4 regions (2x2).
  csh7->cd(1);                      // Go to region 1.
  T->Draw("fR_TSH_e>>h5","fR_TSH_id>0");   // E total (in Preshower + in Shower)
  gStyle->SetOptFit();              // Output Fit Statistic.
  h5->SetFillColor(3);              // Histogram Filling Color.
  h5->Fit("gaus");                  // Gaussian Fit.
  csh7->cd(2);                      // Go to region 2.
  T->Draw("fR_SHR_e:fR_TSH_e","fR_TSH_id>0");  // E Shower cluster vs E total.
  csh7->cd(3);                      // Go to region 3.
  T->Draw("fR_TSH_e:fR_SHR_x","fR_TSH_id>0");  // E total vs X Shower cluster.
  csh7->cd(4);                      // Go to region 4.
  T->Draw("fR_TSH_e:fR_SHR_y","fR_TSH_id>0");  // E total vs Y Shower cluster.
  csh7->Update();                   // Draw the Canvas.

  // Total Energy deposited in Preshower Versus total Energy in Shower.
  csh8 = new TCanvas("csh8", "The Shower Demo Canvas 8");
  T->Draw("fR_PSH_e:fR_SHR_e","fR_PSH_e>0 && fR_SHR_e>0","surf1"); // Eps vs Esh
}
