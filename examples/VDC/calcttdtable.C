#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

const int kBUFLEN = 150;

//const Int_t kNumBins = 700;
//const Int_t kHistCenter = 1500;

const Double_t kLongestDist = 15.12; // (mm)
const Double_t kTimeRes = 0.5e-9;    // (s)

const int kTrue = 1;
const int kFalse = 0;

Double_t CalcT0(int num_bins, TH1F *hist)
{
  // Calculate TOffset, based on the contents of the Calibration Table
  // Start from left
  int maxBin = -1; //Bin containing the most counts
  int maxCnt = 0;  //Number of counts in maxBin
  double T0;

  for (int i = 0; i < num_bins; i++) {
    if (hist->GetBinContent(i) > maxCnt) { 
      // New max bin
      maxCnt = hist->GetBinContent(i);
      maxBin = i;
    }
    
  }

  if (maxBin == -1) {
    cout<<"T0 calculation failed.n"<<endl;
    return 0;
  }

  int leftBin, rightBin;
  int leftCnt, rightCnt;

  for (int i = maxBin; i >=0; i--) {
    // Now find bins for fit
    if (hist->GetBinContent(i) > 0.9 * maxCnt) {
      rightBin = i;
      rightCnt = hist->GetBinContent(i);

      rightBin = i;
      rightCnt = hist->GetBinContent(i);

    }
    if (hist->GetBinContent(i) < 0.1 * maxCnt) {
      leftBin = i;
      leftCnt = hist->GetBinContent(i);
      break;
      
    }
  }

  // Now fit a line to the bins between left and right
  
  Double_t sumX  = 0.0;   //Counts
  Double_t sumXX = 0.0; 
  Double_t sumY  = 0.0;   //Bins 
  Double_t sumXY = 0.0; 

  for (int i = leftBin; i <= rightBin; i++) { 
    Double_t x = hist->GetBinContent(i); 
    Double_t y = i; 
    sumX  += x; 
    sumXX += x * x; 
    sumY  += y; 
    sumXY += x * y; 
  }
    
  //These formulas should be available in any statistics book
  //Double_t N = leftBin - rightBin + 1; 
  Double_t N = rightBin - leftBin + 1; 
  Double_t yInt = (sumXX * sumY - sumX * sumXY) / (N * sumXX - sumX * sumX); 
  T0 = yInt; 
  
  cout<<"calculated t0 = "<<T0<<endl;

  return T0;
}

int SaveNewTTDData(TDatime &run_date, const char *planename, Double_t new_t0,
		   Int_t new_num_bins, Int_t table_start, 
		   vector<Float_t> new_table)
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
    cerr<<"Database entry "<<tag<<" not found!"<<endl;
    return kFalse;
  }
  
  // read in other info until we get to the stuff we want
  while(fgets(buff, kBUFLEN, db_file)) {
    // check for signs that we've run into the section
    // for a different plane
    if(buff[0] == '[') {
      cerr<<"Could not find lookup table info."<<endl;
      return kFalse;
    }
    
    // check for the lookup table header
    if(strncmp(buff, "TDC Lookup Table", 16) == 0)
      break;
  }

  if(feof(db_file) != 0) {
    cerr<<"Could not find lookup table in database."<<endl;
    return kFalse;
  }
  if(ferror(db_file) != 0) {
    cerr<<"Error reading database file."<<endl;
    return kFalse;
  }

  // save the header info
  fprintf(db_file, "%7.3e\n", new_t0);
  fprintf(db_file, "%d\n", new_num_bins);

  // save the new table
  for(int i=table_start; i<=new_num_bins; ++i) {
    fprintf(db_file, "%7.4f", new_table[i]);
    //fprintf(db_file, "%7.2e", new_table[i]);

    if((i-table_start+1)%10 == 0)
      fprintf(db_file, "\n");
    else
      fprintf(db_file, " ");
  }

  fclose(db_file);

  return kTrue;
}

void GetTTDData(const char *planename, THaRawEvent *event, Int_t &hits,
		Int_t **rawtimes, Double_t **times)
{
 
  if(strcmp(planename, "L.vdc.u1") == 0) {
      hits = event->fL_U1_nhit;
      *rawtimes = event->fL_U1_rawtime;
      *times = event->fL_U1_time;
  } else if(strcmp(planename, "L.vdc.v1") == 0) {
      hits = event->fL_V1_nhit;
      *rawtimes = event->fL_V1_rawtime;
      *times = event->fL_V1_time;
  } else if(strcmp(planename, "L.vdc.u2") == 0) {
      hits = event->fL_U2_nhit;
      *rawtimes = event->fL_U2_rawtime;
      *times = event->fL_U2_time;
  } else if(strcmp(planename, "L.vdc.v2") == 0) {
      hits = event->fL_V2_nhit;
      *rawtimes = event->fL_V2_rawtime;
      *times = event->fL_V2_time;
  } else if(strcmp(planename, "R.vdc.u1") == 0) {
      hits = event->fR_U1_nhit;
      *rawtimes = event->fR_U1_rawtime;
      *times = event->fR_U1_time;
  } else if(strcmp(planename, "R.vdc.v1") == 0) {
      hits = event->fR_V1_nhit;
      *rawtimes = event->fR_V1_rawtime;
      *times = event->fR_V1_time;
  } else if(strcmp(planename, "R.vdc.u2") == 0) {
      hits = event->fR_U2_nhit;
      *rawtimes = event->fR_U2_rawtime;
      *times = event->fR_U2_time;
  } else if(strcmp(planename, "R.vdc.v2") == 0) {
      hits = event->fR_V2_nhit;
      *rawtimes = event->fR_V2_rawtime;
      *times = event->fR_V2_time;
  } else { 
    cout<<"Bad Plane Specified"<<endl;
  }
}

/*
  Creates a dynamically sized TTD lookup table based on
  a low and high time supplied

  hist_center is a rawtime and used to pick out the relevant events
  planename should be a string like "L.u1.vdc"
  nentries is the number of entries in the supplied treefile to
    use in making the table. a value of 0 means to use all of the
    availible entries
 */
void CalcTTDTable(const char *treefile, const char *planename, 
		  Double_t low, Double_t high, Int_t hist_center,
		  Int_t nentries=0, Double_t K=1.0)
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
  Int_t num_bins = (high - low) / kTimeRes;

  cout<<"num bins = "<<num_bins<<endl;

  vector<Float_t> table(num_bins, 0);
  TH1F *hist = new TH1F("hist", "TTD Spectrum", num_bins, low, high);

  // set up to read tree
  tt->SetBranchAddress("Event Branch", &event);
  
  // make an array big enough to hold all of the time entries
  if(nentries==0)
    nentries = (Int_t)T->GetEntries();

  // deal with all the data points
  for(Int_t i=0; i<nentries; i++) {
    Int_t hits, bin, *rawtimes;
    Double_t *times;
    tt->GetEntry(i);  // gets the data from the tree

    if(i%5000 == 0)
      cout<<i<<endl;

    GetTTDData(planename, event, hits, &rawtimes, &times);

    // deal with each hit individually
    if(hits > 0) {
      for(Int_t j=0; j<hits; j++) {
	// cut if the rawtime is more than 400 away 
	// from the center of the data
	// FIXME: make 400 changeable?
	if(abs(rawtimes[j] - hist_center) < 400) {
	  // save the calculated time
	  hist->Fill(times[j]);
	  /**timelist[num_items] = times[j];**/
	  num_items++;
	}
      }

    }
  }

  //hist->Draw();
  //gPad->Update();

  // figure out the adjusted t0 from the data
  Int_t t0 = (int)CalcT0(num_bins, hist);
  cout<<"calculated t0 = "<<t0<<endl;

  // create lookup table
  table[0] = hist->GetBinContent(1);
  for(Int_t i=t0; i<num_bins; i++) {
    table[i] = table[i-1] + K*hist->GetBinContent(i+1);
  }

  // autocalculate the scale factor, if the user'd like
  K = kLongestDist/table[num_bins-1];
  cout<<"K estimate: "<<K<<endl;

  char input[kBUFLEN];
  cout<<"Do you want to use this value? [y/n] ";
  input[0] = '\0';
  fgets(input, kBUFLEN, stdin);
  
  if(input[0] == 'y') {
    for(Int_t i=t0; i<num_bins; i++) 
      table[i] *= K;
   
    cout<<"Done."<<endl;
  }

  // ask whether to replace the values in the database
  cout<<"Do you want to rebuild the database using these values? [y/n] ";
  input[0] = '\0';
  fgets(input, kBUFLEN, stdin);

  if(input[0] != 'y') {
    cout<<"Exiting without rebuilding database."<<endl;
    goto cleanup;
  }

  cout<<"Rebuilding database..."<<endl;

  // write the new lookup table to the database.
  if(SaveNewTTDData(run_date, planename, ((double)t0*kTimeRes) + low, 
		    num_bins-(t0+1), t0, table))
    cout<<"Done."<<endl;
  else
    cout<<"Failed."<<endl;
  

 cleanup:
  delete event;
  delete f;

  return;
}
