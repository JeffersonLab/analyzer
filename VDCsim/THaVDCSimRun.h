#ifndef Podd_THaVDCSimRun_h_
#define Podd_THaVDCSimRun_h_

#include "THaRunBase.h"
#include "THaVDCSim.h"
#include "TString.h"

class TFile;
class TTree;
class TBranch;

class THaVDCSimRun : public THaRunBase {
 public:
  THaVDCSimRun(const char* filename = "", const char* description = "");
  THaVDCSimRun(const THaVDCSimRun &run);
  virtual ~THaVDCSimRun();
  virtual THaVDCSimRun &operator=(const THaRunBase &rhs);

  Int_t Close();
  Int_t Open();
  const Int_t* GetEvBuffer() const;
  Int_t ReadEvent();
  Int_t Init();
  const char* GetFileName() const { return rootFileName.Data(); }
  void SetFileName( const char* name ) { rootFileName = name; }

 protected:
  virtual Int_t ReadDatabase() {return 0;}

  TString rootFileName;  //  Name of input file
  TFile *rootFile;       //! Input ROOT file
  TTree *tree;           //! Input Tree with simulation data
  TBranch *branch;       //! Branch holding event objects
  THaVDCSimEvent *event; //! Current event

  Int_t nentries;        //! Number of entries in tre e
  Int_t entry;           //! Current entry number

  ClassDef(THaVDCSimRun, 1) // Run class for simulated VDC data
};

#endif
