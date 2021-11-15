#ifndef Podd_THaBenchmark_h_
#define Podd_THaBenchmark_h_

//_____________________________________________________________________________
//
// THaBenchmark utility class
//
// Provides start/stop mode for ROOT's TBenchmark class
//_____________________________________________________________________________

#include "TBenchmark.h"
#include "TMath.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>

//_____________________________________________________________________________
class THaBenchmark : public TBenchmark {
public:
  THaBenchmark() { fNmax = 50; }
  virtual ~THaBenchmark() = default;

  virtual void Begin(const char *name) {
    if (!fNbench)
      TBenchmark::Start(name);
    else {
      Int_t bench = GetBench(name);
      if (bench < 0 && fNbench < fNmax )
        TBenchmark::Start(name);
      else if (bench>=0) 
        fTimer[bench].Start(false);
      else
        Warning("Start","too many benches");
    }
  }

  void PrintByName(const std::vector<TString>& names) const {
    int width = 10;
    for( const auto& name : names )
      width = TMath::Max(width, name.Length());
    for( const auto& name : names )
      PrintBenchmark(name.Data(), width);
  }

  virtual void Print(Option_t *name="") const {
    if( name && name[0] != '\0' )
      PrintBenchmark(name);
    else {
      std::vector<TString> names{ fNames, fNames + fNbench };
      PrintByName(names);
    }
  }

private:
  void PrintBenchmark( const char* name, int width = 10 ) const {
    auto fmt =  std::cout.flags();
    auto prec = std::cout.precision();
    Int_t bench = GetBench(name);
    if (bench < 0) return;
    std::cout << std::left << std::setw(width) << name << ": " << std::right
              << "Real Time = "
              << std::fixed << std::setw(6) << std::setprecision(2)
              << fRealTime[bench] << " seconds "
              << "Cpu Time = "
              << std::fixed << std::setw(6) << std::setprecision(2)
              << fCpuTime[bench] << " seconds" << std::endl;
    std::cout.flags(fmt);
    std::cout.precision(prec);
  }
  ClassDef(THaBenchmark,0)   // TBenchmark with true start/stop mode
};

#endif
