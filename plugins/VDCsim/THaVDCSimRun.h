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
  explicit THaVDCSimRun(const char* filename = "", const char* description = "");
  THaVDCSimRun(const THaVDCSimRun &run);
  ~THaVDCSimRun() override;
  THaVDCSimRun &operator=(const THaRunBase &rhs) override;

  Int_t Close() override;
  Int_t Open() override;
  const UInt_t* GetEvBuffer() const override;
  Int_t ReadEvent() override;
  Int_t Init() override;
  const char* GetFileName() const { return rootFileName.Data(); }
  void SetFileName( const char* name ) { rootFileName = name; }

 protected:
  Int_t ReadDatabase() override {return 0;}

  TString rootFileName;  //  Name of input file
  TFile *rootFile;       //! Input ROOT file
  TTree *tree;           //! Input Tree with simulation data
  TBranch *branch;       //! Branch holding event objects
  THaVDCSimEvent *event; //! Current event

  Int_t nentries;        //! Number of entries in tre e
  Int_t entry;           //! Current entry number

  ClassDefOverride(THaVDCSimRun, 1) // Run class for simulated VDC data
};

#endif
