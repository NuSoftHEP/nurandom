cet_make_library(SOURCE PolicyNames.cxx
  LIBRARIES
    fhiclcpp::fhiclcpp
    cetlib::cetlib
    cetlib_except::cetlib_except
)

install_source()
install_headers()
install_fhicl()
