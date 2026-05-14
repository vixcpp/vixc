/**
 *
 *  @file main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vixc
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix++
 *
 */

#include <vixc/Diagnostic.hpp>
#include <vixc/Runner.hpp>
#include <vixc/Version.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  void print_version()
  {
    std::cout << vixc::product_name() << '\n';
    std::cout << "  version : " << vixc::version() << '\n';
    std::cout << "  author  : Gaspard Kirira\n";
    std::cout << "  source  : " << vixc::repository_url() << "\n\n";
  }

  void print_help()
  {
    std::cout
        << "Vix++\n"
        << "Thin language layer for Vix-powered C++ applications.\n\n"

        << "Usage:\n"
        << "  vixc <command> <file.vix> [options]\n"
        << "  vixc run <file.vix> [-- args]\n"
        << "  vixc build <file.vix> [options]\n"
        << "  vixc check <file.vix> [options]\n"
        << "  vixc <file.vix>\n"
        << "  vixc run\n"
        << "  vixc build\n"
        << "  vixc check\n\n"

        << "Commands:\n"
        << "  run       Transpile and run a .vix file or vix.app project through vix\n"
        << "  build     Transpile and build a .vix file or vix.app project through vix\n"
        << "  check     Transpile and validate a .vix file or vix.app project through vix\n"
        << "  help      Show this help message\n"
        << "  version   Show version information\n\n"

        << "Options:\n"
        << "  --vix <path>       Path to the vix binary. Default: vix\n"
        << "  --build-dir <dir>  Directory for generated C++ files\n"
        << "                     Default: .vix/build/vixc\n"
        << "  -h, --help         Show help\n"
        << "  -v, --version      Show version\n\n"

        << "Examples:\n"
        << "  vixc run main.vix\n"
        << "  vixc build main.vix --out app\n"
        << "  vixc check main.vix\n"
        << "  vixc main.vix\n"
        << "  vixc run\n\n"

        << "Project mode:\n"
        << "  When vix.app exists, vixc run, build, and check use project mode.\n\n"

        << "Source:\n"
        << "  " << vixc::repository_url() << "\n\n";
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

  vixc::RunnerCommand parse_command(const std::string &arg)
  {
    if (arg == "build")
    {
      return vixc::RunnerCommand::Build;
    }

    if (arg == "check")
    {
      return vixc::RunnerCommand::Check;
    }

    return vixc::RunnerCommand::Run;
  }

  int fail_with_usage(const std::string &message)
  {
    std::cerr << "vixc: error: " << message << '\n';
    std::cerr << "run 'vixc help' for usage.\n";
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

  vixc::RunnerOptions options{};

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
      options.command = vixc::RunnerCommand::Run;
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
    const std::filesystem::path app_manifest = std::filesystem::current_path() / "vix.app";

    if (std::filesystem::exists(app_manifest))
    {
      options.project_mode = true;
      options.project_dir = std::filesystem::current_path();
    }
    else
    {
      return fail_with_usage("missing input file or vix.app");
    }
  }

  vixc::DiagnosticBag diagnostics{};
  vixc::Runner runner{};

  const vixc::RunnerResult result = runner.execute(options, diagnostics);

  if (!diagnostics.empty())
  {
    vixc::print_diagnostics(std::cerr, diagnostics);
  }

  return result.exit_code;
}
