{
// R. Michaels, April 2017.
// Usage:  "root .x comp_histo.C"
// This is a root script to compare the histograms in two root files 
// Afile1.root and Afile2.root.  
// If there is any difference in any bin, a warning is issued.
// Use this script as part of the certification process to verify that
// changes in Podd that are not supposed to affect the results, do not.

  Int_t debug=0;
  Int_t idummy;

  TFile *fin1 = TFile::Open("Afile1.root");
  TFile *fin2 = TFile::Open("Afile2.root");

  if (!fin1->IsOpen()) {
    printf("<E> Cannot open input file Afile1.root") ;
    exit(1) ;
  }
  if (!fin2->IsOpen()) {
    printf("<E> Cannot open input file Afile2.root") ;
    exit(1) ;
  }

  Int_t Nbinx1, Nbinx2, Nbiny1, Nbiny2;
  
  TList* xlist1 = fin1->GetListOfKeys() ;
  if (!xlist1) { printf("<E> No keys found in Afile1.root \n") ; exit(1) ; }
  TList* xlist2 = fin2->GetListOfKeys() ;
  if (!xlist2) { printf("<E> No keys found in Afile2.root \n") ; exit(1) ; }

  TIter next1(xlist1) ;
  TKey* key ;
  TObject *obj1, *obj2 ;
  TH1 *h1, *h2;
  Double_t val1, val2;
  obj2=0;


  while ( key = (TKey*)next1() ) {
    obj1 = key->ReadObj() ;
    if (    (strcmp(obj1->IsA()->GetName(),"TProfile")!=0)
         && (!obj1->InheritsFrom("TH2"))
	 && (!obj1->InheritsFrom("TH1")) 
       ) {
      printf("<W> Object %s is not 1D or 2D histogram : "
             "will not be converted\n",obj1->GetName()) ;
      continue;
    }
    printf("Histo name:%s title:%s\n",obj1->GetName(),obj1->GetTitle());
    //    cout << "acknowledge (enter 1)"<<endl;
    //    cin >> idummy;
    
    obj2 = xlist2->FindObject(obj1->GetName());
    h1 = fin1->Get(obj1->GetName());

    if (!obj2) {
       printf("ERROR: histo not found in Afile2.root\n");
       exit(1);
    } else {
       h2 = fin2->Get(obj2->GetName());       
       if(debug) printf("obj2 = %s title %s \n",obj2->GetName(),obj2->GetTitle());
       if (obj1->InheritsFrom("TH1")) {
 	  Nbinx1 = h1->GetNbinsX();
	  Nbinx2 = h2->GetNbinsX();
          printf("Num bins %d  %d  \n",Nbinx1, Nbinx2);
          if (Nbinx1 != Nbinx2) {
	    cout << "ERROR: unequal number of bins for 1D !"<<endl;
            exit(1);
	  }
          for (Int_t j = 0; j<Nbinx1; j++) {
	    val1=h1->GetBinContent(j);
	    val2=h2->GetBinContent(j);
            if (debug) cout << "Values "<<j<<"   "<<val1<<"  "<<val2<<endl;
            if (val1 != val2) {
              cout << "1D Histo: Values not equal for bin "<<j<<"  "<<val1<<"  "<<val2<<endl;
              exit(1);
	    }
	  }
       }
       if (obj1->InheritsFrom("TH2")) {
 	  Nbinx1 = h1->GetNbinsX();
	  Nbinx2 = h2->GetNbinsX();
 	  Nbiny1 = h1->GetNbinsY();
	  Nbiny2 = h2->GetNbinsY();
          cout << "TH2F num bins x and y "<<Nbinx1<<"  "<<Nbiny1<<endl;
          if ((Nbinx1 != Nbinx2) || (Nbiny1 != Nbiny2)) {
	    cout << "ERROR:  Num bins unequal for 2D !! "<<endl;
            exit(1);            
	  }
          for (Int_t j = 0; j<Nbinx1; j++) {
            for (Int_t k = 0; k<Nbiny1; k++) {

	      val1=h1->GetBinContent(j,k);
	      val2=h2->GetBinContent(j,k);
              if (debug) cout << "(2D) Values "<<j<<"   "<<k<<"   "<<val1<<"  "<<val2<<endl;
              if (val1 != val2) {
                cout << "2D histo: Values not equal for bin "<<j<<"  "<<val1<<"  "<<val2<<endl;
                exit(1);
	      }

	    }

	  }


       }
    }
  }
 
  cout << endl << endl << "ALL GOOD.  All bins agree "<<endl;

}
