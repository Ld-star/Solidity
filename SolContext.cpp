/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @author Lefteris <lefteris@ethdev.com>
 * @date 2014
 * Solidity compiler context class.
 */
#include "SolContext.h"

#include <string>
#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>

#include "BuildInfo.h"
#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonIO.h>
#include <libevmcore/Instruction.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/Parser.h>
#include <libsolidity/ASTPrinter.h>
#include <libsolidity/NameAndTypeResolver.h>
#include <libsolidity/Exceptions.h>
#include <libsolidity/CompilerStack.h>
#include <libsolidity/SourceReferenceFormatter.h>

using namespace std;
namespace po = boost::program_options;

namespace dev
{
namespace solidity
{

static void version()
{
	cout << "solc, the solidity complier commandline interface " << dev::Version << endl
		 << "  by Christian <c@ethdev.com> and Lefteris <lefteris@ethdev.com>, (c) 2014." << endl
		 << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << endl;
	exit(0);
}

static inline bool argToStdout(po::variables_map const& _args, const char* _name)
{
	return _args.count(_name) && _args[_name].as<OutputType>() != OutputType::FILE;
}

static bool needStdout(po::variables_map const& _args)
{
	return argToStdout(_args, "abi") || argToStdout(_args, "natspec-user") || argToStdout(_args, "natspec-dev") ||
		   argToStdout(_args, "asm") || argToStdout(_args, "opcodes") || argToStdout(_args, "binary");
}

static inline bool outputToFile(OutputType type)
{
    return type == OutputType::FILE || type == OutputType::BOTH;
}

static inline bool outputToStdout(OutputType type)
{
    return type == OutputType::STDOUT || type == OutputType::BOTH;
}


static std::istream& operator>>(std::istream& _in, OutputType& io_output)
{
	std::string token;
	_in >> token;
	if (token == "stdout")
		io_output = OutputType::STDOUT;
	else if (token == "file")
		io_output = OutputType::FILE;
	else if (token == "both")
		io_output = OutputType::BOTH;
	else
		throw boost::program_options::invalid_option_value(token);
	return _in;
}

void SolContext::handleBytecode(string const& _argName,
                                string const& _title,
                                string const& _contract,
                                string const& _suffix)
{
	if (m_args.count(_argName))
	{
		auto choice = m_args[_argName].as<OutputType>();
		if (outputToStdout(choice))
		{
			cout << _title << endl;
			if (_suffix == "opcodes")
                ;
                // TODO: Figure out why after moving to own class ostream operator does not work for vector of bytes
				// cout << m_compiler.getBytecode(_contract) << endl;
			else
				cout << toHex(m_compiler.getBytecode(_contract)) << endl;
		}

		if (outputToFile(choice))
		{
			ofstream outFile(_contract + _suffix);
			if (_suffix == "opcodes")
                ;
                // TODO: Figure out why after moving to own class ostream operator does not work for vector of bytes
				// outFile << m_compiler.getBytecode(_contract);
			else
				outFile << toHex(m_compiler.getBytecode(_contract));
			outFile.close();
		}
	}
}

void SolContext::handleJson(DocumentationType _type,
                                   string const& _contract)
{
	std::string argName;
	std::string suffix;
	std::string title;
	switch(_type)
	{
	case DocumentationType::ABI_INTERFACE:
		argName = "abi";
		suffix = ".abi";
		title = "Contract ABI";
		break;
	case DocumentationType::NATSPEC_USER:
		argName = "natspec-user";
		suffix = ".docuser";
		title = "User Documentation";
		break;
	case DocumentationType::NATSPEC_DEV:
		argName = "natspec-dev";
		suffix = ".docdev";
		title = "Developer Documentation";
		break;
	default:
		// should never happen
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown documentation _type"));
	}

	if (m_args.count(argName))
	{
		auto choice = m_args[argName].as<OutputType>();
		if (outputToStdout(choice))
		{
			cout << title << endl;
			cout << m_compiler.getJsonDocumentation(_contract, _type);
		}

		if (outputToFile(choice))
		{
			ofstream outFile(_contract + suffix);
			outFile << m_compiler.getJsonDocumentation(_contract, _type);
			outFile.close();
		}
	}
}










bool SolContext::parseArguments(int argc, char** argv)
{
#define OUTPUT_TYPE_STR "Legal values:\n"               \
        "\tstdout: Print it to standard output\n"       \
        "\tfile: Print it to a file with same name\n"   \
        "\tboth: Print both to a file and the stdout\n"
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Show help message and exit")
        ("version", "Show version and exit")
        ("optimize", po::value<bool>()->default_value(false), "Optimize bytecode for size")
        ("input-file", po::value<vector<string>>(), "input file")
        ("ast", po::value<OutputType>(),
         "Request to output the AST of the contract. " OUTPUT_TYPE_STR)
        ("asm", po::value<OutputType>(),
         "Request to output the EVM assembly of the contract. "  OUTPUT_TYPE_STR)
        ("opcodes", po::value<OutputType>(),
         "Request to output the Opcodes of the contract. "  OUTPUT_TYPE_STR)
        ("binary", po::value<OutputType>(),
         "Request to output the contract in binary (hexadecimal). "  OUTPUT_TYPE_STR)
        ("abi", po::value<OutputType>(),
         "Request to output the contract's ABI interface. "  OUTPUT_TYPE_STR)
        ("natspec-user", po::value<OutputType>(),
         "Request to output the contract's Natspec user documentation. "  OUTPUT_TYPE_STR)
        ("natspec-dev", po::value<OutputType>(),
         "Request to output the contract's Natspec developer documentation. "  OUTPUT_TYPE_STR);
#undef OUTPUT_TYPE_STR

    // All positional options should be interpreted as input files
    po::positional_options_description p;
    p.add("input-file", -1);

    // parse the compiler arguments
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).allow_unregistered().run(), m_args);
    }
    catch (po::error const& exception)
    {
        cout << exception.what() << endl;
        return false;
    }
    po::notify(m_args);

    if (m_args.count("help"))
    {
        cout << desc;
        return false;
    }

    if (m_args.count("version"))
    {
        version();
        return false;
    }

    return true;
}

bool SolContext::processInput()
{
	if (!m_args.count("input-file"))
	{
		string s;
		while (!cin.eof())
		{
			getline(cin, s);
			m_sourceCodes["<stdin>"].append(s);
		}
	}
	else
		for (string const& infile: m_args["input-file"].as<vector<string>>())
			m_sourceCodes[infile] = asString(dev::contents(infile));

	try
	{
		for (auto const& sourceCode: m_sourceCodes)
			m_compiler.addSource(sourceCode.first, sourceCode.second);
        // TODO: Perhaps we should not compile unless requested
		m_compiler.compile(m_args["optimize"].as<bool>());
	}
	catch (ParserError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Parser error", m_compiler);
		return false;
	}
	catch (DeclarationError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Declaration error", m_compiler);
		return false;
	}
	catch (TypeError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Type error", m_compiler);
		return false;
	}
	catch (CompilerError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Compiler error", m_compiler);
		return false;
	}
	catch (InternalCompilerError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Internal compiler error", m_compiler);
		return false;
	}
	catch (Exception const& exception)
	{
		cerr << "Exception during compilation: " << boost::diagnostic_information(exception) << endl;
		return false;
	}
	catch (...)
	{
		cerr << "Unknown exception during compilation." << endl;
		return false;
	}

    return true;
}

void SolContext::actOnInput()
{
	// do we need AST output?
	if (m_args.count("ast"))
	{
		auto choice = m_args["ast"].as<OutputType>();
		if (outputToStdout(choice))
		{
			cout << "Syntax trees:" << endl << endl;
			for (auto const& sourceCode: m_sourceCodes)
			{
				cout << endl << "======= " << sourceCode.first << " =======" << endl;
				ASTPrinter printer(m_compiler.getAST(sourceCode.first), sourceCode.second);
				printer.print(cout);
			}
		}

		if (outputToFile(choice))
		{
			for (auto const& sourceCode: m_sourceCodes)
			{
				boost::filesystem::path p(sourceCode.first);
				ofstream outFile(p.stem().string() + ".ast");
				ASTPrinter printer(m_compiler.getAST(sourceCode.first), sourceCode.second);
				printer.print(outFile);
				outFile.close();
			}
		}
	}

	vector<string> contracts = m_compiler.getContractNames();
	for (string const& contract: contracts)
	{
		if (needStdout(m_args))
			cout << endl << "======= " << contract << " =======" << endl;

		// do we need EVM assembly?
		if (m_args.count("asm"))
		{
			auto choice = m_args["asm"].as<OutputType>();
            if (outputToStdout(choice))
			{
				cout << "EVM assembly:" << endl;
				m_compiler.streamAssembly(cout, contract);
			}

            if (outputToFile(choice))
			{
				ofstream outFile(contract + ".evm");
				m_compiler.streamAssembly(outFile, contract);
				outFile.close();
			}
		}

		handleBytecode("opcodes", "Opcodes:", contract, ".opcodes");
		handleBytecode("binary", "Binary:", contract, ".binary");
		handleJson(DocumentationType::ABI_INTERFACE, contract);
		handleJson(DocumentationType::NATSPEC_DEV, contract);
		handleJson(DocumentationType::NATSPEC_USER, contract);
	} // end of contracts iteration
}

}
}
