/**
 * @file   ValidatedConfigSeedTest_module.cc
 * @brief  Tests setting of seed via validated configuration.
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   March 18, 2016
 */


// support libraries
#include "nurandom/RandomUtils/NuRandomService.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Registry/ServiceDefinitionMacros.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/types/OptionalAtom.h"

// scientific libraries
#include "CLHEP/Random/RandomEngine.h" // CLHEP::HepRandomEngine


// forward declarations
namespace art {
  class Run;
  class SubRun;
  class Event;
} // namespace art

// -----------------------------------------------------------------------------
namespace testing { class ValidatedConfigSeedTest; }

/**
 * @brief Test module for `rndm::NuRandomService`.
 *
 * The test tries to set seeds for engines from validated configuration.
 * It initializes three random generator engines, the seed of two of which can
 * be controlled via configuration (including validation).
 * The seed of the third engine is fully under `rndm::NuRandomService` control.
 *
 *
 * Configuration parameters
 * -------------------------
 *
 * * **SeedOne**: seed for random engine `"one"`
 * * **SeedTwo**: seed for random engine `"two"`
 *
 */
class testing::ValidatedConfigSeedTest: public art::EDAnalyzer {

  CLHEP::HepRandomEngine& fEngineOne; ///< Random engine "one".
  CLHEP::HepRandomEngine& fEngineTwo; ///< Random engine "two".
  CLHEP::HepRandomEngine& fEngineThree; ///< Random engine "three".

    public:

  struct Config {

    using Name = fhicl::Name;
    using Comment = fhicl::Comment;

    rndm::SeedAtom SeedOne{
      Name("SeedOne"),
      Comment("optional seed for engine \"one\"")
      };
    fhicl::OptionalAtom<rndm::NuRandomService::seed_t> SeedTwo{
      Name("SeedTwo"),
      Comment("optional seed for engine \"two\"")
      };

  }; // struct Config

  using Parameters = art::EDAnalyzer::Table<Config>;

  explicit ValidatedConfigSeedTest(Parameters const& config);

  virtual void analyze(art::Event const&) override {}

}; // class testing::ValidatedConfigSeedTest


//----------------------------------------------------------------------------
testing::ValidatedConfigSeedTest::ValidatedConfigSeedTest
  (Parameters const& config)
  : art::EDAnalyzer(config)
  , fEngineOne(art::ServiceHandle<rndm::NuRandomService>()->registerAndSeedEngine
               (createEngine(0, "HepJamesRandom", "one"), "HepJamesRandom", "one", config().SeedOne))
  , fEngineTwo(art::ServiceHandle<rndm::NuRandomService>()->registerAndSeedEngine
               (createEngine(0, "HepJamesRandom", "two"), "HepJamesRandom", "two", config().SeedTwo))
  , fEngineThree(art::ServiceHandle<rndm::NuRandomService>()->registerAndSeedEngine
                 (createEngine(0, "HepJamesRandom", "three"), "HepJamesRandom", "three"))
{

  auto const& randomService = *(art::ServiceHandle<rndm::NuRandomService>());

  rndm::NuRandomService::seed_t seed, expectedSeed;
  std::string engineName;

  engineName = "one";
  seed = randomService.getCurrentSeed(engineName);
  if (config().SeedOne(expectedSeed)) {
    if (seed != expectedSeed) {
      throw art::Exception(art::errors::LogicError)
        << "Seed for engine '" << engineName << "' expected to be "
        << expectedSeed << ", got " << seed << "\n";
    }
  }
  mf::LogVerbatim("ValidatedConfigSeedTest")
    << "Engine '" << engineName << "' seeded with " << seed;

  engineName = "two";
  seed = randomService.getCurrentSeed(engineName);
  if (config().SeedTwo(expectedSeed)) {
    if (seed != expectedSeed) {
      throw art::Exception(art::errors::LogicError)
        << "Seed for engine '" << engineName << "' expected to be "
        << expectedSeed << ", got " << seed << "\n";
    }
  }
  mf::LogVerbatim("ValidatedConfigSeedTest")
    << "Engine '" << engineName << "' seeded with " << seed;

  engineName = "three";
  seed = randomService.getCurrentSeed(engineName);
  mf::LogVerbatim("ValidatedConfigSeedTest")
    << "Engine '" << engineName << "' seeded with " << seed;

} // testing::ValidatedConfigSeedTest::ValidatedConfigSeedTest()


//----------------------------------------------------------------------------
DEFINE_ART_MODULE(testing::ValidatedConfigSeedTest)


//----------------------------------------------------------------------------
