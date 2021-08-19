/**
 * @file   nurandom/RandomUtils/Providers/PolicyFactory.h
 * @brief  Helper to instantiate a random number policy class.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   20210809
 * 
 * This is the random policy "factory", except that it does work very poorly as
 * a factory (no automatic policy discovery, all policies must be linked in).
 * As proven by the need for `PoliciesFwd.h` header.
 * 
 */

#ifndef NURANDOM_RANDOMUTILS_PROVIDERS_POLICYFACTORY_H
#define NURANDOM_RANDOMUTILS_PROVIDERS_POLICYFACTORY_H


// nurandom libraries
#include "nurandom/RandomUtils/Providers/RandomSeedPolicyBase.h"
#include "nurandom/RandomUtils/Providers/PolicyNames.h"
#include "nurandom/RandomUtils/Providers/PoliciesFwd.h"

// framework libraries
#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"

// C/C++ standard libraries
#include <memory> // std::make_unique()
#include <string>


// -----------------------------------------------------------------------------
namespace rndm::details {
  
  // ---------------------------------------------------------------------------
  /// Return value of `makeRandomSeedPolicy()`: a pointer to
  /// `RandomSeedPolicyBase<SEED>` travelling with a policy number.
  template <typename SEED>
  struct PolicyStruct_t {
    Policy policy { Policy::unDefined };
    std::unique_ptr<RandomSeedPolicyBase<SEED>> ptr;
    
    operator bool() const { return bool(ptr); }
    bool operator!() const { return !ptr; }
    RandomSeedPolicyBase<SEED> const* operator->() const { return ptr.get(); }
    RandomSeedPolicyBase<SEED>* operator->() { return ptr.get(); }
    
  }; // PolicyStruct_t
  
  
  /**
   * @brief Constructs and returns a `RandomSeedPolicyBase` based on `config`.
   * @tparam SEED the type of seed `RandomSeedPolicyBase` is serving
   * @param config configuration of the policy object
   * @return a new `RandomSeedPolicyBase` object
   * 
   * The policy class is created according to the parameters in the specified
   * `config` parameter set.
   * The type of policy is determined by the `"policy"` key in that
   * parameter set.
   * 
   */
  template <typename SEED>
  PolicyStruct_t<SEED> makeRandomSeedPolicy(fhicl::ParameterSet const& config) {
    std::string const& policyName = config.get<std::string>("policy");
    
    // Throws if policy is not recognized.
    Policy const policy = policyFromName(policyName);
    
    switch (policy) {
      case Policy::autoIncrement:
        return { policy, std::make_unique<AutoIncrementPolicy<SEED>>(config) };
      case Policy::linearMapping:
        return { policy, std::make_unique<LinearMappingPolicy<SEED>>(config) };
      case Policy::preDefinedOffset:
        return { policy, std::make_unique<PredefinedOffsetPolicy<SEED>>(config) };
      case Policy::preDefinedSeed:
        return { policy, std::make_unique<PredefinedSeedPolicy<SEED>>(config) };
      case Policy::random:
        return { policy, std::make_unique<RandomPolicy<SEED>>(config) };
      case Policy::perEvent:
        return { policy, std::make_unique<PerEventPolicy<SEED>>(config) };
      case Policy::unDefined:
      default:
        // this should have been prevented by an exception by `policyFromName()`
        throw cet::exception("rndm::details::makeRandomSeedPolicy")
          << "Internal error: unknown policy '" << policyName << "'\n";
    } // switch
    
  } // rndm::details::makeRandomSeedPolicy()


  // ---------------------------------------------------------------------------
  
  
} // namespace rndm::details


#endif // NURANDOM_RANDOMUTILS_PROVIDERS_POLICYFACTORY_H
