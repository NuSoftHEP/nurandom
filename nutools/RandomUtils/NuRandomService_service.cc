/**
 * @file NuRandomService_service.cc
 * @brief Assists in the distribution of guaranteed unique seeds to all engines within a job.
 * @author Rob Kutschke (kutschke@fnal.gov)
 * @see NuRandomService.h SeedMaster.h
 */

// NuRandomService header
#include "nutools/RandomUtils/NuRandomService.h"

// Art include files
#include "canvas/Utilities/Exception.h"
#include "art/Framework/Core/EngineCreator.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "canvas/Persistency/Provenance/ModuleDescription.h"
#include "canvas/Persistency/Provenance/EventID.h"

// Supporting library include files
#include "messagefacility/MessageLogger/MessageLogger.h"

// CLHEP libraries
#include "CLHEP/Random/RandomEngine.h" // CLHEP::HepRandomEngine

// C++ include files
#include <iostream>
#include <iomanip>

namespace rndm {

  //----------------------------------------------------------------------------
  NuRandomService::NuRandomService
    (fhicl::ParameterSet const& paramSet, art::ActivityRegistry& iRegistry)
    : seeds(paramSet)
    , state()
    , verbosity(paramSet.get<int>("verbosity", 0))
    , bPrintEndOfJobSummary(paramSet.get<bool>("endOfJobSummary",false))
  {
    state.transit_to(NuRandomServiceHelper::ArtState::inServiceConstructor);
    
    // Register callbacks.
    iRegistry.sPreModuleConstruction.watch  (this, &NuRandomService::preModuleConstruction  );
    iRegistry.sPostModuleConstruction.watch (this, &NuRandomService::postModuleConstruction );
    iRegistry.sPreModuleBeginRun.watch      (this, &NuRandomService::preModuleBeginRun      );
    iRegistry.sPostModuleBeginRun.watch     (this, &NuRandomService::postModuleBeginRun     );
    iRegistry.sPreProcessEvent.watch        (this, &NuRandomService::preProcessEvent        );
    iRegistry.sPreModule.watch              (this, &NuRandomService::preModule              );
    iRegistry.sPostModule.watch             (this, &NuRandomService::postModule             );
    iRegistry.sPostProcessEvent.watch       (this, &NuRandomService::postProcessEvent       );
    iRegistry.sPreModuleEndJob.watch        (this, &NuRandomService::preModuleEndJob        );
    iRegistry.sPostModuleEndJob.watch       (this, &NuRandomService::postModuleEndJob       );
    iRegistry.sPostEndJob.watch             (this, &NuRandomService::postEndJob             );
    
  } // NuRandomService::NuRandomService()
  
  
  
  //----------------------------------------------------------------------------
  NuRandomService::EngineId NuRandomService::qualify_engine_label
    (std::string moduleLabel, std::string instanceName) const
    { return { moduleLabel, instanceName }; }
  
  NuRandomService::EngineId NuRandomService::qualify_engine_label
    (std::string instanceName /* = "" */) const
    { return qualify_engine_label( state.moduleLabel(), instanceName); }
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::getSeed() {
    return getSeed(qualify_engine_label());
  } // NuRandomService::getSeed()
  
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::getSeed(std::string instanceName) {
    return getSeed(qualify_engine_label(instanceName));
  } // NuRandomService::getSeed(string)
  
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::getGlobalSeed(std::string instanceName) {
    EngineId ID(instanceName, EngineId::global);
    LOG_DEBUG("NuRandomService")
      << "NuRandomService::getGlobalSeed(\"" << instanceName << "\")";
    return getSeed(ID);
  } // NuRandomService::getGlobalSeed()
  
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::getSeed(EngineId const& id) {
    
    // We require an engine to have been registered before we yield seeds;
    // this should minimise unexpected conflicts.
    if (hasEngine(id)) return querySeed(id); // ask the seed to seed master
    
    // if it hasn't been declared, we declare it now
    // (this is for backward compatibility with the previous behaviour).
    // registerEngineID() will eventually call this function again to get the
    // seed... so we return it directly.
    // Also note that this effectively "freezes" the engine since no seeder
    // is specified.
    return registerEngineID(id);
        
  } // NuRandomService::getSeed(EngineId)
  
  
  NuRandomService::seed_t NuRandomService::querySeed(EngineId const& id) {
    return seeds.getSeed(id); // ask the seed to seed master
  } // NuRandomService::querySeed()
  
  
  std::pair<NuRandomService::seed_t, bool> NuRandomService::findSeed(
    EngineId const& id,
    fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
  ) {
    seed_t seed = InvalidSeed;
    // try and read the seed from configuration; if succeed, it's "frozen"
    bool const bFrozen = readSeedParameter(seed, pset, pnames);
    
    // if we got a valid seed, use it as frozen
    if (bFrozen && (seed != InvalidSeed))
      return { seed, true };  
    
    // seed was not good enough; get the seed from the master
    return { querySeed(id), false };
    
  } // NuRandomService::findSeed()
  
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::createEngine(
    art::EngineCreator& module, std::string type,
    std::string instance /* = "" */
  ) {
    EngineId id = qualify_engine_label(instance);
    const seed_t seed = prepareEngine(id, RandomNumberGeneratorSeeder);
    module.createEngine(seed, type, instance);
    mf::LogInfo("NuRandomService")
      << "Seeding " << type << " engine \"" << id.artName()
      << "\" with seed " << seed << ".";
    return seed;
  } // NuRandomService::createEngine(strings)
  
  
  NuRandomService::seed_t NuRandomService::createEngine(art::EngineCreator& module) {
    EngineId id = qualify_engine_label();
    const seed_t seed = prepareEngine(id, RandomNumberGeneratorSeeder);
    module.createEngine(seed);
    mf::LogInfo("NuRandomService")
      << "Seeding default-type engine \"" << id.artName()
      << "\" with seed " << seed << ".";
    return seed;
  } // NuRandomService::createEngine()
  
  
  NuRandomService::seed_t NuRandomService::createEngine(
    art::EngineCreator& module, std::string type,
    std::string instance,
    fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
  ) {
    EngineId id = qualify_engine_label(instance);
    registerEngineAndSeeder(id, RandomNumberGeneratorSeeder);
    std::pair<seed_t, bool> seedInfo = findSeed(id, pset, pnames);
    module.createEngine(seedInfo.first, type, instance);
    mf::LogInfo("NuRandomService")
      << "Seeding " << type << " engine \"" << id.artName()
      << "\" with seed " << seedInfo.first << ".";
    if (seedInfo.second) freezeSeed(id, seedInfo.first);
    return seedInfo.first;
  } // NuRandomService::createEngine(ParameterSet)
  
  
  NuRandomService::seed_t NuRandomService::createEngine(
    art::EngineCreator& module,
    fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
  ) {
    EngineId id = qualify_engine_label();
    registerEngineAndSeeder(id, RandomNumberGeneratorSeeder);
    std::pair<seed_t, bool> seedInfo = findSeed(id, pset, pnames);
    module.createEngine(seedInfo.first);
    mf::LogInfo("NuRandomService")
      << "Seeding default-type engine \"" << id.artName()
      << "\" with seed " << seedInfo.first << ".";
    if (seedInfo.second) freezeSeed(id, seedInfo.first);
    return seedInfo.first;
  } // NuRandomService::createEngine(ParameterSet)
  
  
  NuRandomService::seed_t NuRandomService::registerEngine
    (SeedMaster_t::Seeder_t seeder, std::string instance /* = "" */)
  {
    return registerEngineID(qualify_engine_label(instance), seeder);
  } // NuRandomService::registerEngine(Seeder_t, string)
  
  
  NuRandomService::seed_t NuRandomService::registerEngine(
    SeedMaster_t::Seeder_t seeder, std::string instance,
    fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
  ) {
    EngineId id = qualify_engine_label(instance);
    registerEngineAndSeeder(id, seeder);
    std::pair<seed_t, bool> seedInfo = findSeed(id, pset, pnames);
    seedEngine(id); // seed it before freezing
    if (seedInfo.second) freezeSeed(id, seedInfo.first);
    seed_t const seed = seedInfo.first;
    return seed;
  } // NuRandomService::registerEngine(Seeder_t, string, ParameterSet, init list)
  
    
  NuRandomService::seed_t NuRandomService::declareEngine(std::string instance) {
    return registerEngine(SeedMaster_t::Seeder_t(), instance);
  } // NuRandomService::declareEngine(string)
  
  
  NuRandomService::seed_t NuRandomService::declareEngine(
    std::string instance,
    fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
  ) {
    return registerEngine(SeedMaster_t::Seeder_t(), instance, pset, pnames);
  } // NuRandomService::declareEngine(string, ParameterSet, init list)
  
  
  NuRandomService::seed_t NuRandomService::defineEngine
    (SeedMaster_t::Seeder_t seeder, std::string instance /* = {} */)
  {
    return defineEngineID(qualify_engine_label(instance), seeder);
  } // NuRandomService::defineEngine(string, Seeder_t)
  
    
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::registerEngineID(
    EngineId const& id,
    SeedMaster_t::Seeder_t seeder /* = SeedMaster_t::Seeder_t() */
  ) {
    prepareEngine(id, seeder);
    return seedEngine(id);
  } // NuRandomService::registerEngine()
  
  
  NuRandomService::seed_t NuRandomService::defineEngineID
    (EngineId const& id, SeedMaster_t::Seeder_t seeder)
  {
    if (!hasEngine(id)) {
      throw art::Exception(art::errors::LogicError)
        << "Attempted to define engine '" << id.artName()
        << "', that was not declared\n";
    }
    
    if (seeds.hasSeeder(id)) {
      throw art::Exception(art::errors::LogicError)
        << "Attempted to redefine engine '" << id.artName()
        << "', that has already been defined\n";
    }
    
    ensureValidState();
    
    seeds.registerSeeder(id, seeder);
    seed_t const seed = seedEngine(id);
    return seed;
  } // NuRandomService::defineEngineID()
  
  
  //----------------------------------------------------------------------------
  void NuRandomService::ensureValidState(bool bGlobal /* = false */) const {
    if (bGlobal) {
      // registering engines may only happen in a service c'tor
      // In all other cases, throw.
      if ( (state.state() != NuRandomServiceHelper::ArtState::inServiceConstructor))
      {
        throw art::Exception(art::errors::LogicError)
          << "NuRandomService: not in a service constructor."
          << " May not register \"global\" engines.\n";
      }
    }
    else { // context-aware engine
      // registering engines may only happen in a c'tor
      // (disabling the ability to do that or from a beginRun method)
      // In all other cases, throw.
      if ( (state.state() != NuRandomServiceHelper::ArtState::inModuleConstructor)
      //  && (state.state() != NuRandomServiceHelper::ArtState::inModuleBeginRun)
        )
      {
        throw art::Exception(art::errors::LogicError)
          << "NuRandomService: not in a module constructor."
          << " May not register engines.\n";
      }
    } // if
  } // NuRandomService::ensureValidState()
  
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::reseedInstance(EngineId const& id) {
    // get all the information on the current process, event and module from
    // ArtState:
    SeedMaster_t::EventData_t const data(state.getEventSeedInputData());
    seed_t const seed = seeds.reseedEvent(id, data);
    if (seed == InvalidSeed) {
      mf::LogDebug("NuRandomService")
        << "No random seed specific to this event for engine '" << id << "'";
    }
    else {
      mf::LogInfo("NuRandomService") << "Random seed for this event, engine '"
        << id << "': " << seed;
    }
    return seed;
  } // NuRandomService::reseedInstance()
  
  
  void NuRandomService::reseedModule(std::string currentModule) {
    for (EngineId const& ID: seeds.engineIDsRange()) {
      if (ID.moduleLabel != currentModule) continue; // not our module? neeext!!
      reseedInstance(ID);
    } // for
  } // NuRandomService::reseedModule(string)
  
  void NuRandomService::reseedModule() { reseedModule(state.moduleLabel()); }
  
  
  void NuRandomService::reseedGlobal() {
    for (EngineId const& ID: seeds.engineIDsRange()) {
      if (!ID.isGlobal()) continue; // not global? neeext!!
      reseedInstance(ID);
    } // for
  } // NuRandomService::reseedGlobal()
  
  
  //----------------------------------------------------------------------------
  void NuRandomService::RandomNumberGeneratorSeeder
    (EngineId const& id, seed_t seed)
  {
    // no check is performed to verify that the current module is the one
    // specified in id.moduleLabel -- but that is required!
    art::ServiceHandle<art::RandomNumberGenerator> rng;
    CLHEP::HepRandomEngine& engine = rng->getEngine(id.instanceName);
    engine.setSeed(seed, 0); // the 0 is dummy... or so one hopes
  } // NuRandomService::RandomNumberGeneratorSeeder()
  
  
  //----------------------------------------------------------------------------
  void NuRandomService::registerEngineAndSeeder
    (EngineId const& id, SeedMaster_t::Seeder_t seeder)
  {
    // Are we being called from the right place?
    ensureValidState(id.isGlobal());
    
    if (hasEngine(id)) {
      throw art::Exception(art::errors::LogicError)
        << "NuRandomService: an engine with ID '" << id.artName()
        << "' has already been created!\n";
    }
    seeds.registerNewSeeder(id, seeder);
  } // NuRandomService::registerEngineAndSeeder()
  
  
  //----------------------------------------------------------------------------
  void NuRandomService::freezeSeed(EngineId const& id, seed_t frozen_seed)
    { seeds.freezeSeed(id, frozen_seed); }
  
  
  //----------------------------------------------------------------------------
  NuRandomService::seed_t NuRandomService::prepareEngine
    (EngineId const& id, SeedMaster_t::Seeder_t seeder)
  {
    registerEngineAndSeeder(id, seeder);
    return querySeed(id);
  } // NuRandomService::prepareEngine()
  
  
  //----------------------------------------------------------------------------
  bool NuRandomService::readSeedParameter(
    seed_t& seed, fhicl::ParameterSet const& pset,
    std::initializer_list<std::string> pnames
  ) {
    for (std::string const& key: pnames)
      if (pset.get_if_present(key, seed)) return true;
    seed = InvalidSeed;
    return false;
  } // NuRandomService::readSeedParameter()
  
  
  //----------------------------------------------------------------------------
  // Callbacks called by art.  Used to maintain information about state.
  void NuRandomService::preModuleConstruction(art::ModuleDescription const& md)  {
    state.transit_to(NuRandomServiceHelper::ArtState::inModuleConstructor);
    state.set_module(md);
  } // NuRandomService::preModuleConstruction()
  
  void NuRandomService::postModuleConstruction(art::ModuleDescription const&) {
    state.reset_state();
  } // NuRandomService::postModuleConstruction()
  
  void NuRandomService::preModuleBeginRun(art::ModuleDescription const& md) {
    state.transit_to(NuRandomServiceHelper::ArtState::inModuleBeginRun);
    state.set_module(md);
  } // NuRandomService::preModuleBeginRun()
  
  void NuRandomService::postModuleBeginRun(art::ModuleDescription const&) {
    state.reset_state();
  } // NuRandomService::postModuleBeginRun()
  
  void NuRandomService::preProcessEvent(art::Event const& evt) {
    state.transit_to(NuRandomServiceHelper::ArtState::inEvent);
    state.set_event(evt);
    seeds.onNewEvent(); // inform the seed master that a new event has come
    
    LOG_DEBUG("NuRandomService") << "preProcessEvent(): will reseed global engines";
    reseedGlobal(); // why don't we do them all?!?
    
  } // NuRandomService::preProcessEvent()
  
  void NuRandomService::preModule(art::ModuleDescription const& md) {
    state.transit_to(NuRandomServiceHelper::ArtState::inModuleEvent);
    state.set_module(md);
    
    // Reseed all the engine of this module... maybe
    // (that is, if the current policy alows it).
    LOG_DEBUG("NuRandomService") << "preModule(): will reseed engines for module '"
      << md.moduleLabel() << "'";
    reseedModule(md.moduleLabel());
  } // NuRandomService::preModule()
  
  void NuRandomService::postModule(art::ModuleDescription const&) {
    state.reset_module();
    state.reset_state();
  } // NuRandomService::postModule()
  
  void NuRandomService::postProcessEvent(art::Event const&) {
    state.reset_event();
    state.reset_state();
  } // NuRandomService::postProcessEvent()
  
  void NuRandomService::preModuleEndJob(art::ModuleDescription const& md) {
    state.transit_to(NuRandomServiceHelper::ArtState::inEndJob);
    state.set_module(md);
  } // NuRandomService::preModuleBeginRun()
  
  void NuRandomService::postModuleEndJob(art::ModuleDescription const&) {
    state.reset_state();
  } // NuRandomService::preModuleBeginRun()
  
  void NuRandomService::postEndJob() {
    if ((verbosity > 0) || bPrintEndOfJobSummary)
      print(); // framework logger decides whether and where it shows up
  } // NuRandomService::postEndJob()
  
  //----------------------------------------------------------------------------

} // end namespace rndm

DEFINE_ART_SERVICE(rndm::NuRandomService)
