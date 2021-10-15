cet_make( ### MIGRATE-ACTION-RECOMMENDED: use cet_make_library(), build_dictionary(), basic_plugin() with explicit source lists
  LIBRARIES
    fhiclcpp::fhiclcpp
    cetlib::cetlib
    cetlib_except::cetlib_except
)


install_source()
install_headers()
install_fhicl()
