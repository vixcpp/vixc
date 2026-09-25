/**
 *
 *  @file vixc.hpp
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

#if !defined(VIXC_VIXC_HPP)
#define VIXC_VIXC_HPP

/**
 * @brief Main public include for the VixC frontend.
 *
 * Including <vixc/vixc.hpp> provides the complete public embedding surface of
 * VixC without exposing parser, semantic-analysis, IR, lowering, or backend
 * implementation headers.
 *
 * The public API contains the Frontend entry point, frontend configuration,
 * frontend results, structured diagnostics, source-position types, and version
 * information.
 *
 * Internal frontend stages remain private implementation details. Embedding
 * applications should normally depend on this header or on individual public
 * headers under <vixc/...> rather than including files from the VixC source
 * tree.
 *
 * A typical embedding application creates a vixc::Frontend and processes
 * source text through Frontend::process(). The returned FrontendResult owns
 * the diagnostics and generated output required after the frontend invocation
 * completes.
 */

#include <vixc/Diagnostic.hpp>
#include <vixc/DiagnosticSeverity.hpp>
#include <vixc/Frontend.hpp>
#include <vixc/FrontendOptions.hpp>
#include <vixc/FrontendResult.hpp>
#include <vixc/SourceLocation.hpp>
#include <vixc/SourceRange.hpp>
#include <vixc/Version.hpp>

#endif // VIXC_VIXC_HPP
