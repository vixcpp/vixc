/**
 *
 *  @file main.cpp
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

#include <vixc/vixc.hpp>

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace
{

  struct CommandLineOptions final
  {
    vixc::FrontendOptions frontend{};
    std::string input_path;
    std::optional<std::string> output_path;
    bool show_help{false};
    bool show_version{false};
  };

  void print_usage(std::ostream &stream)
  {
    stream
        << "Usage: vixc [options] <source>\n"
        << '\n'
        << "Compile and analyze source with the VixC frontend.\n"
        << '\n'
        << "Options:\n"
        << "  --analyze              Stop after semantic analysis\n"
        << "  --lower                Stop after IR lowering\n"
        << "  --emit                 Emit backend output (default)\n"
        << "  -o, --output <file>    Write generated output to a file\n"
        << "  --no-source-map        Do not retain generated source mappings\n"
        << "  --version              Print the VixC version\n"
        << "  -h, --help             Show this help\n"
        << '\n'
        << "Use '-' as the source path to read from standard input.\n";
  }

  void print_version(std::ostream &stream)
  {
    stream
        << "vixc "
        << vixc::version::string
        << '\n';
  }

  bool parse_command_line(
      int argc,
      char **argv,
      CommandLineOptions &options,
      std::string &error)
  {
    bool input_seen = false;

    for (int index = 1;
         index < argc;
         ++index)
    {
      const std::string_view argument{
          argv[index]};

      if (
          argument == "-h" || argument == "--help")
      {
        options.show_help = true;
        continue;
      }

      if (argument == "--version")
      {
        options.show_version = true;
        continue;
      }

      if (argument == "--analyze")
      {
        options.frontend.action =
            vixc::FrontendAction::Analyze;

        continue;
      }

      if (argument == "--lower")
      {
        options.frontend.action =
            vixc::FrontendAction::Lower;

        continue;
      }

      if (argument == "--emit")
      {
        options.frontend.action =
            vixc::FrontendAction::Emit;

        continue;
      }

      if (argument == "--no-source-map")
      {
        options.frontend.retain_source_map =
            false;

        continue;
      }

      if (
          argument == "-o" || argument == "--output")
      {
        if (index + 1 >= argc)
        {
          error =
              "missing output path after " + std::string{argument};

          return false;
        }

        if (options.output_path.has_value())
        {
          error =
              "output path was specified more than once";

          return false;
        }

        ++index;

        options.output_path =
            std::string{
                argv[index]};

        continue;
      }

      if (
          !argument.empty() && argument.front() == '-' && argument != "-")
      {
        error =
            "unknown option: " + std::string{argument};

        return false;
      }

      if (input_seen)
      {
        error =
            "more than one source input was specified";

        return false;
      }

      options.input_path =
          std::string{argument};

      input_seen = true;
    }

    if (
        options.show_help || options.show_version)
    {
      return true;
    }

    if (!input_seen)
    {
      error =
          "no source input was specified";

      return false;
    }

    if (
        options.output_path.has_value() && options.frontend.action != vixc::FrontendAction::Emit)
    {
      error =
          "--output requires --emit";

      return false;
    }

    return true;
  }

  bool read_stream(
      std::istream &stream,
      std::string &contents)
  {
    std::ostringstream buffer;
    buffer << stream.rdbuf();

    if (
        !stream.good() && !stream.eof())
    {
      return false;
    }

    contents =
        std::move(buffer).str();

    return true;
  }

  bool read_source(
      const std::string &path,
      std::string &contents,
      std::string &error)
  {
    if (path == "-")
    {
      if (!read_stream(
              std::cin,
              contents))
      {
        error =
            "unable to read source from standard input";

        return false;
      }

      return true;
    }

    std::ifstream file{
        path,
        std::ios::binary};

    if (!file)
    {
      error =
          "unable to open source file: " + path;

      return false;
    }

    if (!read_stream(
            file,
            contents))
    {
      error =
          "unable to read source file: " + path;

      return false;
    }

    return true;
  }

  bool write_output(
      const std::string &path,
      std::string_view contents,
      std::string &error)
  {
    std::ofstream file{
        path,
        std::ios::binary | std::ios::trunc};

    if (!file)
    {
      error =
          "unable to open output file: " + path;

      return false;
    }

    file.write(
        contents.data(),
        static_cast<std::streamsize>(
            contents.size()));

    if (!file)
    {
      error =
          "unable to write output file: " + path;

      return false;
    }

    return true;
  }

  void print_diagnostics(
      const vixc::FrontendResult &result,
      std::string_view source_name,
      std::string_view source)
  {
    const vixc::DiagnosticRenderer renderer;

    for (const vixc::Diagnostic &diagnostic :
         result.diagnostics())
    {
      std::cerr
          << renderer.render(
                 diagnostic,
                 source_name,
                 source);
    }
  }

} // namespace

int main(
    int argc,
    char **argv)
{
  CommandLineOptions options;
  std::string error;

  if (!parse_command_line(
          argc,
          argv,
          options,
          error))
  {
    std::cerr
        << "vixc: "
        << error
        << '\n'
        << '\n';

    print_usage(std::cerr);

    return 2;
  }

  if (options.show_help)
  {
    print_usage(std::cout);
    return 0;
  }

  if (options.show_version)
  {
    print_version(std::cout);
    return 0;
  }

  std::string source;

  if (!read_source(
          options.input_path,
          source,
          error))
  {
    std::cerr
        << "vixc: "
        << error
        << '\n';

    return 2;
  }

  const std::string_view source_name =
      options.input_path == "-"
          ? std::string_view{"<stdin>"}
          : std::string_view{options.input_path};

  vixc::Frontend frontend;

  vixc::FrontendResult result =
      frontend.process(
          source_name,
          source,
          options.frontend);

  print_diagnostics(
      result,
      source_name,
      source);

  if (!result.success())
    return 1;

  if (
      options.frontend.action != vixc::FrontendAction::Emit)
  {
    return 0;
  }

  if (options.output_path.has_value())
  {
    if (!write_output(
            *options.output_path,
            result.generated_output(),
            error))
    {
      std::cerr
          << "vixc: "
          << error
          << '\n';

      return 2;
    }

    return 0;
  }

  std::cout.write(
      result.generated_output().data(),
      static_cast<std::streamsize>(
          result.generated_output().size()));

  if (!std::cout)
  {
    std::cerr
        << "vixc: unable to write generated output\n";

    return 2;
  }

  return 0;
}
