#include <math>

const int kBUFLEN = 150;

const int kNumWires = 368;
const int kNumBins = 3500;

const int kTrue = 1;
const int kFalse = 0;

Double_t CalcT0(int *table)
{
  // Calculate TOffset, based on the contents of the Calibration Table
  // Start from left
  int maxBin = -1; //Bin containing the most counts
  int maxCnt = 0;  //Number of counts in maxBin
  Double_t T0;

  for (int i = kNumBins-1; i >= 0; i--) {
    if (table[i] > maxCnt) { 
      // New max bin
      maxCnt = table[i];
      maxBin = i;
    }

  }
  if (maxBin == -1) {
    cout<<"T0 calculation failed"<<endl;
    return 0.0;
  }
  int leftBin, rightBin;
  int leftCnt, rightCnt;
  for (int i = maxBin; i < kNumBins; i++) {
    // Now find bins for fit
    if (table[i] > 0.9 * maxCnt) {
      leftBin = i;
      leftCnt = table[i];
    }
    if (table[i] < 0.1 * maxCnt) {
      rightBin = i;
      rightCnt = table[i];
      break;
  
    }
  }
  // Now fit a line to the bins between left and right

  Double_t sumX  = 0.0;   //Counts
  Double_t sumXX = 0.0;
  Double_t sumY  = 0.0;   //Bins
  Double_t sumXY = 0.0;

  for (int i = leftBin; i <= rightBin; i++) {
    Double_t x = table[i];
    Double_t y = i;
    sumX  += x;
    sumXX += x * x;
    sumY  += y;
    sumXY += x * y;
  }
    
  //These formulas should be available in any statistics book
  Double_t N = rightBin - leftBin + 1;
  Double_t yInt = (sumXX * sumY - sumX * sumXY) / (N * sumXX - sumX * sumX);
  T0 = yInt;
    
  return T0;
}

int LoadOldT0Data(TDatime &run_date, Double_t *old_t0, const char *planename)
{
  char buff[kBUFLEN], db_filename[kBUFLEN], tag[kBUFLEN];

  sprintf(db_filename, "%c.vdc.", planename[0]);

  FILE *db_file = THaDetectorBase::OpenFile(db_filename, run_date);

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  sprintf(tag, "[ %s ]", planename);
  bool found = false;
  while (!found && fgets (buff, kBUFLEN, db_file) != NULL) {
    if(strlen(buff) > 0 && buff[strlen(buff)-1] == '\n')
      buff[strlen(buff)-1] = '\0';

    if(strcmp(buff, tag) == 0)
      found = true;
  }
  if( !found ) {
    cerr<<"Database entry "<<tag<<" not found!"<<endl;;
    return kFalse;
  }
  
  // read in other info -- skip 7 lines
  for(int i=0; i<7; ++i)
    fgets(buff, kBUFLEN, db_file);

  // read in the current t0 values
  for(int i=0; i<kNumWires; ++i) {
    float the_t0;
    fscanf(db_file, " %*d %f", &the_t0);
    old_t0[i] = the_t0;
  }
  
  fclose(db_file);

  return kTrue;
}

int SaveNewT0Data(TDatime &run_date, Double_t *new_t0, const char *planename)
{
  char buff[kBUFLEN], db_filename[kBUFLEN], tag[kBUFLEN];

  sprintf(db_filename, "%c.vdc.", planename[0]);

  FILE *db_file = THaDetectorBase::OpenFile(db_filename, run_date, 
					    "OpenFile()", "r+");

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  sprintf(tag, "[ %s ]", planename);
  bool found = false;
  while (!found && fgets (buff, kBUFLEN, db_file) != NULL) {
    if(strlen(buff) > 0 && buff[strlen(buff)-1] == '\n')
      buff[strlen(buff)-1] = '\0';

    if(strcmp(buff, tag) == 0)
      found = true;
  }
  if( !found ) {
    cerr<<"Database entry "<<tag<<" not found!"<<endl;;
    return kFalse;
  }
  
  // read in other info -- skip 7 lines
  for(int i=0; i<7; ++i)
    fgets(buff, kBUFLEN, db_file);

  // save the new t0 values
  for(int i=1; i<=kNumWires; ++i) {
    // if we have an offset of 0, then something went wrong in
    // our calculations, so we don't want to overwrite anything
    if(new_t0[i] == 0.0) {
      int oldi;
      float oldf;
      char endc;

      fscanf(db_file, "%d %f%c", &oldi, &oldf, &endc);
      continue;
    }

    fprintf(db_file, "%4d %5.1f", i, new_t0[i]);

    if(i%8 == 0)
      fprintf(db_file, "\n");
    else
      fprintf(db_file, " ");
  }

  fclose(db_file);

  return kTrue;
}

void GetT0Data(const char *planename, THaRawEvent *event, Int_t &hits,
	       Int_t **rawtimes, Int_t **wires)
{
 
  if(strcmp(planename, "L.vdc.u1") == 0) {
      hits = event->fL_U1_nhit;
      *rawtimes = event->fL_U1_rawtime;
      *wires = event->fL_U1_wire;
  } else if(strcmp(planename, "L.vdc.v1") == 0) {
      hits = event->fL_V1_nhit;
      *rawtimes = event->fL_V1_rawtime;
      *wires = event->fL_V1_wire;
  } else if(strcmp(planename, "L.vdc.u2") == 0) {
      hits = event->fL_U2_nhit;
      *rawtimes = event->fL_U2_rawtime;
      *wires = event->fL_U2_wire;
  } else if(strcmp(planename, "L.vdc.v2") == 0) {
      hits = event->fL_V2_nhit;
      *rawtimes = event->fL_V2_rawtime;
      *wires = event->fL_V2_wire;
  } else if(strcmp(planename, "R.vdc.u1") == 0) {
      hits = event->fR_U1_nhit;
      *rawtimes = event->fR_U1_rawtime;
      *wires = event->fR_U1_wire;
  } else if(strcmp(planename, "R.vdc.v1") == 0) {
      hits = event->fR_V1_nhit;
      *rawtimes = event->fR_V1_rawtime;
      *wires = event->fR_V1_wire;
  } else if(strcmp(planename, "R.vdc.u2") == 0) {
      hits = event->fR_U2_nhit;
      *rawtimes = event->fR_U2_rawtime;
      *wires = event->fR_U2_wire;
  } else if(strcmp(planename, "R.vdc.v2") == 0) {
      hits = event->fR_V2_nhit;
      *rawtimes = event->fR_V2_rawtime;
      *wires = event->fR_V2_wire;
  } else { 
    cout<<"Bad Plane Specified"<<endl;
  }
}

void CalcT0Table(const char *treefile, const char *planename)
{
  TFile *f = new TFile(treefile);        // file to read from
  TTree *tt = (TTree *)f->Get("T");      // tree name in file
  THaRawEvent *event = new THaRawEvent();// the format the data was stored in

  // read in header info
  THaRun *the_run = (THaRun *)f->Get("Run Data");
  if(the_run == NULL) {
    cerr<<"Could not read run header info. Exiting."<<endl;
    return;
  }

  TDatime run_date = the_run->GetDate();
  Int_t num_items=0;
  Int_t calibration_table[kNumWires][kNumBins];

  Double_t t0_table[kNumBins];   // new t0s we calculate
  Double_t old_t0[kNumWires];    // old t0s read in from database
  Double_t t0_mean, t0_stddev;

  // set up to read tree
  tt->SetBranchAddress("Event Branch", &event);
  
  Int_t nentries = (Int_t)T->GetEntries();

  // deal with all the data points
  for(Int_t i=0; i<nentries; i++) {
    Int_t hits, *rawtimes, *wires;
    tt->GetEntry(i);  // gets the data from the tree

    if(i%5000 == 0)
      cout<<i<<endl;

    GetT0Data(planename, event, hits, &rawtimes, &wires);

    // deal with each hit individually
    if(hits > 0) {
      // save the time
      for(Int_t j=0; j<hits; j++)
	calibration_table[wires[j]][rawtimes[j]]++;      
    }
  }

  // for each wire, compute its t0, and run some stats on
  // their differences with previously computed values
  for(int i=0; i<kNumWires; i++) 
    t0_table[i] = CalcT0(calibration_table[i]);

  // load current values from disk
  if(!LoadOldT0Data(run_date, old_t0, planename)) {
    cerr<<"Could not load existing calibration data!"<<endl;
    cerr<<"Exiting."<<endl;
    return;
  }

  // calculate statistics about the new t0s
  t0_mean = 0.0;
  for(int i=0; i<kNumWires; i++)
    t0_mean += abs(old_t0[i] - t0_table[i]);
  t0_mean /= (float)kNumWires;

  t0_stddev = 0.0;
  for(int i=0; i<kNumWires; i++)
    t0_stddev += (abs(old_t0[i] - t0_table[i]) - t0_mean) * 
                 (abs(old_t0[i] - t0_table[i]) - t0_mean);
  
  t0_stddev = sqrt(t0_stddev / (float)(kNumWires - 1));

  // ask whether to replace the values in the database
  cout<<"mean difference: "<<t0_mean<<endl;
  cout<<"sigma: "<<t0_stddev<<endl;

  cout<<"Do you want to rebuild the database using these values? [y/n] ";


  char input[kBUFLEN];
  fgets(input, kBUFLEN, stdin);
  if(input[0] != 'y') {
    cout<<"Exiting without rebuilding database."<<endl;
    goto cleanup;
  }

  cout<<"Rebuilding database..."<<endl;

  if(SaveNewT0Data(run_date, t0_table, planename))
    cout<<"Done."<<endl;
  else
    cout<<"Failed."<<endl;

 cleanup:
  delete f;
  delete event;

  return;
}
