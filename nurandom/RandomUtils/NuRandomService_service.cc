/**
 * @file NuRandomService_service.cc
 * @brief Assists in the distribution of guaranteed unique seeds to all engines within a job.
 * @author Rob Kutschke (kutschke@fnal.gov)
 * @see `nurandom/RandomUtils/NuRandomService.h`
 *      `nurandom/RandomUtils/Providers/SeedMaster.h`
 */

// NuRandomService header
#include "nurandom/RandomUtils/NuRandomService.h"

// Art include files
#include "art/Framework/Services/Registry/ServiceDefinitionMacros.h"

DEFINE_ART_SERVICE(rndm::NuRandomService)
