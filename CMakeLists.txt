#
#  VixC
#
#  Copyright (c) 2026 Gaspard Kirira.
#  https://github.com/vixcpp/vixc
#
#  Licensed under the MIT License.
#  See LICENSE in the project root for license information.
#

cmake_minimum_required(VERSION 3.20)

project(
  vixc
  VERSION 0.1.0
  DESCRIPTION "A programming-language frontend for native software."
  HOMEPAGE_URL "https://github.com/vixcpp/vixc"
  LANGUAGES CXX
)

#
# Project options
#

include(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VixCOptions.cmake"
)

#
# Library
#

add_library(vixc)

add_library(
  vixc::vixc
  ALIAS
  vixc
)

target_sources(
  vixc
  PRIVATE
    src/Frontend.cpp

    src/source/SourceFile.cpp
    src/source/SourceManager.cpp

    src/diagnostics/DiagnosticEngine.cpp

    src/syntax/Lexer.cpp
    src/syntax/Parser.cpp

    src/semantic/SemanticContext.cpp
    src/semantic/SemanticAnalyzer.cpp
    src/semantic/failure/OutcomeModel.cpp
    src/semantic/failure/FailureAnalyzer.cpp

    src/ir/Program.cpp
    src/ir/failure/Outcome.cpp
    src/ir/failure/Failure.cpp

    src/lowering/LoweringContext.cpp
    src/lowering/LoweringPipeline.cpp
    src/lowering/failure/FailureLowering.cpp

    src/backends/cxx/CxxSourceMap.cpp
    src/backends/cxx/CxxEmitter.cpp
    src/backends/cxx/CxxBackend.cpp
)

target_include_directories(
  vixc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(
  vixc
  PUBLIC
    cxx_std_20
)

set_target_properties(
  vixc
  PROPERTIES
    CXX_EXTENSIONS OFF
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

vixc_configure_target(vixc)

#
# Command-line frontend
#

if(VIXC_BUILD_TOOLS)
  add_executable(
    vixc_cli
    tools/vixc/main.cpp
  )

  target_link_libraries(
    vixc_cli
    PRIVATE
      vixc::vixc
  )

  target_compile_features(
    vixc_cli
    PRIVATE
      cxx_std_20
  )

  set_target_properties(
    vixc_cli
    PROPERTIES
      OUTPUT_NAME vixc
      CXX_EXTENSIONS OFF
  )

  vixc_configure_target(vixc_cli)
endif()

#
# Tests
#

if(VIXC_BUILD_TESTS)
  enable_testing()

  add_subdirectory(tests)
endif()

#
# Installation
#

if(VIXC_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(
    TARGETS vixc
    EXPORT vixcTargets
    ARCHIVE
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  if(VIXC_BUILD_TOOLS)
    install(
      TARGETS vixc_cli
      RUNTIME
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  endif()

  install(
    DIRECTORY
      "${CMAKE_CURRENT_SOURCE_DIR}/include/vixc"
    DESTINATION
      "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING
      PATTERN "*.hpp"
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vixcConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vixcConfig.cmake"
    INSTALL_DESTINATION
      "${CMAKE_INSTALL_LIBDIR}/cmake/vixc"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/vixcConfigVersion.cmake"
    VERSION
      "${PROJECT_VERSION}"
    COMPATIBILITY
      SameMajorVersion
  )

  install(
    EXPORT vixcTargets
    FILE
      vixcTargets.cmake
    NAMESPACE
      vixc::
    DESTINATION
      "${CMAKE_INSTALL_LIBDIR}/cmake/vixc"
  )

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/vixcConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/vixcConfigVersion.cmake"
    DESTINATION
      "${CMAKE_INSTALL_LIBDIR}/cmake/vixc"
  )
endif()
