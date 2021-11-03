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
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

find_package(cetmodules 2.29.06 REQUIRED)
project(nurandom VERSION 1.08.00 LANGUAGES CXX)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS WERROR
                       NO_UNDEFINED
                       EXTRA_FLAGS -pedantic 
                                   $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-local-typedefs
                                                             -Wno-variadic-macros>)
cet_report_compiler_flags(REPORT_THRESHOLD VERBOSE)

find_package(art_root_io REQUIRED)
find_package(CLHEP REQUIRED)

# macros
include(ArtMake)
include(BuildPlugins)

# source
add_subdirectory (nurandom)
add_subdirectory (test)

# ups - table and config files

# packaging utility

cet_cmake_config()
