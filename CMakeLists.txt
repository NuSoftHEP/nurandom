# ======================================================================
#  nurandom main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/nurandom/ups/setup_for_development
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================

cmake_minimum_required(VERSION 3.18.2 FATAL_ERROR)

project(nurandom VERSION 1.05.02 LANGUAGES CXX)

find_package(cetmodules REQUIRED)
find_package(canvas_root_io)
find_package(art)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS WERROR
                       NO_UNDEFINED
                       EXTRA_FLAGS -pedantic
                       EXTRA_CXX_FLAGS -Wno-unused-local-typedefs -Wno-variadic-macros)
cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product(art_root_io)
find_ups_product( clhep )
find_ups_root()
find_package(messagefacility)

# macros
include(ArtMake)
include(BuildPlugins)

# source
add_subdirectory (nurandom)
add_subdirectory (test)

# ups - table and config files
if(WANT_UPS)
add_subdirectory(ups)
endif()

# packaging utility
include(UseCPack)

