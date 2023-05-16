/**
 * @file nurandom/RandomUtils/NuRandomService.h
 * @brief An art service to assist in the distribution of guaranteed unique seeds to all engines within an art job.
 * @author Gianluca Petrillo (petrillo@fnal.gov), Rob Kutschke (kutschke@fnal.gov)
 * @date   2013/03/14 19:54:49
 * @see NuRandomService_service.cc
 */


#ifndef NURANDOM_RANDOMUTILS_NuRandomService_H
#define NURANDOM_RANDOMUTILS_NuRandomService_H 1

#ifndef NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP
/// Define to zero to exclude special CLHEP random engine support
#  define NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP 1
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP

// ROOT libraries
#ifndef NURANDOM_RANDOMUTILS_NuRandomService_USEROOT
/// Define to non-zero to include special ROOT random generator support
#  define NURANDOM_RANDOMUTILS_NuRandomService_USEROOT 0
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USEROOT


// C/C++ standard libraries
#include <functional>
#include <string>
#include <utility> // std::forward()
#include <initializer_list>

// Some helper classes.
#include "nurandom/RandomUtils/ArtState.h"
#include "nurandom/RandomUtils/Providers/SeedMaster.h"

// CLHEP libraries
#if (NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP)
#  include "CLHEP/Random/RandomEngine.h"
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP

// ROOT libraries
#if (NURANDOM_RANDOMUTILS_NuRandomService_USEROOT)
#  include "TRandom.h"
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USEROOT

// From art and its tool chain.
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/detail/EngineCreator.h"
#include "art/Framework/Services/Optional/RandomNumberGenerator.h"
#include "art/Framework/Services/Registry/ServiceDeclarationMacros.h"
#include "art/Persistency/Provenance/ScheduleContext.h"

// Forward declarations
namespace art {
  class ActivityRegistry;
  class ModuleDescription;
  class ModuleContext;
  class Run;
  class SubRun;
}

namespace rndm {

  /// Type of seed used in _art_ and by us.
  using seed_t = art::detail::EngineCreator::seed_t;

  /// Type of FHiCL parameter to be used to read a random seed.
  using SeedAtom = fhicl::OptionalAtom<seed_t>;

  /**
   * @brief An art service to assist in the distribution of guaranteed unique
   * seeds to all engines within an art job.
   * @see `rndm::SeedMaster`
   *
   * `rndm::NuRandomService` centrally manages seeds for random generator
   * engines.
   *
   * The `NuRandomService` acts as an interface between art framework and the
   * `rndm::SeedMaster` class.
   *
   * The documentation is mantained in the `rndm::SeedMaster` class.
   * The configuration of `NuRandomService` is exactly the same as
   * `SeedMaster`'s,  and in art it's read from `services.NuRandomService`.
   * The following documentation describes features of `NuRandomService` that
   * are built on top of `rndm::SeedMaster` to have a more convenient
   * interaction within the art framework.
   *
   * Before asking `NuRandomService` for its seed, an engine must be in some way
   * registered. Once the engine is registered, its original seed can be queried
   * again by calling `getSeed()` methods.
   *
   *
   * Glossary
   * ---------
   *
   * Here "engine" means a class that is able to generate random numbers
   * according to a flat distribution.
   * Both art and `NuRandomService` are module-based, that means that the
   * engines are in the context of a specific module instance, and different
   * module instances have independent engines.
   * That is the reason why you don't need to specify anything about the module
   * when creating or obtaining a random engine, and it is also the reason why
   * engines outside module context are not supported by the framework.
   *
   * Each module can need more than one engine. A module can have any number of
   * engines, and each of them is identified by an "instance name" that is
   * unique within the module.
   * Nonetheless, most modules need just one engine. In that case, a default
   * instance name can be used (that is an empty string).
   *
   * A "seeder" is a callable object (e.g. a function) that sets the seed of
   * a certain engine. The seeder is expected to find out by its own which
   * engine it has to seed, and for that it is provided an engine ID.
   *
   *
   * Registration of a random generator engine
   * ------------------------------------------
   *
   * Registration must happen in art module constructor, in one of the following
   * ways:
   *  * by registering an existing engine and its seeding function
   *    (see `registerAndSeedEngine()` and `registerEngine()` methods)
   *  * by just declaring that an engine exists
   *    (see `declareEngine()` and `getSeed()` methods)
   * The first methods set the seed of the engine they register (see below).
   * In the second case, it is generally the caller's responsibility to seed the
   * engine. The registration of an engine which has been only declared can be
   * "completed" by calling `defineEngine()` to provide the actual seeder for
   * that engine. The pair of calls `declareEngine()`/`defineEngine()` (or
   * `getSeed()`/`defineEngine()`) is equivalent to a single call to
   * `registerEngine()`, with the added flexibility of having the seed for the
   * engine already available before the registration is completed.
   *
   * Here is an example for an engine with a non-default instance name:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * std::string const instanceName = "instanceName";
   * auto& Seeds = *art::ServiceHandle<rndm::NuRandomService>();
   *
   * // declare an engine; NuRandomService associates an (unknown) engine, in
   * // the current module and an instance name, with a seed (returned)
   * auto const seed = Seeds.declareEngine(instanceName);
   *
   * // now create the engine (for example, use art); seed will be set
   * auto& engine = createEngine(seed, "HepJamesRandom", instanceName);
   *
   * // finally, complete the registration; seed will be set again
   * Seeds.defineEngine(engine);
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * This is equivalent to the call
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * auto& Seeds = *(art::ServiceHandle<rndm::NuRandomService>());
   * Seeds.registerAndSeedEngine(createEngine(0, "HepJamesRandom", "instanceName"),
   *                             "HepJamesRandom", "instanceName");
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * Please read carefully the documentation of the method of your choice, since
   * they have different requirements and apply to different usage patterns.
   *
   * The registration must happen in the constructor of the module.
   * That is because we don't want engines to be initialized in the middle of a
   * job.
   *
   *
   * Setting the seed of an engine
   * ------------------------------
   *
   * `rndm::NuRandomService` is able to set the seed of an engine when the
   * engine is registered via either:
   *  * `registerAndSeedEngine()` (creation of a new CLHEP engine)
   *  * `registerEngine()` (registration of an engine or a seeder function),
   *    if the registered seeder function is valid (non-null) or if a CLHEP
   *    engine is being registered (in which case the seeder is automatically
   *    created valid)
   *  * `defineEngine()` (registration of a seeder for an engine that was
   *    already declared), again if the seed is valid
   * `NuRandomService` is *not* able to automatically set the seed of an engine
   * if it was registered via either:
   *  * `declareEngine()` (declaration of the existence of an engine),
   *    that does not even require the engine to exist
   *  * `getSeed()` (query of a seed), when it (implicitly) declares an engine
   *    which had not been declared yet
   *
   * If `NuRandomService` is able to set the seed, it will do so only once, as
   * soon as it can.
   * This means that if the policy allows the seed to be known immediately,
   * the seed will be set on registration. In the case of a per-event policy
   * that requires the presence of an event, the seed can be known only
   * when the event is available, and `NuRandomService` will set the seed before
   * the module the engine is associated with starts its main processing method
   * (`produce()`, `filter()` or `analyze()`).
   *
   *
   * Changing the seeder
   * --------------------
   *
   * Currently, changing the seeder of an engine after the engine has been
   * fully registered is not supported. As a consequence, changing the engine
   * is also not supported.
   *
   * Since only the seeder function is registered in `NuRandomService`, a seeder
   * function that is flexible enough to change the engine it seeds may work
   * around this limitation.
   *
   *
   * Querying the seed of an engine
   * -------------------------------
   *
   * If necessary, the seed that `NuRandomService` has assigned to an engine can
   * be requested by one of the following two calls:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * art::ServiceHandle<NuRandomService>()->getSeed();
   * art::ServiceHandle<NuRandomService>()->getSeed("instanceName");
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * depending on whether the engine has a non-empty instance name,
   * Note that this call implicitly "declares" the engine it refers to.
   * A call not declaring anything is instead:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * art::ServiceHandle<NuRandomService>()->getCurrentSeed();
   * art::ServiceHandle<NuRandomService>()->getCurrentSeed("instanceName");
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * For most policies, the seed is set according to the configuration, once
   * for all. In those cases, `getSeed()` will always return the same value.
   * If the policy prescribes different seeds at different times, the method
   * returns the seed that is assigned to the engine at the time of the call.
   *
   * Also note that the seed assigned by `NuRandomService` might not match the
   * current seed of the engine, if:
   *  * `NuRandomService` is not in charge of setting the seed of the engine,
   *    and the engine seed has not been set yet
   *  * the seed was reset directly after `NuRandomService` set the engine seed
   * Both circumstances should be avoided.
   *
   *
   * Creating the engines independently of `rndm::NuRandomService`
   * --------------------------------------------------------------
   *
   * A number of things must happen for an engine to correctly work with
   * `NuRandomService`:
   *  * the engine instance needs to exist or be created
   *  * the engine must be "registered" into `NuRandomService`
   *  * the seed must be obtained from `NuRandomService`
   *  * the seed must be provided to the engine
   *
   * A recipe for creating a ROOT engine and let `NuRandomService` take care of
   * its seeds is:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * // create the engine; ROOT will set some (temporary) seed
   * fRandom = std::make_unique<TRandom3>();
   *
   * // declare the engine; NuRandomService associates its seeder, in
   * // the current module and an instance name, with a seed (returned);
   * // the seed is also set in the engine
   * auto& Seeds = *(art::ServiceHandle<rndm::NuRandomService>());
   * Seeds.registerEngine(TRandomSeeder(fRandom), "instanceName");
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * Here `fRandom` is supposed to be a member function of the module.
   * The `TRandomSeeder` object is an object that knows how to set the seed
   * of the `TRandom`-derived ROOT generator passed as constructor argument.
   * For an example of implementation, see the source code of `NuRandomService`.
   *
   *
   * Overriding the seed from `rndm::NuRandomService` at run time
   * -------------------------------------------------------------
   *
   * `NuRandomService` (and `SeedMaster`, which the former relies upon) will
   * decide which seed to give to each registered engine and, when possible
   * (see above), will set that seed too.
   *
   * All registration functions offer an extended signature to tell
   * `NuRandomService` that if there is an explicitly configured seed, that
   * should take precedence over the one automatically assigned by
   * `rndm::SeedMaster` policy. This extended signature includes:
   *  * a FHiCL parameter set
   *  * a configuration parameter name, or a list of them
   *  * a optional atom of type `rndm::seed_t`, also available with the name
   *    `rndm::SeedAtom`
   * `NuRandomService` will look in the specified parameter set and if it finds
   * a value corresponding to any of the specified parameter names, will set
   * the seed of the engine to that value, and it will mark the engine as
   * "frozen" (meaning that `NuRandomService` will not ever set a seed again on
   * that engine). [see also the
   * @link nurandom_NuRandomService_SpecialSeedValue "exception to this rule"
   * below]
   * Likewise, if the optional parameter is specified, its value will be
   * honored, and the service will be queried for an automatic seed otherwise.
   * The typical use of this function is to read a parameter ("Seed" is the
   * suggested name) from the configuration of the module. Note that this is in
   * contrast with the location where `NuRandomService` seeds are normally
   * configured, that is in the configuration of `NuRandomService` service
   * itself. For example:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * // within module construction:
   * art::ServiceHandle<rndm::NuRandomService>()->registerEngine
   *   (engine, "instanceName", pset, { "Seed", "MySeed" });
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * Here `NuRandomService` will provide a seed only if it does not find in the
   * parameter set `pset` any of the specified configuration parameters (first
   * "Seed", then "MySeed" in this example). In other words, if "Seed" exists,
   * its value is used as seed, otherwise if "MySeed" exists, its value is used
   * instead, and otherwise NuRandomService is given control on that seed.
   * @anchor nurandom_NuRandomService_SpecialSeedValue
   * The exception is that if the specified seed is a "magic value", the
   * `InvalidSeed` (`0`), it is interpreted as a request to ignore the parameter
   * and use the service to get the seed. This is made as a quick way to remove
   * the seed override from an existing FHiCL file with one line.
   * Note that if NuRandomService does not get the control on the seed,
   * policies that reseed on event-by-event basis will not act on the engine.
   *
   * An example of the use of the validated configuration, for an analyzer
   * containing in its definition:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * CLHEP::HepRandomEngine& fEngine;
   *
   *   public:
   *
   * struct Config {
   *
   *   rndm::SeedAtom Seed{
   *     fhicl::Name("Seed"),
   *     fhicl::Comment("optional seed for random engine")
   *     };
   *
   * }; // struct Config
   *
   * using Parameters = art::EDAnalyzer::Table<Config>;
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * is to have an entry in the initializer list of the constructor such as:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * my_ns::MyAnalyzer::MyAnalyzer(Parameters const& config)
   *   : art::EDAnalyzer(config)
   *   , fEngine(art::ServiceHandle<rndm::NuRandomService>()->registerAndSeedEngine
   *     (*this, config().Seed))
   *   // ...
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   *
   * Engines outside of the module context
   * ======================================
   *
   * It is possible to have engines that are not associated with any module.
   * If no module is current, an engine will be registered in a "global"
   * context. This happens during service construction, and once the service
   * construction phase is completed, no more global engines can be registered.
   * Would one ever need to access the seed of such engine, a specific
   * interface needs to be used: `getGlobalSeed()` to get the configured seed
   * at the beginning of the job, or `getGlobalCurrentSeed()` to get the seed
   * specific for the current event, if any. These are equivalent to the module
   * context methods `getSeed()` and `getCurrentSeed()`.
   *
   * The art service RandomNumberGenerator does not support the creation of
   * module-independent engines. The ownership of each global engine is by the
   * using service, as well as the management of engine's lifetime.
   * After an engine has been instantiated, it can be registered with
   * `registerEngine()`. Likewise, a global engine can be declared first with
   * `declareEngine()`, then instantiated, and then optionally defined with
   * `defineEngine()`. This is completely analogous to the module-context
   * engines.
   * Whether these methods create a global or module engine depends only
   * on the context they are called in.
   *
   * NuRandomService does not manage the engine life in any way. If a service owns
   * an engine, it also needs a way to access it, as nothing equivalent to
   * RandomNumberGenerator's `getEngine()` is provided.
   * NuRandomService does manage the seeding of the registered engines, even the
   * global ones. If the seed policy involves a event-dependent seed, all global
   * engines are seeded together at the beginning of the event, before any
   * module is executed. This mechanism relies on the fact that NuRandomService gets
   * its preProcessEvent callback executed before the ones of the services that
   * own any engine. This is guaranteed if the service constructors invoke
   * NuRandomService before they register their callbacks.
   *
   * For an example of usage, see GlobalEngineUserTestService service in the
   * test suite. Here is an excerpt using a ROOT TRandom3 engine, that can
   * be constructed without seed. In the service constructor, the code:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * // create a new engine, that we own (engine be a class data member)
   * engine = std::make_unique<TRandom3>();
   * auto seed = Seeds.registerEngine
   *   (rndm::NuRandomService::TRandomSeeder(engine.get()), "MyService");
   *
   * // MyService callback registrations in the ActivityRegistry
   * // should follow the first call to NuRandomService!
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * where rndm::NuRandomService::TRandomSeeder is a seeder class for TRandom
   * engines (optionally provided in NuRandomService.h, and very easy to write).
   * This excerpt creates and owns a TRandom3 engine, and then it registers it
   * to NuRandomService, which immediately sets its seed and will take care of
   * reseeding the engine at each event, should the policy require it.
   * The service will access the engine as a data member, and should it need the
   * seed it will use:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * art::ServiceHandle<rndm::NuRandomService>()->getGlobalSeed("MyService");
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * Note that is a good idea to give the engine an instance name after the
   * service name itself, since the instance name is global and it's the only
   * thing distinguishing global engines, and name conflicts between different
   * services may easily arise.
   *
   */
  class NuRandomService {
      public:
    using seed_t = art::detail::EngineCreator::seed_t;
    using engine_t = CLHEP::HepRandomEngine;

    using SeedMaster_t = SeedMaster<seed_t>; ///< type of object providing seeds

    using EngineId = SeedMaster_t::EngineId; ///< type of random engine ID

    /// An invalid seed
    static constexpr seed_t InvalidSeed = SeedMaster_t::InvalidSeed;

    NuRandomService(const fhicl::ParameterSet&, art::ActivityRegistry&);

    // Accept compiler written d'tor.  Not copyable or assignable.
    // This class is not copyable or assignable: these methods are not implemented.
    NuRandomService(NuRandomService const&) = delete;
    NuRandomService const& operator=(NuRandomService const&) = delete;
    NuRandomService(NuRandomService&&) = delete;
    NuRandomService const& operator=(NuRandomService&&) = delete;
    ~NuRandomService() = default;

    /// Returns whether the specified seed is valid
    static constexpr bool isSeedValid(seed_t seed)
      { return seed != InvalidSeed; }


    /**
     * @brief Returns a seed for the engine with specified instance name.
     * @param moduleLabel label of the module the engine is assigned to
     * @param instanceName name of the engine instance within that module
     * @return a seed for the engine with specified instance name
     * @see `getGlobalSeed()`
     *
     * The seed for an engine in the context of the specified module label is
     * returned.
     * If you need the seed for an engine outside that context, use
     * `getGlobalSeed()` instead.
     *
     * The engine needs to have been registered before, in any of the supported
     * ways. If it has not, this call will declare it with `declareEngine()`
     * and no further registration will be allowed.
     *
     * While this method can be called at any time, the registration of an
     * engine can happen only at construction time: if it is called at any other
     * time and if the call triggers such registration as described above,
     * it will make the call to this method fail.
     *
     * @note This method is thread-safe.
     */
    seed_t getSeed
      (std::string const& moduleLabel, std::string const& instanceName);

    /**
     * @brief Returns a seed for the engine with specified instance name.
     * @param instanceName name of the engine instance
     * @return a seed for the engine with specified instance name
     * @see `getGlobalSeed()`
     *
     * The seed for an engine in the context of the **current module** is
     * returned. See `getSeed(std::string const&, std::string const&)` for
     * details.
     *
     * @note This method is **not thread-safe**.
     */
    seed_t getSeed(std::string instanceName = "");


    /**
     * @brief Returns a seed for the global engine with specified instance name
     * @param instanceName name of the engine instance
     * @return a seed for the global engine with specified instance name
     * @see getSeed()
     *
     * A "global" engine is not bound to a specific execution context.
     * The only context NuRandomService is aware of is the module, so this
     * translates into engines that are not bound to any module.
     * To instruct NuRandomService to ignore the current context (that may be
     * a running module, or no running module at all), `getGlobalSeed()` is
     * used instead of `getSeed()`, that will consider the context and in fact
     * consider the absence of context an error.
     *
     * The engine needs to have been registered before, in any of the supported
     * ways.
     * If it has not, this call will declare it with declareEngine()
     * and no further registration will be allowed.
     *
     * While this method can be called at any time, the registration of an
     * engine can happen only at construction time, and it will make the call to
     * this method fail if it is called at any other time.
     */
    seed_t getGlobalSeed(std::string instanceName);


    /// Returns the last computed seed for specified engine of current module
    seed_t getCurrentSeed(std::string instanceName) const
      { return seeds.getCurrentSeed(qualify_engine_label(instanceName)); }

    /// Returns the last computed seed for the default engine of current module
    seed_t getCurrentSeed() const
      { return seeds.getCurrentSeed(qualify_engine_label()); }

    /// Returns the last computed seed for the specified global engine
    seed_t getGlobalCurrentSeed(std::string instanceName) const
      { return seeds.getCurrentSeed(qualify_global_engine(instanceName)); }


    // --- BEGIN --- Create and register an engine -----------------------------
#if (NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP)
    /**
     * @name Create and register an engine
     *
     * The life time of the engine is managed by art::RandomNumberGenerator,
     * while the seeding is managed by this service.
     *
     * Here an example for an engine with a non-default instance name:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * std::string const instanceName = "instanceName";
     * auto& Seeds = *art::ServiceHandle<rndm::NuRandomService>();
     *
     * // declare an engine; NuRandomService associates an (unknown) engine, in
     * // the current module and an instance name, with a seed (returned)
     * auto const seed = Seeds.declareEngine(instanceName);
     *
     * // now create the engine (for example, use art); seed will be set
     * auto& engine = createEngine(seed, "HepJamesRandom", instanceName);
     *
     * // finally, complete the registration; seed will be set again
     * Seeds.defineEngine(engine);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    /// @{

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param instance the name of the engine instance
     * @param seed the seed to use for this engine (optional)
     * @return the seed used
     * @see `registerEngine()`
     *
     * The engine seed is set. If the `seed` optional parameter has a value,
     * that value is used as seed. Otherwise, the seed is obtained from
     * `rndm::NuRandomService`.
     *
     * If the `seed` is not specified, it is obtained in a way equivalent to
     * `getSeed()` (but multi-treading safe).
     *
     * If also `instance` is not specified, the engine is created with no
     * instance name (equivalent to an empty instance name).
     *
     * If also `type` is not specified, the type of the engine is the default
     * one from `art::RandomNumberGenerator`.
     *
     * @note The module parameter is needed since the interface to create
     *       an engine by `art::RandomNumberGenerator` service is private
     *       and open only to friends.
     */
    std::reference_wrapper<NuRandomService::engine_t> registerAndSeedEngine(
      engine_t& engine,
      std::string type = "", std::string instance = "",
      std::optional<seed_t> const seed = std::nullopt
      );

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param instance the name of the engine instance
     * @param seedParam the optional seed configuration parameter
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, std::optional<seed_t> const)`
     * with the difference that the seed is read from `seedParam`; if
     * that optional parameter is not present, then the seed is obtained from
     * `rndm:::NuRandomService`.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine, std::string type, std::string instance,
      SeedAtom const& seedParam
      );

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param seedParam the optional seed configuration parameter
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, std::optional<seed_t> const)`
     * with the difference that the seed is read from `seedParam`; if
     * that optional parameter is not present, then the seed is obtained from
     * `rndm:::NuRandomService`.
     * Also, the engine is always associated with an empty instance name.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine
      (engine_t& engine, std::string type, SeedAtom const& seedParam)
      { return registerAndSeedEngine(engine, type, "", seedParam); }

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param seedParam the optional seed configuration parameter
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, std::optional<seed_t> const)`
     * with the difference that the seed is read from `seedParam`; if
     * that optional parameter is not present, then the seed is obtained from
     * `rndm:::NuRandomService`.
     * Also, the engine is always created of the default type and associated
     * with an empty instance name.
     */
    template <typename Module>
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine
      (engine_t& engine, SeedAtom const& seedParam)
      { return registerAndSeedEngine(engine, "", "", seedParam); }

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param instance the name of the engine instance
     * @param pset parameter set to read parameters from
     * @param pname name of the seed parameters
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, std::optional<seed_t> const)`
     * with the difference that the seed is retrieved from the specified
     * configuration, looking for the first of the parameters in `pname` that is
     * available. If no parameter is found, the seed is obtained from
     * `rndm::NuRandomService`.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine, std::string type, std::string instance,
      fhicl::ParameterSet const& pset, std::string pname
      )
      { return registerAndSeedEngine(engine, type, instance, pset, { pname }); }

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param instance the name of the engine instance
     * @param pset parameter set to read parameters from
     * @param pnames names of the seed parameters
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, fhicl::ParameterSet const&, std::string)`
     * with the difference that the seed can be specified by any of the
     * parameters named in `pnames`: the first match will be used. As usual,
     * if no parameter is found, the seed is obtained from
     * `rndm:::NuRandomService`.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine, std::string type, std::string instance,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      );


    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param pset parameter set to read parameters from
     * @param pname name of the seed parameters
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, fhicl::ParameterSet const&, std::string)`
     * with the difference that the engine is given no instance name.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine, std::string type,
      fhicl::ParameterSet const& pset, std::string pname
      )
      { return registerAndSeedEngine(engine, type, "", pset, pname); }

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param type the type of engine
     * @param instance the name of the engine instance
     * @param pset parameter set to read parameters from
     * @param pnames names of the seed parameters
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, fhicl::ParameterSet const&, std::string)`
     * with the difference that the engine is not given any instance name and
     * that seed can be specified by any of the parameters named in `pnames`:
     * the first match will be used. As usual, if no parameter is found, the
     * seed is obtained from `rndm:::NuRandomService`.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine, std::string type,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      )
      { return registerAndSeedEngine(engine, type, "", pset, pnames); }

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param instance the name of the engine instance
     * @param pset parameter set to read parameters from
     * @param pname name or names of the seed parameters
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, fhicl::ParameterSet const&, std::string)`
     * with the difference that the default engine is used (determined by
     * `art::RandomNumberGenerator`) and no instance name is given to it.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine,
      fhicl::ParameterSet const& pset, std::string pname
      )
      { return registerAndSeedEngine(engine, pset, { pname }); }

    /**
     * @brief Creates an engine with `art::RandomNumberGenerator` service.
     * @param module module who will own the new engine
     * @param pset parameter set to read parameters from
     * @param pnames names of the seed parameters
     * @return the seed used
     * @see `registerEngine()`
     *
     * This method operates like
     * `registerAndSeedEngine(engine_t&, std::string, std::string, fhicl::ParameterSet const&, std::string)`
     * with the difference that the engine is created with the default type
     * (determined in `art::RandomNumberGenerator`), not given any instance name
     * and that seed can be specified by any of the parameters named in
     * `pnames`: the first match will be used. As usual, if no parameter is
     * found, the seed is obtained from `rndm:::NuRandomService`.
     */
    [[nodiscard]] std::reference_wrapper<engine_t> registerAndSeedEngine(
      engine_t& engine,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      )
      { return registerAndSeedEngine(engine, "", "", pset, pnames); }

#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP
    /// @}
    // --- END --- Create and register an engine -------------------------------



    // --- BEGIN --- Register an existing engine -------------------------------
    /**
     * @name Register an existing engine
     *
     * The life time of the engine is under user's control, while the seeding
     * managed by this service.
     */
    /// @{

    /**
     * @brief Registers an existing engine with `rndm::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param instance name of the engine
     * @param seed the optional seed configuration parameter
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerAndSeedEngine()`
     *
     * This function works similarly to `registerAndSeedEngine()`, but it uses an
     * existing engine instead of creating a new one via
     * `art::RandomNumberGenerator` service.
     * The seeder function must be provided for the service to be of any use:
     * `registerEngine()` will set the seed immediately, and the seeder function
     * will be used to set the seed for policies that do that on each event.
     * The instance name must also be unique, since for `NuRandomService`
     * purposes the registered engine is no different from any other, created by
     * `art::RandomNumberGenerator` or not.
     *
     * Three standard functions are provided as seeders, for use with
     * `art::RandomNumberGenerator` engines (`RandomNumberGeneratorSeeder()`),
     * with a `CLHEP::HepRandomEngine` (`CLHEPengineSeeder` class) and with
     * ROOT's `TRandom` (`TRandomSeeder` class). Note that CLHEP and ROOT
     * classes are not compiled in `NuRandomService` by default, and the
     * recommendation is to take their implementation as an example and create
     * your own after them).
     * Any seeder function with the prototype of `NuRandomService::Seeder_t`:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * void Seeder(EngineId const&, seed_t);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * or a functor with
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * void operator() (EngineId const&, seed_t);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * can be used as seeder.
     *
     * The engine seed will be set. If the `seed` optional parameter has a
     * value, that value is used as seed. Otherwise, the seed is obtained from
     * `rndm::NuRandomService`.
     *
     * If the `seed` is not specified, it is obtained in a way equivalent to
     * `getSeed()` (but multi-treading safe).
     *
     * If also `instance` is not specified, the engine is created with no
     * instance name (equivalent to an empty instance name).
     */
    seed_t registerEngine(
      SeedMaster_t::Seeder_t seeder, std::string const instance = "",
      std::optional<seed_t> const seed = std::nullopt
      );


    /**
     * @brief Registers an existing engine with `art::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param instance name of the engine
     * @param seedParam the optional seed configuration parameter
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerEngine()`
     *
     * This method works similarly to
     * `registerEngine(SeedMaster_t::Seeder_t, std::string const, std::optional<seed_t> const`,
     * with the difference that the seed is read from `seedParam`; if
     * that optional parameter is not present, then the seed is obtained from
     * `rndm:::NuRandomService`.
     */
    seed_t registerEngine(
      SeedMaster_t::Seeder_t seeder, std::string const instance,
      SeedAtom const& seedParam
      );

    /**
     * @brief Registers an existing engine with `art::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param seedParam the optional seed configuration parameter
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerEngine()`
     *
     * This method works similarly to
     * `registerEngine(SeedMaster_t::Seeder_t, std::string const, std::optional<seed_t> const`,
     * with the difference that the seed is read from `seedParam`; if
     * that optional parameter is not present, then the seed is obtained from
     * `rndm:::NuRandomService`.
     * In addition, the engine is always registered with no instance name.
     */
    seed_t registerEngine
      (SeedMaster_t::Seeder_t seeder, SeedAtom const& seedParam)
      { return registerEngine(seeder, "", seedParam); }

    /**
     * @brief Registers an existing engine with `art::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param instance name of the engine
     * @param pset parameter set to read parameters from
     * @param pname name of the optional seed parameter
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerAndSeedEngine()`
     *
     * This method works similarly to
     * `registerEngine(SeedMaster_t::Seeder_t, std::string const, std::optional<seed_t> const`,
     * but the preferred way to obtain the seed is from the configuration
     * parameter named `pname` in `pset`. If that parameter is found, the seed
     * is obtained from `rndm::NuRandomService`.
     */
    seed_t registerEngine(
      SeedMaster_t::Seeder_t seeder, std::string instance,
      fhicl::ParameterSet const& pset, std::string pname
      )
      { return registerEngine(seeder, instance, pset, { pname }); }

    /**
     * @brief Registers an existing engine with `art::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param instance name of the engine
     * @param pset parameter set to read parameters from
     * @param pnames names of the seed parameters
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerAndSeedEngine()`
     *
     * This method works similarly to
     * `registerEngine(SeedMaster_t::Seeder_t, std::string const, std::optional<seed_t> const`,
     * but the preferred way to obtain the seed is from the first available
     * configuration parameter in `pset` among the ones listed in `pnames`.
     * If that parameter is found, the seed is obtained from
     * `rndm::NuRandomService`.
     */
    seed_t registerEngine(
      SeedMaster_t::Seeder_t seeder, std::string instance,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      );

    /**
     * @brief Registers an existing engine with `art::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param pset parameter set to read parameters from
     * @param pname name of the optional seed parameter
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerAndSeedEngine()`
     *
     * This method works similarly to
     * `registerEngine(SeedMaster_t::Seeder_t, std::string const, std::optional<seed_t> const`,
     * but the preferred way to obtain the seed is from the configuration
     * parameter named `pname` in `pset`. If that parameter is found, the seed
     * is obtained from `rndm::NuRandomService`. Also, the engine is assigned
     * no instance name.
     */
    seed_t registerEngine(
      SeedMaster_t::Seeder_t seeder,
      fhicl::ParameterSet const& pset, std::string pname
      )
      { return registerEngine(seeder, "", pset, pname); }

    /**
     * @brief Registers an existing engine with `art::NuRandomService`.
     * @param seeder function used to set the seed of the existing engine
     * @param pset parameter set to read parameters from
     * @param pnames names of the seed parameters
     * @return the seed assigned to the engine (may be invalid)
     * @see `registerAndSeedEngine()`
     *
     * This method works similarly to
     * `registerEngine(SeedMaster_t::Seeder_t, std::string const, std::optional<seed_t> const`,
     * but the preferred way to obtain the seed is from the first available
     * configuration parameter in `pset` among the ones listed in `pnames`.
     * If that parameter is found, the seed is obtained from
     * `rndm::NuRandomService`. Also, the engine is assigned no instance name.
     */
    seed_t registerEngine(
      SeedMaster_t::Seeder_t seeder,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      )
      { return registerEngine(seeder, "", pset, pnames); }

#if (NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP)
    /**
     * @brief Registers an existing CLHEP engine with `art::NuRandomService`.
     * @param engine a reference to the CLHEP random generator engine
     * @param instance (default: none) name of the engine
     * @return the seed assigned to the engine (may be invalid)
     *
     * The specified engine is not managed.
     * It may be owned by `art::RandomNumberGenerator` service.
     *
     * The engine is expected to be valid as long as this service performs
     * reseeding.
     */
    seed_t registerEngine
      (CLHEP::HepRandomEngine& engine, std::string instance = "")
      { return registerEngine(CLHEPengineSeeder(engine), instance); }

    /**
     * @brief Registers an existing CLHEP engine with `art::NuRandomService`.
     * @param engine a reference to the CLHEP random generator engine
     * @param instance name of the engine
     * @param seedParam the optional seed configuration parameter
     * @return the seed assigned to the engine (may be invalid)
     *
     * The specified engine is not managed.
     * It may be owned by `art::RandomNumberGenerator` service.
     *
     * The engine is expected to be valid as long as this service performs
     * reseeding.
     */
    seed_t registerEngine(
      CLHEP::HepRandomEngine& engine, std::string instance,
      SeedAtom const& seedParam
      )
      { return registerEngine(CLHEPengineSeeder(engine), instance, seedParam); }

    /**
     * @brief Registers an existing CLHEP engine with `art::NuRandomService`.
     * @param engine a reference to the CLHEP random generator engine
     * @param instance name of the engine
     * @param pset parameter set to read parameters from
     * @param pnames names of the seed parameters
     * @return the seed assigned to the engine (may be invalid)
     *
     * The specified engine is not managed.
     * It may be owned by `art::RandomNumberGenerator` service.
     *
     * The engine is expected to be valid as long as this service performs
     * reseeding.
     */
    seed_t registerEngine(
      CLHEP::HepRandomEngine& engine, std::string instance,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      )
      {
        return registerEngine
          (CLHEPengineSeeder(engine), instance, pset, pnames);
      }
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP

    /// @}
    // --- END --- Register an existing engine ---------------------------------


    /// @{
    /// @name Declare the presence of an engine

    /**
     * @brief Declares the presence of an engine with a given instance name
     * @param instance name of the instance of the engine (empty by default)
     * @return the seed assigned to the engine (may be invalid)
     *
     * The existence of an engine with the specified instance name is recorded,
     * and a seed is assigned to it. The engine will be identified by the
     * instance name and by context information (the current module).
     *
     * Differently from registerAndSeedEngine() and registerEngine(), the actual
     * existence of a engine is not required. It is up to the user to manage
     * the engine, if any at all, including the seeding.
     */
    seed_t declareEngine(std::string instance = "");

    /**
     * @brief Declares the presence of an engine with a given instance name
     * @param instance name of the instance of the engine
     * @param pset parameter set where to find a possible fixed seed request
     * @param pname the name of the parameter for the fixed seed request
     * @return the seed assigned to the engine (may be invalid)
     *
     * The existence of an engine with the specified instance name is recorded,
     * and a seed is assigned to it. The engine will be identified by the
     * instance name and by context information (the current module).
     *
     * The preferred way to obtain the seed is from configuration.
     * First, the seed is retrieved from the specified configuration,
     * looking for the first of the parameters in pname that is available.
     * If no parameter is found, the seed is obtained from NuRandomService.
     *
     * Differently from registerAndSeedEngine() and registerEngine(), the actual
     * existence of a engine is not required. It is up to the user to manage
     * the engine, if any at all, including the seeding.
     */
    seed_t declareEngine
      (std::string instance, fhicl::ParameterSet const& pset, std::string pname)
      { return declareEngine(instance, pset, { pname }); }

    /**
     * @brief Declares the presence of an engine with a given instance name
     * @param instance name of the instance of the engine
     * @param pset parameter set where to find a possible fixed seed request
     * @param pnames name of the parameters for the fixed seed request
     * @return the seed assigned to the engine (may be invalid)
     * @see declareEngine(std::string, fhicl::ParameterSet const&, std::string)
     *
     * This method provides the same function as
     * declareEngine(std::string, fhicl::ParameterSet const&, std::string),
     * but it can pick the seed from the first parameter among the ones in pset
     * whose name is in pnames.
     */
    seed_t declareEngine(
      std::string instance,
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      );

    /**
     * @brief Declares the presence of an engine with a default instance name
     * @param pset parameter set where to find a possible fixed seed request
     * @param pname the name of the parameter for the fixed seed request
     * @return the seed assigned to the engine (may be invalid)
     * @see declareEngine(fhicl::ParameterSet const&, std::string)
     *
     * This method provides the same function as
     * declareEngine(std::string, fhicl::ParameterSet const&, std::string),
     * but it gives the engine a empty instance name.
     */
    seed_t declareEngine(fhicl::ParameterSet const& pset, std::string pname)
      { return declareEngine("", pset, pname); }

    /**
     * @brief Declares the presence of an engine with a default instance name
     * @param pset parameter set where to find a possible fixed seed request
     * @param pnames name of the parameters for the fixed seed request
     * @return the seed assigned to the engine (may be invalid)
     * @see declareEngine(std::string, fhicl::ParameterSet const&, std::string)
     *
     * This method provides the same function as
     * declareEngine(std::string, fhicl::ParameterSet const&, std::string),
     * but it can pick the seed from the first parameter among the ones in pset
     * whose name is in pnames.
     * Also, it gives the engine a empty instance name.
     */
    seed_t declareEngine(
      fhicl::ParameterSet const& pset, std::initializer_list<std::string> pnames
      )
      { return declareEngine("", pset, pnames); }
    /// @}

    /// @{
    /**
     * @brief Defines a seeder for a previously declared engine
     * @param seeder seeder associated to the engine
     * @param instance name of engine instance (default: empty)
     * @return the seed assigned to the engine (may be invalid)
     * @see declareEngine()
     *
     * The seeder is the same object as in registerEngine().
     * This function can be used to finalise the declaration of an engine.
     * If the engine was just declared with declareEngine() (as opposed to
     * registered with registerEngine() or created with registerAndSeedEngine()),
     * "defining" the engine will hook it to NuRandomService, that will take care
     * of setting seeds automatically when needed.
     * This step is not mandatory, but no automatic seeding will happen if it is
     * omitted.
     */
    seed_t defineEngine
      (SeedMaster_t::Seeder_t seeder, std::string instance = {});

#if (NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP)
    /**
     * @brief Defines a seeder for a previously declared engine
     * @param instance name of engine instance
     * @param engine CLHEP engine to be associated to the instance
     * @return the seed assigned to the engine (may be invalid)
     * @see declareEngine()
     *
     * This method operates on the default engine instance and performs the
     * same operations as defineEngine(std::string, Seeder_t).
     * A seeder is internally created for the CLHEP random engine.
     */
    seed_t defineEngine
      (CLHEP::HepRandomEngine& engine, std::string instance = {})
      { return defineEngine(CLHEPengineSeeder(engine), instance); }
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP
    /// @}


    /// Prints known (EngineId,seed) pairs.
    template<class Stream>
    void print(Stream&& out) const
      { seeds.print(std::forward<Stream>(out)); }

    /// Prints to the framework Info logger
    void print() const { print(mf::LogInfo("NuRandomService")); }

#if (NURANDOM_RANDOMUTILS_NuRandomService_USEROOT)
    /// Seeder_t functor setting the seed of a ROOT TRandom engine (untested!)
    class TRandomSeeder {
        public:
      TRandomSeeder(TRandom* engine): pRandom(engine) {}
      void operator() (EngineId const&, seed_t seed)
        { if (pRandom) pRandom->SetSeed(seed); }
        protected:
      TRandom* pRandom = nullptr;
    }; // class TRandomSeeder
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USEROOT

#if (NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP)
    /// Seeder_t functor setting the seed of a CLHEP::HepRandomEngine engine (untested!)
    class CLHEPengineSeeder {
        public:
      CLHEPengineSeeder(CLHEP::HepRandomEngine& e): engine(e) {}
      CLHEPengineSeeder(CLHEP::HepRandomEngine* e): engine(*e) {}
      void operator() (EngineId const&, seed_t seed)
        {
          engine.setSeed(seed, 0);
          MF_LOG_DEBUG("CLHEPengineSeeder")
            << "CLHEP engine: '" << engine.name() << "'[" << ((void*) &engine)
            << "].setSeed(" << seed << ", 0)";
        }
        protected:
      CLHEP::HepRandomEngine& engine;
    }; // class CLHEPengineSeeder
#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP

  private:

    /// Class managing the seeds
    SeedMaster_t seeds;

    /**
     * Helper to track state of art.
     *
     * The state is updated by NuRandomService itself, and therefore knows only
     * about what it is notified about, when it is notified about.
     * For example, service construction phase may start before the service
     * was even constructed, but the state will be updated only on NuRandomService
     * construction.
     */
    NuRandomServiceHelper::ArtState state;

    /// Control the level of information messages.
    int verbosity = 0;
    bool bPrintEndOfJobSummary = false; ///< print a summary at the end of job

    /// Register an engine and seeds it with the seed from the master
    seed_t registerEngineID(
      EngineId const& id,
      SeedMaster_t::Seeder_t seeder = SeedMaster_t::Seeder_t()
      );

    /// Set the seeder of an existing engine
    seed_t defineEngineID(EngineId const& id, SeedMaster_t::Seeder_t seeder);


    /// Returns whether the specified engine is already registered
    bool hasEngine(EngineId const& id) const { return seeds.hasEngine(id); }

    // Main logic for computing and validating a seed.
    seed_t getSeed(EngineId const&);

    // Main logic for computing and validating a seed.
    seed_t getEventSeed(EngineId const&);

    /**
     * @brief Reseeds the specified engine instance in the current module
     * @param instance the name of the engine instance
     * @return the seed set, or InvalidSeed if no reseeding happened
     */
    seed_t reseedInstance(EngineId const& id);

    //@{
    /// Reseeds all the engines in the current module
    void reseedModule(std::string currentModule);
    void reseedModule();
    //@}

    /// Reseed all the global engines
    void reseedGlobal();

    /// Registers the engine ID into SeedMaster
    seed_t prepareEngine(EngineId const& id, SeedMaster_t::Seeder_t seeder);

    // Helper functions for all policies
    void ensureValidState(bool bGlobal = false) const;

    //@{
    /// Returns a fully qualified EngineId
    EngineId qualify_engine_label
      (std::string moduleLabel, std::string instanceName) const;
    EngineId qualify_engine_label(std::string instanceName = "") const;
    EngineId qualify_global_engine(std::string instanceName = "") const
      { return EngineId(instanceName, EngineId::global); }
    //@}

    //@{
    /// Reads the seed from the first of the specified parameters available.
    /// @return the value of the seed if found, no value otherwise
    static std::optional<seed_t> readSeedParameter
      (fhicl::ParameterSet const& pset, std::string pname)
      { return readSeedParameter(pset, { pname }); }
    static std::optional<seed_t> readSeedParameter(
      fhicl::ParameterSet const& pset,
      std::initializer_list<std::string> pnames
      );
    static std::optional<seed_t> readSeedParameter(SeedAtom const& param);
    //@}

    /// Query a seed from the seed master
    seed_t querySeed(EngineId const& id);


    /// Helper to retrieve a seed including configuration.
    /// @return the seed, and whether it is fixed (that is, from configuration)
    std::pair<seed_t, bool> extractSeed
      (EngineId const& id, std::optional<seed_t> seed);

    /// Forces NuRandomService not to change the seed of the specified engine
    void freezeSeed(EngineId const& id, seed_t frozen_seed);

    /// Registers an engine and its seeder
    void registerEngineIdAndSeeder
      (EngineId const& id, SeedMaster_t::Seeder_t seeder);

    /// Calls the seeder with the specified seed and engine ID
    seed_t seedEngine(EngineId const& id) { return seeds.reseed(id); }

    // Call backs that will be called by art.
    void preModuleConstruction (art::ModuleDescription const& md);
    void postModuleConstruction(art::ModuleDescription const&);
    void preModuleBeginRun     (art::ModuleContext const& mc);
    void postModuleBeginRun    (art::ModuleContext const&);
    void preProcessEvent       (art::Event const& evt, art::ScheduleContext);
    void preModule             (art::ModuleContext const& mc);
    void postModule            (art::ModuleContext const&);
    void postProcessEvent      (art::Event const&, art::ScheduleContext);
    void preModuleEndJob       (art::ModuleDescription const& md);
    void postModuleEndJob      (art::ModuleDescription const&);
    void postEndJob            ();


  }; // class NuRandomService


#if (NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP)
  //----------------------------------------------------------------------------
  // FIXME: See if the engine preparation can be done similarly to how
  //        it is described in the "Create and register an engine"
  //        documentation above.

  //----------------------------------------------------------------------------
  inline std::reference_wrapper<NuRandomService::engine_t>
  NuRandomService::registerAndSeedEngine(engine_t& engine,
                                         std::string type,
                                         std::string instance,
                                         std::optional<seed_t> const seed)
  {
    EngineId const id = qualify_engine_label(instance);
    registerEngineIdAndSeeder(id, CLHEPengineSeeder{engine});
    auto const [seedValue, frozen] = extractSeed(id, seed);
    engine.setSeed(seedValue, 0);
    mf::LogInfo("NuRandomService")
      << "Seeding " << type << " engine \"" << id.artName()
      << "\" with seed " << seedValue << ".";
    if (frozen) freezeSeed(id, seedValue);
    return engine;
  } // NuRandomService::registerAndSeedEngine(seed_t)


  inline std::reference_wrapper<NuRandomService::engine_t>
  NuRandomService::registerAndSeedEngine(engine_t& engine,
                                std::string type,
                                std::string instance,
                                SeedAtom const& seedParam)
  {
    return registerAndSeedEngine(engine, type, instance, readSeedParameter(seedParam));
  } // NuRandomService::registerAndSeedEngine(ParameterSet)


  inline std::reference_wrapper<NuRandomService::engine_t>
  NuRandomService::registerAndSeedEngine(engine_t& engine,
                                std::string type,
                                std::string instance,
                                fhicl::ParameterSet const& pset,
                                std::initializer_list<std::string> pnames)
  {
    return
      registerAndSeedEngine(engine, type, instance, readSeedParameter(pset, pnames));
  }


#endif // NURANDOM_RANDOMUTILS_NuRandomService_USECLHEP

} // namespace rndm

DECLARE_ART_SERVICE(rndm::NuRandomService, LEGACY)

#endif // NURANDOM_RANDOMUTILS_NuRandomService_H
