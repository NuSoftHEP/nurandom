# unit tests
cet_test( SeedMaster_test
  NO_AUTO
  LIBRARIES
    art_Utilities
    canvas
    ${MF_MESSAGELOGGER}
    ${MF_UTILITIES}
    ${FHICLCPP}
    cetlib
    cetlib_except
)


#
# Some tests are going to fail at configuration phase. Those are "failing".
# Some other tests will have failures when getting a seed:
#   the errors are expected and those tests are considered successful.
# Some more are art-specific and don't test anything in SeedMaster.
#


# This first batch of tests is for SeedMaster (no art involved, full unit test).
# skipped tests: perEvent01, perEventErr01 (require an event loop)
set( SuccessfulSeedMasterTests
  LinearMap01
  LinearMapDepr01
  LinearMapErr01
  PredefinedOfs01
  PredefinedOfs02
  PredefinedOfsErr01
  PredefinedOfsErr02
  PredefinedOfsErr04
  PredefinedOfsErr05
  PredefinedSeed01
  PredefinedSeed02
  PredefinedSeed03
  PredefinedSeed04
  PredefinedSeedErr01
  PredefinedSeedErr02
  PredefinedSeedErr03
  Random01
  Random02
  )
foreach( SeedMasterTestName ${SuccessfulSeedMasterTests} )
  cet_test( SeedMaster_test${SeedMasterTestName} HANDBUILT
            TEST_EXEC SeedMaster_test
            TEST_ARGS test${SeedMasterTestName}.fcl "services.NuRandomService"
            DATAFILES test${SeedMasterTestName}.fcl messageService.fcl standardMessageDestinations.fcl
          )
endforeach( SeedMasterTestName )

set( FailingSeedMasterTests
  InvalidPolicy
  PredefinedOfsErr03
  )
foreach( SeedMasterTestName ${FailingSeedMasterTests} )
  cet_test( SeedMaster_test${SeedMasterTestName} HANDBUILT
            TEST_EXEC SeedMaster_test
            TEST_ARGS test${SeedMasterTestName}.fcl "services.NuRandomService"
            TEST_PROPERTIES WILL_FAIL true
            DATAFILES test${SeedMasterTestName}.fcl messageService.fcl standardMessageDestinations.fcl
          )
endforeach( SeedMasterTestName )
