/**
 * @file    GlobalEngineUserService_service.cc
 * @brief   Test service registering its own engine (implementation file)
 * @author  Gianluca Petrillo
 * @date    March 22, 2016
 * @see     GlobalEngineUserTestService.h
 * 
 */

// my header
#include "test/RandomUtils/GlobalEngineUserTestService.h"

// framework libraries
#include "art/Framework/Services/Registry/ServiceDefinitionMacros.h"

DEFINE_ART_SERVICE(testing::GlobalEngineUserTestService)

//------------------------------------------------------------------------------
