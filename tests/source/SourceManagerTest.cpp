/**
 *
 *  @file SourceManagerTest.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright (c) 2026 Gaspard Kirira.
 *  https://github.com/vixcpp/vixc
 *
 *  Licensed under the MIT License.
 *  See LICENSE in the project root for license information.
 *
 *  VixC
 *
 */

#include "../../src/source/SourceFile.hpp"
#include "../../src/source/SourceManager.hpp"

#include <cassert>
#include <cstddef>
#include <string>

namespace
{

  void test_empty_manager()
  {
    vixc::source::SourceManager manager;

    assert(manager.empty());
    assert(manager.source_count() == 0);
  }

  void test_add_source()
  {
    vixc::source::SourceManager manager;

    const auto id =
        manager.add_source(
            "main.cpp",
            "int main() {}\n");

    assert(!manager.empty());
    assert(manager.source_count() == 1);
    assert(manager.contains(id));

    const vixc::source::SourceFile *file =
        manager.get(id);

    assert(file != nullptr);
    assert(file->path() == "main.cpp");
    assert(file->contents() == "int main() {}\n");
  }

  void test_source_ids_are_distinct()
  {
    vixc::source::SourceManager manager;

    const auto first =
        manager.add_source(
            "first.cpp",
            "int first = 1;\n");

    const auto second =
        manager.add_source(
            "second.cpp",
            "int second = 2;\n");

    assert(first != second);

    assert(manager.contains(first));
    assert(manager.contains(second));

    assert(manager.source_count() == 2);
  }

  void test_source_order_is_stable()
  {
    vixc::source::SourceManager manager;

    const auto first =
        manager.add_source(
            "first.cpp",
            "first");

    const auto second =
        manager.add_source(
            "second.cpp",
            "second");

    assert(first == 0);
    assert(second == 1);
  }

  void test_source_object_address_remains_stable()
  {
    vixc::source::SourceManager manager;

    const auto first_id =
        manager.add_source(
            "first.cpp",
            "first");

    const vixc::source::SourceFile *first_before =
        manager.get(first_id);

    assert(first_before != nullptr);

    for (std::size_t index = 0;
         index < 128;
         ++index)
    {
      static_cast<void>(
          manager.add_source(
              "generated-" + std::to_string(index) + ".cpp",
              "int value = " + std::to_string(index) + ";"));
    }

    const vixc::source::SourceFile *first_after =
        manager.get(first_id);

    assert(first_after != nullptr);
    assert(first_before == first_after);

    assert(first_after->path() == "first.cpp");
    assert(first_after->contents() == "first");
  }

  void test_get_returns_correct_source()
  {
    vixc::source::SourceManager manager;

    const auto first =
        manager.add_source(
            "alpha.cpp",
            "alpha");

    const auto second =
        manager.add_source(
            "beta.cpp",
            "beta");

    const auto third =
        manager.add_source(
            "gamma.cpp",
            "gamma");

    const vixc::source::SourceFile *first_file =
        manager.get(first);

    const vixc::source::SourceFile *second_file =
        manager.get(second);

    const vixc::source::SourceFile *third_file =
        manager.get(third);

    assert(first_file != nullptr);
    assert(second_file != nullptr);
    assert(third_file != nullptr);

    assert(first_file->path() == "alpha.cpp");
    assert(first_file->contents() == "alpha");

    assert(second_file->path() == "beta.cpp");
    assert(second_file->contents() == "beta");

    assert(third_file->path() == "gamma.cpp");
    assert(third_file->contents() == "gamma");
  }

  void test_invalid_source_id()
  {
    vixc::source::SourceManager manager;

    const auto valid_id =
        manager.add_source(
            "main.cpp",
            "int main() {}\n");

    const vixc::source::SourceId invalid_id =
        valid_id + 100;

    assert(!manager.contains(invalid_id));
    assert(manager.get(invalid_id) == nullptr);
  }

  void test_empty_source()
  {
    vixc::source::SourceManager manager;

    const auto id =
        manager.add_source(
            "empty.cpp",
            "");

    assert(manager.contains(id));
    assert(manager.source_count() == 1);

    const vixc::source::SourceFile *file =
        manager.get(id);

    assert(file != nullptr);
    assert(file->path() == "empty.cpp");
    assert(file->contents().empty());
    assert(file->empty());
    assert(file->size() == 0);
  }

  void test_same_path_is_not_deduplicated()
  {
    vixc::source::SourceManager manager;

    const auto first =
        manager.add_source(
            "main.cpp",
            "first");

    const auto second =
        manager.add_source(
            "main.cpp",
            "second");

    assert(first != second);
    assert(manager.source_count() == 2);

    const vixc::source::SourceFile *first_file =
        manager.get(first);

    const vixc::source::SourceFile *second_file =
        manager.get(second);

    assert(first_file != nullptr);
    assert(second_file != nullptr);

    assert(first_file->path() == "main.cpp");
    assert(second_file->path() == "main.cpp");

    assert(first_file->contents() == "first");
    assert(second_file->contents() == "second");
  }

  void test_source_contents_are_owned()
  {
    vixc::source::SourceManager manager;

    std::string path =
        "owned.cpp";

    std::string contents =
        "int value = 42;";

    const auto id =
        manager.add_source(
            path,
            contents);

    path =
        "changed.cpp";

    contents =
        "changed";

    const vixc::source::SourceFile *file =
        manager.get(id);

    assert(file != nullptr);
    assert(file->path() == "owned.cpp");
    assert(file->contents() == "int value = 42;");
  }

} // namespace

int main()
{
  test_empty_manager();
  test_add_source();
  test_source_ids_are_distinct();
  test_source_order_is_stable();
  test_source_object_address_remains_stable();
  test_get_returns_correct_source();
  test_invalid_source_id();
  test_empty_source();
  test_same_path_is_not_deduplicated();
  test_source_contents_are_owned();

  return 0;
}
