/**
 * @file   RandomManagerTest_module.cc
 * @brief  Test of the random engine managing interface of NuRandomService
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   February 19th, 2015
 */


// art extensions
#define NUTOOLS_RANDOMUTILS_NURANDOMSERVICE_USECLHEP 1 // to have NuSeedService.h define CLHEPengineSeeder
#include "nutools/RandomUtils/NuRandomService.h"

#include "test/RandomUtils/SeedTestUtils.h"

// C++ includes.
#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <algorithm> // std::generate()
#include <iomanip> // std::setw()

// CLHEP libraries
#include "CLHEP/Random/RandomEngine.h" // CLHEP::HepRandomEngine
#include "CLHEP/Random/Ranlux64Engine.h" // CLHEP::Ranlux64Engine
#include "CLHEP/Random/RandFlat.h"

// Supporting library include files
#include "messagefacility/MessageLogger/MessageLogger.h"

// Framework includes.
#include "canvas/Utilities/Exception.h"
#include "cetlib/exempt_ptr.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/detail/EngineCreator.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/RandomNumberGenerator.h"

namespace testing {
  
  /**
   * @brief Test module for random engine managing interface of NuRandomService
   * 
   * The test writes on screen the random seeds it gets.
   * 
   * Configuration parameters:
   * - *instanceNames* (string list, optional): use one random number
   *   generator for each instance name here specified; if not specified,
   *   an anonymous engine is used
   * - *externalInstance* (string, optional): if specified, an engine not
   *   managed by RandomNumberGenerator is also used, with this instance name
   * - *standardInstance* (string, optional): if specified, an engine
   *   is created by RandomNumberGenerator but not registered is NuRandomService
   *   is also used, with this instance name
   * - *Seed*, *Seed_XXX* (strings, optional): set the seed of instance `XXX`
   *   to a set value ("Seed" sets the seed of the anonymous instance)
   * 
   */
  class RandomManagerTest: public art::EDAnalyzer {
      public:
    typedef art::detail::EngineCreator::seed_t seed_t;
    
    explicit RandomManagerTest(fhicl::ParameterSet const& pset);
    
    void analyze(const art::Event& event) override;
    
      private:
    std::string moduleLabel;
    
    std::map<std::string, cet::exempt_ptr<CLHEP::HepRandomEngine>> engines;
    std::unique_ptr<CLHEP::HepRandomEngine> extEngine{nullptr};
    cet::exempt_ptr<CLHEP::HepRandomEngine> stdEngine{nullptr};
  }; // class RandomManagerTest
  
  
  
  //****************************************************************************
  //--- RandomManagerTest implementation
  //---
  RandomManagerTest::RandomManagerTest(fhicl::ParameterSet const& pset):
    art::EDAnalyzer{pset},
    moduleLabel{pset.get<std::string>("module_label")}
  {
    art::ServiceHandle<rndm::NuRandomService> EngineManager;
    
    // check if we want an "external" engine
    std::string externalInstanceName{};
    if (pset.get_if_present("externalInstance", externalInstanceName)) {
      mf::LogInfo("RandomManagerTest") << "Creating an unmanaged engine '"
        << externalInstanceName << "' in module '" << moduleLabel << "'";
      extEngine = std::make_unique<CLHEP::Ranlux64Engine>();
      
      EngineManager->registerEngine(
        [this](rndm::NuRandomService::EngineId const&, seed_t seed)
          { this->extEngine->setSeed(seed, 0); },
        externalInstanceName, pset, "Seed_" + externalInstanceName
        );
    } // if we have the external engine
    
    // check if we want an unmanaged standard engine
    std::string standardInstanceName{};
    if (pset.get_if_present("standardInstance", standardInstanceName)) {
      mf::LogInfo("RandomManagerTest") << "Creating a standard engine '"
        << standardInstanceName << "' in module '" << moduleLabel
        << "' with RandomNumberGenerator";
      seed_t seed
        = pset.get<unsigned int>("Seed_" + standardInstanceName, 0);
      stdEngine = &createEngine(seed, "HepJamesRandom", standardInstanceName);
    } // if we have the external engine
    
    // initialize the standard engines with RandomNumberGenerator
    auto const instanceNames = pset.get<std::vector<std::string>>("instanceNames", {});
      for (std::string const& instanceName: instanceNames) {
        mf::LogInfo("RandomManagerTest") << "Creating a default engine '"
                                       << instanceName << "' in module '" << moduleLabel << "'";
      auto& engine = EngineManager->createEngine
          (*this, "HepJamesRandom", instanceName, pset, "Seed_" + instanceName);
      engines.emplace(instanceName, &engine);
    }
    
    // create a default engine, if needed
    if (instanceNames.empty() && !extEngine && !stdEngine) {
      mf::LogInfo("RandomManagerTest")
        << "Creating a nameless default engine in module '"
        << moduleLabel << "'";
      auto& engine = EngineManager->createEngine(*this, pset, "Seed");
      engines.emplace("", &engine);
    }
    
    { // anonymous block
      mf::LogInfo log("RandomManagerTest");
      log << "RandomManagerTest[" << moduleLabel << "]: instances:";
      for (std::string const& instanceName: instanceNames)
        log << " " << instanceName;
    } // anonymous block
    
    // Add non-art-managed engines to list of engines
    if (extEngine) {
      assert(!externalInstanceName.empty());
      engines.emplace(externalInstanceName, extEngine.get());
    }
    
    if (stdEngine) {
      assert(!standardInstanceName.empty());
      engines.emplace(standardInstanceName, stdEngine);
    }
    
  } // RandomManagerTest::RandomManagerTest()


  //----------------------------------------------------------------------------
  void RandomManagerTest::analyze(const art::Event& event)
  {
    mf::LogVerbatim("RandomManagerTest") << "RandomManagerTest[" << moduleLabel << "]::analyze "
                                         << event.id();
      
    for (auto& [instanceName, engine] : engines) {
      seed_t actualSeed = testing::NuRandomService::readSeed(*engine);
      mf::LogVerbatim("RandomManagerTest")
        << std::setw(12) << (instanceName.empty()? "<default>": instanceName)
        << ": " << testing::NuRandomService::CreateCharacter(*engine)
        << "   (seed: " << actualSeed << ")";
    }
  } // RandomManagerTest::analyze()
  
  
} // end namespace testing

DEFINE_ART_MODULE(testing::RandomManagerTest)
