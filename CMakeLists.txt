# ======================================================================
#  nutools main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/nutools/ups/setup_for_development
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================


# use cmake 2.8 or later
cmake_minimum_required (VERSION 3.3)

project(nutools)

# cetbuildtools contains our cmake modules
SET ( CETBUILDTOOLS_VERSION $ENV{CETBUILDTOOLS_VERSION} )
IF (NOT CETBUILDTOOLS_VERSION)
    MESSAGE (FATAL_ERROR "ERROR: setup cetbuildtools to get the cmake modules")
ENDIF()

set(CMAKE_MODULE_PATH $ENV{CANVAS_ROOT_IO_DIR}/Modules
                      $ENV{CETBUILDTOOLS_DIR}/Modules
                      ${CMAKE_MODULE_PATH})

include(CetCMakeEnv)
cet_cmake_env()

# -D_GLIBCXX_USE_NANOSLEEP is needed to make std::this_thread::sleep_for available.
cet_set_compiler_flags(DIAGS CAUTIOUS WERROR
                       NO_UNDEFINED
                       EXTRA_FLAGS -pedantic
                       EXTRA_CXX_FLAGS -D_GLIBCXX_USE_NANOSLEEP
                                       -Wno-unused-local-typedefs -Wno-variadic-macros)
cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( art )
find_ups_product(art_root_io)
find_ups_product( genie )
find_ups_product( dk2nudata )
find_ups_product( dk2nugenie )
find_ups_product( libwda )
find_ups_product( postgresql )
find_ups_product( nusimdata )
find_ups_product( cetlib )
find_ups_product( cetlib_except )
find_ups_geant4( )
find_ups_product( cetbuildtools )

message(STATUS "ROOTSYS is ${ROOTSYS}")

## find_library directives
# other dependencies
cet_find_library( PYTHIA6 NAMES Pythia6 PATHS ENV PYLIB NO_DEFAULT_PATH )
cet_find_library( LHAPDF NAMES LHAPDF PATHS ENV LHAPDF_LIB NO_DEFAULT_PATH )
cet_find_library( LOG4CPP NAMES log4cpp PATHS ENV LOG4CPP_LIB NO_DEFAULT_PATH )
cet_find_library( XML2 NAMES xml2 PATHS $ENV{LIBXML2_FQ_DIR}/lib NO_DEFAULT_PATH )
cet_find_library( CRY NAMES CRY PATHS ENV CRY_LIB NO_DEFAULT_PATH )


# genie-config for R-2 series didn't support --version (returns blank)
# execute_process(COMMAND genie-config --version OUTPUT_VARIABLE GENIE_VERSION)
# this is the equivalent:
#execute_process(COMMAND cat ${GENIE}/VERSION OUTPUT_VARIABLE GENIE_VERSION)
#string(STRIP ${GENIE_VERSION} GENIE_VERSION)
#string(SUBSTRING ${GENIE_VERSION} 0 1 GENIE_MAJOR_VERSION)
#MESSAGE("--NUTOOLS-- GENIE_VERSION=${GENIE_VERSION} major=${GENIE_MAJOR_VERSION}")
#if( ${GENIE_MAJOR_VERSION} LESS 3 )

# use GENIE UPS version to decide ...
parse_ups_version( ${GENIE_VERSION} )
if( ${VMAJ} LESS 3 )
  add_definitions(-DGENIE_PRE_R3)
#  MESSAGE("--NUTOOLS-- GENIE_VERSION=${VMAJ} ${VMIN} -DGENIE_PRE_R3")
# these are the pre-R-3 library names
  cet_find_library( GALGORITHM NAMES GAlgorithm PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GBARYONRESONANCE NAMES GBaryonResonance PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GBASE NAMES GBase PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GBODEKYANG NAMES GBodekYang PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GCHARM NAMES GCharm PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GCOH NAMES GCoh PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GDFRC NAMES GDfrc PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GDIS NAMES GDIS PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GCROSSSECTIONS NAMES GCrossSections PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GDECAY NAMES GDecay PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GELAS NAMES GElas PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GELFF NAMES GElFF PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GHEP NAMES GHEP PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GEVGCORE NAMES GEVGCore  PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GEVGMODULES NAMES GEVGModules PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GEVGDRIVERS NAMES GEVGDrivers PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GGIBUU NAMES GGiBUU PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GHADRONTRANSP NAMES GHadronTransp PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFRAGMENTATION NAMES GFragmentation PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GINTERACTION NAMES GInteraction PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GLLEWELLYNSMITH NAMES GLlewellynSmith  PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GMEC NAMES GMEC PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GMESSENGER NAMES GMessenger PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GNUGAMMA NAMES GNuGamma PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GNUE NAMES GNuE PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GNTUPLE NAMES GNtuple PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GNUCLEAR NAMES GNuclear PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GNUMERICAL NAMES GNumerical PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GQPM NAMES GQPM PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPDG NAMES GPDG PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPDF NAMES GPDF PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GQEL NAMES GQEL PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GRES NAMES GRES PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GREGISTRY NAMES GRegistry PATHS ENV GENIE_LIB NO_DEFAULT_PATH )

  # prior to GENIE R-2_9_0 Sehgal's name was spelled incorrectly
  # so if minor version # is 8 or less use GReinSeghal, otherwise GReinSehgal
  # parse_ups_version returns VMAJ, VMIN, and VPRJ
  parse_ups_version( ${GENIE_VERSION} )
  set ( GRSSPELLING GReinSehgal )
  if ( ${VMAJ} EQUAL 2 AND ${VMIN} LESS 9 )
    set ( GRSSPELLING GReinSeghal )
  endif ( ${VMAJ} EQUAL 2 AND ${VMIN} LESS 9 )
  message(STATUS "GENIE version ${GENIE_VERSION} major ${VMAJ} minor ${VMIN} ${GRSSPELLING}")
  cet_find_library( GREINSEHGAL NAMES ${GRSSPELLING} PATHS ENV GENIE_LIB NO_DEFAULT_PATH )

  cet_find_library( GUTILS NAMES GUtils PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GGEO NAMES GGeo PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFLUXDRIVERS NAMES GFluxDrivers PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GMUELOSS NAMES GMuELoss PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GREWEIGHT NAMES GReWeight PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GNUCLEONDECAY NAMES GNucleonDecay PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  set(GENIE_LIB_LIST  ${GALGORITHM}
                      ${GBARYONRESONANCE}
                      ${GBASE}
                      ${GBODEKYANG}
                      ${GCHARM}
                      ${GCOH}
                      ${GDFRC}
                      ${GDIS}
                      ${GCROSSSECTIONS}
                      ${GDECAY}
                      ${GELAS}
                      ${GELFF}
                      ${GHEP}
                      ${GEVGCORE}
                      ${GEVGMODULES}
                      ${GEVGDRIVERS}
                      ${GGIBUU}
                      ${GHADRONTRANSP}
                      ${GFRAGMENTATION}
                      ${GINTERACTION}
                      ${GLLEWELLYNSMITH}
                      ${GMEC}
                      ${GMESSENGER}
                      ${GNUGAMMA}
                      ${GNUE}
                      ${GNTUPLE}
                      ${GNUCLEAR}
                      ${GNUMERICAL}
                      ${GQPM}
                      ${GPDG}
                      ${GPDF}
                      ${GQEL}
                      ${GRES}
                      ${GREGISTRY}
                      ${GREINSEHGAL}
                      ${GUTILS}
                      ${GGEO}
                      ${GFLUXDRIVERS}
                      ${GMUELOSS}
                      ${GREWEIGHT}
                      ${GNUCLEONDECAY}
                      )
else()
# these are the POST-R-3 library names
#  MESSAGE("--NUTOOLS-- GENIE_VERSION=${VMAJ} ${VMIN} not defining -DGENIE_PRE_R3")

  cet_find_library( GFWMSG       NAMES GFwMsg       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWREG       NAMES GFwReg       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWALG       NAMES GFwAlg       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWINT       NAMES GFwInt       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWGHEP      NAMES GFwGHEP      PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWNUM       NAMES GFwNum       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWUTL       NAMES GFwUtl       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWPARDAT    NAMES GFwParDat    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWEG        NAMES GFwEG        PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GFWNTP       NAMES GFwNtp       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHXSIG      NAMES GPhXSIg      PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHPDF       NAMES GPhPDF       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHNUCLST    NAMES GPhNuclSt    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHCMN       NAMES GPhCmn       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHDCY       NAMES GPhDcy       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHHADTRANSP NAMES GPhHadTransp PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHHADNZ     NAMES GPhHadnz     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHDEEX      NAMES GPhDeEx      PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHAMNGXS    NAMES GPhAMNGXS    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHAMNGEG    NAMES GPhAMNGEG    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHCHMXS     NAMES GPhChmXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHCOHXS     NAMES GPhCohXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHCOHEG     NAMES GPhCohEG     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHDISXS     NAMES GPhDISXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHDISEG     NAMES GPhDISEG     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHDFRCXS    NAMES GPhDfrcXS    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHDFRCEG    NAMES GPhDfrcEG    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHGLWRESXS  NAMES GPhGlwResXS  PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHGLWRESEG  NAMES GPhGlwResEG  PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHIBDXS     NAMES GPhIBDXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHIBDEG     NAMES GPhIBDEG     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHMNUCXS    NAMES GPhMNucXS    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHMNUCEG    NAMES GPhMNucEG    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHMEL       NAMES GPhMEL       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHNUELXS    NAMES GPhNuElXS    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHNUELEG    NAMES GPhNuElEG    PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHQELXS     NAMES GPhQELXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHQELEG     NAMES GPhQELEG     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHRESXS     NAMES GPhResXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHRESEG     NAMES GPhResEG     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHSTRXS     NAMES GPhStrXS     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHSTREG     NAMES GPhStrEG     PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GPHNDCY      NAMES GPhNDcy      PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GTLGEO       NAMES GTlGeo       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GTLFLX       NAMES GTlFlx       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )

  # if GENIE's Reweight gets spun into separate product
  # then the following must change for GENIE_LIB to ? GENIERW_LIB
  cet_find_library( GRWFWK       NAMES GRwFwk       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GRWIO        NAMES GRwIO        PATHS ENV GENIE_LIB NO_DEFAULT_PATH )
  cet_find_library( GRWCLC       NAMES GRwClc       PATHS ENV GENIE_LIB NO_DEFAULT_PATH )

  # GENIE pre-R-3 used GSL via ROOT's Math libs, now makes direct call not available via that route
  cet_find_library( GSL NAMES gsl PATHS ENV GSL_LIB NO_DEFAULT_PATH )

# this is the order that GENIE "recommends"
# genie-config --libs
#   -lGFwMsg -lGFwReg -lGFwAlg -lGFwInt -lGFwGHEP -lGFwNum -lGFwUtl -lGFwParDat
#   -lGFwEG -lGFwNtp -lGPhXSIg -lGPhPDF -lGPhNuclSt -lGPhCmn -lGPhDcy -lGPhHadTransp
#   -lGPhHadnz -lGPhDeEx -lGPhAMNGXS -lGPhAMNGEG -lGPhChmXS -lGPhCohXS -lGPhCohEG
#   -lGPhDISXS -lGPhDISEG -lGPhDfrcXS -lGPhDfrcEG -lGPhGlwResXS -lGPhGlwResEG
#   -lGPhIBDXS -lGPhIBDEG -lGPhMNucXS -lGPhMNucEG -lGPhMEL -lGPhNuElXS -lGPhNuElEG
#   -lGPhQELXS -lGPhQELEG -lGPhResXS -lGPhResEG -lGPhStrXS
#   -lGPhStrEG -lGPhNDcy -lGTlGeo -lGTlFlx -lGTlRew

  set (GENIE_LIB_LIST ${GFWMSG}
                      ${GFWREG}
                      ${GFWALG}
                      ${GFWINT}
                      ${GFWGHEP}
                      ${GFWNUM}
                      ${GSL}  # FWNUM relies on GSL
                      ${GFWUTL}
                      ${GFWPARDAT}
                      ${GFWEG}
                      ${GFWNTP}
                      ${GPHXSIG}
                      ${GPHPDF}
                      ${GPHNUCLST}
                      ${GPHCMN}
                      ${GPHDCY}
                      ${GPHHADTRANSP}
                      ${GPHHADNZ}
                      ${GPHDEEX}
                      ${GPHAMNGXS}
                      ${GPHAMNGEG}
                      ${GPHCHMXS}
                      ${GPHCOHXS}
                      ${GPHCOHEG}
                      ${GPHDISXS}
                      ${GPHDISEG}
                      ${GPHDFRCXS}
                      ${GPHDFRCEG}
                      ${GPHGLWRESXS}
                      ${GPHGLWRESEG}
                      ${GPHIBDXS}
                      ${GPHIBDEG}
                      ${GPHMNUCXS}
                      ${GPHMNUCEG}
                      ${GPHMEL}
                      ${GPHNUELXS}
                      ${GPHNUELEG}
                      ${GPHQELXS}
                      ${GPHQELEG}
                      ${GPHRESXS}
                      ${GPHRESEG}
                      ${GPHSTRXS}
                      ${GPHSTREG}
                      ${GPHNDCY}
                      ${GTLGEO}
                      ${GTLFLX}
                      ${GRWFWK}
                      ${GRWIO}
                      ${GRWCLC}
                      )

endif()
#MESSAGE("--NUTOOLS-- GENIE ${VMAJ} messenger ${GMESSENGER} gfwmsg ${GFWMSG} GENIE_LIB_LIST=${GENIE_LIB_LIST}")

# dk2nu
cet_find_library( DK2NU_TREE  NAMES dk2nuTree  PATHS ENV DK2NUDATA_LIB NO_DEFAULT_PATH )
cet_find_library( DK2NU_GENIE NAMES dk2nuGenie PATHS ENV DK2NUGENIE_LIB NO_DEFAULT_PATH )
include_directories( $ENV{DK2NUDATA_INC} $ENV{DK2NUGENIE_INC} )

# ifdhc
cet_find_library( IFDH NAMES ifdh PATHS ENV IFDHC_LIB NO_DEFAULT_PATH )
include_directories( $ENV{IFDHC_FQ_DIR}/inc )

# ifdhc_art  # as a service
cet_find_library( IFDH_SERVICE NAMES IFDH_service PATHS ENV IFDH_ART_LIB NO_DEFAULT_PATH )
include_directories( $ENV{IFDH_ART_FQ_DIR}/inc )

# macros
include(ArtMake)
include(ArtDictionary)
include(BuildPlugins)
include(CetRootCint)

# include search path
include_directories ( $ENV{GENIE_INC}/GENIE )
include_directories ( $ENV{GENIE_REWEIGHT}/src )
include_directories ( $ENV{LIBXML2_INC} )
include_directories ( $ENV{G4INCLUDE} )
include_directories ( $ENV{XERCES_C_INC} )
include_directories ( $ENV{CRYHOME}/src )
include_directories ( $ENV{LOG4CPP_INC} )

# included for Mac OSX using XQuartz for X11
if(APPLE)
  include_directories ( /opt/X11/include )
endif()

# source
add_subdirectory (nutools)
add_subdirectory (tools)
add_subdirectory (test)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)

