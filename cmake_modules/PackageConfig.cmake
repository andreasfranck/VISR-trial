# Package generation configuration

SET( CPACK_DEBIAN_PACKAGE_MAINTAINER "Andreas Franck A.Franck@soton.ac.uk" )
SET( CPACK_PACKAGE_NAME "VISR" )
SET( CPACK_PACKAGE_VENDOR "ISVR")


# Use the version numbers centrally defined for the VISR project
set( CPACK_PACKAGE_VERSION_MAJOR ${VISR_MAJOR_VERSION} )
set( CPACK_PACKAGE_VERSION_MINOR ${VISR_MINOR_VERSION} )
set( CPACK_PACKAGE_VERSION_PATCH ${VISR_PATCH_VERSION} )


set( PKG_FILE_NAME ${VISR_TOPLEVEL_NAME}-${CMAKE_SYSTEM_NAME})
# Encode the Python major/minor version in the package file name if Python is enabled.
if( BUILD_PYTHON_BINDINGS )
  string( REGEX MATCH "[0-9]+.[0-9]+" PYTHON_MAJOR_MINOR ${PYTHONLIBS_VERSION_STRING} )
  set( PKG_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-python${PYTHON_MAJOR_MINOR}-${CMAKE_SYSTEM_NAME}" )
endif( BUILD_PYTHON_BINDINGS )

set( CPACK_PACKAGE_FILE_NAME ${PKG_FILE_NAME} )
#set( CPACK_SOURCE_PACKAGE_FILE_NAME ${CMAKE_INSTALL_PREFIX})
#set( CPACK_PACKAGE_INSTALL_DIRECTORY ${CMAKE_INSTALL_PREFIX})
SET( CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)



IF( WIN32 )
SET( CPACK_GENERATOR NSIS ZIP )
SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
GET_FILENAME_COMPONENT( PORTAUDIO_LIBRARY_DIR ${PORTAUDIO_LIBRARY} DIRECTORY )
INSTALL( FILES ${PORTAUDIO_LIBRARY_DIR}/portaudio_x64.dll DESTINATION 3rd)

GET_FILENAME_COMPONENT( SNDFILE_LIBRARY_DIR ${SNDFILE_LIBRARY} DIRECTORY )
INSTALL( FILES ${SNDFILE_LIBRARY_DIR}/libsndfile-1.dll DESTINATION 3rd)

# Boost
  GET_FILENAME_COMPONENT( BOOST_CHRONO_LIBRARY_BASENAME ${Boost_CHRONO_LIBRARY_RELEASE} NAME_WE )
  GET_FILENAME_COMPONENT( BOOST_DATE_TIME_LIBRARY_BASENAME ${Boost_DATE_TIME_LIBRARY_RELEASE} NAME_WE )
  GET_FILENAME_COMPONENT( BOOST_FILESYSTEM_LIBRARY_BASENAME ${Boost_FILESYSTEM_LIBRARY_RELEASE} NAME_WE )
  GET_FILENAME_COMPONENT( BOOST_PROGRAM_OPTIONS_LIBRARY_BASENAME ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE} NAME_WE )
  GET_FILENAME_COMPONENT( BOOST_SYSTEM_LIBRARY_BASENAME ${Boost_SYSTEM_LIBRARY_RELEASE} NAME_WE )
  GET_FILENAME_COMPONENT( BOOST_THREAD_LIBRARY_BASENAME ${Boost_THREAD_LIBRARY_RELEASE} NAME_WE )
  GET_FILENAME_COMPONENT( BOOST_REGEX_LIBRARY_BASENAME ${Boost_REGEX_LIBRARY_RELEASE} NAME_WE )
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_CHRONO_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_DATE_TIME_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_FILESYSTEM_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_PROGRAM_OPTIONS_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_SYSTEM_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_THREAD_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_CHRONO_LIBRARY_BASENAME}.dll DESTINATION 3rd)
  INSTALL( FILES ${Boost_LIBRARY_DIR_RELEASE}/${BOOST_REGEX_LIBRARY_BASENAME}.dll DESTINATION 3rd)
ENDIF( WIN32 )

IF( VISR_SYSTEM_NAME MATCHES "Linux" )
  SET( CPACK_GENERATOR DEB TBZ2 )
  SET( CPACK_DEBIAN_HOMEPAGE "http://www.s3a-spatialaudio.org" )
  SET( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
ENDIF(VISR_SYSTEM_NAME MATCHES "Linux")

IF( VISR_SYSTEM_NAME MATCHES "MacOS" )
  #SET( CPACK_GENERATOR PackageMaker )
  SET( CPACK_GENERATOR DragNDrop ZIP TBZ2 )
  #SET( CPACK_GENERATOR Bundle )
  #SET( CPACK_PACKAGE_INSTALL_DIRECTORY "/Applications" )
  set( CPACK_PACKAGING_INSTALL_PREFIX "/${VISR_TOPLEVEL_NAME}")

  SET( CPACK_DMG_BACKGROUND_IMAGE ${CMAKE_SOURCE_DIR}/cmake_modules/resources/s3a_logo.jpg )
#  SET( CPACK_BUNDLE_NAME "VISR-0.9.0-Darwin/VISR" )
  SET( CPACK_BUNDLE_NAME "VISR" )
  SET( CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/cmake_modules/resources/s3a_logo.png )
  SET( CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/cmake_modules/Info.plist )
  SET( CPACK_BUNDLE_STARTUP_COMMAND ${CMAKE_SOURCE_DIR}/cmake_modules/postscript.sh )  
  INSTALL( FILES ${PORTAUDIO_LIBRARIES} DESTINATION 3rd)
  INSTALL( FILES ${SNDFILE_LIBRARY} DESTINATION 3rd)  
    INSTALL( FILES ${FLAC_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${OGG_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${VORBIS_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${VORBISENC_LIBRARY} DESTINATION 3rd )


   #SET(CPACK_PACKAGE_FILE_NAME VISR)

   # CPACK_PACKAGE_FILE_NAME - provides the name of the final compressed disk image (the name of the file that is distributed).
   # CPACK_PACKAGE_ICON - provides the icon for the mounted disk image (appears after the user mounts the disk image).
   

  If( NOT Boost_USE_STATIC_LIBS )
    INSTALL( FILES ${Boost_FILESYSTEM_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${Boost_SYSTEM_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${Boost_THREAD_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${Boost_PROGRAM_OPTIONS_LIBRARY} DESTINATION 3rd )
    INSTALL( FILES ${CMAKE_THREAD_LIBS_INIT} DESTINATION 3rd )
    INSTALL( FILES ${Boost_REGEX_LIBRARY} DESTINATION 3rd )
  ENDIF( NOT Boost_USE_STATIC_LIBS )

ENDIF( VISR_SYSTEM_NAME MATCHES "MacOS" )

INSTALL( DIRECTORY config DESTINATION ${VISR_TOPLEVEL_INSTALL_DIRECTORY} )
INSTALL( FILES ${CMAKE_SOURCE_DIR}/licence.txt DESTINATION ${VISR_TOPLEVEL_INSTALL_DIRECTORY} )
SET( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/licence.txt )

# Install Python example scripts and templates
#INSTALL( DIRECTORY src/python/scripts DESTINATION python )
INSTALL( DIRECTORY src/python/templates DESTINATION ${PYTHON_MODULE_INSTALL_DIRECTORY} )
set( CPACK_DMG_VOLUME_NAME ${PKG_FILE_NAME})
#set(CPACK_POSTUPGRADE_SCRIPT "${CMAKE_SOURCE_DIR}/cmake_modules/pkg_rename.sh" ${PROJECT_BINARY_DIR}/${VISR_TOPLEVEL_NAME}.dmg ${PROJECT_BINARY_DIR}/PKG_FILE_NAME.dmg )
#set(CPACK_POSTUPGRADE_SCRIPT ${CMAKE_SOURCE_DIR}/cmake_modules/pkg_rename.sh)

INCLUDE( CPack )
