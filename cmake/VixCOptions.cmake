#
#  VixC CMake Options
#
#  Copyright (c) 2026 Gaspard Kirira.
#  https://github.com/vixcpp/vixc
#
#  Licensed under the MIT License.
#  See LICENSE in the project root for license information.
#

include_guard(GLOBAL)

#
# Build configuration
#

option(
  VIXC_BUILD_TOOLS
  "Build VixC command-line tools."
  ${PROJECT_IS_TOP_LEVEL}
)

option(
  VIXC_BUILD_TESTS
  "Build VixC tests."
  ${PROJECT_IS_TOP_LEVEL}
)

option(
  VIXC_INSTALL
  "Generate installation and package configuration rules."
  ${PROJECT_IS_TOP_LEVEL}
)

#
# Compiler diagnostics
#

option(
  VIXC_ENABLE_WARNINGS
  "Enable additional compiler warnings for VixC targets."
  ON
)

option(
  VIXC_WARNINGS_AS_ERRORS
  "Treat VixC compiler warnings as errors."
  OFF
)

#
# Development instrumentation
#

option(
  VIXC_ENABLE_ADDRESS_SANITIZER
  "Enable AddressSanitizer for supported VixC development builds."
  OFF
)

option(
  VIXC_ENABLE_UNDEFINED_SANITIZER
  "Enable UndefinedBehaviorSanitizer for supported VixC development builds."
  OFF
)

#
# Internal validation
#

if(
  VIXC_ENABLE_ADDRESS_SANITIZER
  OR VIXC_ENABLE_UNDEFINED_SANITIZER
)
  if(MSVC)
    if(VIXC_ENABLE_UNDEFINED_SANITIZER)
      message(
        WARNING
        "VIXC_ENABLE_UNDEFINED_SANITIZER is not supported by the configured MSVC toolchain."
      )
    endif()
  endif()
endif()

#
# Helper for project-owned targets
#
# This function applies development-oriented compiler settings to a VixC
# target without changing the compilation policy of applications embedding the
# installed library.
#

function(vixc_configure_target target)
  if(NOT TARGET ${target})
    message(
      FATAL_ERROR
      "vixc_configure_target(): '${target}' is not a known CMake target."
    )
  endif()

  if(VIXC_ENABLE_WARNINGS)
    if(MSVC)
      target_compile_options(
        ${target}
        PRIVATE
          /W4
      )
    elseif(
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
      OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    )
      target_compile_options(
        ${target}
        PRIVATE
          -Wall
          -Wextra
          -Wpedantic
      )
    endif()
  endif()

  if(VIXC_WARNINGS_AS_ERRORS)
    if(MSVC)
      target_compile_options(
        ${target}
        PRIVATE
          /WX
      )
    elseif(
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
      OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    )
      target_compile_options(
        ${target}
        PRIVATE
          -Werror
      )
    endif()
  endif()

  if(VIXC_ENABLE_ADDRESS_SANITIZER)
    if(
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
      OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    )
      target_compile_options(
        ${target}
        PRIVATE
          -fsanitize=address
          -fno-omit-frame-pointer
      )

      target_link_options(
        ${target}
        PRIVATE
          -fsanitize=address
      )
    elseif(MSVC)
      target_compile_options(
        ${target}
        PRIVATE
          /fsanitize=address
      )
    endif()
  endif()

  if(VIXC_ENABLE_UNDEFINED_SANITIZER)
    if(
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
      OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    )
      target_compile_options(
        ${target}
        PRIVATE
          -fsanitize=undefined
          -fno-omit-frame-pointer
      )

      target_link_options(
        ${target}
        PRIVATE
          -fsanitize=undefined
      )
    endif()
  endif()
endfunction()
