// Global initialization of Podd library for unit tests

#ifdef HAVE_CATCH3
# include <catch2/reporters/catch_reporter_event_listener.hpp>
# include <catch2/reporters/catch_reporter_registrars.hpp>
# define EVENT_LISTENER_BASE EventListenerBase
#else
# define CATCH_CONFIG_MAIN
# include <catch2/catch.hpp>
# define EVENT_LISTENER_BASE TestEventListenerBase
#endif

#include "THaGlobals.h"
#include "THaVarList.h"

class testRunListener : public Catch::EVENT_LISTENER_BASE {
public:
  using Catch::EVENT_LISTENER_BASE::EVENT_LISTENER_BASE;

  void testRunStarting(Catch::TestRunInfo const&) override {
    delete gHaVars;
    gHaVars = new THaVarList;
  }

  void testRunEnded( Catch::TestRunStats const& ) override {
    delete gHaVars;
    gHaVars = nullptr;
  }

};

CATCH_REGISTER_LISTENER(testRunListener)
