include(CetTest)

cet_enable_asserts()
cet_test_env("FHICL_FILE_PATH=.")

add_subdirectory(RandomUtils)
add_subdirectory(GeneratedEventTimestamp)

