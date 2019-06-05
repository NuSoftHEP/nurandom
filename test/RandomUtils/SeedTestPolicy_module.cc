/**
 * @file SeedTestPolicy_module.cc
 * @brief Test the NuRandomService.
 * @author Rob Kutschke (kutschke@fnal.gov), Gianluca Petrillo (petrillo@fnal.gov)
 * @see NuRandomService.hh
 */

// test library
#include "SeedTestUtils.h"

// nurandom libraries
#define NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP 1
#include "nurandom/RandomUtils/NuRandomService.h"

// framework
#include "canvas/Utilities/Exception.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/RandomNumberGenerator.h"

// supporting libraries
#include "messagefacility/MessageLogger/MessageLogger.h"

// CLHEP libraries
#include "CLHEP/Random/RandomEngine.h" // CLHEP::HepRandomEngine
#include "CLHEP/Random/JamesRandom.h" // CLHEP::HepJamesRandom
#include "CLHEP/Random/RandFlat.h"

// C/C++ standard library
#include <string>
#include <vector>
#include <sstream>
#include <iomanip> // std::setw()
#include <memory> // std::unique_ptr<>


namespace testing {

  /**
   * @brief Test module for NuRandomService
   *
   * The test writes on screen the random seeds it gets.
   *
   * Note that the test does not actually get any random number, unless the
   * *useGenerators* option is turned on.
   *
   * Configuration parameters:
   * - <b>instanceNames</b> (string list): use one random number generator
   *   for each instance name here specified; if not specified, a single
   *   default instance is used
   * - <b>expectedErrors</b> (unsigned integer, default: 0): expect this number
   *   of errors from NuRandomService, and throw an exception if we get a different
   *   number
   * - <b>useGenerators</b> (boolean, default: true): uses
   *   art RandomGeneratorService with the seeds from NuRandomService
   * - <b>perEventSeeds</b> (boolean, default: false): set it to true if the
   *   selected policy gives per-event seeds; in this case, the check of
   *   seed always being the same is skipped
   *
   */
  class SeedTestPolicy: public art::EDAnalyzer {

  public:

    explicit SeedTestPolicy(fhicl::ParameterSet const& pset);

    virtual void analyze(art::Event const& event) override;

    virtual void endJob() override;

  private:
    using seed_t = testing::NuRandomService::seed_t;

    std::vector<std::string> instanceNames;
    std::map<std::string, seed_t> startSeeds; ///< seeds after the constructor
    unsigned int nExpectedErrors; ///< number of expected errors
    bool useGenerators; ///< instanciate and use random number generators
    bool perEventSeeds; ///< whether we expect different seeds on each event
    std::string const moduleLabel; ///< configured module label

    unsigned int nErrors = 0; ///< Number of errors detected so far

    std::unique_ptr<CLHEP::HepRandomEngine> localEngine; ///< self-managed
    std::map<std::string, CLHEP::HepRandomEngine*> engines;

    /// Returns whether the engine associated with the specified index is local
    bool isLocalEngine(size_t iEngine) const;

    seed_t verifySeed(CLHEP::HepRandomEngine&, std::string const& instanceName);

    seed_t obtainSeed(std::string instanceName = "");

    /// Returns whether e is an exception we can handle (and, if so, it handles)
    bool handleSeedServiceException(art::Exception& e);

  }; // class SeedTestPolicy

} // namespace testing



//------------------------------------------------------------------------------
//--- implementation
//---

testing::SeedTestPolicy::SeedTestPolicy(fhicl::ParameterSet const& pset)
  : art::EDAnalyzer(pset)
  , instanceNames  (pset.get<std::vector<std::string>>("instanceNames", {}))
  , nExpectedErrors(pset.get<unsigned int>            ("expectedErrors", 0U))
  , useGenerators  (pset.get<bool>                    ("useGenerators", true))
  , perEventSeeds  (pset.get<bool>                    ("perEventSeeds", false))
  , moduleLabel    (pset.get<std::string>             ("module_label"))
{

  //
  // print some configuration information
  //
  { // anonymous block
    mf::LogInfo log("SeedTestPolicy");
    log << "Construct SeedTestPolicy with "
        << instanceNames.size() <<  " engine instances:";
    for (auto const& instanceName: instanceNames)
      log << " " << instanceName;

  } // anonymous block

  auto* Seeds = &*(art::ServiceHandle<rndm::NuRandomService>());

  // by default, have at least one, default engine instance
  if (instanceNames.empty()) instanceNames.push_back("");

  mf::LogInfo log("SeedTestPolicy"); // cumulative log

  //
  // register all the engines, and store their seeds
  //
  for (std::string const& instanceName: instanceNames) {
    seed_t const seed = obtainSeed(instanceName);
    log << "\nSeed for '" << instanceName << "' is: " << seed;
    startSeeds.emplace(instanceName, seed);
  } // for first loop (declaration)


  //
  // verify the seed of each instance
  //
  for (size_t iEngine = 0; iEngine < instanceNames.size(); ++iEngine) {
    std::string const& instanceName = instanceNames[iEngine];

    // This involved condition tree ensures that SeedMaster is queried
    // for a seed exactly once per instance, no matter what.
    // This is relevant for the error count.
    // Out of it, a seed is returned.
    seed_t seed = rndm::NuRandomService::InvalidSeed;
    if (isLocalEngine(iEngine)) {
      if (useGenerators) {
        localEngine = std::make_unique<CLHEP::HepJamesRandom>();
        engines.emplace(instanceName, localEngine.get());
        try {
          seed = Seeds->defineEngine(*localEngine, instanceName);
        }
        catch(art::Exception& e) {
          if (!handleSeedServiceException(e)) throw;
        }
        mf::LogInfo("SeedTestConstruct")
          << "Engine instance '" << instanceName
          << "' will be owned by the test module.";
      } // if use generators
      else seed = obtainSeed(instanceName);
    } // if local
    else { // if managed by art
      seed = obtainSeed(instanceName);
      if (useGenerators) {
        auto& engine = createEngine(seed, "HepJamesRandom", instanceName);
        // registration still matters for per-event policies
        Seeds->defineEngine(engine, instanceName);
        verifySeed(engine, instanceName);
        engines.emplace(instanceName, &engine);
      }
    } // if ... else

    // check that the seed returned by the service is still the same
    seed_t const expectedSeed = startSeeds.at(instanceName);
    if (seed != expectedSeed) {
      throw art::Exception(art::errors::LogicError)
        << "NuRandomService returned different seed values for engine instance '"
        << instanceName << "': first " << expectedSeed << ", now " << seed
        << "\n";
    } // if unexpected seed
  } // for second loop

  //
  // An engine with the following label has already been registered
  // (it's the one managed by RandomGeneratorService).
  // Registering another should raise an exception
  // (incidentally, if useGenerators is false we are trying to register nullptr)
  //
  bool bBug = false;
  try {
    Seeds->declareEngine(instanceNames.front());
    bBug = true;
  }
  catch(std::exception const& e) {
    if (!testing::NuRandomService::isSeedServiceException(e)) throw;
  }
  if (bBug) {
    throw art::Exception(art::errors::LogicError)
      << "Registration of local engine with duplicate label"
      " did not throw an exception";
  }

} // testing::SeedTestPolicy::SeedTestPolicy()


//------------------------------------------------------------------------------
void testing::SeedTestPolicy::analyze(art::Event const& event) {

  mf::LogVerbatim("SeedTestPolicy")
    << "SeedTestPolicy::analyze() " << event.id() << " with "
    << instanceNames.size() << " random engines";

  if (useGenerators) {
    for (auto const& instanceName : instanceNames) {
      //
      // collect information and resources
      //
      seed_t const startSeed = startSeeds.at(instanceName);
      CLHEP::HepRandomEngine& engine = *engines.at(instanceName);

      //
      // check seed (if per event, it should be the opposite)
      //
      seed_t const actualSeed = testing::NuRandomService::readSeed(engine);
      if (perEventSeeds) {
        if (actualSeed == startSeed) {
          // this has a ridiculously low chance of begin fortuitous
          throw art::Exception(art::errors::LogicError)
            << "per event seed " << actualSeed << " of engine '" << instanceName
            << "' is the same as at beginning!\n";
        }
      }
      else {
        if (actualSeed != startSeed) {
          throw art::Exception(art::errors::LogicError)
            << "expected seed " << startSeed << " for engine '" << instanceName
            << "', got " << actualSeed << " instead!\n";
        }
      }

      //
      // print character statistics
      //
      mf::LogVerbatim("SeedTestPolicy")
        << std::setw(12) << (instanceName.empty()? "<default>": instanceName)
        << ": " << testing::NuRandomService::CreateCharacter(engine)
        << "   (seed: " << actualSeed << ")";

    } // for
  } // if use generators

} // testing::SeedTestPolicy::analyze()


//------------------------------------------------------------------------------
void
testing::SeedTestPolicy::endJob()
{
  // if we have an unexpected amount of errors, bail out
  if (nExpectedErrors != nErrors) {
    art::Exception e(art::errors::Configuration);
    e << "SeedTestPolicy: detected " << nErrors << " errors";
    if (nExpectedErrors) e << ", " << nExpectedErrors << " expected";
    throw e << "!\n";
  }
} // testing::SeedTestPolicy::endJob()


//------------------------------------------------------------------------------

testing::SeedTestPolicy::seed_t
testing::SeedTestPolicy::obtainSeed(std::string instanceName /* = "" */)
{
  // Returns the seed for the specified engine instance, or 0 in case of
  // configuration error (in which case, an error counter is increased)
  seed_t seed = rndm::NuRandomService::InvalidSeed;
  try {
    art::ServiceHandle<rndm::NuRandomService> seeds;
    // currently (v0_00_03), the two calls are actually equivalent
    seed
      = instanceName.empty()? seeds->getSeed(): seeds->getSeed(instanceName);
  }
  catch(art::Exception& e) {
    if (!testing::NuRandomService::isSeedServiceException(e)) throw;

    ++nErrors;
    mf::LogError log("SeedTestPolicy");
    log << "Detected";
    if (nErrors > nExpectedErrors) log << " UNEXPECTED";
    log << " error #" << nErrors << ":\n" << e;
  }
  return seed;
} // testing::SeedTestPolicy::obtainSeed()


bool
testing::SeedTestPolicy::handleSeedServiceException(art::Exception& e)
{
  if (!testing::NuRandomService::isSeedServiceException(e)) return false;

  ++nErrors;
  mf::LogError log("SeedTest01");
  log << "Detected";
  if (nErrors > nExpectedErrors) log << " UNEXPECTED";
  log << " error #" << nErrors << ":\n" << e << "\n";
  return true;
} // testing::SeedTestPolicy::handleSeedServiceException()


bool testing::SeedTestPolicy::isLocalEngine(size_t i) const {
  return (i == 0) && (instanceNames.size() != 1);
} // testing::SeedTestPolicy::isLocalEngine()


testing::SeedTestPolicy::seed_t
testing::SeedTestPolicy::verifySeed(CLHEP::HepRandomEngine& engine,
                                    std::string const& instanceName)
{
  seed_t const actualSeed = testing::NuRandomService::readSeed(engine);
  seed_t const expectedSeed = startSeeds.at(instanceName);
  // if the expected seed is invalid, we are not even sure it was ever set;
  // the engine is in an invalid state and that's it
  if (!rndm::NuRandomService::isSeedValid(expectedSeed)) return actualSeed;

  if (actualSeed != expectedSeed) {
    throw art::Exception(art::errors::LogicError)
      << "expected seed " << expectedSeed << " for engine '" << instanceName
      << "', got " << actualSeed << " instead!";
  }
  return actualSeed;
} // testing::SeedTestPolicy::verifySeed()


//------------------------------------------------------------------------------
DEFINE_ART_MODULE(testing::SeedTestPolicy)

//------------------------------------------------------------------------------
