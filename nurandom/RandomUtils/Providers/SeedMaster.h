/**
 * @file   SeedMaster.h
 * @brief  A class to assist in the distribution of guaranteed unique seeds
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   20141111
 * @see    NuRandomService.h SeedMaster.cc
 */

#ifndef NURANDOM_RANDOMUTILS_PROVIDERS_SEEDMASTER_H
#define NURANDOM_RANDOMUTILS_PROVIDERS_SEEDMASTER_H 1

// C/C++ standard libraries
#include <vector>
#include <string>
#include <map>
#include <memory> // std::unique_ptr<>
#include <utility> // std::forward()

// From art and its tool chain
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"

// Some helper classes
#include "nurandom/RandomUtils/Providers/PolicyFactory.h" // makeRandomSeedPolicy
#include "nurandom/RandomUtils/Providers/PolicyNames.h" // rndm::details::Policy
#include "nurandom/RandomUtils/Providers/MapKeyIterator.h"
#include "nurandom/RandomUtils/Providers/EngineId.h"
#include "nurandom/RandomUtils/Providers/Policies.h"
#include "nurandom/RandomUtils/Providers/EventSeedInputData.h"

// more headers included in the implementation section below


namespace rndm {
  
  /**
   * @brief A class to assist in the distribution of guaranteed unique seeds to all engine IDs.
   * @tparam SEED type of random engine seed
   * @see NuRandomService
   *
   * @attention direct use of this class is limited to art-less contexts;
   * within art, use `rndm::NuRandomService` instead.
   * 
   * This class is configured from a FHiCL parameter set.
   * The complete configuration depends on the policy chosen; the following
   * parameters are common to all the policies:
   *     
   *     NuRandomService : {
   *        policy           : "autoIncrement" // Required: Other legal value are listed in SEED_SERVICE_POLICIES
   *        verbosity        : 0               // Optional: default=0, no informational printout
   *        endOfJobSummary  : false           // Optional: print list of all managed seeds at end of job.
   *     }
   *     
   * The policy parameter tells the service to which algorithm to use.
   * If the value of the policy parameter is not one of the known policies, the code will
   * throw an exception.
   *
   * Code instanciating a SeedMaster can request a seed by making one of the
   * following two calls:
   *     
   *     SeedMasterInstance.getSeed("moduleLabel");
   *     SeedMasterInstance.getSeed("moduleLabel", "instanceName");
   *     
   * art module code requests a seed directly to NuRandomService service.
   *
   * It is the callers responsibility to use the appropriate form.
   *
   * When `getSeed` is called with a particular module label and instance name, it computes
   * a seed value, saves it and returns it.  If there is a subsequent call to `getSeed` with
   * the same module label and instance name, the class will return the saved value of
   * the seed.  The following text will use the phrase "unique calls to `getSeed`"; two
   * calls with the same module label and instance names are not considered unique.
   *
   * If the policy is defined as `autoIncrement`, the additional configurable
   * items are:
   *     
   *     NuRandomService : {
   *        policy           : "autoIncrement"
   *        // ... and all the common ones, plus:
   *        baseSeed         : 0     // Required: An integer >= 0.
   *        checkRange       : true  // Optional: legal values true (default) or false
   *        maxUniqueEngines : 20    // Required iff checkRange is true.
   *     }
   *     
   * In this policy, the seed is set to `baseSeed`+`offset`, where
   * on the first unique call to `getSeed` the `offset` is set to 0; on the
   * second unique call to `getSeed` it is set to 1, and so on.
   *
   * If the policy is defined as `linearMapping`, the additional configurable
   * items are:
   *     
   *     NuRandomService : {
   *        policy           : "linearMapping"
   *        // ... and all the common ones, plus:
   *        nJob             : 0     // Required: An integer >= 0.
   *        checkRange       : true  // Optional: legal values true (default) or false
   *        maxUniqueEngines : 20    // Required iff checkRange is true.
   *     }
   *     
   * In this policy, the seed is set to
   * `maxUniqueEngines`*`nJob`+`offset`, where on the first unique call to `getSeed`
   * the `offset` is set to 0; on the second unique call to `getSeed` it is set to 1,
   * and so on.
   *
   * If the policy is defined as `preDefinedOffset`, the additional configurable
   * items are:
   *     
   *     NuRandomService : {
   *        policy           : "preDefinedOffset"
   *        // ... and all the common ones, plus:
   *        baseSeed         : 0     // Required: An integer >= 0.
   *        checkRange       : true  // Optional: legal values true (default) or false
   *        maxUniqueEngines : 20    // Required iff checkRange is true
   *        
   *        module_label1: offset1      // for each module with a nameless engine
   *        module_label2: {            // for each module with nemed engine instances
   *          instance_name1: offset21  //   ... one entry for each instance name
   *          instance_name2: offset22
   *        }
   *     }
   *     
   * In this policy, when `getSeed` is called, the class will look
   * into the parameter set to find a defined offset for the specified module label
   * and instance name.  The returned value of the seed will be `baseSeed`+`offset`.
   *
   * If the policy is defined as `preDefinedSeed`, the additional configurable
   * items are:
   *     
   *     NuRandomService : {
   *        policy           : "preDefinedSeed"
   *        // ... and all the common ones, plus:
   *        
   *        module_label1: seed1      // for each module with a nameless engine
   *        module_label2: {          // for each module with nemed engine instances
   *          instance_name1: seed21  //   ... one entry for each instance name
   *          instance_name2: seed22
   *        }
   *     }
   *     
   * This policy allows to specify the actual seed to be used.
   * Note that the policy
   * does not impose any constraint on the user-provided set of seeds.
   * In particular, the uniqueness of the seeds is not
   * enforced. Intended for debugging and special tests, use with care.
   *
   * If the policy is defined as `random`, the additional configurable
   * items are:
   *     
   *     NuRandomService : {
   *        policy           : "random"
   *        // ... and all the common ones, plus:
   *        masterSeed: master_seed // optional: an integer >= 0
   *     }
   *     
   * With this policy, the seed is extracted from a local random number
   * generator.
   * The seed used to initialize this additional random number generatot is
   * taken from the clock, unless the `masterSeed` parameter is set to specify
   * the actual seed.
   * This policy is meant as a quick way to disentangle the code from the
   * random seed policy used, and it's meant for special needs only and
   * definitely not for production.
   * You can enable this policy instead of whatever is already in your
   * configuration by adding at the end of your configuration:
   *     
   *     services.NuRandomService.policy: "random"
   *     
   * (this assumes that the configuration of the SeedMaster is read from
   * `services.NuRandomService`, that is the case in the art framework).
   * 
   * The FHiCL grammar to specify the offsets takes two forms.  If no instance name
   * is given, the offset is given by:
   *     
   *     moduleLabel : offset
   *     
   * When a module has multiple instances, the offsets are given by:
   *     
   *     moduleLabel : {
   *        instanceName1 : offset1
   *        instanceName2 : offset2
   *     }
   *     
   * `SeedMaster` does several additional checks, except for the `preDefinedSeed` policy.
   *
   * If one (module label, instance name) has the same seed as another (module label, instance name),
   * the class will throw an exception.
   *
   * If the `checkRange` parameter is set to `true`, and if an offset is generated with a value outside
   * the allowed range (typically `0<= offset < maxUniqueEngines-1`) then the class will also throw an exception.
   *
   * It is the responsibility of the user to ensure that the parameters
   * (e.g. `nJob` and `maxUniqueEngines`) are chosen
   * it a way that ensures the required level of uniqueness of seeds.  The example grid jobs have
   * a single point of maintenance to achieve this: the user must specify the starting job number
   * for each grid submission.
   */
  template <typename SEED>
  class SeedMaster {
      public:
    using seed_t = SEED;                   ///< type of served seeds
    using SeedMaster_t = SeedMaster<SEED>; ///< type of this class
      
    using EngineId = SeedMasterHelper::EngineId; ///< type of engine ID
    
    /// type of a function setting a seed
    using Seeder_t = std::function<void(EngineId const&, seed_t)>;
    
      private:
    /// Type of abstract class for policy implementation
    using PolicyImpl_t = details::RandomSeedPolicyBase<seed_t>;
    
    /// Type for seed data base
    using map_type = std::map<EngineId, seed_t>;
    
    /// Information for each engine
    struct EngineInfo_t {
        public:
      bool hasSeeder() const { return bool(seeder); }
      bool isFrozen() const  { return !autoseed; }
      
      void freeze(bool doFreeze = true) { autoseed = !doFreeze; }
      void setSeeder(Seeder_t new_seeder) { seeder = new_seeder; }
      
      /// Execute the seeder (whatever arguments it has...)
      template <typename... Args>
      void applySeed(Args... args) const
        { if (hasSeeder()) seeder(std::forward<Args>(args)...); }
      
      /// Applies the seed unless frozen
      template <typename... Args>
      void autoApplySeed(Args... args) const
        { if (!isFrozen()) applySeed(std::forward<Args>(args)...); }
      
        private:
      Seeder_t seeder;      ///< engine seeder
      bool autoseed = true; ///< whether seeding can be automatic
      
    }; // EngineInfo_t
    
    /// type of map of seeders associated with the engines
    using EngineData_t = std::map<EngineId, EngineInfo_t>;
    
      public:
    /// type of data used for event seeds
    using EventData_t = typename PolicyImpl_t::EventData_t;
    
    /// An invalid seed
    static constexpr seed_t InvalidSeed = PolicyImpl_t::InvalidSeed;
    
    /// Enumeration of the available policies.
    using Policy = details::Policy;
    
    static const std::vector<std::string>& policyNames();
    
    /// An iterator to the configured engine IDs
    using EngineInfoIteratorBox
      = NuRandomServiceHelper::MapKeyConstIteratorBox<EngineData_t>;
    
    SeedMaster(const fhicl::ParameterSet&);
    
    // Accept compiler written c'tors, assignments and d'tor.
    
    /// Returns whether the specified engine is already registered
    bool hasEngine(EngineId const& id) const
      { return engineData.count(id) > 0; }
    
    /// Returns whether the specified engine has a valid seeder
    bool hasSeeder(EngineId const& id) const
      { 
        auto iEngineInfo = engineData.find(id);
        return
          (iEngineInfo != engineData.end()) && iEngineInfo->second.hasSeeder();
      }
    
    /// Returns the seed value for this module label
    seed_t getSeed(std::string moduleLabel);
    
    /// Returns the seed value for this module label and instance name
    seed_t getSeed(std::string moduleLabel, std::string instanceName);
    
    /// Returns the seed value for the engine with the specified ID
    seed_t getSeed(EngineId const&);
    
    //@{
    /// Returns the seed value for the event with specified data
    seed_t getEventSeed
      (EventData_t const& data, std::string instanceName);
    seed_t getEventSeed(EventData_t const& data, EngineId const& id);
    //@}
    
    /// Returns the last computed seed value for the specified engine ID
    seed_t getCurrentSeed(EngineId const& id) const
      { return getSeedFromMap(currentSeeds, id); }
    
    
    /**
     * @brief Register the specified function to reseed the engine id
     * @param id ID of the engine to be associated to the seeder
     * @param seeder function to be used for seeding the engine
     *
     * SeedMaster keeps a list of functions that can be used to reseed an
     * existing engine.
     * When reseedEvent() (or reseed()) is called, these functions are invoked
     * to set the seed to the engine.
     */
    void registerSeeder(EngineId const& id, Seeder_t seeder);
    
    
    /**
     * @brief Register the specified function to reseed the engine id
     * @param id ID of the engine to be associated to the seeder
     * @param seeder function to be used for seeding the engine
     * @throw art::Exception (art::errors::LogicError) if already registered
     * @see registerSeeder()
     *
     * This method registers a seeder for a given engine ID, just as
     * registerSeeder() does, except that it throws an exception if a seeder has
     * already been registered for it.
     */
    void registerNewSeeder(EngineId const& id, Seeder_t seeder);
    
    
    /// Forces SeedMaster not to change the seed of a registered engine
    void freezeSeed(EngineId const& id, seed_t seed);
    
    
    /**
     * @brief Reseeds the specified engine with a global seed (if any)
     * @param id ID of the engine to be reseeded
     * @return the seed used to reseed, InvalidSeed if no reseeding happened
     *
     * Reseeding does not happen if either there is no seeder registered with
     * that engine, or if that engine is already frozen.
     */
    seed_t reseed(EngineId const& id);
    
    /**
     * @brief Reseeds the specified engine with an event seed (if any)
     * @param id ID of the engine to be reseeded
     * @param data event data to extract the seed from
     * @return the seed used to reseed, InvalidSeed if no reseeding happened
     *
     * Reseeding does not happen if either there is no seeder registered with
     * that engine, or if that engine is already frozen.
     */
    seed_t reseedEvent(EngineId const& id, EventData_t const& data);
    
    /// Prints known (EngineId,seed) pairs
    template<typename Stream> void print(Stream&&) const;
    
    /// Returns an object to iterate in range-for through configured engine IDs
    EngineInfoIteratorBox engineIDsRange() const { return { engineData }; }
    
    /// Prepares for a new event
    void onNewEvent();
    
    /// Prints to the framework Info logger
    void print() const { print(mf::LogVerbatim("SEEDS")); }    
    
      private:
    /// Control the level of information messages
    int verbosity;
    
    /// Which of the supported policies to use?
    Policy policy;
    
    /// List of seeds computed from configuration information.
    map_type configuredSeeds;
    
    /// List of event seeds already computed.
    map_type knownEventSeeds;
    
    /// List of seeds already computed.
    map_type currentSeeds;
    
    EngineData_t engineData; ///< list of all engine information

    /// Helper function to parse the policy name
    void setPolicy(std::string policyName);
    
    /// @{
    /// @brief Throws if the seed has already been used
    /// 
    /// It does not take into account per-event seeds, but only configured ones.
    void ensureUnique
      (EngineId const& id, seed_t seed, map_type const& map) const;
    void ensureUnique(EngineId const& id, seed_t seed) const
      { return ensureUnique(id, seed, configuredSeeds); }
    /// @}
    
    /// the instance of the random policy
    std::unique_ptr<PolicyImpl_t> policy_impl;
    
    
    /// Returns a seed from the specified map, or InvalidSeed if not present
    static seed_t getSeedFromMap(map_type const& seeds, EngineId const& id)
      {
        auto iSeed = seeds.find(id);
        return (iSeed == seeds.end())? InvalidSeed: iSeed->second;
      }
    
  }; // class SeedMaster
  
} // namespace rndm


//==============================================================================
//===  Template implementation
//===


// C++ include files
#include <ostream>
#include <iomanip> // std::setw()
#include <ostream> // std::endl
#include <algorithm> // std::find(), std::copy()
#include <iterator> // std::ostream_iterator<>, std::distance()

// Supporting library include files
#include "messagefacility/MessageLogger/MessageLogger.h"

// Art include files
#include "canvas/Utilities/Exception.h"


//----------------------------------------------------------------------------
template <typename SEED>
std::vector<std::string> const& rndm::SeedMaster<SEED>::policyNames()
  { return details::policyNames(); }



//----------------------------------------------------------------------------
template <typename SEED>
rndm::SeedMaster<SEED>::SeedMaster(fhicl::ParameterSet const& pSet):
  verbosity(pSet.get<int>("verbosity",0)),
  policy(Policy::unDefined),
  configuredSeeds(),
  knownEventSeeds(),
  currentSeeds(),
  engineData()
{
  
  policy_impl = std::move(details::makeRandomSeedPolicy<seed_t>(pSet).ptr);
  
  if ( verbosity > 0 )
    print(mf::LogVerbatim("SeedMaster"));
  
} // SeedMaster<SEED>::SeedMaster()



//----------------------------------------------------------------------------
template <typename SEED>
inline typename rndm::SeedMaster<SEED>::seed_t
rndm::SeedMaster<SEED>::getSeed
  (std::string moduleLabel)
{
  return getSeed(EngineId(moduleLabel));
} // SeedMaster<SEED>::getSeed(string)


//----------------------------------------------------------------------------
template <typename SEED>
typename rndm::SeedMaster<SEED>::seed_t rndm::SeedMaster<SEED>::getSeed
  (std::string moduleLabel, std::string instanceName)
{
  return getSeed(EngineId(moduleLabel,instanceName));
} // SeedMaster<SEED>::getSeed(string, string)


//----------------------------------------------------------------------------
template <typename SEED>
void rndm::SeedMaster<SEED>::registerSeeder
  (EngineId const& id, Seeder_t seeder)
{
  engineData[id].setSeeder(seeder); // creates anew and sets
} // SeedMaster<SEED>::registerSeeder()


//----------------------------------------------------------------------------
template <typename SEED>
void rndm::SeedMaster<SEED>::registerNewSeeder
  (EngineId const& id, Seeder_t seeder)
{
  if (hasEngine(id)) {
    throw art::Exception(art::errors::LogicError)
      << "SeedMaster(): Engine with ID='" << id << "' already registered";
  }
  registerSeeder(id, seeder);
} // SeedMaster<SEED>::registerNewSeeder()


//----------------------------------------------------------------------------
template <typename SEED>
void rndm::SeedMaster<SEED>::freezeSeed(EngineId const& id, seed_t seed) {
  engineData.at(id).freeze();
  configuredSeeds[id] = seed;
  currentSeeds[id] = seed;
} // SeedMaster<>::freezeSeed()


//----------------------------------------------------------------------------
template <typename SEED>
typename rndm::SeedMaster<SEED>::seed_t rndm::SeedMaster<SEED>::reseed
  (EngineId const& id)
{
  auto const& engineInfo = engineData.at(id);
  if (engineInfo.isFrozen()) return InvalidSeed;
  seed_t seed = getSeed(id);
  if (seed != InvalidSeed) { // reseed
    engineInfo.applySeed(id, seed);
  }
  return seed;
} // SeedMaster<SEED>::reseed()


template <typename SEED>
typename rndm::SeedMaster<SEED>::seed_t rndm::SeedMaster<SEED>::reseedEvent
  (EngineId const& id, EventData_t const& data)
{
  auto const& engineInfo = engineData.at(id);
  if (engineInfo.isFrozen()) return InvalidSeed;
  seed_t seed = getEventSeed(data, id);
  if (seed != InvalidSeed) { // reseed
    engineInfo.autoApplySeed(id, seed);
  }
  return seed;
} // SeedMaster<SEED>::reseedEvent()



//----------------------------------------------------------------------------
template <typename SEED> template <typename Stream>
void rndm::SeedMaster<SEED>::print(Stream&& log) const {
  log << "\nSummary of seeds computed by the NuRandomService";
  
  // allow the policy implementation to print whatever it feels to
  std::ostringstream sstr;
  policy_impl->print(sstr);
  if (!sstr.str().empty()) log << '\n' << sstr.str();
  
  if ( !currentSeeds.empty() ) {
    
    constexpr unsigned int ConfSeedWidth = 18;
    constexpr unsigned int SepWidth1 = 2;
    constexpr unsigned int LastSeedWidth = 18;
    constexpr unsigned int SepWidth2 = SepWidth1 + 1;
    
    log << "\n "
      << std::setw(ConfSeedWidth) << "Configured value"
      << std::setw(SepWidth1) << ""
      << std::setw(LastSeedWidth) << "Last value"
      << std::setw(SepWidth2) << ""
      << "ModuleLabel.InstanceName";
    
    for (auto const& p: currentSeeds) {
      EngineId const& ID = p.first;
      seed_t configuredSeed = getSeedFromMap(configuredSeeds, ID);
      seed_t currentSeed = p.second;
      
      if (configuredSeed == InvalidSeed) {
        if (currentSeed == InvalidSeed) {
          log << "\n "
            << std::setw(ConfSeedWidth) << "INVALID!!!"
            << std::setw(SepWidth1) << ""
            << std::setw(LastSeedWidth) << ""
            << std::setw(SepWidth2) << ""
            << ID;
        }
        else { // if seed was configured, it should be that one all the way!!
          log << "\n "
            << std::setw(ConfSeedWidth) << "(per event)"
            << std::setw(SepWidth1) << ""
            << std::setw(LastSeedWidth) << currentSeed
            << std::setw(SepWidth2) << ""
            << ID;
        }
      }
      else {
        if (configuredSeed == currentSeed) {
          log << "\n "
            << std::setw(ConfSeedWidth) << configuredSeed
            << std::setw(SepWidth1) << ""
            << std::setw(LastSeedWidth) << "(same)"
            << std::setw(SepWidth2) << ""
            << ID;
        }
        else { // if seed was configured, it should be that one all the way!!
          log << "\n "
            << std::setw(ConfSeedWidth) << configuredSeed
            << std::setw(SepWidth1) << ""
            << std::setw(LastSeedWidth) << currentSeed
            << std::setw(SepWidth2) << ""
            << ID << "  [[ERROR!!!]]";
        }
      } // if per job
      if (ID.isGlobal()) log << " (global)";
      if (hasEngine(ID) && engineData.at(ID).isFrozen()) log << " [overridden]";
    } // for all seeds
  } // if any seed
  log << '\n' << std::endl;
} // SeedMaster<SEED>::print(Stream)


//----------------------------------------------------------------------------
template <typename SEED>
typename rndm::SeedMaster<SEED>::seed_t rndm::SeedMaster<SEED>::getSeed
  (EngineId const& id)
{
  // Check for an already computed seed.
  typename map_type::const_iterator iSeed = configuredSeeds.find(id);
  seed_t seed = InvalidSeed;
  if (iSeed != configuredSeeds.end()) return iSeed->second;

  // Compute the seed.
  seed = policy_impl->getSeed(id);
  if (policy_impl->yieldsUniqueSeeds()) ensureUnique(id, seed);
  
  // Save the result.
  configuredSeeds[id] = seed;
  
  // for per-event policies, configured seed is invalid;
  // in that case we don't expect to change the seed,
  // and we should not record it as current; this should not matter anyway
  // we still store it if there is nothing (emplace does not overwrite)
  if (seed != InvalidSeed) currentSeeds[id] = seed;
  else                     currentSeeds.emplace(id, seed);
  
  return seed;
} // SeedMaster<SEED>::getSeed()


//----------------------------------------------------------------------------
template <typename SEED>
typename rndm::SeedMaster<SEED>::seed_t rndm::SeedMaster<SEED>::getEventSeed
  (EventData_t const& data, EngineId const& id)
{
  // Check for an already computed seed.
  typename map_type::iterator iSeed = knownEventSeeds.find(id);
  seed_t seed = InvalidSeed;
  if (iSeed != knownEventSeeds.end()) return iSeed->second;

  // Compute the seed.
  seed = policy_impl->getEventSeed(id, data);
  if ((seed != InvalidSeed) && policy_impl->yieldsUniqueSeeds())
    ensureUnique(id, seed, knownEventSeeds);
    
  // Save the result.
  knownEventSeeds[id] = seed;
  
  // for configured-seed policies, per-event seed is invalid;
  // in that case we don't expect to change the seed,
  // and we should not record it as current
  // we still store it if there is nothing (emplace does not overwrite)
  if (seed != InvalidSeed) currentSeeds[id] = seed;
  else                     currentSeeds.emplace(id, seed);
  
  return seed;
} // SeedMaster<SEED>::getEventSeed(EngineId)


template <typename SEED>
typename rndm::SeedMaster<SEED>::seed_t rndm::SeedMaster<SEED>::getEventSeed
  (EventData_t const& data, std::string instanceName)
{
  return getEventSeed(data, EngineId(data.moduleLabel, instanceName));
} // SeedMaster<SEED>::getEventSeed(string)



//----------------------------------------------------------------------------
template <typename SEED>
inline void rndm::SeedMaster<SEED>::onNewEvent() {
  // forget all we know about the current event
  knownEventSeeds.clear();
} // SeedMaster<SEED>::onNewEvent()


//----------------------------------------------------------------------------
template <typename SEED>
void rndm::SeedMaster<SEED>::setPolicy(std::string policyName) {
  
  policy = details::policyFromName(policyName);
  
} // SeedMaster<SEED>::setPolicy()


//----------------------------------------------------------------------------
template <typename SEED>
void rndm::SeedMaster<SEED>::ensureUnique
  (EngineId const& id, seed_t seed, map_type const& seeds) const
{
  
  for (auto const& p: seeds) {
    
    // Do not compare to self
    if ( p.first == id ) continue;
    
    if ( p.second == seed ){
      throw art::Exception(art::errors::LogicError)
        << "NuRandomService::ensureUnique() seed: "<<seed
        << " already used by module.instance: " << p.first << "\n"
        << "May not be reused by module.instance: " << id;
    }
  } // for
} // SeedMaster<SEED>::ensureUnique()


#endif // NURANDOM_RANDOMUTILS_PROVIDERS_SEEDMASTER_H
