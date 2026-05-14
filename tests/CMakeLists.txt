#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira.
#  All rights reserved.
#  https://github.com/vixcpp/vixc
#
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix++ - Tests
# ====================================================================

add_executable(vixc_import_resolver_test
  ImportResolverTest.cpp
)

target_link_libraries(vixc_import_resolver_test PRIVATE
  vixc::core
)

add_test(
  NAME vix_import_resolver_test
  COMMAND vixc_import_resolver_test
)

add_executable(vixc_transpiler_test
  TranspilerTest.cpp
)

target_link_libraries(vixc_transpiler_test PRIVATE
  vixc::core
)

add_test(
  NAME vixc_transpiler_test
  COMMAND vixc_transpiler_test
)
