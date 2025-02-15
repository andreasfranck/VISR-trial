# Increase CMake version requirement to silence Cmake deprecation warnings.
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(ffts C ASM)

# TODO: to support AutoConfigure building, this should came from "template" file
set(FFTS_MAJOR 0)
set(FFTS_MINOR 9)
set(FFTS_MICRO 0)

set(FFTS_VERSION "${FFTS_MAJOR}.${FFTS_MINOR}.${FFTS_MICRO}")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# default build type is Debug which means no optimization
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

# installation parameters
# Note (Andreas Franck): Replaced by own install logic in order to create installation packages.
# set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/ffts)
# set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)

# common options

# !!!! FOR TESTING ONLY !!!!
option(ENABLE_AVX
  "Enables AVX instructions." OFF
)
# !!!! FOR TESTING ONLY !!!!
option(ENABLE_DOUBLE
  "Enables double precision" OFF
)

option(ENABLE_NEON
  "Enables the use of NEON instructions." OFF
)

option(ENABLE_VFP
  "Enables the use of VFP instructions." OFF
)

option(DISABLE_DYNAMIC_CODE
  "Disables the use of dynamic machine code generation." OFF
)

option(GENERATE_POSITION_INDEPENDENT_CODE
  "Generate position independent code" OFF
)

option(ENABLE_SHARED
  "Enable building a shared library." OFF
)

option(ENABLE_STATIC
  "Enable building a static library." ON
)

include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)

# Ensure defined when building FFTS (as opposed to using it from
# another project). Used to export functions from Windows DLL.
add_definitions(-DFFTS_BUILD)

# check existence of various headers
check_include_file(inttypes.h  HAVE_INTTYPES_H)
check_include_file(malloc.h    HAVE_MALLOC_H)
check_include_file(mm_malloc.h HAVE_MM_MALLOC_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stdlib.h    HAVE_STDLIB_H)
check_include_file(string.h    HAVE_STRING_H)
check_include_file(sys/mman.h  HAVE_SYS_MMAN_H)
check_include_file(unistd.h    HAVE_UNISTD_H)

if(HAVE_INTTYPES_H)
  add_definitions(-DHAVE_INTTYPES_H)
endif(HAVE_INTTYPES_H)

if(HAVE_MALLOC_H)
  add_definitions(-DHAVE_MALLOC_H)
endif(HAVE_MALLOC_H)

if(HAVE_MM_MALLOC_H)
  add_definitions(-DHAVE_MM_MALLOC_H)
endif(HAVE_MM_MALLOC_H)

if(HAVE_STDINT_H)
  add_definitions(-DHAVE_STDINT_H)
endif(HAVE_STDINT_H)

if(HAVE_STDLIB_H)
  add_definitions(-DHAVE_STDLIB_H)
endif(HAVE_STDLIB_H)

if(HAVE_STRING_H)
  add_definitions(-DHAVE_STRING_H)
endif(HAVE_STRING_H)

if(HAVE_SYS_MMAN_H)
  add_definitions(-DHAVE_SYS_MMAN_H)
endif(HAVE_SYS_MMAN_H)

if(HAVE_UNISTD_H)
  add_definitions(-DHAVE_UNISTD_H)
endif(HAVE_UNISTD_H)

# check existence of various declarations
check_symbol_exists(memalign       malloc.h HAVE_DECL_MEMALIGN)
check_symbol_exists(posix_memalign stdlib.h HAVE_DECL_POSIX_MEMALIGN)
check_symbol_exists(valloc         stdlib.h HAVE_DECL_VALLOC)
check_symbol_exists(_mm_malloc     malloc.h HAVE_DECL__MM_MALLOC)

if(HAVE_DECL_MEMALIGN)
  add_definitions(-DHAVE_DECL_MEMALIGN)
endif(HAVE_DECL_MEMALIGN)

if(HAVE_DECL_POSIX_MEMALIGN)
  add_definitions(-DHAVE_DECL_POSIX_MEMALIGN)
endif(HAVE_DECL_POSIX_MEMALIGN)

if(HAVE_DECL_VALLOC)
  add_definitions(-DHAVE_DECL_VALLOC)
endif(HAVE_DECL_VALLOC)

if(HAVE_DECL__MM_MALLOC)
  add_definitions(-DHAVE_DECL__MM_MALLOC)
endif(HAVE_DECL__MM_MALLOC)

# check existence of various functions
check_function_exists(memalign       HAVE_MEMALIGN)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(valloc         HAVE_VALLOC)
check_function_exists(_mm_malloc     HAVE__MM_MALLOC)

if(HAVE_MEMALIGN)
  add_definitions(-DHAVE_MEMALIGN)
endif(HAVE_MEMALIGN)

if(HAVE_POSIX_MEMALIGN)
  add_definitions(-DHAVE_POSIX_MEMALIGN)
endif(HAVE_POSIX_MEMALIGN)

if(HAVE_VALLOC)
  add_definitions(-DHAVE_VALLOC)
endif(HAVE_VALLOC)

if(HAVE__MM_MALLOC)
  add_definitions(-DHAVE__MM_MALLOC)
endif(HAVE__MM_MALLOC)

# backup flags
set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})

# Determinate if we are cross-compiling
if(NOT CMAKE_CROSSCOMPILING)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|ARM64|aarch64|armv8")
    # currently NEON is only supported for 32-bit ARM
    message("ffts: NEON not supported on arm64")
    set(ENABLE_NEON OFF)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(TEST_SOURCE_CODE "
      #include <arm_neon.h>
      int main()
      {
       float32x4_t v;
       float zeros[4] = {0.0f, 0.0f, 0.0f, 0.0f};
       v = vld1q_f32(zeros);
       return 0;
      }"
    )
    check_c_source_runs("${TEST_SOURCE_CODE}" NEON_SUPPORTED)

    if(NEON_SUPPORTED)
      set(ENABLE_NEON ON)
      message("ffts: NEON enabled")
    else()
      set(ENABLE_NEON OFF)
      message("ffts: NEON disabled")
    endif()
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|X86|i686")
    # enable SSE code generation
    if(CMAKE_COMPILER_IS_GNUCC)
      set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVE} -msse")
    endif(CMAKE_COMPILER_IS_GNUCC)

    # check if the platform has support for SSE intrinsics
    check_include_file(xmmintrin.h HAVE_XMMINTRIN_H)
    if(HAVE_XMMINTRIN_H)
      add_definitions(-DHAVE_SSE)
      set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})

      # TODO: not the right place
      if(ENABLE_AVX)
        add_definitions(-DHAVE_AVX)
      endif(ENABLE_AVX)
      if(ENABLE_DOUBLE)
        add_definitions(-DFFTS_DOUBLE)
      endif(ENABLE_DOUBLE)
    endif(HAVE_XMMINTRIN_H)

    # enable SSE2 code generation
    if(CMAKE_COMPILER_IS_GNUCC)
      set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVE} -msse2")
    endif(CMAKE_COMPILER_IS_GNUCC)

    # check if the platform has support for SSE2 intrinsics
    check_include_file(emmintrin.h HAVE_EMMINTRIN_H)
    if(HAVE_EMMINTRIN_H)
      add_definitions(-DHAVE_SSE2)
      set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
    endif(HAVE_EMMINTRIN_H)

    # enable SSE3 code generation
    if(CMAKE_COMPILER_IS_GNUCC)
      set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVE} -msse3")
    endif(CMAKE_COMPILER_IS_GNUCC)

    # check if the platform has support for SSE3 intrinsics
    check_include_file(pmmintrin.h HAVE_PMMINTRIN_H)
    if(HAVE_PMMINTRIN_H)
      add_definitions(-DHAVE_PMMINTRIN_H)
      add_definitions(-DHAVE_SSE3)
      set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
    else()
      # check if the platform has specific intrinsics
      check_include_file(intrin.h HAVE_INTRIN_H)
      if(HAVE_INTRIN_H)
        add_definitions(-DHAVE_INTRIN_H)

        check_c_source_compiles("
          #include<intrin.h>
          int main(int argc, char** argv)
          {
           (void) argv;
           (void) argc;
           return _mm_movemask_ps(_mm_moveldup_ps(_mm_set_ss(1.0f)));
          }" HAVE__MM_MOVELDUP_PS
        )

        if(HAVE__MM_MOVELDUP_PS)
          # assume that we have all SSE3 intrinsics
          add_definitions(-DHAVE_SSE3)
        endif(HAVE__MM_MOVELDUP_PS)
      endif(HAVE_INTRIN_H)
    endif(HAVE_PMMINTRIN_H)
  endif()
else()
  # TODO: Add detections for compiler support and headers
endif(NOT CMAKE_CROSSCOMPILING)

# restore flags
set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})

# compiler settings
if(MSVC)
  # enable all warnings but also disable some..
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /wd4127")

  # mark debug versions
  set(CMAKE_DEBUG_POSTFIX "d")

  add_definitions(-D_USE_MATH_DEFINES)
elseif((CMAKE_C_COMPILER_ID STREQUAL "GNU") OR (CMAKE_C_COMPILER_ID STREQUAL "Clang") )
  include(CheckCCompilerFlag)
  include(CheckLibraryExists)

  # enable all warnings
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

  # check if we can control visibility of symbols
  check_c_compiler_flag(-fvisibility=hidden HAVE_GCC_VISIBILITY)
  if(HAVE_GCC_VISIBILITY)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    add_definitions(-DHAVE_GCC_VISIBILITY)
  endif(HAVE_GCC_VISIBILITY)

  # some systems need libm for the math functions to work
  check_library_exists(m pow "" HAVE_LIBM)
  if(HAVE_LIBM)
    list(APPEND CMAKE_REQUIRED_LIBRARIES m)
    list(APPEND FFTS_EXTRA_LIBRARIES m)
  endif(HAVE_LIBM)

  if(HAVE_PMMINTRIN_H)
    add_definitions(-msse3)
  elseif(HAVE_EMMINTRIN_H)
    add_definitions(-msse2)
  elseif(HAVE_XMMINTRIN_H)
    add_definitions(-msse)
  endif(HAVE_PMMINTRIN_H)
endif(MSVC)

include_directories(include)
include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(FFTS_HEADERS
  include/ffts.h
)

set(FFTS_SOURCES
  src/ffts_attributes.h
  src/ffts.c
  src/ffts_chirp_z.c
  src/ffts_chirp_z.h
  src/ffts_cpu.c
  src/ffts_cpu.h
  src/ffts_internal.h
  src/ffts_nd.c
  src/ffts_nd.h
  src/ffts_real.h
  src/ffts_real.c
  src/ffts_real_nd.c
  src/ffts_real_nd.h
  src/ffts_transpose.c
  src/ffts_transpose.h
  src/ffts_trig.c
  src/ffts_trig.h
  src/ffts_static.c
  src/ffts_static.h
  src/macros.h
  src/patterns.h
  src/types.h
)

if(NOT DISABLE_DYNAMIC_CODE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    list(APPEND FFTS_SOURCES
      src/codegen_sse.h
    )
  else()
    message(WARNING "Dynamic code is only supported with x64, disabling dynamic code.")
    set(DISABLE_DYNAMIC_CODE ON)
  endif(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
endif(NOT DISABLE_DYNAMIC_CODE)

if(ENABLE_NEON)
  list(APPEND FFTS_SOURCES
    src/neon.s
  )

  if(DISABLE_DYNAMIC_CODE)
    list(APPEND FFTS_SOURCES
      src/neon_static.s
    )
  endif(DISABLE_DYNAMIC_CODE)

  add_definitions(-DHAVE_NEON)
elseif(ENABLE_VFP)
  if(NOT DISABLE_DYNAMIC_CODE)
    list(APPEND FFTS_SOURCES
      src/vfp.s
    )
  endif(NOT DISABLE_DYNAMIC_CODE)

  add_definitions(-DHAVE_VFP)
elseif(HAVE_XMMINTRIN_H)
  add_definitions(-DHAVE_SSE)

  list(APPEND FFTS_SOURCES
    src/macros-avx.h
    src/macros-sse.h
  )
endif(ENABLE_NEON)

if(DISABLE_DYNAMIC_CODE)
  add_definitions(-DDYNAMIC_DISABLED)
else()
  list(APPEND FFTS_SOURCES
    src/codegen.c
    src/codegen.h
  )
endif(DISABLE_DYNAMIC_CODE)

if(GENERATE_POSITION_INDEPENDENT_CODE)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif(GENERATE_POSITION_INDEPENDENT_CODE)

if(ENABLE_SHARED)
  add_library(ffts_shared SHARED
    ${FFTS_HEADERS}
    ${FFTS_SOURCES}
  )

  # On unix-like platforms the library is called "libffts.so" and on Windows "ffts.dll"
  set_target_properties(ffts_shared PROPERTIES
    DEFINE_SYMBOL FFTS_SHARED
    OUTPUT_NAME ffts
    VERSION ${FFTS_MAJOR}.${FFTS_MINOR}.${FFTS_MICRO}
  )

  # Note (Andreas Franck) superseded by install( EXPORT ... ) logic below.
  # install( TARGETS ffts_shared DESTINATION ${LIB_INSTALL_DIR} )
endif(ENABLE_SHARED)

if(ENABLE_STATIC)
  add_library(ffts_static STATIC
    ${FFTS_HEADERS}
    ${FFTS_SOURCES}
  )

  if(UNIX)
    # On unix-like platforms the library is called "libffts.a"
    set_target_properties(ffts_static PROPERTIES OUTPUT_NAME ffts)
  endif(UNIX)

  # Note (Andreas Franck) superseded by install( EXPORT ... ) logic below.
  # install( TARGETS ffts_static DESTINATION ${LIB_INSTALL_DIR} )
endif(ENABLE_STATIC)

if(ENABLE_STATIC OR ENABLE_SHARED)
  find_path(MPFR_INCLUDES
    NAMES mpfr.h
    PATHS ${INCLUDE_INSTALL_DIR}
  )
  find_library(MPFR_LIBRARIES mpfr PATHS ${LIB_INSTALL_DIR})
  find_package(OpenMP)

  if(MPFR_INCLUDES)
    add_definitions(-DHAVE_MPFR_H)
    include_directories(${MPFR_INCLUDES})
  endif(MPFR_INCLUDES)

  add_executable(ffts_trig_test
    tests/trig_test.c
  )

  target_link_libraries(ffts_trig_test ffts)
  if(MPFR_LIBRARIES)
    target_link_libraries(ffts_trig_test ${MPFR_LIBRARIES})
  endif(MPFR_LIBRARIES)

  if(OPENMP_FOUND)
    if(MSVC)
      set_target_properties(ffts_trig_test PROPERTIES
        COMPILE_FLAGS "${OpenMP_C_FLAGS}"
        LINK_FLAGS "${OpenMP_EXE_LINKER_FLAGS}"
      )
    else()
      set_target_properties(ffts_trig_test PROPERTIES
        COMPILE_FLAGS "${OpenMP_C_FLAGS}"
        LINK_FLAGS "${OpenMP_C_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}"
      )
    endif(MSVC)
  endif(OPENMP_FOUND)

  add_executable(ffts_test
    tests/test.c
  )

  # link with static library by default
  if(ENABLE_STATIC)
    add_library(ffts ALIAS ffts_static)
  else()
    add_library(ffts ALIAS ffts_shared)
  endif(ENABLE_STATIC)

  target_link_libraries(ffts_test
    ffts
    ${FFTS_EXTRA_LIBRARIES}
  )

  add_executable(ffts_cpu_test
    src/ffts_cpu.c
    src/ffts_cpu.h
    tests/cpu_test.c
  )

  set_target_properties(ffts_cpu_test PROPERTIES COMPILE_DEFINITIONS FFTS_BUILDING_CPU_TEST)
endif(ENABLE_STATIC OR ENABLE_SHARED)

# generate packageconfig file
if(UNIX)
  include(FindPkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
      # convert lists of link libraries into -lstdc++ -lm etc..
      foreach(LIB ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} ${PLATFORM_LIBS})
          set(PRIVATE_LIBS "${PRIVATE_LIBS} -l${LIB}")
      endforeach()
      # Produce a pkg-config file for linking against the shared lib
      configure_file("ffts.pc.cmake.in" "ffts.pc" @ONLY)
      # Note (Andreas Franck) Absolute locations are incompatible with CPack
      # installer generation.
      # install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/ffts.pc"
      #         DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
  endif(PKG_CONFIG_FOUND)
endif(UNIX)

# Override global visibility preset to avoid warnings.
# This is necessary because the global setting for policy CMP0063 is not heeded
# in add_subdirectory() subprojects.
set_property( TARGET ffts_cpu_test PROPERTY C_VISIBILITY_PRESET)
set_property( TARGET ffts_test PROPERTY C_VISIBILITY_PRESET )
set_property( TARGET ffts_trig_test PROPERTY C_VISIBILITY_PRESET )
set_property( TARGET ffts_cpu_test PROPERTY FOLDER 3rd/test)
set_property( TARGET ffts_test PROPERTY FOLDER 3rd/test)
set_property( TARGET ffts_trig_test PROPERTY FOLDER 3rd/test)

# =============================================================================
# Installing / exporting ffts targets as part of VISR.
# This part of the file is solely for including ffts as a git subtree of the
# VISR repository.
# It is necessary to do this in the ffts CMakeLists.txt file, because before
# CMake 3.13, install() commands can only be used for targets created in the same
# directory.
# DO NOT MERGE THIS PART INTO THE UPSTREAM FFTS REPOSITORY!

set( FFTS_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/ffts.h)
if( ENABLE_STATIC )
  set_property( TARGET ffts_static PROPERTY PUBLIC_HEADER
    "${FFTS_PUBLIC_HEADERS}" )
  # Override global visibility preset to avoid warnings.
  # This is necessary because the global setting for policy CMP0063 is not heeded
  # in add_subdirectory() subprojects.
  set_property( TARGET ffts_static PROPERTY C_VISIBILITY_PRESET )
  set_target_properties( ffts_static PROPERTIES FOLDER 3rd)
  get_filename_component(PARENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    DIRECTORY )
  target_include_directories(ffts_static PUBLIC
      $<BUILD_INTERFACE:${PARENT_SOURCE_DIR}/ffts/include>
      $<INSTALL_INTERFACE:include/visr/ffts>
  )
  # Disable specific warning for 3rd-party projects.
  if( WIN32 )
    target_compile_options( ffts_static PRIVATE /wd4244 /wd4267 )
  else() # Covers gcc and clang, incl. Xcode
    target_compile_options( ffts_static PRIVATE -Wno-unused-function)
  endif( WIN32 )
  set_target_properties( ffts_static PROPERTIES
    ARCHIVE_OUTPUT_NAME ffts
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/3rd
  )
  install(TARGETS ffts_static
          EXPORT VISR_3rd_party_static_exports
          PUBLIC_HEADER DESTINATION include/visr/3rd/ffts COMPONENT development_files
          ARCHIVE DESTINATION ${THIRDPARTY_ARCHIVE_INSTALL_DIRECTORY} COMPONENT development_files
          )
endif( ENABLE_STATIC )
if( ENABLE_SHARED )
  set_property( TARGET ffts_static PROPERTY PUBLIC_HEADER
    "${FFTS_PUBLIC_HEADERS}" )
  set_property( TARGET ffts_shared PROPERTY FOLDER 3rd)
  if( WIN32 )
    # Avoid warnings about potentially lossy conversions at 'pedantic'
    # warning level.
    target_compile_options( ffts_shared PRIVATE /wd4244 /wd4267 )
  else() # Covers gcc and clang, incl. Xcode
    target_compile_options( ffts_shared PRIVATE -Wno-unused-function)
  endif( WIN32 )
  set_target_properties( ffts_shared PROPERTIES
    ARCHIVE_OUTPUT_NAME ffts_shared # Import library on Windows
    LIBRARY_OUTPUT_NAME ffts # Shared library on Unix platforms
    RUNTIME_OUTPUT_NAME ffts # DLL on Windows
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/3rd
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/3rd
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/3rd
  )
  get_filename_component(PARENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    DIRECTORY )
  target_include_directories(ffts_shared PUBLIC
    $<BUILD_INTERFACE:${PARENT_SOURCE_DIR}/3rd/ffts/include>
    $<INSTALL_INTERFACE:include/visr/3rd/ffts>
  )
  install(TARGETS ffts_shared
          EXPORT VISR_3rd_party_shared_exports
          PUBLIC_HEADER DESTINATION include/visr/3rd/ffts COMPONENT development_files
          ARCHIVE DESTINATION ${THIRDPARTY_ARCHIVE_INSTALL_DIRECTORY} COMPONENT development_files
          LIBRARY DESTINATION ${THIRDPARTY_LIBRARY_INSTALL_DIRECTORY} COMPONENT thirdparty_libraries
          RUNTIME DESTINATION ${THIRDPARTY_RUNTIME_INSTALL_DIRECTORY} COMPONENT thirdparty_libraries
          )
endif( ENABLE_SHARED )
