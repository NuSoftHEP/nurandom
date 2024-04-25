add_subdirectory(Providers)

find_package(ROOT COMPONENTS Core REQUIRED)

cet_build_plugin(NuRandomService art::service
  LIBRARIES
    PUBLIC
    nurandom::RandomUtils_Providers
    canvas::canvas
    messagefacility::MF_MessageLogger
    CLHEP::Random
    PRIVATE
    art::Framework_Principal
    art::Persistency_Provenance
    )
cet_build_plugin(GeneratedEventTimestamp art::EmptyEventTimeStampPlugin
  LIBRARIES PRIVATE
    canvas::canvas
    messagefacility::MF_MessageLogger
  )

install_source()
install_headers()
install_fhicl()
