/**
 * @file   nurandom/RandomUtils/Providers/PoliciesFwd.h
 * @brief  Forward declaration of random policy classes.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   20210809
 */

#ifndef NURANDOM_RANDOMUTILS_PROVIDERS_POLICIESFWD_H
#define NURANDOM_RANDOMUTILS_PROVIDERS_POLICIESFWD_H


// -----------------------------------------------------------------------------
namespace rndm::details {
  
  template <typename SEED>
  class AutoIncrementPolicy;
  
  template <typename SEED>
  class LinearMappingPolicy;
  
  template <typename SEED>
  class PredefinedOffsetPolicy;
  
  template <typename SEED>
  class PredefinedSeedPolicy;
  
  template <typename SEED>
  class RandomPolicy;
  
  template <typename SEED>
  class PerEventPolicy;
  
} // namespace rndm::details


// -----------------------------------------------------------------------------


#endif // NURANDOM_RANDOMUTILS_PROVIDERS_POLICIESFWD_H
