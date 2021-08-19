/**
 * @file   nurandom/RandomUtils/Providers/PolicyNames.h
 * @brief  Declaration of policy enumerator and names.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   20210809
 * @see    nurandom/RandomUtils/Providers/PolicyNames.cxx
 */

#ifndef NURANDOM_RANDOMUTILS_PROVIDERS_POLICYNAMES_H
#define NURANDOM_RANDOMUTILS_PROVIDERS_POLICYNAMES_H


// framework libraries
#include "cetlib_except/exception.h"

// C/C++ standard libraries
#include <vector>
#include <string>


// Preprocessor macro to create a master list of all the policies
#define NURANDOM_SEED_SERVICE_POLICIES                   \
  NURANDOM_SEED_SERVICE_POLICY(unDefined)                \
  NURANDOM_SEED_SERVICE_POLICY(autoIncrement)            \
  NURANDOM_SEED_SERVICE_POLICY(linearMapping)            \
  NURANDOM_SEED_SERVICE_POLICY(preDefinedOffset)         \
  NURANDOM_SEED_SERVICE_POLICY(preDefinedSeed)           \
  NURANDOM_SEED_SERVICE_POLICY(random)                   \
  NURANDOM_SEED_SERVICE_POLICY(perEvent)                 \
  /**/


// -----------------------------------------------------------------------------
namespace rndm::details {
  
  // ---------------------------------------------------------------------------
  /// Enumeration of all supported random seed policies.
  enum class Policy: unsigned {
    #define NURANDOM_SEED_SERVICE_POLICY(x) x,
    NURANDOM_SEED_SERVICE_POLICIES
    #undef NURANDOM_SEED_SERVICE_POLICY
  };
  
  
  //----------------------------------------------------------------------------
  /// Returns a list of names of policies, in the same order as `Policy` enum.
  std::vector<std::string> const& policyNames();
  

  //----------------------------------------------------------------------------
  /**
   * @brief Returns the name of the specified policy.
   * @throw cet::exception (category: `rndm::details::policyName`) if the name
   *        is unknown
   */
  std::string const& policyName(Policy policy);
  

  // ---------------------------------------------------------------------------
  /**
   * @brief Returns the policy with the specified `name`.
   * @param name name of the desired policy
   * @return enumerator value matching the policy with the specified `name`
   * @throw cet::exception (category: `rndm::details::policyFromName`) if
   *        the name is unknown or if it matches `Policy::unDefined`
   * 
   * The policy placeholder `unDefined` is not accepted by this function and
   * treated as a non-existing policy.
   */
  Policy policyFromName(std::string const& name);

  // ---------------------------------------------------------------------------
  
  
} // namespace rndm::details


#endif // NURANDOM_RANDOMUTILS_PROVIDERS_POLICYNAMES_H
