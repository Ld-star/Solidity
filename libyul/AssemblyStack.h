/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * Full assembly stack that can support EVM-assembly and Yul as input and EVM, EVM1.5 and
 * Ewasm as output.
 */

#pragma once

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/EVMVersion.h>

#include <libyul/Object.h>
#include <libyul/ObjectParser.h>

#include <libsolidity/interface/OptimiserSettings.h>

#include <libevmasm/LinkerObject.h>

#include <memory>
#include <string>

namespace solidity::langutil
{
class Scanner;
}

namespace solidity::yul
{
class AbstractAssembly;


struct MachineAssemblyObject
{
	std::shared_ptr<evmasm::LinkerObject> bytecode;
	std::string assembly;
	std::unique_ptr<std::string> sourceMappings;
};

/*
 * Full assembly stack that can support EVM-assembly and Yul as input and EVM, EVM1.5 and
 * Ewasm as output.
 */
class AssemblyStack
{
public:
	enum class Language { Yul, Assembly, StrictAssembly, Ewasm };
	enum class Machine { EVM, Ewasm };

	AssemblyStack():
		AssemblyStack(langutil::EVMVersion{}, Language::Assembly, solidity::frontend::OptimiserSettings::none())
	{}
	AssemblyStack(langutil::EVMVersion _evmVersion, Language _language, solidity::frontend::OptimiserSettings _optimiserSettings):
		m_language(_language),
		m_evmVersion(_evmVersion),
		m_optimiserSettings(std::move(_optimiserSettings)),
		m_errorReporter(m_errors)
	{}

	/// @returns the scanner used during parsing
	langutil::Scanner const& scanner() const;

	/// Runs parsing and analysis steps, returns false if input cannot be assembled.
	/// Multiple calls overwrite the previous state.
	bool parseAndAnalyze(std::string const& _sourceName, std::string const& _source);

	/// Run the optimizer suite. Can only be used with Yul or strict assembly.
	/// If the settings (see constructor) disabled the optimizer, nothing is done here.
	void optimize();

	/// Translate the source to a different language / dialect.
	void translate(Language _targetLanguage);

	/// Run the assembly step (should only be called after parseAndAnalyze).
	MachineAssemblyObject assemble(Machine _machine) const;

	/// Run the assembly step (should only be called after parseAndAnalyze).
	/// In addition to the value returned by @a assemble, returns
	/// a second object that is guessed to be the runtime code.
	/// Only available for EVM.
	std::pair<MachineAssemblyObject, MachineAssemblyObject> assembleAndGuessRuntime() const;

	/// Run the assembly step (should only be called after parseAndAnalyze).
	/// In addition to the value returned by @a assemble, returns
	/// a second object that is the runtime code.
	/// Only available for EVM.
	std::pair<MachineAssemblyObject, MachineAssemblyObject> assembleWithDeployed(std::optional<std::string_view> _deployeName = {}) const;

	/// @returns the errors generated during parsing, analysis (and potentially assembly).
	langutil::ErrorList const& errors() const { return m_errors; }

	/// Pretty-print the input after having parsed it.
	std::string print() const;

	/// Return the parsed and analyzed object.
	std::shared_ptr<Object> parserResult() const;

private:
	bool analyzeParsed();
	bool analyzeParsed(yul::Object& _object);

	void compileEVM(yul::AbstractAssembly& _assembly, bool _optimize) const;

	void optimize(yul::Object& _object, bool _isCreation);

	Language m_language = Language::Assembly;
	langutil::EVMVersion m_evmVersion;
	solidity::frontend::OptimiserSettings m_optimiserSettings;

	std::shared_ptr<langutil::Scanner> m_scanner;

	bool m_analysisSuccessful = false;
	std::shared_ptr<yul::Object> m_parserResult;
	langutil::ErrorList m_errors;
	langutil::ErrorReporter m_errorReporter;

	std::unique_ptr<std::string> m_sourceMappings;
};

}
