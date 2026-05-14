/**
 *
 *  @file Runner.hpp
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
#ifndef VIXC_RUNNER_HPP
#define VIXC_RUNNER_HPP

#include <filesystem>
#include <string>
#include <vector>

#include <vixc/Diagnostic.hpp>
#include <vixc/Transpiler.hpp>

namespace vixc
{
  /**
   * @brief Command requested by the Vix++ CLI.
   */
  enum class RunnerCommand
  {
    Run,
    Build,
    Check
  };

  /**
   * @brief Options used by the Vix++ runner.
   */
  struct RunnerOptions
  {
    RunnerCommand command{RunnerCommand::Run};
    std::filesystem::path input_file{};
    std::vector<std::string> forwarded_args{};
    std::filesystem::path build_dir{".vix/build/vixc"};
    std::string vix_binary{"vix"};
    bool keep_generated{true};
  };

  /**
   * @brief Result returned after preparing and delegating to Vix.
   */
  struct RunnerResult
  {
    int exit_code{1};
    std::filesystem::path generated_file{};

    /**
     * @brief Checks whether the delegated Vix command succeeded.
     *
     * @return True if the exit code is zero.
     */
    [[nodiscard]] bool success() const noexcept;
  };

  /**
   * @brief Runs Vix++ files by transpiling them to C++ and delegating to Vix.
   *
   * Runner does not compile C++ by itself. It loads a .vix file, generates a
   * standard C++ file, then calls the installed Vix CLI.
   */
  class Runner
  {
  public:
    /**
     * @brief Creates a runner with the default transpiler.
     */
    Runner();

    /**
     * @brief Creates a runner with a custom transpiler.
     *
     * @param transpiler Transpiler used to generate C++ code.
     */
    explicit Runner(Transpiler transpiler);

    /**
     * @brief Executes the requested runner command.
     *
     * @param options Runner options.
     * @param diagnostics Diagnostic collection used for reporting errors.
     * @return Runner result containing the exit code and generated file path.
     */
    [[nodiscard]] RunnerResult execute(const RunnerOptions &options,
                                       DiagnosticBag &diagnostics) const;

  private:
    Transpiler transpiler_{};

    /**
     * @brief Writes generated C++ code to the configured build directory.
     *
     * @param input_file Original .vix input file.
     * @param code Generated C++ code.
     * @param build_dir Directory where generated files are written.
     * @param diagnostics Diagnostic collection used for reporting errors.
     * @return Generated file path on success, empty path on failure.
     */
    [[nodiscard]] std::filesystem::path write_generated_file(
        const std::filesystem::path &input_file,
        const std::string &code,
        const std::filesystem::path &build_dir,
        DiagnosticBag &diagnostics) const;

    /**
     * @brief Builds the command line used to call Vix.
     *
     * @param options Runner options.
     * @param generated_file Generated C++ file path.
     * @return Full command line string.
     */
    [[nodiscard]] static std::string build_vix_command(
        const RunnerOptions &options,
        const std::filesystem::path &generated_file);

    /**
     * @brief Converts a runner command to the equivalent Vix command.
     *
     * @param command Runner command.
     * @return Vix command name.
     */
    [[nodiscard]] static const char *to_vix_command(RunnerCommand command) noexcept;

    /**
     * @brief Quotes an argument for portable command-line execution.
     *
     * The quoting is intentionally conservative and works for common POSIX
     * shells and Windows cmd.exe usage.
     *
     * @param value Argument to quote.
     * @return Quoted argument.
     */
    [[nodiscard]] static std::string quote_arg(const std::string &value);

    /**
     * @brief Converts a filesystem path to a command-line argument.
     *
     * @param path Path to convert.
     * @return Path string.
     */
    [[nodiscard]] static std::string path_arg(const std::filesystem::path &path);
  };
}

#endif
