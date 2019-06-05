add_subdirectory(Providers)

art_make(
  SERVICE_LIBRARIES
    art_Framework_Core
    art_Framework_Principal
    art_Framework_Services_Registry
    art_Persistency_Provenance
    art_Utilities
    art_Framework_Services_Optional_RandomNumberGenerator_service
    canvas
    MF_MessageLogger
    fhiclcpp
    cetlib
    cetlib_except
    ${ROOT_CORE}
)


install_source()
install_headers()
install_fhicl()
