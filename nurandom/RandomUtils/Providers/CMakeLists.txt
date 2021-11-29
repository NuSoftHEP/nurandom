cet_make_library(SOURCE PolicyNames.cxx
  LIBRARIES
    PUBLIC
    cetlib_except::cetlib_except
    INTERFACE
    fhiclcpp::fhiclcpp
    cetlib::cetlib
)

install_source()
install_headers()
install_fhicl()
