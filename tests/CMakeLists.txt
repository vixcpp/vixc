#  @file CMakeLists.txt
#  @author Gaspard Kirira
#
#  Copyright 2026, Gaspard Kirira.
#  All rights reserved.
#  https://github.com/vixcpp/vixpp
#
#  Use of this source code is governed by a MIT license
#  that can be found in the License file.
#
# ====================================================================
# Vix++ - Tests
# ====================================================================

add_executable(vixpp_import_resolver_test
  ImportResolverTest.cpp
)

target_link_libraries(vixpp_import_resolver_test PRIVATE
  vixpp::core
)

add_test(
  NAME vixpp_import_resolver_test
  COMMAND vixpp_import_resolver_test
)

add_executable(vixpp_transpiler_test
  TranspilerTest.cpp
)

target_link_libraries(vixpp_transpiler_test PRIVATE
  vixpp::core
)

add_test(
  NAME vixpp_transpiler_test
  COMMAND vixpp_transpiler_test
)
