//============================================================================
// Name        : test2.cpp
// Author      : Mikhail Zulkarneev
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "fst/fst.h"
#include "fst/shm.h"
#include "fst/vector-fst.h"

#include "base/kaldi-error.h"
#include "util/kaldi-io.h"

using namespace std;
using namespace fst;


Fst<StdArc> *ReadFstKaldi(string rxfilename, bool use_shared) {
  if (rxfilename == "") rxfilename = "-"; // interpret "" as stdin,
  // for compatibility with OpenFst conventions.
  string md5 = "";

  if (use_shared) {
      ifstream iStr(rxfilename);
      md5 = GetHash(iStr);
      // md5 = kaldi::Input(rxfilename).Reopen_with_MD5();
      if (!md5.empty()) {
        //ShmModelsManagerBase::Model model = ShmModelsManagerBase::get_instance().create_model_empty(md5);
        //boost::interprocess::scoped_lock<boost::interprocess::interprocess_upgradable_mutex> lock(*model->get_mutex());
        //if (!model->is_empty() )
        bool in_memory = ShmModelsManagerBase::get_instance().is_in_memory(md5);
        if (in_memory)
        {
          std::ifstream fakeStream;
          FstReadOptions ropts;
          ropts.md5 = md5;
          Fst<StdArc> *fst = Fst<StdArc>::Read(fakeStream, ropts);
          if (!fst)
            KALDI_ERR << "Could not read memory fst from shared memory "
                      << kaldi::PrintableRxfilename(rxfilename);
          return fst;
        }
      }
  }

  kaldi::Input ki(rxfilename);

  fst::FstHeader hdr;
  if (!hdr.Read(ki.Stream(), rxfilename))
    KALDI_ERR << "Reading FST: error reading FST header from "
              << kaldi::PrintableRxfilename(rxfilename);
  FstReadOptions ropts("<unspecified>", &hdr);
  ropts.md5 = md5;

  Fst<StdArc> *fst = Fst<StdArc>::Read(ki.Stream(), ropts);
  if (!fst)
    KALDI_ERR << "Could not read fst from "
              << kaldi::PrintableRxfilename(rxfilename);
  return fst;
}


int main(int argc, char** argv) {

	Fst<fst::StdArc> *fst = ReadFstKaldi(string(argv[1]), true);

	cout << "" << endl; // prints 
	return 0;
}
