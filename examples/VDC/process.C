// produces a slimmed-down tree for easier analysis of particle tracking

{
TFile *f = new TFile("/data/Afile.root");  // C++ data
TTree *t = (TTree *)f->Get("T");
TFile *of = new TFile("/data/1146_dp.root");  // left ESPACE data
TTree *ot = (TTree *)of->Get("h7");        // left ESPACE data
TTree *rt = (TTree *)of->Get("h1");        // right ESPACE data
TTree *clt = (TTree *)of->Get("h8");       // left ESPACE cluster data
TTree *crt = (TTree *)of->Get("h2");       // right ESPACE cluster data

// comparison
TFile *newf = new TFile("/data/proc_data.root", "recreate");
TTree *newtl = new TTree("TL", "Left Arm - C++ and Fortran Combined Data");
TTree *newtr = new TTree("TR", "Right Arm - C++ and Fortran Combined Data");
TTree *newclt = new TTree("CTL", "Left Arm Cluster Quantities Combined");

Int_t new_nentries, old_nentries, l_nentries, r_nentries;
Int_t lc_nentries, rc_nentries;
new_nentries = (Int_t)t->GetEntries();
old_nentries = (Int_t)ot->GetEntries();
l_nentries = (new_nentries<old_nentries)?new_nentries:old_nentries;
old_nentries = (Int_t)rt->GetEntries();
r_nentries = (new_nentries<old_nentries)?new_nentries:old_nentries;
old_nentries = (Int_t)crt->GetEntries();
rc_nentries = (new_nentries<old_nentries)?new_nentries:old_nentries;
old_nentries = (Int_t)clt->GetEntries();
lc_nentries = (new_nentries<old_nentries)?new_nentries:old_nentries;


THaRawEvent *event = new THaRawEvent();
Float_t x, y, theta, phi;
Float_t nx, ny, ntheta, nphi;
Float_t rx, ry, rtheta, rphi;
Float_t nrx, nry, nrtheta, nrphi;
Float_t ndp, nty, nttheta, ntphi;
Float_t dp, ty, ttheta, tphi;

Float_t rt_x, rt_y, rt_theta, rt_phi;
Float_t rt_nx, rt_ny, rt_ntheta, rt_nphi;
Float_t rt_rx, rt_ry, rt_rtheta, rt_rphi;
Float_t rt_nrx, rt_nry, rt_nrtheta, rt_nrphi;
Float_t rt_ndp, rt_nty, rt_nttheta, rt_ntphi;
Float_t rt_dp, rt_ty, rt_ttheta, rt_tphi;

Float_t u1_piv, u1_pos, u1_slope, u1_chisq;
Float_t nu1_piv, nu1_pos, nu1_slope, nu1_chisq;
Float_t v1_piv, v1_pos, v1_slope, v1_chisq;
Float_t nv1_piv, nv1_pos, nv1_slope, nv1_chisq;
Float_t u2_piv, u2_pos, u2_slope, u2_chisq;
Float_t nu2_piv, nu2_pos, nu2_slope, nu2_chisq;
Float_t v2_piv, v2_pos, v2_slope, v2_chisq;
Float_t nv2_piv, nv2_pos, nv2_slope, nv2_chisq;

int old_evnum, new_evnum, right_evnum, j;
t->SetBranchAddress("Event Branch", &event);
t->SetBranchAddress("fEvtHdr.fEvtNum", &new_evnum);
ot->SetBranchAddress("Spec_l_x_tra", &x);
ot->SetBranchAddress("Spec_l_y_tra", &y);
ot->SetBranchAddress("Spec_l_th_tra", &theta);
ot->SetBranchAddress("Spec_l_ph_tra", &phi);
ot->SetBranchAddress("Spec_l_x_rot", &rx);
ot->SetBranchAddress("Spec_l_y_rot", &ry);
ot->SetBranchAddress("Spec_l_th_rot", &rtheta);
ot->SetBranchAddress("Spec_l_ph_rot", &rphi);
ot->SetBranchAddress("Spec_l_dp", &dp);
ot->SetBranchAddress("Spec_l_y_tg", &ty);
ot->SetBranchAddress("Spec_l_th_tg", &ttheta);
ot->SetBranchAddress("Spec_l_ph_tg", &tphi);
ot->SetBranchAddress("Event", &old_evnum);

rt->SetBranchAddress("Spec_r_x_tra", &rt_x);
rt->SetBranchAddress("Spec_r_y_tra", &rt_y);
rt->SetBranchAddress("Spec_r_th_tra", &rt_theta);
rt->SetBranchAddress("Spec_r_ph_tra", &rt_phi);
rt->SetBranchAddress("Spec_r_x_rot", &rt_rx);
rt->SetBranchAddress("Spec_r_y_rot", &rt_ry);
rt->SetBranchAddress("Spec_r_th_rot", &rt_rtheta);
rt->SetBranchAddress("Spec_r_ph_rot", &rt_rphi);
rt->SetBranchAddress("Spec_r_dp", &rt_dp);
rt->SetBranchAddress("Spec_r_y_tg", &rt_ty);
rt->SetBranchAddress("Spec_r_th_tg", &rt_ttheta);
rt->SetBranchAddress("Spec_r_ph_tg", &rt_tphi);
rt->SetBranchAddress("Event", &old_evnum);

clt->SetBranchAddress("Spec_l_u1_coars", &u1_piv);
clt->SetBranchAddress("Spec_l_u1_pos", &u1_pos);
clt->SetBranchAddress("Spec_l_u1_slope", &u1_slope);
clt->SetBranchAddress("Spec_l_u1_chisq", &u1_chisq);
clt->SetBranchAddress("Spec_l_u2_coars", &u2_piv);
clt->SetBranchAddress("Spec_l_u2_pos", &u2_pos);
clt->SetBranchAddress("Spec_l_u2_slope", &u2_slope);
clt->SetBranchAddress("Spec_l_u2_chisq", &u2_chisq);
clt->SetBranchAddress("Spec_l_v1_coars", &v1_piv);
clt->SetBranchAddress("Spec_l_v1_pos", &v1_pos);
clt->SetBranchAddress("Spec_l_v1_slope", &v1_slope);
clt->SetBranchAddress("Spec_l_v1_chisq", &v1_chisq);
clt->SetBranchAddress("Spec_l_v2_coars", &v2_piv);
clt->SetBranchAddress("Spec_l_v2_pos", &v2_pos);
clt->SetBranchAddress("Spec_l_v2_slope", &v2_slope);
clt->SetBranchAddress("Spec_l_v2_chisq", &v2_chisq);
clt->SetBranchAddress("Event", &old_evnum);

// set up the new tree for the left arm
newtl->Branch("Event Number", &new_evnum, "Event Number/I");

newtl->Branch("CL_TR_x", &nx, "CL_TR_x/F");
newtl->Branch("CL_TR_y", &ny, "CL_TR_y/F");
newtl->Branch("CL_TR_th", &ntheta, "CL_TR_th/F");
newtl->Branch("CL_TR_ph", &nphi, "CL_TR_ph/F");
  
newtl->Branch("FL_TR_x", &x, "FL_TR_x/F");
newtl->Branch("FL_TR_y", &y, "FL_TR_y/F");
newtl->Branch("FL_TR_th", &theta, "FL_TR_th/F");
newtl->Branch("FL_TR_ph", &phi, "FL_TR_ph/F");

newtl->Branch("CL_TR_rx", &nrx, "CL_TR_rx/F");
newtl->Branch("CL_TR_ry", &nry, "CL_TR_ry/F");
newtl->Branch("CL_TR_rth", &nrtheta, "CL_TR_rth/F");
newtl->Branch("CL_TR_rph", &nrphi, "CL_TR_rph/F");
  
newtl->Branch("FL_TR_rx", &rx, "FL_TR_rx/F");
newtl->Branch("FL_TR_ry", &ry, "FL_TR_ry/F");
newtl->Branch("FL_TR_rth", &rtheta, "FL_TR_rth/F");
newtl->Branch("FL_TR_rph", &rphi, "FL_TR_rph/F");

newtl->Branch("CL_TG_dp", &ndp, "CL_TG_dp/F");
newtl->Branch("CL_TG_y", &nty, "CL_TG_y/F");
newtl->Branch("CL_TG_th", &nttheta, "CL_TG_th/F");
newtl->Branch("CL_TG_ph", &ntphi, "CL_TG_ph/F");
  
newtl->Branch("FL_TG_dp", &dp, "FL_TG_dp/F");
newtl->Branch("FL_TG_y", &ty, "FL_TG_y/F");
newtl->Branch("FL_TG_th", &ttheta, "FL_TG_th/F");
newtl->Branch("FL_TG_ph", &tphi, "FL_TG_ph/F");

// set up the new tree for the right arm
newtr->Branch("Event Number", &new_evnum, "Event Number/I");

newtr->Branch("CR_TR_x", &rt_nx, "CR_TR_x/F");
newtr->Branch("CR_TR_y", &rt_ny, "CR_TR_y/F");
newtr->Branch("CR_TR_th", &rt_ntheta, "CR_TR_th/F");
newtr->Branch("CR_TR_ph", &rt_nphi, "CR_TR_ph/F");
  
newtr->Branch("FR_TR_x", &rt_x, "FR_TR_x/F");
newtr->Branch("FR_TR_y", &rt_y, "FR_TR_y/F");
newtr->Branch("FR_TR_th", &rt_theta, "FR_TR_th/F");
newtr->Branch("FR_TR_ph", &rt_phi, "FR_TR_ph/F");

newtr->Branch("CR_TR_rx", &rt_nrx, "CR_TR_rx/F");
newtr->Branch("CR_TR_ry", &rt_nry, "CR_TR_ry/F");
newtr->Branch("CR_TR_rth", &rt_nrtheta, "CR_TR_rth/F");
newtr->Branch("CR_TR_rph", &rt_nrphi, "CR_TR_rph/F");
  
newtr->Branch("FR_TR_rx", &rt_rx, "FR_TR_rx/F");
newtr->Branch("FR_TR_ry", &rt_ry, "FR_TR_ry/F");
newtr->Branch("FR_TR_rth", &rt_rtheta, "FR_TR_rth/F");
newtr->Branch("FR_TR_rph", &rt_rphi, "FR_TR_rph/F");

newtr->Branch("CR_TG_dp", &rt_ndp, "CR_TG_dp/F");
newtr->Branch("CR_TG_y", &rt_nty, "CR_TG_y/F");
newtr->Branch("CR_TG_th", &rt_nttheta, "CR_TG_th/F");
newtr->Branch("CR_TG_ph", &rt_ntphi, "CR_TG_ph/F");
  
newtr->Branch("FR_TG_dp", &rt_dp, "FR_TG_dp/F");
newtr->Branch("FR_TG_y", &rt_ty, "FR_TG_y/F");
newtr->Branch("FR_TG_th", &rt_ttheta, "FR_TG_th/F");
newtr->Branch("FR_TG_ph", &rt_tphi, "FR_TG_ph/F");

// set up the new tree for the left arm cluster data
newclt->Branch("Event Number", &new_evnum, "Event Number/I");

newclt->Branch("FL_U1_pos", &u1_pos, "FL_U1_pos/F");
newclt->Branch("FL_U1_pivot", &u1_piv, "FL_U1_pivot/F");
newclt->Branch("FL_U1_slope", &u1_slope, "FL_U1_slope/F");

newclt->Branch("CL_U1_pos", &nu1_pos, "CL_U1_pos/F");
newclt->Branch("CL_U1_pivot", &nu1_piv, "CL_U1_pivot/F");
newclt->Branch("CL_U1_slope", &nu1_slope, "CL_U1_slope/F");

newclt->Branch("FL_U2_pos", &u2_pos, "FL_U2_pos/F");
newclt->Branch("FL_U2_pivot", &u2_piv, "FL_U2_pivot/F");
newclt->Branch("FL_U2_slope", &u2_slope, "FL_U2_slope/F");

newclt->Branch("CL_U2_pos", &nu2_pos, "CL_U2_pos/F");
newclt->Branch("CL_U2_pivot", &nu2_piv, "CL_U2_pivot/F");
newclt->Branch("CL_U2_slope", &nu2_slope, "CL_U2_slope/F");

newclt->Branch("FL_V1_pos", &v1_pos, "FL_V1_pos/F");
newclt->Branch("FL_V1_pivot", &v1_piv, "FL_V1_pivot/F");
newclt->Branch("FL_V1_slope", &v1_slope, "FL_V1_slope/F");

newclt->Branch("CL_V1_pos", &nv1_pos, "CL_V1_pos/F");
newclt->Branch("CL_V1_pivot", &nv1_piv, "CL_V1_pivot/F");
newclt->Branch("CL_V1_slope", &nv1_slope, "CL_V1_slope/F");

newclt->Branch("FL_V2_pos", &v2_pos, "FL_V2_pos/F");
newclt->Branch("FL_V2_pivot", &v2_piv, "FL_V2_pivot/F");
newclt->Branch("FL_V2_slope", &v2_slope, "FL_V2_slope/F");

newclt->Branch("CL_V2_pos", &nv2_pos, "CL_V2_pos/F");
newclt->Branch("CL_V2_pivot", &nv2_piv, "CL_V2_pivot/F");
newclt->Branch("CL_V2_slope", &nv2_slope, "CL_V2_slope/F");


cout<<"Left Arm:"<<endl;
for(Int_t i=0, j=0; i<l_nentries; i++) {
  t->GetEntry(j);
  ot->GetEntry(i);
  
  // make sure we get the same event in both Fortran and C++
  while(new_evnum < old_evnum) {
    j++;
    t->GetEntry(j);
  }
  while(new_evnum > old_evnum) {
    i++;
    ot->GetEntry(i);
  }

  if((i%5000) == 0) cout<<i<<endl;

  if(event->fL_TR_n != 1) continue;  

  nx = event->fL_TR_x[0];
  ny = event->fL_TR_y[0];
  ntheta = event->fL_TR_th[0];
  nphi = event->fL_TR_ph[0];
  
  nrx = event->fL_TR_rx[0];
  nry = event->fL_TR_ry[0];
  nrtheta = event->fL_TR_rth[0];
  nrphi = event->fL_TR_rph[0];  

  ndp = event->fL_TG_dp[0];
  nty = event->fL_TG_y[0];
  nttheta = event->fL_TG_th[0];
  ntphi = event->fL_TG_ph[0];

  newtl->Fill();

}

cout<<"Right Arm:"<<endl;
for(Int_t i=0, j=0; i<r_nentries; i++) {
  t->GetEntry(j);
  rt->GetEntry(i);
  
  while(new_evnum < old_evnum) {
    j++;
    t->GetEntry(j);
  }
  while(new_evnum > old_evnum) {
    i++;
    rt->GetEntry(i);
  }

  if((i%5000) == 0) cout<<i<<endl;

  if(event->fR_TR_n != 1) continue;  

  rt_nx = event->fR_TR_x[0];
  rt_ny = event->fR_TR_y[0];
  rt_ntheta = event->fR_TR_th[0];
  rt_nphi = event->fR_TR_ph[0];
  
  rt_nrx = event->fR_TR_rx[0];
  rt_nry = event->fR_TR_ry[0];
  rt_nrtheta = event->fR_TR_rth[0];
  rt_nrphi = event->fR_TR_rph[0];  
    
  rt_ndp = event->fR_TG_dp[0];
  rt_nty = event->fR_TG_y[0];
  rt_nttheta = event->fR_TG_th[0];
  rt_ntphi = event->fR_TG_ph[0];

  newtr->Fill();
}

cout<<"Left Arm Clusters:"<<endl;
for(Int_t i=0, j=0; i<lc_nentries; i++) {
  t->GetEntry(j);
  clt->GetEntry(i);
  
  while(new_evnum < old_evnum) {
    j++;
    t->GetEntry(j);
  }
  while(new_evnum > old_evnum) {
    i++;
    clt->GetEntry(i);
  }

  if((i%5000) == 0) cout<<i<<endl;

  if((event->fL_U1_nclust != 1) || (event->fL_U2_nclust != 1) ||
     (event->fL_V1_nclust != 1) || (event->fL_V2_nclust != 1) )
    continue;

  nu1_pos = event->fL_U1_clpos[0];
  nu1_piv = event->fL_U1_clpiv[0];
  nu1_slope = event->fL_U1_slope[0];

  nu2_pos = event->fL_U2_clpos[0];
  nu2_piv = event->fL_U2_clpiv[0];
  nu2_slope = event->fL_U2_slope[0];

  nv1_pos = event->fL_V1_clpos[0];
  nv1_piv = event->fL_V1_clpiv[0];
  nv1_slope = event->fL_V1_slope[0];

  nv2_pos = event->fL_V2_clpos[0];
  nv2_piv = event->fL_V2_clpiv[0];
  nv2_slope = event->fL_V2_slope[0];

  newclt->Fill();
}

newtl->Write();
newtr->Write();
newclt->Write();

newf->Close();
}
