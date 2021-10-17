add_subdirectory(Providers)

find_package(ROOT COMPONENTS Core MathCore REQUIRED PRIVATE)

art_make_library(SOURCE SeedTestUtils.cxx
  LIBRARIES
    nurandom::RandomUtils_NuRandomService_service
    art::Framework_Core
    ${ART_FRAMEWORK_UTILITIES}
    ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR_SERVICE}
    messagefacility::MF_MessageLogger
    fhiclcpp::fhiclcpp
    cetlib::cetlib
    cetlib_except::cetlib_except
    CLHEP::CLHEP
    NO_INSTALL
)

cet_build_plugin(GlobalEngineUserTestService art::service
  LIBRARIES nurandom::RandomUtils_NuRandomService_service art::Utilities canvas::canvas
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except ROOT::Core
  ROOT::MathCore NO_INSTALL)
cet_build_plugin(RandomManagerTest art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)
cet_build_plugin(SeedTestPolicy art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)
cet_build_plugin(SeedTestQuerySeeds art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)
cet_build_plugin(SeedTestRegisterOnProcessEvent art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)
cet_build_plugin(SeedTestRegisterOnRun art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)
cet_build_plugin(SeedTestRegisterOnSubRun art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)
cet_build_plugin(ValidatedConfigSeedTest art::module
  LIBRARIES PRIVATE nurandom::test_RandomUtils nurandom::RandomUtils_NuRandomService_service
  art::Framework_Core art::Framework_Principal
  art::Framework_Services_Registry messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp cetlib::cetlib cetlib_except::cetlib_except CLHEP::CLHEP
  NO_INSTALL)

#
# The following tests verify that engine declaration may happen only on
# construction, and that seed query can happen at any time.
#
cet_test( SeedTestQuerySeeds_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c seedtest_query_seeds.fcl
  DATAFILES seedtest_query_seeds.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
)

cet_test( SeedTestRegisterOnProcessEvent_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c seedtest_register_on_process_event.fcl
  DATAFILES seedtest_register_on_process_event.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
  TEST_PROPERTIES WILL_FAIL true
)

cet_test( SeedTestRegisterOnSubRun_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c seedtest_register_on_subrun.fcl
  DATAFILES seedtest_register_on_subrun.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
  TEST_PROPERTIES WILL_FAIL true
)

cet_test( SeedTestRegisterOnRunRegistered_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c seedtest_register_on_run_registered.fcl
  DATAFILES seedtest_register_on_run_registered.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
  TEST_PROPERTIES WILL_FAIL false
)

cet_test( SeedTestRegisterOnRunUnregistered_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c seedtest_register_on_run_unregistered.fcl
  DATAFILES seedtest_register_on_run_unregistered.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
  TEST_PROPERTIES WILL_FAIL true
)

cet_test( GlobalSeedTestLinear_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c globalseedtest_linear.fcl
  DATAFILES globalseedtest_linear.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
)

cet_test( GlobalSeedTestPredefinedSeed_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c globalseedtest_predefined.fcl
  DATAFILES globalseedtest_predefined.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
)

cet_test( GlobalSeedTestPerEvent_test HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all -c globalseedtest_perevent.fcl
  DATAFILES globalseedtest_perevent.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
)

#
# Some tests are going to fail at configuration phase. Those are "failing".
# Some other tests will have failures when getting a seed:
#   the errors are expected and those tests are considered successful.
# Some more are art-specific and don't test anything in SeedMaster.
#


# The first batch of tests is for NuRandomService ("integration" tests shared with SeedMaster)
set( SuccessfulServiceSharedTests
  LinearMap01
  LinearMapDepr01
  PredefinedOfs01
  PredefinedOfs02
  PredefinedSeed01
  PredefinedSeed02
  PredefinedSeed03
  PredefinedSeed04
  Random01
  Random02
  )
foreach( ServiceTestName ${SuccessfulServiceSharedTests} )
  cet_test( test${ServiceTestName} HANDBUILT
            TEST_EXEC art
            TEST_ARGS --rethrow-all --config test${ServiceTestName}.fcl
            DATAFILES Providers/test${ServiceTestName}.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
          )
endforeach( ServiceTestName )

set( FailingServiceSharedTests
  InvalidPolicy
  LinearMapErr01
  PredefinedOfsErr01
  PredefinedOfsErr02
  PredefinedOfsErr04
  PredefinedOfsErr05
  PredefinedSeedErr01
  PredefinedSeedErr02
  PredefinedSeedErr03
  PredefinedOfsErr03
  )
foreach( ServiceTestName ${FailingServiceSharedTests} )
  cet_test( test${ServiceTestName} HANDBUILT
            TEST_EXEC art
            TEST_ARGS --rethrow-all --config test${ServiceTestName}.fcl
            TEST_PROPERTIES WILL_FAIL true
            DATAFILES Providers/test${ServiceTestName}.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
          )
endforeach( ServiceTestName )


# The second batch of tests is for NuRandomService ("integration" tests for art service only)
set( SuccessfulServiceOnlyTests
  PerEvent01
  ValidatedConfigLinear
  ValidatedConfigPerEvent
  )
foreach( ServiceTestName ${SuccessfulServiceOnlyTests} )
  cet_test( test${ServiceTestName} HANDBUILT
            TEST_EXEC art
            TEST_ARGS --rethrow-all --config test${ServiceTestName}.fcl
            DATAFILES test${ServiceTestName}.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
          )
endforeach( ServiceTestName )

set( FailingServiceOnlyTests
  PerEventErr01
  )
foreach( ServiceTestName ${FailingServiceOnlyTests} )
  cet_test( test${ServiceTestName} HANDBUILT
            TEST_EXEC art
            TEST_ARGS --rethrow-all --config test${ServiceTestName}.fcl
            TEST_PROPERTIES WILL_FAIL true
            DATAFILES test${ServiceTestName}.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
          )
endforeach( ServiceTestName )


# The third batch of tests is for NuRandomService engine manager features
set( ServiceManagingTests
  RandomManagerTest01
  RandomManagerTest03
  )
foreach( ServiceManagingTestName ${ServiceManagingTests} )
  cet_test( ${ServiceManagingTestName} HANDBUILT
            TEST_EXEC art
            TEST_ARGS --rethrow-all --config ${ServiceManagingTestName}.fcl
            DATAFILES ${ServiceManagingTestName}.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
          )
endforeach( ServiceManagingTestName )

set( FailingServiceManagingTests
  RandomManagerTest02
  )
foreach( ServiceManagingTestName ${FailingServiceManagingTests} )
  cet_test( ${ServiceManagingTestName} HANDBUILT
            TEST_EXEC art
            TEST_ARGS --rethrow-all --config ${ServiceManagingTestName}.fcl
            TEST_PROPERTIES WILL_FAIL true
            DATAFILES ${ServiceManagingTestName}.fcl Providers/messageService.fcl Providers/standardMessageDestinations.fcl
          )
endforeach( ServiceManagingTestName )
