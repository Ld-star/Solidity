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
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Solidity commandline compiler.
 */

#include <string>
#include <iostream>

#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonIO.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/Parser.h>
#include <libsolidity/ASTPrinter.h>
#include <libsolidity/NameAndTypeResolver.h>
#include <libsolidity/Exceptions.h>
#include <libsolidity/Compiler.h>
#include <libsolidity/SourceReferenceFormatter.h>

using namespace std;
using namespace dev;
using namespace solidity;

void help()
{
	cout << "Usage solc [OPTIONS] <file>" << endl
		 << "Options:" << endl
		 << "    -h,--help  Show this help message and exit." << endl
		 << "    -V,--version  Show the version and exit." << endl;
	exit(0);
}

void version()
{
	cout << "solc, the solidity complier commandline interface " << dev::Version << endl
		 << "  by Christian <c@ethdev.com>, (c) 2014." << endl
		 << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << endl;
	exit(0);
}


/// Helper class that extracts the first expression in an AST.
class FirstExpressionExtractor: private ASTVisitor
{
public:
	FirstExpressionExtractor(ASTNode& _node): m_expression(nullptr) { _node.accept(*this); }
	Expression* getExpression() const { return m_expression; }
private:
	virtual bool visit(Expression& _expression) override { return checkExpression(_expression); }
	virtual bool visit(Assignment& _expression) override { return checkExpression(_expression); }
	virtual bool visit(UnaryOperation& _expression) override { return checkExpression(_expression); }
	virtual bool visit(BinaryOperation& _expression) override { return checkExpression(_expression); }
	virtual bool visit(FunctionCall& _expression) override { return checkExpression(_expression); }
	virtual bool visit(MemberAccess& _expression) override { return checkExpression(_expression); }
	virtual bool visit(IndexAccess& _expression) override { return checkExpression(_expression); }
	virtual bool visit(PrimaryExpression& _expression) override { return checkExpression(_expression); }
	virtual bool visit(Identifier& _expression) override { return checkExpression(_expression); }
	virtual bool visit(ElementaryTypeNameExpression& _expression) override { return checkExpression(_expression); }
	virtual bool visit(Literal& _expression) override { return checkExpression(_expression); }
	bool checkExpression(Expression& _expression)
	{
		if (m_expression == nullptr)
			m_expression = &_expression;
		return false;
	}
private:
	Expression* m_expression;
};

int main(int argc, char** argv)
{
	string infile;
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "-V" || arg == "--version")
			version();
		else
			infile = argv[i];
	}
	string sourceCode;
	if (infile.empty())
	{
		string s;
		while (!cin.eof())
		{
			getline(cin, s);
			sourceCode.append(s);
		}
	}
	else
		sourceCode = asString(dev::contents(infile));

	ASTPointer<ContractDefinition> ast;
	shared_ptr<Scanner> scanner = make_shared<Scanner>(CharStream(sourceCode));
	Parser parser;
	try
	{
		ast = parser.parse(scanner);
	}
	catch (ParserError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Parser error", *scanner);
		return -1;
	}

	NameAndTypeResolver resolver;
	try
	{
		resolver.resolveNamesAndTypes(*ast.get());
	}
	catch (DeclarationError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Declaration error", *scanner);
		return -1;
	}
	catch (TypeError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Type error", *scanner);
		return -1;
	}

	cout << "Syntax tree for the contract:" << endl;
	dev::solidity::ASTPrinter printer(ast, sourceCode);
	printer.print(cout);

	FirstExpressionExtractor extractor(*ast);

	CompilerContext context;
	ExpressionCompiler compiler(context);
	try
	{
		compiler.compile(*extractor.getExpression());
	}
	catch (CompilerError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Compiler error", *scanner);
		return -1;
	}

	bytes instructions = compiler.getAssembledBytecode();

	cout << "Bytecode for the first expression: " << endl;
	cout << eth::disassemble(instructions) << endl;

	return 0;
}
