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
/**
 * @author Lefteris <lefteris@ethdev.com>
 * @author Gav Wood <g@ethdev.com>
 * @date 2014
 * Solidity command line interface.
 */
#include "CommandLineInterface.h"

#include "solidity/BuildInfo.h"

#include <libsolidity/interface/Version.h>
#include <libsolidity/parsing/Scanner.h>
#include <libsolidity/parsing/Parser.h>
#include <libsolidity/ast/ASTPrinter.h>
#include <libsolidity/ast/ASTJsonConverter.h>
#include <libsolidity/analysis/NameAndTypeResolver.h>
#include <libsolidity/interface/Exceptions.h>
#include <libsolidity/interface/CompilerStack.h>
#include <libsolidity/interface/SourceReferenceFormatter.h>
#include <libsolidity/interface/GasEstimator.h>
#include <libsolidity/formal/Why3Translator.h>

#include <libevmasm/Instruction.h>
#include <libevmasm/GasMeter.h>

#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/JSON.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#ifdef _WIN32 // windows
	#include <io.h>
	#define isatty _isatty
	#define fileno _fileno
#else // unix
	#include <unistd.h>
#endif
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
namespace po = boost::program_options;

namespace dev
{
namespace solidity
{

static string const g_strAbi = "abi";
static string const g_strAddStandard = "add-std";
static string const g_strAsm = "asm";
static string const g_strAsmJson = "asm-json";
static string const g_strAssemble = "assemble";
static string const g_strAst = "ast";
static string const g_strAstJson = "ast-json";
static string const g_strBinary = "bin";
static string const g_strBinaryRuntime = "bin-runtime";
static string const g_strCloneBinary = "clone-bin";
static string const g_strCombinedJson = "combined-json";
static string const g_strContracts = "contracts";
static string const g_strFormal = "formal";
static string const g_strGas = "gas";
static string const g_strHelp = "help";
static string const g_strInputFile = "input-file";
static string const g_strInterface = "interface";
static string const g_strLibraries = "libraries";
static string const g_strLink = "link";
static string const g_strMetadata = "metadata";
static string const g_strNatspecDev = "devdoc";
static string const g_strNatspecUser = "userdoc";
static string const g_strOpcodes = "opcodes";
static string const g_strOptimize = "optimize";
static string const g_strOptimizeRuns = "optimize-runs";
static string const g_strOutputDir = "output-dir";
static string const g_strSignatureHashes = "hashes";
static string const g_strSources = "sources";
static string const g_strSourceList = "sourceList";
static string const g_strSrcMap = "srcmap";
static string const g_strSrcMapRuntime = "srcmap-runtime";
static string const g_strVersion = "version";
static string const g_stdinFileNameStr = "<stdin>";
static string const g_strMetadataLiteral = "metadata-literal";

static string const g_argAbi = g_strAbi;
static string const g_argAddStandard = g_strAddStandard;
static string const g_argAsm = g_strAsm;
static string const g_argAsmJson = g_strAsmJson;
static string const g_argAssemble = g_strAssemble;
static string const g_argAst = g_strAst;
static string const g_argAstJson = g_strAstJson;
static string const g_argBinary = g_strBinary;
static string const g_argBinaryRuntime = g_strBinaryRuntime;
static string const g_argCloneBinary = g_strCloneBinary;
static string const g_argCombinedJson = g_strCombinedJson;
static string const g_argFormal = g_strFormal;
static string const g_argGas = g_strGas;
static string const g_argHelp = g_strHelp;
static string const g_argInputFile = g_strInputFile;
static string const g_argLibraries = g_strLibraries;
static string const g_argLink = g_strLink;
static string const g_argMetadata = g_strMetadata;
static string const g_argNatspecDev = g_strNatspecDev;
static string const g_argNatspecUser = g_strNatspecUser;
static string const g_argOpcodes = g_strOpcodes;
static string const g_argOptimize = g_strOptimize;
static string const g_argOptimizeRuns = g_strOptimizeRuns;
static string const g_argOutputDir = g_strOutputDir;
static string const g_argSignatureHashes = g_strSignatureHashes;
static string const g_argVersion = g_strVersion;
static string const g_stdinFileName = g_stdinFileNameStr;
static string const g_argMetadataLiteral = g_strMetadataLiteral;

/// Possible arguments to for --combined-json
static set<string> const g_combinedJsonArgs{
	g_strAbi,
	g_strAsm,
	g_strAst,
	g_strBinary,
	g_strBinaryRuntime,
	g_strCloneBinary,
	g_strInterface,
	g_strMetadata,
	g_strNatspecUser,
	g_strNatspecDev,
	g_strOpcodes,
	g_strSrcMap,
	g_strSrcMapRuntime
};

static void version()
{
	cout <<
		"solc, the solidity compiler commandline interface" <<
		endl <<
		"Version: " <<
		dev::solidity::VersionString <<
		endl;
	exit(0);
}

static bool needsHumanTargetedStdout(po::variables_map const& _args)
{
	if (_args.count(g_argGas))
		return true;
	if (_args.count(g_argOutputDir))
		return false;
	for (string const& arg: {
		g_argAbi,
		g_argAsm,
		g_argAsmJson,
		g_argAstJson,
		g_argBinary,
		g_argBinaryRuntime,
		g_argCloneBinary,
		g_argFormal,
		g_argMetadata,
		g_argNatspecUser,
		g_argNatspecDev,
		g_argOpcodes,
		g_argSignatureHashes
	})
		if (_args.count(arg))
			return true;
	return false;
}

void CommandLineInterface::handleBinary(string const& _contract)
{
	if (m_args.count(g_argBinary))
	{
		if (m_args.count(g_argOutputDir))
			createFile(m_compiler->filesystemFriendlyName(_contract) + ".bin", m_compiler->object(_contract).toHex());
		else
		{
			cout << "Binary: " << endl;
			cout << m_compiler->object(_contract).toHex() << endl;
		}
	}
	if (m_args.count(g_argCloneBinary))
	{
		if (m_args.count(g_argOutputDir))
			createFile(_contract + ".clone_bin", m_compiler->cloneObject(_contract).toHex());
		else
		{
			cout << "Clone Binary: " << endl;
			cout << m_compiler->cloneObject(_contract).toHex() << endl;
		}
	}
	if (m_args.count(g_argBinaryRuntime))
	{
		if (m_args.count(g_argOutputDir))
			createFile(_contract + ".bin-runtime", m_compiler->runtimeObject(_contract).toHex());
		else
		{
			cout << "Binary of the runtime part: " << endl;
			cout << m_compiler->runtimeObject(_contract).toHex() << endl;
		}
	}
}

void CommandLineInterface::handleOpcode(string const& _contract)
{
	if (m_args.count(g_argOutputDir))
		createFile(_contract + ".opcode", solidity::disassemble(m_compiler->object(_contract).bytecode));
	else
	{
		cout << "Opcodes: " << endl;
		cout << solidity::disassemble(m_compiler->object(_contract).bytecode);
		cout << endl;
	}
}

void CommandLineInterface::handleBytecode(string const& _contract)
{
	if (m_args.count(g_argOpcodes))
		handleOpcode(_contract);
	if (m_args.count(g_argBinary) || m_args.count(g_argCloneBinary) || m_args.count(g_argBinaryRuntime))
		handleBinary(_contract);
}

void CommandLineInterface::handleSignatureHashes(string const& _contract)
{
	if (!m_args.count(g_argSignatureHashes))
		return;

	string out;
	for (auto const& it: m_compiler->contractDefinition(_contract).interfaceFunctions())
		out += toHex(it.first.ref()) + ": " + it.second->externalSignature() + "\n";

	if (m_args.count(g_argOutputDir))
		createFile(_contract + ".signatures", out);
	else
		cout << "Function signatures: " << endl << out;
}

void CommandLineInterface::handleOnChainMetadata(string const& _contract)
{
	if (!m_args.count(g_argMetadata))
		return;

	string data = m_compiler->onChainMetadata(_contract);
	if (m_args.count("output-dir"))
		createFile(_contract + "_meta.json", data);
	else
		cout << "Metadata: " << endl << data << endl;
}

void CommandLineInterface::handleMeta(DocumentationType _type, string const& _contract)
{
	std::string argName;
	std::string suffix;
	std::string title;
	switch(_type)
	{
	case DocumentationType::ABIInterface:
		argName = g_argAbi;
		suffix = ".abi";
		title = "Contract JSON ABI";
		break;
	case DocumentationType::NatspecUser:
		argName = g_argNatspecUser;
		suffix = ".docuser";
		title = "User Documentation";
		break;
	case DocumentationType::NatspecDev:
		argName = g_argNatspecDev;
		suffix = ".docdev";
		title = "Developer Documentation";
		break;
	default:
		// should never happen
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown documentation _type"));
	}

	if (m_args.count(argName))
	{
		std::string output;
		if (_type == DocumentationType::ABIInterface)
			output = dev::jsonCompactPrint(m_compiler->metadata(_contract, _type));
		else
			output = dev::jsonPrettyPrint(m_compiler->metadata(_contract, _type));

		if (m_args.count(g_argOutputDir))
			createFile(_contract + suffix, output);
		else
		{
			cout << title << endl;
			cout << output << endl;
		}

	}
}

void CommandLineInterface::handleGasEstimation(string const& _contract)
{
	using Gas = GasEstimator::GasConsumption;
	if (!m_compiler->assemblyItems(_contract) && !m_compiler->runtimeAssemblyItems(_contract))
		return;
	cout << "Gas estimation:" << endl;
	if (eth::AssemblyItems const* items = m_compiler->assemblyItems(_contract))
	{
		Gas gas = GasEstimator::functionalEstimation(*items);
		u256 bytecodeSize(m_compiler->runtimeObject(_contract).bytecode.size());
		cout << "construction:" << endl;
		cout << "   " << gas << " + " << (bytecodeSize * eth::GasCosts::createDataGas) << " = ";
		gas += bytecodeSize * eth::GasCosts::createDataGas;
		cout << gas << endl;
	}
	if (eth::AssemblyItems const* items = m_compiler->runtimeAssemblyItems(_contract))
	{
		ContractDefinition const& contract = m_compiler->contractDefinition(_contract);
		cout << "external:" << endl;
		for (auto it: contract.interfaceFunctions())
		{
			string sig = it.second->externalSignature();
			GasEstimator::GasConsumption gas = GasEstimator::functionalEstimation(*items, sig);
			cout << "   " << sig << ":\t" << gas << endl;
		}
		if (contract.fallbackFunction())
		{
			GasEstimator::GasConsumption gas = GasEstimator::functionalEstimation(*items, "INVALID");
			cout << "   fallback:\t" << gas << endl;
		}
		cout << "internal:" << endl;
		for (auto const& it: contract.definedFunctions())
		{
			if (it->isPartOfExternalInterface() || it->isConstructor())
				continue;
			size_t entry = m_compiler->functionEntryPoint(_contract, *it);
			GasEstimator::GasConsumption gas = GasEstimator::GasConsumption::infinite();
			if (entry > 0)
				gas = GasEstimator::functionalEstimation(*items, entry, *it);
			FunctionType type(*it);
			cout << "   " << it->name() << "(";
			auto paramTypes = type.parameterTypes();
			for (auto it = paramTypes.begin(); it != paramTypes.end(); ++it)
				cout << (*it)->toString() << (it + 1 == paramTypes.end() ? "" : ",");
			cout << "):\t" << gas << endl;
		}
	}
}

void CommandLineInterface::handleFormal()
{
	if (!m_args.count(g_argFormal))
		return;

	if (m_args.count(g_argOutputDir))
		createFile("solidity.mlw", m_compiler->formalTranslation());
	else
		cout << "Formal version:" << endl << m_compiler->formalTranslation() << endl;
}

void CommandLineInterface::readInputFilesAndConfigureRemappings()
{
	bool addStdin = false;
	if (!m_args.count(g_argInputFile))
		addStdin = true;
	else
		for (string path: m_args[g_argInputFile].as<vector<string>>())
		{
			auto eq = find(path.begin(), path.end(), '=');
			if (eq != path.end())
				path = string(eq + 1, path.end());
			else if (path == "-")
				addStdin = true;
			else
			{
				auto infile = boost::filesystem::path(path);
				if (!boost::filesystem::exists(infile))
				{
					cerr << "Skipping non-existent input file \"" << infile << "\"" << endl;
					continue;
				}

				if (!boost::filesystem::is_regular_file(infile))
				{
					cerr << "\"" << infile << "\" is not a valid file. Skipping" << endl;
					continue;
				}

				m_sourceCodes[infile.string()] = dev::contentsString(infile.string());
				path = boost::filesystem::canonical(infile).string();
			}
			m_allowedDirectories.push_back(boost::filesystem::path(path).remove_filename());
		}
	if (addStdin)
	{
		string s;
		while (!cin.eof())
		{
			getline(cin, s);
			m_sourceCodes[g_stdinFileName].append(s + '\n');
		}
	}
}

bool CommandLineInterface::parseLibraryOption(string const& _input)
{
	namespace fs = boost::filesystem;
	string data = fs::is_regular_file(_input) ? contentsString(_input) : _input;

	vector<string> libraries;
	boost::split(libraries, data, boost::is_space() || boost::is_any_of(","), boost::token_compress_on);
	for (string const& lib: libraries)
		if (!lib.empty())
		{
			//search for last colon in string as our binaries output placeholders in the form of file:Name
			//so we need to search for the second `:` in the string
			auto colon = lib.rfind(':');
			if (colon == string::npos)
			{
				cerr << "Colon separator missing in library address specifier \"" << lib << "\"" << endl;
				return false;
			}
			string libName(lib.begin(), lib.begin() + colon);
			string addrString(lib.begin() + colon + 1, lib.end());
			boost::trim(libName);
			boost::trim(addrString);
			if (!passesAddressChecksum(addrString, false))
			{
				cerr << "Invalid checksum on library address \"" << libName << "\": " << addrString << endl;
				return false;
			}
			bytes binAddr = fromHex(addrString);
			h160 address(binAddr, h160::AlignRight);
			if (binAddr.size() > 20 || address == h160())
			{
				cerr << "Invalid address for library \"" << libName << "\": " << addrString << endl;
				return false;
			}
			m_libraries[libName] = address;
		}

	return true;
}

void CommandLineInterface::createFile(string const& _fileName, string const& _data)
{
	namespace fs = boost::filesystem;
	// create directory if not existent
	fs::path p(m_args.at(g_argOutputDir).as<string>());
	fs::create_directories(p);
	string pathName = (p / _fileName).string();
	ofstream outFile(pathName);
	outFile << _data;
	if (!outFile)
		BOOST_THROW_EXCEPTION(FileError() << errinfo_comment("Could not write to file: " + pathName));
}

bool CommandLineInterface::parseArguments(int _argc, char** _argv)
{
	// Declare the supported options.
	po::options_description desc(
		R"(solc, the Solidity commandline compiler.
Usage: solc [options] [input_file...]
Compiles the given Solidity input files (or the standard input if none given or
"-" is used as a file name) and outputs the components specified in the options
at standard output or in files in the output directory, if specified.
Imports are automatically read from the filesystem, but it is also possible to
remap paths using the context:prefix=path syntax.
Example:
    solc --bin -o /tmp/solcoutput dapp-bin=/usr/local/lib/dapp-bin contract.sol

Allowed options)",
		po::options_description::m_default_line_length,
		po::options_description::m_default_line_length - 23);
	desc.add_options()
		(g_argHelp.c_str(), "Show help message and exit.")
		(g_argVersion.c_str(), "Show version and exit.")
		(g_argOptimize.c_str(), "Enable bytecode optimizer.")
		(
			g_argOptimizeRuns.c_str(),
			po::value<unsigned>()->value_name("n")->default_value(200),
			"Estimated number of contract runs for optimizer tuning."
		)
		(g_argAddStandard.c_str(), "Add standard contracts.")
		(
			g_argLibraries.c_str(),
			po::value<vector<string>>()->value_name("libs"),
			"Direct string or file containing library addresses. Syntax: "
			"<libraryName>: <address> [, or whitespace] ...\n"
			"Address is interpreted as a hex string optionally prefixed by 0x."
		)
		(
			(g_argOutputDir + ",o").c_str(),
			po::value<string>()->value_name("path"),
			"If given, creates one file per component and contract/file at the specified directory."
		)
		(
			g_argCombinedJson.c_str(),
			po::value<string>()->value_name(boost::join(g_combinedJsonArgs, ",")),
			"Output a single json document containing the specified information."
		)
		(g_argGas.c_str(), "Print an estimate of the maximal gas usage for each function.")
		(
			g_argAssemble.c_str(),
			"Switch to assembly mode, ignoring all options and assumes input is assembly."
		)
		(
			g_argLink.c_str(),
			"Switch to linker mode, ignoring all options apart from --libraries "
			"and modify binaries in place."
		)
		(g_argMetadataLiteral.c_str(), "Store referenced sources are literal data in the metadata output.");
	po::options_description outputComponents("Output Components");
	outputComponents.add_options()
		(g_argAst.c_str(), "AST of all source files.")
		(g_argAstJson.c_str(), "AST of all source files in JSON format.")
		(g_argAsm.c_str(), "EVM assembly of the contracts.")
		(g_argAsmJson.c_str(), "EVM assembly of the contracts in JSON format.")
		(g_argOpcodes.c_str(), "Opcodes of the contracts.")
		(g_argBinary.c_str(), "Binary of the contracts in hex.")
		(g_argBinaryRuntime.c_str(), "Binary of the runtime part of the contracts in hex.")
		(g_argCloneBinary.c_str(), "Binary of the clone contracts in hex.")
		(g_argAbi.c_str(), "ABI specification of the contracts.")
		(g_argSignatureHashes.c_str(), "Function signature hashes of the contracts.")
		(g_argNatspecUser.c_str(), "Natspec user documentation of all contracts.")
		(g_argNatspecDev.c_str(), "Natspec developer documentation of all contracts.")
		(g_argMetadata.c_str(), "Combined Metadata JSON whose Swarm hash is stored on-chain.")
		(g_argFormal.c_str(), "Translated source suitable for formal analysis.");
	desc.add(outputComponents);

	po::options_description allOptions = desc;
	allOptions.add_options()(g_argInputFile.c_str(), po::value<vector<string>>(), "input file");

	// All positional options should be interpreted as input files
	po::positional_options_description filesPositions;
	filesPositions.add(g_argInputFile.c_str(), -1);

	// parse the compiler arguments
	try
	{
		po::command_line_parser cmdLineParser(_argc, _argv);
		cmdLineParser.options(allOptions).positional(filesPositions);
		po::store(cmdLineParser.run(), m_args);
	}
	catch (po::error const& _exception)
	{
		cerr << _exception.what() << endl;
		return false;
	}

	if (m_args.count(g_argHelp) || (isatty(fileno(stdin)) && _argc == 1))
	{
		cout << desc;
		return false;
	}

	if (m_args.count(g_argVersion))
	{
		version();
		return false;
	}

	if (m_args.count(g_argCombinedJson))
	{
		vector<string> requests;
		for (string const& item: boost::split(requests, m_args[g_argCombinedJson].as<string>(), boost::is_any_of(",")))
			if (!g_combinedJsonArgs.count(item))
			{
				cerr << "Invalid option to --combined-json: " << item << endl;
				return false;
			}
	}
	po::notify(m_args);

	return true;
}

bool CommandLineInterface::processInput()
{
	readInputFilesAndConfigureRemappings();

	if (m_args.count(g_argLibraries))
		for (string const& library: m_args[g_argLibraries].as<vector<string>>())
			if (!parseLibraryOption(library))
				return false;

	if (m_args.count(g_argAssemble))
	{
		// switch to assembly mode
		m_onlyAssemble = true;
		return assemble();
	}
	if (m_args.count(g_argLink))
	{
		// switch to linker mode
		m_onlyLink = true;
		return link();
	}

	CompilerStack::ReadFileCallback fileReader = [this](string const& _path)
	{
		auto path = boost::filesystem::path(_path);
		if (!boost::filesystem::exists(path))
			return CompilerStack::ReadFileResult{false, "File not found."};
		auto canonicalPath = boost::filesystem::canonical(path);
		bool isAllowed = false;
		for (auto const& allowedDir: m_allowedDirectories)
		{
			// If dir is a prefix of boostPath, we are fine.
			if (
				std::distance(allowedDir.begin(), allowedDir.end()) <= std::distance(canonicalPath.begin(), canonicalPath.end()) &&
				std::equal(allowedDir.begin(), allowedDir.end(), canonicalPath.begin())
			)
			{
				isAllowed = true;
				break;
			}
		}
		if (!isAllowed)
			return CompilerStack::ReadFileResult{false, "File outside of allowed directories."};
		else if (!boost::filesystem::is_regular_file(canonicalPath))
			return CompilerStack::ReadFileResult{false, "Not a valid file."};
		else
		{
			auto contents = dev::contentsString(canonicalPath.string());
			m_sourceCodes[path.string()] = contents;
			return CompilerStack::ReadFileResult{true, contents};
		}
	};

	m_compiler.reset(new CompilerStack(fileReader));
	auto scannerFromSourceName = [&](string const& _sourceName) -> solidity::Scanner const& { return m_compiler->scanner(_sourceName); };
	try
	{
		if (m_args.count(g_argMetadataLiteral) > 0)
			m_compiler->useMetadataLiteralSources(true);
		if (m_args.count(g_argInputFile))
			m_compiler->setRemappings(m_args[g_argInputFile].as<vector<string>>());
		for (auto const& sourceCode: m_sourceCodes)
			m_compiler->addSource(sourceCode.first, sourceCode.second);
		// TODO: Perhaps we should not compile unless requested
		bool optimize = m_args.count(g_argOptimize) > 0;
		unsigned runs = m_args[g_argOptimizeRuns].as<unsigned>();
		bool successful = m_compiler->compile(optimize, runs, m_libraries);

		if (successful && m_args.count(g_argFormal))
			if (!m_compiler->prepareFormalAnalysis())
				successful = false;

		for (auto const& error: m_compiler->errors())
			SourceReferenceFormatter::printExceptionInformation(
				cerr,
				*error,
				(error->type() == Error::Type::Warning) ? "Warning" : "Error",
				scannerFromSourceName
			);

		if (!successful)
			return false;
	}
	catch (CompilerError const& _exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, _exception, "Compiler error", scannerFromSourceName);
		return false;
	}
	catch (InternalCompilerError const& _exception)
	{
		cerr << "Internal compiler error during compilation:" << endl
			 << boost::diagnostic_information(_exception);
		return false;
	}
	catch (UnimplementedFeatureError const& _exception)
	{
		cerr << "Unimplemented feature:" << endl
			 << boost::diagnostic_information(_exception);
		return false;
	}
	catch (Error const& _error)
	{
		if (_error.type() == Error::Type::DocstringParsingError)
			cerr << "Documentation parsing error: " << *boost::get_error_info<errinfo_comment>(_error) << endl;
		else
			SourceReferenceFormatter::printExceptionInformation(cerr, _error, _error.typeName(), scannerFromSourceName);

		return false;
	}
	catch (Exception const& _exception)
	{
		cerr << "Exception during compilation: " << boost::diagnostic_information(_exception) << endl;
		return false;
	}
	catch (...)
	{
		cerr << "Unknown exception during compilation." << endl;
		return false;
	}

	return true;
}

void CommandLineInterface::handleCombinedJSON()
{
	if (!m_args.count(g_argCombinedJson))
		return;

	Json::Value output(Json::objectValue);

	output[g_strVersion] = ::dev::solidity::VersionString;
	set<string> requests;
	boost::split(requests, m_args[g_argCombinedJson].as<string>(), boost::is_any_of(","));
	vector<string> contracts = m_compiler->contractNames();

	if (!contracts.empty())
		output[g_strContracts] = Json::Value(Json::objectValue);
	for (string const& contractName: contracts)
	{
		Json::Value contractData(Json::objectValue);
		if (requests.count(g_strAbi))
			contractData[g_strAbi] = dev::jsonCompactPrint(m_compiler->interface(contractName));
		if (requests.count("metadata"))
			contractData["metadata"] = m_compiler->onChainMetadata(contractName);
		if (requests.count(g_strBinary))
			contractData[g_strBinary] = m_compiler->object(contractName).toHex();
		if (requests.count(g_strBinaryRuntime))
			contractData[g_strBinaryRuntime] = m_compiler->runtimeObject(contractName).toHex();
		if (requests.count(g_strCloneBinary))
			contractData[g_strCloneBinary] = m_compiler->cloneObject(contractName).toHex();
		if (requests.count(g_strOpcodes))
			contractData[g_strOpcodes] = solidity::disassemble(m_compiler->object(contractName).bytecode);
		if (requests.count(g_strAsm))
		{
			ostringstream unused;
			contractData[g_strAsm] = m_compiler->streamAssembly(unused, contractName, m_sourceCodes, true);
		}
		if (requests.count(g_strSrcMap))
		{
			auto map = m_compiler->sourceMapping(contractName);
			contractData[g_strSrcMap] = map ? *map : "";
		}
		if (requests.count(g_strSrcMapRuntime))
		{
			auto map = m_compiler->runtimeSourceMapping(contractName);
			contractData[g_strSrcMapRuntime] = map ? *map : "";
		}
		if (requests.count(g_strNatspecDev))
			contractData[g_strNatspecDev] = dev::jsonCompactPrint(m_compiler->metadata(contractName, DocumentationType::NatspecDev));
		if (requests.count(g_strNatspecUser))
			contractData[g_strNatspecUser] = dev::jsonCompactPrint(m_compiler->metadata(contractName, DocumentationType::NatspecUser));
		output[g_strContracts][contractName] = contractData;
	}

	bool needsSourceList = requests.count(g_strAst) || requests.count(g_strSrcMap) || requests.count(g_strSrcMapRuntime);
	if (needsSourceList)
	{
		// Indices into this array are used to abbreviate source names in source locations.
		output[g_strSourceList] = Json::Value(Json::arrayValue);

		for (auto const& source: m_compiler->sourceNames())
			output[g_strSourceList].append(source);
	}

	if (requests.count(g_strAst))
	{
		output[g_strSources] = Json::Value(Json::objectValue);
		for (auto const& sourceCode: m_sourceCodes)
		{
			ASTJsonConverter converter(m_compiler->ast(sourceCode.first), m_compiler->sourceIndices());
			output[g_strSources][sourceCode.first] = Json::Value(Json::objectValue);
			output[g_strSources][sourceCode.first]["AST"] = converter.json();
		}
	}
	cout << dev::jsonCompactPrint(output) << endl;
}

void CommandLineInterface::handleAst(string const& _argStr)
{
	string title;

	if (_argStr == g_argAst)
		title = "Syntax trees:";
	else if (_argStr == g_argAstJson)
		title = "JSON AST:";
	else
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Illegal argStr for AST"));

	// do we need AST output?
	if (m_args.count(_argStr))
	{
		vector<ASTNode const*> asts;
		for (auto const& sourceCode: m_sourceCodes)
			asts.push_back(&m_compiler->ast(sourceCode.first));
		map<ASTNode const*, eth::GasMeter::GasConsumption> gasCosts;
		if (m_compiler->runtimeAssemblyItems())
			gasCosts = GasEstimator::breakToStatementLevel(
				GasEstimator::structuralEstimation(*m_compiler->runtimeAssemblyItems(), asts),
				asts
			);

		if (m_args.count(g_argOutputDir))
		{
			for (auto const& sourceCode: m_sourceCodes)
			{
				stringstream data;
				string postfix = "";
				if (_argStr == g_argAst)
				{
					ASTPrinter printer(m_compiler->ast(sourceCode.first), sourceCode.second);
					printer.print(data);
				}
				else
				{
					ASTJsonConverter converter(m_compiler->ast(sourceCode.first));
					converter.print(data);
					postfix += "_json";
				}
				boost::filesystem::path path(sourceCode.first);
				createFile(path.filename().string() + postfix + ".ast", data.str());
			}
		}
		else
		{
			cout << title << endl << endl;
			for (auto const& sourceCode: m_sourceCodes)
			{
				cout << endl << "======= " << sourceCode.first << " =======" << endl;
				if (_argStr == g_argAst)
				{
					ASTPrinter printer(
						m_compiler->ast(sourceCode.first),
						sourceCode.second,
						gasCosts
					);
					printer.print(cout);
				}
				else
				{
					ASTJsonConverter converter(m_compiler->ast(sourceCode.first));
					converter.print(cout);
				}
			}
		}
	}
}

void CommandLineInterface::actOnInput()
{
	if (m_onlyAssemble)
		outputAssembly();
	else if (m_onlyLink)
		writeLinkedFiles();
	else
		outputCompilationResults();
}

bool CommandLineInterface::link()
{
	// Map from how the libraries will be named inside the bytecode to their addresses.
	map<string, h160> librariesReplacements;
	int const placeholderSize = 40; // 20 bytes or 40 hex characters
	for (auto const& library: m_libraries)
	{
		string const& name = library.first;
		// Library placeholders are 40 hex digits (20 bytes) that start and end with '__'.
		// This leaves 36 characters for the library name, while too short library names are
		// padded on the right with '_' and too long names are truncated.
		string replacement = "__";
		for (size_t i = 0; i < placeholderSize - 4; ++i)
			replacement.push_back(i < name.size() ? name[i] : '_');
		replacement += "__";
		librariesReplacements[replacement] = library.second;
	}
	for (auto& src: m_sourceCodes)
	{
		auto end = src.second.end();
		for (auto it = src.second.begin(); it != end;)
		{
			while (it != end && *it != '_') ++it;
			if (it == end) break;
			if (end - it < placeholderSize)
			{
				cerr << "Error in binary object file " << src.first << " at position " << (end - src.second.begin()) << endl;
				return false;
			}

			string name(it, it + placeholderSize);
			if (librariesReplacements.count(name))
			{
				string hexStr(toHex(librariesReplacements.at(name).asBytes()));
				copy(hexStr.begin(), hexStr.end(), it);
			}
			else
				cerr << "Reference \"" << name << "\" in file \"" << src.first << "\" still unresolved." << endl;
			it += placeholderSize;
		}
	}
	return true;
}

void CommandLineInterface::writeLinkedFiles()
{
	for (auto const& src: m_sourceCodes)
		if (src.first == g_stdinFileName)
			cout << src.second << endl;
		else
			writeFile(src.first, src.second);
}

bool CommandLineInterface::assemble()
{
	bool successful = true;
	map<string, shared_ptr<Scanner>> scanners;
	for (auto const& src: m_sourceCodes)
	{
		auto scanner = make_shared<Scanner>(CharStream(src.second), src.first);
		scanners[src.first] = scanner;
		if (!m_assemblyStacks[src.first].parse(scanner))
			successful = false;
		else
			//@TODO we should not just throw away the result here
			m_assemblyStacks[src.first].assemble();
	}
	for (auto const& stack: m_assemblyStacks)
	{
		for (auto const& error: stack.second.errors())
			SourceReferenceFormatter::printExceptionInformation(
				cerr,
				*error,
				(error->type() == Error::Type::Warning) ? "Warning" : "Error",
				[&](string const& _source) -> Scanner const& { return *scanners.at(_source); }
			);
		if (!Error::containsOnlyWarnings(stack.second.errors()))
			successful = false;
	}

	return successful;
}

void CommandLineInterface::outputAssembly()
{
	for (auto const& src: m_sourceCodes)
	{
		cout << endl << "======= " << src.first << " =======" << endl;
		eth::Assembly assembly = m_assemblyStacks[src.first].assemble();
		cout << assembly.assemble().toHex() << endl;
		assembly.stream(cout, "", m_sourceCodes);
	}
}

void CommandLineInterface::outputCompilationResults()
{
	handleCombinedJSON();

	// do we need AST output?
	handleAst(g_argAst);
	handleAst(g_argAstJson);

	vector<string> contracts = m_compiler->contractNames();
	for (string const& contract: contracts)
	{
		if (needsHumanTargetedStdout(m_args))
			cout << endl << "======= " << contract << " =======" << endl;

		// do we need EVM assembly?
		if (m_args.count(g_argAsm) || m_args.count(g_argAsmJson))
		{
			if (m_args.count(g_argOutputDir))
			{
				stringstream data;
				m_compiler->streamAssembly(data, contract, m_sourceCodes, m_args.count(g_argAsmJson));
				createFile(contract + (m_args.count(g_argAsmJson) ? "_evm.json" : ".evm"), data.str());
			}
			else
			{
				cout << "EVM assembly:" << endl;
				m_compiler->streamAssembly(cout, contract, m_sourceCodes, m_args.count(g_argAsmJson));
			}
		}

		if (m_args.count(g_argGas))
			handleGasEstimation(contract);

		handleBytecode(contract);
		handleSignatureHashes(contract);
		handleOnChainMetadata(contract);
		handleMeta(DocumentationType::ABIInterface, contract);
		handleMeta(DocumentationType::NatspecDev, contract);
		handleMeta(DocumentationType::NatspecUser, contract);
	} // end of contracts iteration

	handleFormal();
}

}
}
