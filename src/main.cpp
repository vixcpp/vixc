/**
 *
 *  @file main.cpp
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

#include <vixpp/Diagnostic.hpp>
#include <vixpp/Runner.hpp>
#include <vixpp/Version.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  void print_version()
  {
    std::cout << vixpp::product_name() << '\n';
    std::cout << "  version : " << vixpp::version() << '\n';
    std::cout << "  author  : Gaspard Kirira\n";
    std::cout << "  source  : " << vixpp::repository_url() << "\n\n";
  }

  void print_help()
  {
    std::cout
        << "Vix++\n"
        << "Thin language layer for Vix-powered C++ applications.\n\n"

        << "Usage:\n"
        << "  vix++ <command> <file.vix> [options]\n"
        << "  vix++ run <file.vix> [-- args]\n"
        << "  vix++ build <file.vix> [options]\n"
        << "  vix++ check <file.vix> [options]\n"
        << "  vix++ <file.vix>\n\n"

        << "Commands:\n"
        << "  run       Transpile a .vix file and run it through vix\n"
        << "  build     Transpile a .vix file and build it through vix\n"
        << "  check     Transpile a .vix file and validate it through vix\n"
        << "  help      Show this help message\n"
        << "  version   Show version information\n\n"

        << "Options:\n"
        << "  --vix <path>       Path to the vix binary. Default: vix\n"
        << "  --build-dir <dir>  Directory for generated C++ files\n"
        << "                     Default: .vix/build/vixpp\n"
        << "  -h, --help         Show help\n"
        << "  -v, --version      Show version\n\n"

        << "Examples:\n"
        << "  vix++ run main.vix\n"
        << "  vix++ build main.vix --out app\n"
        << "  vix++ check main.vix\n"
        << "  vix++ main.vix\n\n"

        << "Source:\n"
        << "  " << vixpp::repository_url() << "\n\n";
  }

  bool is_help_arg(const std::string &arg)
  {
    return arg == "help" || arg == "-h" || arg == "--help";
  }

  bool is_version_arg(const std::string &arg)
  {
    return arg == "version" || arg == "-v" || arg == "--version";
  }

  bool is_command_arg(const std::string &arg)
  {
    return arg == "run" || arg == "build" || arg == "check";
  }

  vixpp::RunnerCommand parse_command(const std::string &arg)
  {
    if (arg == "build")
    {
      return vixpp::RunnerCommand::Build;
    }

    if (arg == "check")
    {
      return vixpp::RunnerCommand::Check;
    }

    return vixpp::RunnerCommand::Run;
  }

  int fail_with_usage(const std::string &message)
  {
    std::cerr << "vix++: error: " << message << '\n';
    std::cerr << "run 'vix++ help' for usage.\n";
    return 1;
  }
}

int main(int argc, char **argv)
{
  if (argc <= 1)
  {
    print_help();
    return 0;
  }

  std::vector<std::string> args{};

  for (int i = 1; i < argc; ++i)
  {
    args.emplace_back(argv[i]);
  }

  if (args.empty())
  {
    print_help();
    return 0;
  }

  if (is_help_arg(args.front()))
  {
    print_help();
    return 0;
  }

  if (is_version_arg(args.front()))
  {
    print_version();
    return 0;
  }

  vixpp::RunnerOptions options{};

  std::size_t index = 0;

  if (is_command_arg(args[index]))
  {
    options.command = parse_command(args[index]);
    ++index;
  }
  else
  {
    std::filesystem::path maybe_file{args[index]};

    if (maybe_file.extension() == ".vix")
    {
      options.command = vixpp::RunnerCommand::Run;
    }
    else
    {
      return fail_with_usage("expected command or .vix file");
    }
  }

  bool forwarding_mode = false;

  for (; index < args.size(); ++index)
  {
    const std::string &arg = args[index];

    if (forwarding_mode)
    {
      options.forwarded_args.push_back(arg);
      continue;
    }

    if (arg == "--")
    {
      forwarding_mode = true;
      continue;
    }

    if (arg == "--vix")
    {
      if (index + 1 >= args.size())
      {
        return fail_with_usage("--vix requires a path");
      }

      options.vix_binary = args[index + 1];
      ++index;
      continue;
    }

    if (arg == "--build-dir")
    {
      if (index + 1 >= args.size())
      {
        return fail_with_usage("--build-dir requires a directory");
      }

      options.build_dir = args[index + 1];
      ++index;
      continue;
    }

    if (arg == "-h" || arg == "--help")
    {
      print_help();
      return 0;
    }

    if (arg == "-v" || arg == "--version")
    {
      print_version();
      return 0;
    }

    if (options.input_file.empty())
    {
      options.input_file = arg;
      continue;
    }

    options.forwarded_args.push_back(arg);
  }

  if (options.input_file.empty())
  {
    return fail_with_usage("missing input file");
  }

  vixpp::DiagnosticBag diagnostics{};
  vixpp::Runner runner{};

  const vixpp::RunnerResult result = runner.execute(options, diagnostics);

  if (!diagnostics.empty())
  {
    vixpp::print_diagnostics(std::cerr, diagnostics);
  }

  return result.exit_code;
}
