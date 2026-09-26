#
#  VixC Tests
#
#  Copyright (c) 2026 Gaspard Kirira.
#  https://github.com/vixcpp/vixc
#
#  Licensed under the MIT License.
#  See LICENSE in the project root for license information.
#

if(NOT TARGET vixc)
  message(FATAL_ERROR
    "VixC tests require the 'vixc' library target.")
endif()

include(CTest)

function(vixc_add_test target source)
  add_executable(${target}
    ${source}
  )

  target_link_libraries(${target}
    PRIVATE
      vixc
  )

  target_compile_features(${target}
    PRIVATE
      cxx_std_20
  )

  set_target_properties(${target}
    PROPERTIES
      CXX_EXTENSIONS OFF
  )

  vixc_configure_target(${target})

  add_test(
    NAME ${target}
    COMMAND ${target}
  )
endfunction()

#
# Source
#

vixc_add_test(
  vixc_source_manager_test
  source/SourceManagerTest.cpp
)

#
# Diagnostics
#

vixc_add_test(
  vixc_diagnostic_engine_test
  diagnostics/DiagnosticEngineTest.cpp
)

vixc_add_test(
  vixc_diagnostic_renderer_test
  diagnostics/DiagnosticRendererTest.cpp
)

#
# Syntax
#

vixc_add_test(
  vixc_lexer_test
  syntax/LexerTest.cpp
)

vixc_add_test(
  vixc_parser_test
  syntax/ParserTest.cpp
)

#
# Semantic analysis
#

vixc_add_test(
  vixc_failure_semantic_test
  semantic/FailureSemanticTest.cpp
)

#
# Intermediate representation
#

vixc_add_test(
  vixc_failure_ir_test
  ir/FailureIrTest.cpp
)

#
# Lowering
#

vixc_add_test(
  vixc_failure_lowering_test
  lowering/FailureLoweringTest.cpp
)

#
# Backends
#

vixc_add_test(
  vixc_cxx_backend_test
  backends/CxxBackendTest.cpp
)

#
# Integration
#

vixc_add_test(
  vixc_failure_pipeline_test
  integration/FailurePipelineTest.cpp
)

vixc_add_test(
  vixc_embedding_test
  integration/EmbeddingTest.cpp
)
