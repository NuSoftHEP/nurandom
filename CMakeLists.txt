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

cmake_minimum_required(VERSION 3.12...3.18 FATAL_ERROR)

project(nurandom LANGUAGES CXX)

# cetbuildtools contains our cmake modules
find_package(cetbuildtools REQUIRED)

list(APPEND CMAKE_MODULE_PATH
  $ENV{CANVAS_ROOT_IO_DIR}/Modules
  $ENV{ART_DIR}/Modules)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS WERROR
                       NO_UNDEFINED
                       EXTRA_FLAGS -pedantic
                       EXTRA_CXX_FLAGS -Wno-unused-local-typedefs -Wno-variadic-macros)
cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product(art_root_io)
find_ups_root()

# macros
include(ArtMake)
include(BuildPlugins)

# source
add_subdirectory (nurandom)
add_subdirectory (test)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)

