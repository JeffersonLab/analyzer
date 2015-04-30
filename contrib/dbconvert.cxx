// ---- VDC ----
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // load global VDC parameters
  static const char* const here = "ReadDatabase";
  const int LEN = 200;
  char buff[LEN];

  // Look for the section [<prefix>.global] in the file, e.g. [ R.global ]
  TString tag(fPrefix);
  Ssiz_t pos = tag.Index(".");
  if( pos != kNPOS )
    tag = tag(0,pos+1);
  else
    tag.Append(".");
  tag.Prepend("[");
  tag.Append("global]");

  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != 0) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section %s not found!", tag.Data() );
    fclose(file);
    return kInitError;
  }

  // We found the section, now read the data

  // read in some basic constants first
  //  fscanf(file, "%lf", &fSpacing);
  // fSpacing is now calculated from the actual z-positions in Init(),
  // so skip the first line after [ global ] completely:
  fgets(buff, LEN, file); // Skip rest of line

  // Read in the focal plane transfer elements
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  //
  // [ 2002-10-10 15:30:00 ]
  // comment line goes here
  // t 0 0 0  ...
  // y 0 0 0  ... etc.
  //
  // or
  //
  // [ config=highmom ]
  // comment line
  // t 0 0 0  ... etc.
  //
  if( (found = SeekDBdate( file, date )) == 0 && !fConfig.IsNull() &&
      (found = SeekDBconfig( file, fConfig.Data() )) == 0 ) {
    // Print warning if a requested (non-empty) config not found
    Warning( Here(here), "Requested configuration section \"%s\" not "
	     "found in database. Using default (first) section.",
	     fConfig.Data() );
  }

  // Second line after [ global ] or first line after a found tag.
  // After a found tag, it must be the comment line. If not found, then it
  // can be either the comment or a non-found tag before the comment...
  fgets(buff, LEN, file);  // Skip line
  if( !found && IsTag(buff) )
    // Skip one more line if this one was a non-found tag
    fgets(buff, LEN, file);

  fTMatrixElems.clear();
  fDMatrixElems.clear();
  fPMatrixElems.clear();
  fPTAMatrixElems.clear();
  fYMatrixElems.clear();
  fYTAMatrixElems.clear();
  fLMatrixElems.clear();

  fFPMatrixElems.clear();
  fFPMatrixElems.resize(3);

  typedef vector<string>::size_type vsiz_t;
  map<string,vsiz_t> power;
  power["t"] = 3;  // transport to focal-plane tensors
  power["y"] = 3;
  power["p"] = 3;
  power["D"] = 3;  // focal-plane to target tensors
  power["T"] = 3;
  power["Y"] = 3;
  power["YTA"] = 4;
  power["P"] = 3;
  power["PTA"] = 4;
  power["L"] = 4;  // pathlength from z=0 (target) to focal plane (meters)
  power["XF"] = 5; // forward: target to focal-plane (I think)
  power["TF"] = 5;
  power["PF"] = 5;
  power["YF"] = 5;

  map<string,vector<THaMatrixElement>*> matrix_map;
  matrix_map["t"] = &fFPMatrixElems;
  matrix_map["y"] = &fFPMatrixElems;
  matrix_map["p"] = &fFPMatrixElems;
  matrix_map["D"] = &fDMatrixElems;
  matrix_map["T"] = &fTMatrixElems;
  matrix_map["Y"] = &fYMatrixElems;
  matrix_map["YTA"] = &fYTAMatrixElems;
  matrix_map["P"] = &fPMatrixElems;
  matrix_map["PTA"] = &fPTAMatrixElems;
  matrix_map["L"] = &fLMatrixElems;

  map <string,int> fp_map;
  fp_map["t"] = 0;
  fp_map["y"] = 1;
  fp_map["p"] = 2;

  // Read in as many of the matrix elements as there are.
  // Read in line-by-line, so as to be able to handle tensors of
  // different orders.
  while( fgets(buff, LEN, file) ) {
    string line(buff);
    // Erase trailing newline
    if( line.size() > 0 && line[line.size()-1] == '\n' ) {
      buff[line.size()-1] = 0;
      line.erase(line.size()-1,1);
    }
    // Split the line into whitespace-separated fields
    vector<string> line_spl = Split(line);

    // Stop if the line does not start with a string referring to
    // a known type of matrix element. In particular, this will
    // stop on a subsequent timestamp or configuration tag starting with "["
    if(line_spl.empty())
      continue; //ignore empty lines
    const char* w = line_spl[0].c_str();
    vsiz_t npow = power[w];
    if( npow == 0 )
      break;

    // Looks like a good line, go parse it.
    THaMatrixElement ME;
    ME.pw.resize(npow);
    ME.iszero = true;  ME.order = 0;
    vsiz_t pos;
    for (pos=1; pos<=npow && pos<line_spl.size(); pos++) {
      ME.pw[pos-1] = atoi(line_spl[pos].c_str());
    }
    vsiz_t p_cnt;
    for ( p_cnt=0; pos<line_spl.size() && p_cnt<kPORDER && pos<=npow+kPORDER;
	  pos++,p_cnt++ ) {
      ME.poly[p_cnt] = atof(line_spl[pos].c_str());
      if (ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    if (p_cnt < 1) {
	Error(Here(here), "Could not read in Matrix Element %s%d%d%d!",
	      w, ME.pw[0], ME.pw[1], ME.pw[2]);
	Error(Here(here), "Line looks like: %s",line.c_str());
	fclose(file);
	return kInitError;
    }
    // Don't bother with all-zero matrix elements
    if( ME.iszero )
      continue;

    // Add this matrix element to the appropriate array
    vector<THaMatrixElement> *mat = matrix_map[w];
    if (mat) {
      // Special checks for focal plane matrix elements
      if( mat == &fFPMatrixElems ) {
	if( ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
	  THaMatrixElement& m = (*mat)[fp_map[w]];
	  if( m.order > 0 ) {
	    Warning(Here(here), "Duplicate definition of focal plane "
		    "matrix element: %s. Using first definition.", buff);
	  } else
	    m = ME;
	} else
	  Warning(Here(here), "Bad coefficients of focal plane matrix "
		  "element %s", buff);
      }
      else {
	// All other matrix elements are just appended to the respective array
	// but ensure that they are defined only once!
	bool match = false;
	for( vector<THaMatrixElement>::iterator it = mat->begin();
	     it != mat->end() && !(match = it->match(ME)); ++it ) {}
	if( match ) {
	  Warning(Here(here), "Duplicate definition of "
		  "matrix element: %s. Using first definition.", buff);
	} else
	  mat->push_back(ME);
      }
    }
    else if ( fDebug > 0 )
      Warning(Here(here), "Not storing matrix for: %s !", w);

  } //while(fgets)

  // Compute derived quantities and set some hardcoded parameters
  const Double_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  // Define the VDC coordinate axes in the "detector system". By definition,
  // the detector system is identical to the VDC origin in the Hall A HRS.
  DefineAxes(0.0*degrad);

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // figure out the track length from the origin to the s1 plane

  // since we take the VDC to be the origin of the coordinate
  // space, this is actually pretty simple
  const THaDetector* s1 = 0;
  if( GetApparatus() )
    s1 = GetApparatus()->GetDetector("s1");
  if(s1 == 0)
    fCentralDist = 0;
  else
    fCentralDist = s1->GetOrigin().Z();

  CalcMatrix(1.,fLMatrixElems); // tensor without explicit polynomial in x_fp

  // FIXME: Set geometry data (fOrigin). Currently fOrigin = (0,0,0).

  fIsInit = true;
  fclose(file);


//----- VDCPlane -----
  const int LEN = 100;
  char buff[LEN];

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix); tag.Chop(); // delete trailing dot of prefix
  tag.Prepend("["); tag.Append("]");
  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section \"%s\" not found! "
	  "Initialization failed", tag.Data() );
    fclose(file);
    return kInitError;
  }

  //Found the entry for this plane, so read data
  Int_t nWires = 0;    // Number of wires to create
  Int_t prev_first = 0, prev_nwires = 0;
  // Set up the detector map
  fDetMap->Clear();
  do {
    fgets( buff, LEN, file );
    // bad kludge to allow a variable number of detector map lines
    if( strchr(buff, '.') ) // any floating point number indicates end of map
      break;
    // Get crate, slot, low channel and high channel from file
    Int_t crate, slot, lo, hi;
    if( sscanf( buff, "%6d %6d %6d %6d", &crate, &slot, &lo, &hi ) != 4 ) {
      if( *buff ) buff[strlen(buff)-1] = 0; //delete trailing newline
      Error( Here(here), "Error reading detector map line: %s", buff );
      fclose(file);
      return kInitError;
    }
    Int_t first = prev_first + prev_nwires;
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi, first);
    prev_first = first;
    prev_nwires = (hi - lo + 1);
    nWires += prev_nwires;
  } while( *buff );  // sanity escape
  // Load z, wire beginning postion, wire spacing, and wire angle
  sscanf( buff, "%15lf %15lf %15lf %15lf", &fZ, &fWBeg, &fWSpac, &fWAngle );
  fWAngle *= TMath::Pi()/180.0; // Convert to radians
  fSinAngle = TMath::Sin( fWAngle );
  fCosAngle = TMath::Cos( fWAngle );

  // Load drift velocity (will be used to initialize crude Time to Distance
  // converter)
  fscanf(file, "%15lf", &fDriftVel);
  fgets(buff, LEN, file); // Read to end of line
  fgets(buff, LEN, file); // Skip line

  // first read in the time offsets for the wires
  float* wire_offsets = new float[nWires];
  int*   wire_nums    = new int[nWires];

  for (int i = 0; i < nWires; i++) {
    int wnum = 0;
    float offset = 0.0;
    fscanf(file, " %6d %15f", &wnum, &offset);
    wire_nums[i] = wnum-1; // Wire numbers in file start at 1
    wire_offsets[i] = offset;
  }


  // now read in the time-to-drift-distance lookup table
  // data (if it exists)
//   fgets(buff, LEN, file); // read to the end of line
//   fgets(buff, LEN, file);
//   if(strncmp(buff, "TTD Lookup Table", 16) == 0) {
//     // if it exists, read the data in
//     fscanf(file, "%le", &fT0);
//     fscanf(file, "%d", &fNumBins);

//     // this object is responsible for the memory management
//     // of the lookup table
//     delete [] fTable;
//     fTable = new Float_t[fNumBins];
//     for(int i=0; i<fNumBins; i++) {
//       fscanf(file, "%e", &(fTable[i]));
//     }
//   } else {
//     // if not, set some reasonable defaults and rewind the file
//     fT0 = 0.0;
//     fNumBins = 0;
//     fTable = NULL;
//     cout<<"Could not find lookup table header: "<<buff<<endl;
//     fseek(file, -strlen(buff), SEEK_CUR);
//   }

  // Define time-to-drift-distance converter
  // Currently, we use the analytic converter.
  // FIXME: Let user choose this via a parameter
  delete fTTDConv;
  fTTDConv = new THaVDCAnalyticTTDConv(fDriftVel);

  //THaVDCLookupTTDConv* ttdConv = new THaVDCLookupTTDConv(fT0, fNumBins, fTable);

  // Now initialize wires (those wires... too lazy to initialize themselves!)
  // Caution: This may not correspond at all to actual wire channels!
  for (int i = 0; i < nWires; i++) {
    THaVDCWire* wire = new((*fWires)[i])
      THaVDCWire( i, fWBeg+i*fWSpac, wire_offsets[i], fTTDConv );
    if( wire_nums[i] < 0 )
      wire->SetFlag(1);
  }
  delete [] wire_offsets;
  delete [] wire_nums;

  // Set location of chamber center (in detector coordinates)
  fOrigin.SetXYZ( 0.0, 0.0, fZ );

  // Read additional parameters (not in original database)
  // For backward compatibility with database version 1, these parameters
  // are in an optional section, labelled [ <prefix>.extra_param ]
  // (e.g. [ R.vdc.u1.extra_param ]) or [ R.extra_param ].  If this tag is
  // not found, a warning is printed and defaults are used.

  tag = "["; tag.Append(fPrefix); tag.Append("extra_param]");
  TString tag2(tag);
  found = false;
  rewind(file);
  while (!found && fgets(buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    // Poor man's default key search
    rewind(file);
    tag2 = fPrefix;
    Ssiz_t pos = tag2.Index(".");
    if( pos != kNPOS )
      tag2 = tag2(0,pos+1);
    else
      tag2.Append(".");
    tag2.Prepend("[");
    tag2.Append("extra_param]");
    while (!found && fgets(buff, LEN, file) != NULL) {
      char* buf = ::Compress(buff);  //strip blanks
      line = buf;
      delete [] buf;
      if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
      if ( tag2 == line )
	found = true;
    }
  }
  if( found ) {
    if( fscanf(file, "%lf %lf", &fTDCRes, &fT0Resolution) != 2) {
      Error( Here(here), "Error reading TDC scale and T0 resolution\n"
	     "Line = %s\nFix database.", buff );
      fclose(file);
      return kInitError;
    }
    fgets(buff, LEN, file); // Read to end of line
    if( fscanf( file, "%d %d %d %d %d %lf %lf", &fMinClustSize, &fMaxClustSpan,
		&fNMaxGap, &fMinTime, &fMaxTime, &fMinTdiff, &fMaxTdiff ) != 7 ) {
      Error( Here(here), "Error reading min_clust_size, max_clust_span, "
	     "max_gap, min_time, max_time, min_tdiff, max_tdiff.\n"
	     "Line = %s\nFix database.", buff );
      fclose(file);
      return kInitError;
    }
    fgets(buff, LEN, file); // Read to end of line
    // Time-to-distance converter parameters
    if( THaVDCAnalyticTTDConv* analytic_conv =
	dynamic_cast<THaVDCAnalyticTTDConv*>(fTTDConv) ) {
      // THaVDCAnalyticTTDConv
      // Format: 4*A1 4*A2 dtime(s)  (9 floats)
      Double_t A1[4], A2[4], dtime;
      if( fscanf(file, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
		 &A1[0], &A1[1], &A1[2], &A1[3],
		 &A2[0], &A2[1], &A2[2], &A2[3], &dtime ) != 9) {
	Error( Here(here), "Error reading THaVDCAnalyticTTDConv parameters\n"
	       "Line = %s\nExpect 9 floating point numbers. Fix database.",
	       buff );
	fclose(file);
	return kInitError;
      } else {
	analytic_conv->SetParameters( A1, A2, dtime );
      }
    }
//     else if( (... *conv = dynamic_cast<...*>(fTTDConv)) ) {
//       // handle parameters of other TTD converters here
//     }

    fgets(buff, LEN, file); // Read to end of line

    Double_t h, w;

    if( fscanf(file, "%lf %lf", &h, &w) != 2) {
	Error( Here(here), "Error reading height/width parameters\n"
	       "Line = %s\nExpect 2 floating point numbers. Fix database.",
	       buff );
	fclose(file);
	return kInitError;
      } else {
	   fSize[0] = h/2.0;
	   fSize[1] = w/2.0;
      }

    fgets(buff, LEN, file); // Read to end of line
    // Sanity checks
    if( fMinClustSize < 1 || fMinClustSize > 6 ) {
      Error( Here(here), "Invalid min_clust_size = %d, must be betwwen 1 and "
	     "6. Fix database.", fMinClustSize );
      fclose(file);
      return kInitError;
    }
    if( fMaxClustSpan < 2 || fMaxClustSpan > 12 ) {
      Error( Here(here), "Invalid max_clust_span = %d, must be betwwen 1 and "
	     "12. Fix database.", fMaxClustSpan );
      fclose(file);
      return kInitError;
    }
    if( fNMaxGap < 0 || fNMaxGap > 2 ) {
      Error( Here(here), "Invalid max_gap = %d, must be betwwen 0 and 2. "
	     "Fix database.", fNMaxGap );
      fclose(file);
      return kInitError;
    }
    if( fMinTime < 0 || fMinTime > 4095 ) {
      Error( Here(here), "Invalid min_time = %d, must be betwwen 0 and 4095. "
	     "Fix database.", fMinTime );
      fclose(file);
      return kInitError;
    }
    if( fMaxTime < 1 || fMaxTime > 4096 || fMinTime >= fMaxTime ) {
      Error( Here(here), "Invalid max_time = %d. Must be between 1 and 4096 "
	     "and >= min_time = %d. Fix database.", fMaxTime, fMinTime );
      fclose(file);
      return kInitError;
    }
  } else {
    Warning( Here(here), "No database section \"%s\" or \"%s\" found. "
	     "Using defaults.", tag.Data(), tag2.Data() );
    fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan
    fT0Resolution = 6e-8; // 60 ns --- crude guess
    fMinClustSize = 4;
    fMaxClustSpan = 7;
    fNMaxGap = 0;
    fMinTime = 800;
    fMaxTime = 2200;
    fMinTdiff = 3e-8;   // 30ns  -> ~20 deg track angle
    fMaxTdiff = 1.5e-7; // 150ns -> ~60 deg track angle

    if( THaVDCAnalyticTTDConv* analytic_conv =
	dynamic_cast<THaVDCAnalyticTTDConv*>(fTTDConv) ) {
      Double_t A1[4], A2[4], dtime = 4e-9;
      A1[0] = 2.12e-3;
      A1[1] = A1[2] = A1[3] = 0.0;
      A2[0] = -4.2e-4;
      A2[1] = 1.3e-3;
      A2[2] = 1.06e-4;
      A2[3] = 0.0;
      analytic_conv->SetParameters( A1, A2, dtime );
    }
  }

  THaDetectorBase *sdet = GetParent();
  if( sdet )
    fOrigin += sdet->GetOrigin();

  // finally, find the timing-offset to apply on an event-by-event basis
  //FIXME: time offset handling should go into the enclosing apparatus -
  //since not doing so leads to exactly this kind of mess:
  THaApparatus* app = GetApparatus();
  const char* nm = "trg"; // inside an apparatus, the apparatus name is assumed
  if( !app ||
      !(fglTrg = dynamic_cast<THaTriggerTime*>(app->GetDetector(nm))) ) {
    Warning(Here(here),"Trigger-time detector \"%s\" not found. "
	    "Event-by-event time offsets will NOT be used!!",nm);
  }

  fIsInit = true;
  fclose(file);


  //----- THaScintillator
  // --- OLD OLD OLD ---
  static const char* const here = "ReadDatabase()";
  const int LEN = 200;
  char buf[LEN];
  Int_t nelem;

  // Read data from database
  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  while ( ReadComment( fi, buf, LEN ) ) {}
  fscanf ( fi, "%5d", &nelem );                        // Number of  paddles
  fgets ( buf, LEN, fi );

  // Reinitialization only possible for same basic configuration
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of paddles. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    fclose(fi);
    return kInitError;
  }
  fNelem = nelem;

  // Read detector map. Unless a model-number is given
  // for the detector type, this assumes that the first half of the entries
  // are for ADCs and the second half, for TDCs.
  while ( ReadComment( fi, buf, LEN ) ) {}
  int i = 0;
  fDetMap->Clear();
  while (1) {
    int pos;
    Int_t first_chan, model;
    Int_t crate, slot, first, last;
    fgets ( buf, LEN, fi );
    sscanf( buf, "%6d %6d %6d %6d %6d %n",
	    &crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    model=atoi(buf+pos); // if there is no model number given, set to zero

    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }
  }
  while ( ReadComment( fi, buf, LEN ) ) {}

  Float_t x,y,z;
  fscanf ( fi, "%15f %15f %15f", &x, &y, &z );       // Detector's X,Y,Z coord
  fgets ( buf, LEN, fi );
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi );
  while ( ReadComment( fi, buf, LEN ) ) {}
  fscanf ( fi, "%15lf %15lf %15lf", fSize, fSize+1, fSize+2 ); // Sizes of det on X,Y,Z
  fgets ( buf, LEN, fi );
  while ( ReadComment( fi, buf, LEN ) ) {}

  Float_t angle;
  fscanf ( fi, "%15f", &angle );                   // Rotation angle of detector
  fgets ( buf, LEN, fi );
  const Float_t degrad = TMath::Pi()/180.0;

  DefineAxes(angle*degrad);

  // Dimension arrays
  if( !fIsInit ) {
    // Calibration data
    fLOff  = new Double_t[ fNelem ];
    fROff  = new Double_t[ fNelem ];
    fLPed  = new Double_t[ fNelem ];
    fRPed  = new Double_t[ fNelem ];
    fLGain = new Double_t[ fNelem ];
    fRGain = new Double_t[ fNelem ];

    fTrigOff = new Double_t[ fNelem ];

    // Per-event data
    fLT   = new Double_t[ fNelem ];
    fLT_c = new Double_t[ fNelem ];
    fRT   = new Double_t[ fNelem ];
    fRT_c = new Double_t[ fNelem ];
    fLA   = new Double_t[ fNelem ];
    fLA_p = new Double_t[ fNelem ];
    fLA_c = new Double_t[ fNelem ];
    fRA   = new Double_t[ fNelem ];
    fRA_p = new Double_t[ fNelem ];
    fRA_c = new Double_t[ fNelem ];

    fNTWalkPar = 2*fNelem; // 1 paramter per paddle
    fTWalkPar = new Double_t[ fNTWalkPar ];

    fHitPad = new Int_t[ fNelem ];
    fTime   = new Double_t[ fNelem ]; // analysis indexed by paddle (yes, inefficient)
    fdTime  = new Double_t[ fNelem ];
    fAmpl   = new Double_t[ fNelem ];

    fYt     = new Double_t[ fNelem ];
    fYa     = new Double_t[ fNelem ];

    fIsInit = true;
  }
  memset(fTrigOff,0,fNelem*sizeof(fTrigOff[0]));

  // Set DEFAULT values here
  // TDC resolution (s/channel)
  fTdc2T = 0.1e-9;      // seconds/channel
  fResolution = fTdc2T; // actual timing resolution
  // Speed of light in the scintillator material
  fCn = 1.7e+8;    // meters/second
  // Attenuation length
  fAttenuation = 0.7; // inverse meters
  // Time-walk correction parameters
  fAdcMIP = 1.e10;    // large number for offset, so reference is effectively disabled
  // timewalk coefficients for tw = coeff*(1./sqrt(ADC-Ped)-1./sqrt(ADCMip))
  for (int i=0; i<fNTWalkPar; i++) fTWalkPar[i]=0;
  // trigger-timing offsets (s)
  for (int i=0; i<fNelem; i++) fTrigOff[i]=0;


  DBRequest list[] = {
    { "TDC_offsetsL", fLOff, kDouble, static_cast<UInt_t>(fNelem) },
    { "TDC_offsetsR", fROff, kDouble, static_cast<UInt_t>(fNelem) },
    { "ADC_pedsL", fLPed, kDouble, static_cast<UInt_t>(fNelem) },
    { "ADC_pedsR", fRPed, kDouble, static_cast<UInt_t>(fNelem) },
    { "ADC_coefL", fLGain, kDouble, static_cast<UInt_t>(fNelem) },
    { "ADC_coefR", fRGain, kDouble, static_cast<UInt_t>(fNelem) },
    { "TDC_res",   &fTdc2T },
    { "TransSpd",  &fCn },
    { "AdcMIP",    &fAdcMIP },
    { "NTWalk",    &fNTWalkPar, kInt },
    { "Timewalk",  fTWalkPar, kDouble, 2*static_cast<UInt_t>(fNelem) },
    { "ReTimeOff", fTrigOff, kDouble, static_cast<UInt_t>(fNelem) },
    { "AvgRes",    &fResolution },
    { "Atten",     &fAttenuation },
    { 0 }
  };

#if 0
  if ( gHaDB && gHaDB->LoadValues(GetPrefix(),list,date) ) {
    goto exit;  // the new database existed -- we're finished
  }
#endif

  // otherwise, gHaDB is unavailable, use the old file database

  // Read in the timing/adc calibration constants
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  //
  // [ 2002-10-10 15:30:00 ]
  // #comment line goes here
  // <left TDC offsets>
  // <right TDC offsets>
  // <left ADC peds>
  // <rigth ADC peds>
  // <left ADC coeff>
  // <right ADC coeff>
  //
  // if below aren't present, 'default' values are used
  // <TDC resolution: seconds/channel>
  // <speed-of-light in medium m/s>
  // <attenuation length m^-1>
  // <ADC of MIP>
  // <number of timewalk parameters>
  // <timewalk paramters>
  //
  //
  // or
  //
  // [ config=highmom ]
  // comment line
  // ...etc.
  //
  if( SeekDBdate( fi, date ) == 0 && fConfig.Length() > 0 &&
      SeekDBconfig( fi, fConfig.Data() )) {}

  while ( ReadComment( fi, buf, LEN ) ) {}
  // Read calibration data
  for (i=0;i<fNelem;i++)
    fscanf(fi,"%15lf",fLOff+i);                    // Left Pads TDC offsets
  fgets ( buf, LEN, fi );   // finish line
  while ( ReadComment( fi, buf, LEN ) ) {}
  for (i=0;i<fNelem;i++)
    fscanf(fi,"%15lf",fROff+i);                    // Right Pads TDC offsets
  fgets ( buf, LEN, fi );   // finish line
  while ( ReadComment( fi, buf, LEN ) ) {}
  for (i=0;i<fNelem;i++)
    fscanf(fi,"%15lf",fLPed+i);                    // Left Pads ADC Pedest-s
  fgets ( buf, LEN, fi );   // finish line, etc.
  while ( ReadComment( fi, buf, LEN ) ) {}
  for (i=0;i<fNelem;i++)
    fscanf(fi,"%15lf",fRPed+i);                    // Right Pads ADC Pedes-s
  fgets ( buf, LEN, fi );
  while ( ReadComment( fi, buf, LEN ) ) {}
  for (i=0;i<fNelem;i++)
    fscanf (fi,"%15lf",fLGain+i);                  // Left Pads ADC Coeff-s
  fgets ( buf, LEN, fi );
  while ( ReadComment( fi, buf, LEN ) ) {}
  for (i=0;i<fNelem;i++)
    fscanf (fi,"%15lf",fRGain+i);                  // Right Pads ADC Coeff-s
  fgets ( buf, LEN, fi );


  while ( ReadComment( fi, buf, LEN ) ) {}
  // Here on down, look ahead line-by-line
  // stop reading if a '[' is found on a line (corresponding to the next date-tag)
  // read in TDC resolution (s/channel)
  if ( ! fgets(buf, LEN, fi) || strchr(buf,'[') ) goto exit;
  sscanf(buf,"%15lf",&fTdc2T);
  fResolution = 3.*fTdc2T;      // guess at timing resolution

  while ( ReadComment( fi, buf, LEN ) ) {}
  // Speed of light in the scintillator material
  if ( !fgets(buf, LEN, fi) ||  strchr(buf,'[') ) goto exit;
  sscanf(buf,"%15lf",&fCn);

  // Attenuation length (inverse meters)
  while ( ReadComment( fi, buf, LEN ) ) {}
  if ( !fgets ( buf, LEN, fi ) ||  strchr(buf,'[') ) goto exit;
  sscanf(buf,"%15lf",&fAttenuation);

  while ( ReadComment( fi, buf, LEN ) ) {}
  // Time-walk correction parameters
  if ( !fgets(buf, LEN, fi) ||  strchr(buf,'[') ) goto exit;
  sscanf(buf,"%15lf",&fAdcMIP);

  while ( ReadComment( fi, buf, LEN ) ) {}
  // timewalk parameters
  {
    int cnt=0;
    while ( cnt<fNTWalkPar && fgets( buf, LEN, fi ) && ! strchr(buf,'[') ) {
      char *ptr = buf;
      int pos=0;
      while ( cnt < fNTWalkPar && sscanf(ptr,"%15lf%n",&fTWalkPar[cnt],&pos)>0 ) {
	ptr += pos;
	cnt++;
      }
    }
  }

  while ( ReadComment( fi, buf, LEN ) ) {}
  // trigger timing offsets
  {
    int cnt=0;
    while ( cnt<fNelem && fgets( buf, LEN, fi ) && ! strchr(buf,'[') ) {
      char *ptr = buf;
      int pos=0;
      while ( cnt < fNelem && sscanf(ptr,"%15lf%n",&fTrigOff[cnt],&pos)>0 ) {
	ptr += pos;
	cnt++;
      }
    }
  }

 exit:
  fclose(fi);

  if ( fDebug > 1 ) {
    cout << '\n' << GetPrefix() << " calibration parameters: " << endl;;
    for ( DBRequest *li = list; li->name; li++ ) {
      cout << "  " << li->name;
      UInt_t maxc = li->nelem;
      if (maxc==0)maxc=1;
      for (UInt_t i=0; i<maxc; i++) {
	if (li->type==kDouble) cout << "  " << ((Double_t*)li->var)[i];
	if (li->type==kInt) cout << "  " << ((Int_t*)li->var)[i];
      }
      cout << endl;
    }
  }


//-------- Cherenkov ------------------

  static const char* const here = "ReadDatabase()";

  // Read database

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  const int LEN = 100;
  char buf[LEN];
  Int_t nelem;

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%5d", &nelem );                      // Number of mirrors

  // Reinitialization only possible for same basic configuration
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of mirrors. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    fclose(fi);
    return kInitError;
  }
  fNelem = nelem;

  // Read detector map.  Assumes that the first half of the entries
  // is for ADCs, and the second half, for TDCs
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  int i = 0;
  fDetMap->Clear();
  while (1) {
    Int_t crate, slot, first, last, first_chan,model;
    int pos;
    fgets ( buf, LEN, fi );
    sscanf( buf, "%6d %6d %6d %6d %6d %n",
	    &crate, &slot, &first, &last, &first_chan, &pos );
    model=atoi(buf+pos); // if there is no model number given, set to zero

    if( crate < 0 ) break;
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }
  }
  fgets ( buf, LEN, fi );

  // Read geometry

  Float_t x,y,z;
  fscanf ( fi, "%15f %15f %15f", &x, &y, &z );        // Detector's X,Y,Z coord
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%15lf %15lf %15lf", fSize, fSize+1, fSize+2 );   // Sizes of det on X,Y,Z
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  Float_t angle;
  fscanf ( fi, "%15f", &angle );                       // Rotation angle of det
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  const Double_t degrad = TMath::Pi()/180.0;

  DefineAxes(angle*degrad);

  // Dimension arrays
  if( !fIsInit ) {
    // Calibration data
    fOff = new Float_t[ fNelem ];
    fPed = new Float_t[ fNelem ];
    fGain = new Float_t[ fNelem ];

    // Per-event data
    fT   = new Float_t[ fNelem ];
    fT_c = new Float_t[ fNelem ];
    fA   = new Float_t[ fNelem ];
    fA_p = new Float_t[ fNelem ];
    fA_c = new Float_t[ fNelem ];

    fIsInit = true;
  }

  // Read calibrations
  for (i=0;i<fNelem;i++)
    fscanf( fi, "%15f", fOff+i );                   // TDC offsets
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++)
    fscanf( fi, "%15f", fPed+i );                   // ADC pedestals
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++)
    fscanf( fi, "%15f", fGain+i);                   // ADC gains
  fgets ( buf, LEN, fi );

// ----- end Cherenkov ------------


// ------ THaShower --------------
  static const char* const here = "ReadDatabase()";
  const int LEN = 100;
  char buf[LEN];
  Int_t nelem, ncols, nrows, nclbl;

  // Read data from database

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // Blocks, rows, max blocks per cluster
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%5d %5d", &ncols, &nrows );

  nelem = ncols * nrows;
  nclbl = TMath::Min( 3, nrows ) * TMath::Min( 3, ncols );
  // Reinitialization only possible for same basic configuration
  if( fIsInit && (nelem != fNelem || nclbl != fNclublk) ) {
    Error( Here(here), "Cannot re-initalize with different number of blocks or "
	   "blocks per cluster. Detector not re-initialized." );
    fclose(fi);
    return kInitError;
  }

  if( nrows <= 0 || ncols <= 0 || nclbl <= 0 ) {
    Error( Here(here), "Illegal number of rows or columns: "
	   "%d %d", nrows, ncols );
    fclose(fi);
    return kInitError;
  }
  fNelem = nelem;
  fNrows = nrows;
  fNclublk = nclbl;

  // Clear out the old detector map before reading a new one
  UShort_t mapsize = fDetMap->GetSize();
  delete [] fNChan;
  if( fChanMap ) {
    for( UShort_t i = 0; i<mapsize; i++ )
      delete [] fChanMap[i];
  }
  delete [] fChanMap;
  fDetMap->Clear();

  // Read detector map

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  while (1) {
    Int_t crate, slot, first, last;
    fscanf ( fi,"%6d %6d %6d %6d", &crate, &slot, &first, &last );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	    THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }
  }

  // Set up the new channel map
  mapsize = fDetMap->GetSize();
  if( mapsize == 0 ) {
    Error( Here(here), "No modules defined in detector map.");
    fclose(fi);
    return kInitError;
  }

  fNChan = new UShort_t[ mapsize ];
  fChanMap = new UShort_t*[ mapsize ];
  for( UShort_t i=0; i < mapsize; i++ ) {
    THaDetMap::Module* module = fDetMap->GetModule(i);
    fNChan[i] = module->hi - module->lo + 1;
    if( fNChan[i] > 0 )
      fChanMap[i] = new UShort_t[ fNChan[i] ];
    else {
      Error( Here(here), "No channels defined for module %d.", i);
      delete [] fNChan; fNChan = 0;
      for( UShort_t j=0; j<i; j++ )
	delete [] fChanMap[j];
      delete [] fChanMap; fChanMap = 0;
      fclose(fi);
      return kInitError;
    }
  }
  // Read channel map
  //
  // Loosen the formatting restrictions: remove from each line the portion
  // after a '#', and do the pattern matching to the remaining string
  fgets ( buf, LEN, fi );

  // get the line and end it at a '#' symbol
  *buf = '\0';
  char *ptr=buf;
  int nchar=0;
  for ( UShort_t i = 0; i < mapsize; i++ ) {
    for ( UShort_t j = 0; j < fNChan[i]; j++ ) {
      while ( !strpbrk(ptr,"0123456789") ) {
	fgets ( buf, LEN, fi );
	if( (ptr = strchr(buf,'#')) ) *ptr = '\0';
	ptr = buf;
	nchar=0;
      }
      sscanf (ptr, "%6hu %n", *(fChanMap+i)+j, &nchar );
      ptr += nchar;
    }
  }

  fgets ( buf, LEN, fi );

  Float_t x,y,z;
  fscanf ( fi, "%15f %15f %15f", &x, &y, &z );               // Detector's X,Y,Z coord
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%15lf %15lf %15lf", fSize, fSize+1, fSize+2 );  // Sizes of det in X,Y,Z
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  Float_t angle;
  fscanf ( fi, "%15f", &angle );                       // Rotation angle of det
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  const Double_t degrad = TMath::Pi()/180.0;

  DefineAxes(angle*degrad);

  // Dimension arrays
  if( !fIsInit ) {
    fBlockX = new Float_t[ fNelem ];
    fBlockY = new Float_t[ fNelem ];
    fPed    = new Float_t[ fNelem ];
    fGain   = new Float_t[ fNelem ];

    // Per-event data
    fA    = new Float_t[ fNelem ];
    fA_p  = new Float_t[ fNelem ];
    fA_c  = new Float_t[ fNelem ];
    fNblk = new Int_t[ fNclublk ];
    fEblk = new Float_t[ fNclublk ];

    fIsInit = true;
  }

  fscanf ( fi, "%15f %15f", &x, &y );                  // Block 1 center position
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  Float_t dx, dy;
  fscanf ( fi, "%15f %15f", &dx, &dy );                // Block spacings in x and y
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%15f", &fEmin );                       // Emin thresh for center
  fgets ( buf, LEN, fi );

  // Read calibrations.
  // Before doing this, search for any date tags that follow, and start reading from
  // the best matching tag if any are found. If none found, but we have a configuration
  // string, search for it.
  if( SeekDBdate( fi, date ) == 0 && fConfig.Length() > 0 &&
      SeekDBconfig( fi, fConfig.Data() )) {}

  fgets ( buf, LEN, fi );
  // Crude protection against a missed date/config tag
  if( buf[0] == '[' ) fgets ( buf, LEN, fi );

  // Read ADC pedestals and gains (in order of logical channel number)
  for (int j=0; j<fNelem; j++)
    fscanf (fi,"%15f",fPed+j);
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (int j=0; j<fNelem; j++)
    fscanf (fi, "%15f",fGain+j);


  // Compute block positions
  for( int c=0; c<ncols; c++ ) {
    for( int r=0; r<nrows; r++ ) {
      int k = nrows*c + r;
      fBlockX[k] = x + r*dx;                         // Units are meters
      fBlockY[k] = y + c*dy;
    }
  }
  fclose(fi);

// ---- end Shower --------------

// ---- THaTotalShower ----

  fgets ( line, LEN, fi ); fgets ( line, LEN, fi );          
  fscanf ( fi, "%15f %15f", &fMaxDx, &fMaxDy );  // Max diff of shower centers

// ---- end TotalShower --------------


// ----- THaBPM ---------

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];

  FILE* fi = OpenFile( date );
  if( !fi) return kInitError;

  // okay, this needs to be changed, but since i dont want to re- or pre-invent 
  // the wheel, i will keep it ugly and read in my own configuration file with 
  // a very fixed syntax: 

  sprintf(keyword,"[%s_detmap]",GetName());
  Int_t n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here(here), "Unexpected end of BPM configuration file");
    fclose(fi);
    return kInitError;
  }

  // again that is not really nice, but since it will be changed anyhow:
  // i dont check each time for end of file, needs to be improved

  fDetMap->Clear();
  int first_chan, crate, dummy, slot, first, last, modulid;
  do {
    fgets( buf, LEN, fi);
    sscanf(buf,"%6d %6d %6d %6d %6d %6d %6d",
	   &first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if (first_chan>=0) {
      if ( fDetMap->AddModule (crate, slot, first, last, first_chan )<0) {
	Error( Here(here), "Couldnt add BPM to DetMap. Good bye, blue sky, good bye!");
	fclose(fi);
	return kInitError;
      }
    }
  } while (first_chan>=0);


  sprintf(keyword,"[%s]",GetName());
  n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here("ReadDataBase()"), "Unexpected end of BPM configuration file");
    fclose(fi);
    return kInitError;
  }

  double dummy1,dummy2,dummy3,dummy4,dummy5,dummy6;
  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf",&dummy1,&dummy2,&dummy3,&dummy4);

  fOffset(2)=dummy1;  // z position of the bpm
  fCalibRot=dummy2;   // calibration constant, historical,
                      // using 0.01887 results in matrix elements 
                      // between 0.0 and 1.0
                      // dummy3 and dummy4 are not used in this 
                      // apparatus, but might be useful for the struck

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf",&dummy1,&dummy2,&dummy3,&dummy4);

  fPedestals(0)=dummy1;
  fPedestals(1)=dummy2;
  fPedestals(2)=dummy3;
  fPedestals(3)=dummy4;

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 &dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);

  fRot2HCSPos(0,0)=dummy1;
  fRot2HCSPos(0,1)=dummy2;
  fRot2HCSPos(1,0)=dummy3;
  fRot2HCSPos(1,1)=dummy4;
  

  fOffset(0)=dummy5;
  fOffset(1)=dummy6;

// ---- end THaBPM ----


//--- THaCoincTime ----

  const int LEN = 200;
  char buf[LEN];

  FILE* fi = OpenFile( date );
  if( !fi ) {
    // look for more general coincidence-timing database
    fi = OpenFile( "CT", date );
  }
  if ( !fi )
    return kFileError;

  //  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fDetMap->Clear();
  
  int cnt = 0;
  
  fTdcRes[0] = fTdcRes[1] = 0.;
  fTdcOff[0] = fTdcOff[1] = 0.;
  
  while ( 1 ) {
    Int_t model;
    Float_t tres;   //  TDC resolution
    Float_t toff;   //  TDC offset (in seconds)
    char label[21]; // string label for TDC = Stop_by_Start
                    // Label is a space-holder to be used in the future
    
    Int_t crate, slot, first, last;

    while ( ReadComment( fi, buf, LEN ) ) {}

    fgets ( buf, LEN, fi );
    
    int nread = sscanf( buf, "%6d %6d %6d %6d %15f %20s %15f",
			&crate, &slot, &first, &model, &tres, label, &toff );
    if ( crate < 0 ) break;
    if ( nread < 6 ) {
      Error( Here(here), "Invalid detector map! Need at least 6 columns." );
      fclose(fi);
      return kInitError;
    }
    last = first; // only one channel per entry (one ct measurement)
    // look for the label in our list of spectrometers
    int ind = -1;
    for (int i=0; i<2; i++) {
      // enforce logical channel number 0 == 2by1, etc.
      // matching between fTdcLabels and the detector map
      if ( fTdcLabels[i] == label ) {
	ind = i;
	break;
      }
    }
    if (ind <0) {
      TString listoflabels;
      for (int i=0; i<2; i++) {
	listoflabels += " " + fTdcLabels[i];
      }
      listoflabels += '\0';
      Error( Here(here), "Invalid detector map! The timing measurement %s does not"
	     " correspond\n to the spectrometers. Expected one of \n"
	     "%s",label, listoflabels.Data());
      fclose(fi);
      return kInitError;
    }
    
    if( fDetMap->AddModule( crate, slot, first, last, ind, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
             THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }

    if ( ind+(last-first) < 2 )
      for (int j=ind; j<=ind+(last-first); j++)  {
	fTdcRes[j] = tres;
	if (nread>6) fTdcOff[j] = toff;
      }
    else 
      Warning( Here(here), "Too many entries. Expected 2 but found %d",
	       cnt+1);
    cnt+= (last-first+1);
  }

  fclose(fi);

  return kOK;
}

// --- end THaCoincTime ----

// --- THaRaster -----

  const int LEN=100;
  char buf[LEN];
  char *filestatus;
  char keyword[LEN];
  
  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // okay, this needs to be changed, but since i dont want to re- or pre-invent 
  // the wheel, i will keep it ugly and read in my own configuration file with 
  // a very fixed syntax: 

  // Seek our detmap section (e.g. "Raster_detmap")
  sprintf(keyword,"[%s_detmap]",GetName());
  Int_t n=strlen(keyword);

  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here("ReadDataBase()"), "Unexpected end of raster configuration file");
    fclose(fi);
    return kInitError;
  }

  // again that is not really nice, but since it will be changed anyhow:
  // i dont check each time for end of file, needs to be improved

  fDetMap->Clear();
  int first_chan, crate, dummy, slot, first, last, modulid;

  do {
    fgets( buf, LEN, fi);
    sscanf(buf,"%6d %6d %6d %6d %6d %6d %6d", 
	   &first_chan, &crate, &dummy, &slot, &first, &last, &modulid);
    if (first_chan>=0) {
      if ( fDetMap->AddModule (crate, slot, first, last, first_chan )<0) {
	Error( Here(here), "Couldnt add Raster to DetMap. Good bye, blue sky, good bye!");
	fclose(fi);
	return kInitError;
      }
    }
  } while (first_chan>=0);

  // Seek our database section
  sprintf(keyword,"[%s]",GetName());
  n=strlen(keyword);
  do {
    filestatus=fgets( buf, LEN, fi);
  } while ((filestatus!=NULL)&&(strncmp(buf,keyword,n)!=0));
  if (filestatus==NULL) {
    Error( Here("ReadDataBase()"), "Unexpected end of raster configuration file");
    fclose(fi);
    return kInitError;
  }
  double dummy1,dummy2,dummy3,dummy4,dummy5,dummy6,dummy7;
  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf %15lf",
	 &dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6,&dummy7);
  fRasterFreq(0)=dummy2;
  fRasterFreq(1)=dummy3;

  fRasterPedestal(0)=dummy4;
  fRasterPedestal(1)=dummy5;

  fSlopePedestal(0)=dummy6;
  fSlopePedestal(1)=dummy7;

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf",&dummy1);
  fPosOff[0].SetZ(dummy1);
  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf",&dummy1);
  fPosOff[1].SetZ(dummy1);
  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf",&dummy1);
  fPosOff[2].SetZ(dummy1);

  // Find timestamp, if any, for the raster constants. 
  // Give up and rewind to current file position on any non-matching tag.
  // Timestamps should be in ascending order
  SeekDBdate( fi, date, true );

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 &dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);
  fRaw2Pos[0](0,0)=dummy3;
  fRaw2Pos[0](1,1)=dummy4;
  fRaw2Pos[0](0,1)=dummy5;
  fRaw2Pos[0](1,0)=dummy6;
  fPosOff[0].SetX(dummy1);
  fPosOff[0].SetY(dummy2);

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 &dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);
  fRaw2Pos[1](0,0)=dummy3;
  fRaw2Pos[1](1,1)=dummy4;
  fRaw2Pos[1](0,1)=dummy5;
  fRaw2Pos[1](1,0)=dummy6;
  fPosOff[1].SetX(dummy1);
  fPosOff[1].SetY(dummy2);

  fgets( buf, LEN, fi);
  sscanf(buf,"%15lf %15lf %15lf %15lf %15lf %15lf",
	 &dummy1,&dummy2,&dummy3,&dummy4,&dummy5,&dummy6);
  fRaw2Pos[2](0,0)=dummy3;
  fRaw2Pos[2](1,1)=dummy4;
  fRaw2Pos[2](0,1)=dummy5;
  fRaw2Pos[2](1,0)=dummy6;
  fPosOff[2].SetX(dummy1);
  fPosOff[2].SetY(dummy2);

// ----- end Raster ----

// ----- THaTriggerTime ----

  const int LEN = 200;
  char buf[LEN];

  // first is the list of channels to watch to determine the event type
  // This could just come from THaDecData, but for robustness we need
  // another copy.

  // Read data from database 
  FILE* fi = OpenFile( date );
  // however, this is not unexpected since most of the time it is un-necessary
  if( !fi ) return kOK;
  
  while ( ReadComment( fi, buf, LEN ) ) {}
  
  // Read in the time offsets, in the format below, to be subtracted from
  // the times measured in other detectors.
  //
  // TrgType 0 is special, in that it is a global offset that is applied
  //  to all triggers. This gives us a simple single value for adjustments.
  //
  // Trigger types NOT listed are assumed to have a zero offset.
  // 
  // <TrgType>   <time offset in seconds>
  // eg:
  //   0              10   -0.5e-9  # global-offset shared by all triggers and s/TDC
  //   1               0       crate slot chan
  //   2              10.e-9
  int trg;
  float toff;
  float ch2t=-0.5e-9;
  int crate,slot,chan;
  fTrgTypes.clear();
  fToffsets.clear();
  fTDCRes = -0.5e-9; // assume 1872 TDC's.
  
  while ( fgets(buf,LEN,fi) ) {
    int fnd = sscanf( buf,"%8d %16f %16f",&trg,&toff,&ch2t);
    if( fnd < 2 ) continue;
    if( trg == 0 ) {
      fGlOffset = toff;
      fTDCRes = ch2t;
    }
    else {
      fnd = sscanf( buf,"%8d %16f %8d %8d %8d",&trg,&toff,&crate,&slot,&chan);
      if( fnd != 5 ) {
	cerr << "Cannot parse line: " << buf << endl;
	continue;
      }
      fTrgTypes.push_back(trg);
      fToffsets.push_back(toff);
      fDetMap->AddModule(crate,slot,chan,chan,trg);
    }
  }    
  fclose(fi);

  // now construct the appropriate arrays
  delete [] fTrgTimes;
  fNTrgType = fTrgTypes.size();
  fTrgTimes = new Double_t[fNTrgType];
//   for (unsigned int i=0; i<fTrgTypes.size(); i++) {
//     if (fTrgTypes[i]==0) continue;
//     fTrg_gl.push_back(gHaVars->Find(Form("%s.bit%d",fDecDataNm.Data(),
// 					 fTrgTypes[i])));
//   }

// ---- End TriggerTime ---
