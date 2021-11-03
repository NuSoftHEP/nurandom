add_subdirectory(Providers)

find_package(ROOT COMPONENTS Core REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)

cet_build_plugin(NuRandomService art::service
  LIBRARIES
    nurandom::RandomUtils_Providers
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
    ROOT::Core)
cet_build_plugin(GeneratedEventTimestamp art::plugin
  LIBRARIES PRIVATE
    art::Framework_Core
    art::Persistency_Provenance
    canvas::canvas
    messagefacility::MF_MessageLogger
    fhiclcpp::fhiclcpp
    cetlib::cetlib
    cetlib_except::cetlib_except
    Boost::filesystem)

install_source()
install_headers()
install_fhicl()
