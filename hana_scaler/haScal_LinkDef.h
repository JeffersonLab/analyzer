#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class THaScaler+;
#pragma link C++ class THaScalerDB+;

#ifdef __MAKECINT__
#ifdef LINUXVERS
#pragma link C++ class std::map<std::string,int>;
#pragma link C++ class std::map<int,std::string>;
#endif
#endif

#endif







