add_subdirectory(Providers)

find_package(ROOT COMPONENTS Core REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)

art_make( ### MIGRATE-ACTION-RECOMMENDED: use art_make_library(), art_dictionary(), simple_plugin() with explicit source lists
  SERVICE_LIBRARIES
    nurandom_RandomUtils_Providers
    art::Framework_Core
    art::Framework_Principal
    art::Framework_Services_Registry
    art::Persistency_Provenance
    art::Utilities
    art::Framework_Services_Optional_RandomNumberGenerator_service
    canvas::canvas
    messagefacility::MF_MessageLogger
    fhiclcpp::fhiclcpp
    cetlib::cetlib
    cetlib_except::cetlib_except
    ROOT::Core
  PLUGIN_LIBRARIES
    art::Framework_Core
    art::Persistency_Provenance
    canvas::canvas
    messagefacility::MF_MessageLogger
    fhiclcpp::fhiclcpp
    cetlib::cetlib
    cetlib_except::cetlib_except
    Boost::filesystem
)

install_source()
install_headers()
install_fhicl()
