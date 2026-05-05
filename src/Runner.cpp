/**
 *
 *  @file Runner.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vixpp
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix++
 *
 */

#include <vixpp/Runner.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace vixpp
{
  Runner::Runner()
      : transpiler_{}
  {
  }

  Runner::Runner(Transpiler transpiler)
      : transpiler_(std::move(transpiler))
  {
  }

  bool RunnerResult::success() const noexcept
  {
    return exit_code == 0;
  }

  RunnerResult Runner::execute(const RunnerOptions &options,
                               DiagnosticBag &diagnostics) const
  {
    if (options.input_file.empty())
    {
      diagnostics.error(
          "missing input file",
          {},
          {},
          "usage: vix++ run <file.vix>");

      return RunnerResult{1, {}};
    }

    if (options.input_file.extension() != ".vix")
    {
      diagnostics.error(
          "expected a .vix source file",
          options.input_file.string(),
          {},
          "Vix++ source files must use the .vix extension");

      return RunnerResult{1, {}};
    }

    auto source = SourceFile::load(options.input_file, diagnostics);

    if (!source.has_value())
    {
      return RunnerResult{1, {}};
    }

    TranspileResult transpiled = transpiler_.transpile(*source, diagnostics);

    if (diagnostics.has_errors())
    {
      return RunnerResult{1, {}};
    }

    const std::filesystem::path generated_file = write_generated_file(
        options.input_file,
        transpiled.code,
        options.build_dir,
        diagnostics);

    if (generated_file.empty() || diagnostics.has_errors())
    {
      return RunnerResult{1, {}};
    }

    const std::string command = build_vix_command(options, generated_file);
    const int exit_code = std::system(command.c_str());

    return RunnerResult{exit_code, generated_file};
  }

  std::filesystem::path Runner::write_generated_file(
      const std::filesystem::path &input_file,
      const std::string &code,
      const std::filesystem::path &build_dir,
      DiagnosticBag &diagnostics) const
  {
    std::error_code ec{};

    std::filesystem::create_directories(build_dir, ec);

    if (ec)
    {
      diagnostics.error(
          "failed to create Vix++ build directory",
          build_dir.string(),
          {},
          ec.message());

      return {};
    }

    const std::string stem = input_file.stem().string().empty()
                                 ? "main"
                                 : input_file.stem().string();

    const std::filesystem::path generated_file =
        build_dir / (stem + ".generated.cpp");

    std::ofstream output(generated_file, std::ios::binary);

    if (!output)
    {
      diagnostics.error(
          "failed to create generated C++ file",
          generated_file.string(),
          {},
          "check that the build directory is writable");

      return {};
    }

    output << code;

    if (!output)
    {
      diagnostics.error(
          "failed to write generated C++ file",
          generated_file.string(),
          {},
          "the generated file could not be written completely");

      return {};
    }

    return generated_file;
  }

  std::string Runner::build_vix_command(const RunnerOptions &options,
                                        const std::filesystem::path &generated_file)
  {
    std::ostringstream command{};

    command << quote_arg(options.vix_binary)
            << ' '
            << to_vix_command(options.command)
            << ' '
            << quote_arg(path_arg(generated_file));

    for (const auto &arg : options.forwarded_args)
    {
      command << ' ' << quote_arg(arg);
    }

    return command.str();
  }

  const char *Runner::to_vix_command(RunnerCommand command) noexcept
  {
    switch (command)
    {
    case RunnerCommand::Run:
      return "run";
    case RunnerCommand::Build:
      return "build";
    case RunnerCommand::Check:
      return "check";
    }

    return "run";
  }

  std::string Runner::quote_arg(const std::string &value)
  {
    if (value.empty())
    {
      return "\"\"";
    }

    bool needs_quotes = false;

    for (const char c : value)
    {
      if (c == ' ' ||
          c == '\t' ||
          c == '\n' ||
          c == '\r' ||
          c == '"' ||
          c == '\'' ||
          c == '\\' ||
          c == '(' ||
          c == ')' ||
          c == '&' ||
          c == '|' ||
          c == ';' ||
          c == '<' ||
          c == '>')
      {
        needs_quotes = true;
        break;
      }
    }

    if (!needs_quotes)
    {
      return value;
    }

    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back('"');

    for (const char c : value)
    {
      if (c == '"' || c == '\\')
      {
        quoted.push_back('\\');
      }

      quoted.push_back(c);
    }

    quoted.push_back('"');

    return quoted;
  }

  std::string Runner::path_arg(const std::filesystem::path &path)
  {
    return path.string();
  }
}
