/**
 * @file   nurandom/RandomUtils/Providers/PolicyNames.cxx
 * @brief  Helper to instantiate a random number policy class.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   20210809
 * @see    nurandom/RandomUtils/Providers/PolicyLoader.h
 */

// library header
#include "nurandom/RandomUtils/Providers/PolicyNames.h"

// framework libraries
#include "cetlib_except/exception.h"

// C/C++ standard libraries
#include <algorithm> // std::find()
#include <vector>
#include <string>
#include <cassert>


// -- BEGIN --- Policy names management  ---------------------------------------

namespace {
  
  std::vector<std::string> BuildPolicyNames() {
    const char* cnames[] = {
      #define NURANDOM_SEED_SERVICE_POLICY(x) #x,
      NURANDOM_SEED_SERVICE_POLICIES
      #undef NURANDOM_SEED_SERVICE_POLICY
      };
    return std::vector<std::string>
      { cnames, cnames + sizeof(cnames)/sizeof(cnames[0]) };
  } // BuildPolicyNames()

  std::vector<std::string> PolicyNames { BuildPolicyNames() };
  
} // local namespace

// -- END ----- Policy names management  ---------------------------------------



// -----------------------------------------------------------------------------
namespace rndm::details {
  
  static_assert(static_cast<unsigned>(Policy::unDefined) == 0,
    "\"unDefined\" policy needs to be the first one.");

  
  //----------------------------------------------------------------------------
  std::vector<std::string> const& policyNames() { return ::PolicyNames; }


  //----------------------------------------------------------------------------
  std::string const& policyName(Policy policy) {
    std::size_t const index { static_cast<unsigned>(policy) };
    if (index < policyNames().size()) return policyNames()[index];
    
    throw cet::exception("rndm::details::policyName")
      << "Invalid policy (index #" << index << ")\n";
    
  } // policyName()


  // ---------------------------------------------------------------------------
  Policy policyFromName(std::string const& policyName) {
    
    auto const& names = policyNames();
    assert(!names.empty());
    auto const nbegin { begin(names) }, nend { end(names) };
    
    auto const iter = std::find(nbegin, nend, policyName);
    
    Policy const policy = (iter == nend)
      ? Policy::unDefined
      : static_cast<Policy>(std::distance(begin(names), iter))
      ;
    
    if (policy != Policy::unDefined) return policy;
    
    // the first policy, `unDefined`, is deliberately omitted from this message
    cet::exception e { "rndm::details::policyFromName" };
    e << "rndm::details::policyFromName(\"" << policyName
      << "\"): unrecognized policy.\nKnown policies are: ";
    auto iName = nbegin;
    while (++iName != nend) e << " '" << (*iName) << '\'';
    throw e << ".\n";
    
  } // policyFromName()
  
  
  // ---------------------------------------------------------------------------
  
  
} // namespace rndm::details

