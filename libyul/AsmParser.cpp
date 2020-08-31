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
 * @author Christian <c@ethdev.com>
 * @date 2016
 * Solidity inline assembly parser.
 */

#include <libyul/AsmParser.h>
#include <libyul/AST.h>
#include <libyul/Exceptions.h>
#include <liblangutil/Scanner.h>
#include <liblangutil/ErrorReporter.h>
#include <libsolutil/Common.h>

#include <boost/algorithm/string.hpp>

#include <cctype>
#include <algorithm>

using namespace std;
using namespace solidity;
using namespace solidity::util;
using namespace solidity::langutil;
using namespace solidity::yul;

unique_ptr<Block> Parser::parse(std::shared_ptr<Scanner> const& _scanner, bool _reuseScanner)
{
	m_recursionDepth = 0;

	_scanner->setScannerMode(ScannerKind::Yul);
	ScopeGuard resetScanner([&]{ _scanner->setScannerMode(ScannerKind::Solidity); });

	try
	{
		m_scanner = _scanner;
		auto block = make_unique<Block>(parseBlock());
		if (!_reuseScanner)
			expectToken(Token::EOS);
		return block;
	}
	catch (FatalError const&)
	{
		yulAssert(!m_errorReporter.errors().empty(), "Fatal error detected, but no error is reported.");
	}

	return nullptr;
}

Block Parser::parseBlock()
{
	RecursionGuard recursionGuard(*this);
	Block block = createWithLocation<Block>();
	expectToken(Token::LBrace);
	while (currentToken() != Token::RBrace)
		block.statements.emplace_back(parseStatement());
	block.location.end = currentLocation().end;
	advance();
	return block;
}

Statement Parser::parseStatement()
{
	RecursionGuard recursionGuard(*this);
	switch (currentToken())
	{
	case Token::Let:
		return parseVariableDeclaration();
	case Token::Function:
		return parseFunctionDefinition();
	case Token::LBrace:
		return parseBlock();
	case Token::If:
	{
		If _if = createWithLocation<If>();
		advance();
		_if.condition = make_unique<Expression>(parseExpression());
		_if.body = parseBlock();
		return Statement{move(_if)};
	}
	case Token::Switch:
	{
		Switch _switch = createWithLocation<Switch>();
		advance();
		_switch.expression = make_unique<Expression>(parseExpression());
		while (currentToken() == Token::Case)
			_switch.cases.emplace_back(parseCase());
		if (currentToken() == Token::Default)
			_switch.cases.emplace_back(parseCase());
		if (currentToken() == Token::Default)
			fatalParserError(6931_error, "Only one default case allowed.");
		else if (currentToken() == Token::Case)
			fatalParserError(4904_error, "Case not allowed after default case.");
		if (_switch.cases.empty())
			fatalParserError(2418_error, "Switch statement without any cases.");
		_switch.location.end = _switch.cases.back().body.location.end;
		return Statement{move(_switch)};
	}
	case Token::For:
		return parseForLoop();
	case Token::Break:
	{
		Statement stmt{createWithLocation<Break>()};
		checkBreakContinuePosition("break");
		advance();
		return stmt;
	}
	case Token::Continue:
	{
		Statement stmt{createWithLocation<Continue>()};
		checkBreakContinuePosition("continue");
		advance();
		return stmt;
	}
	case Token::Leave:
	{
		Statement stmt{createWithLocation<Leave>()};
		if (!m_insideFunction)
			m_errorReporter.syntaxError(8149_error, currentLocation(), "Keyword \"leave\" can only be used inside a function.");
		advance();
		return stmt;
	}
	default:
		break;
	}

	// Options left:
	// Expression/FunctionCall
	// Assignment
	ElementaryOperation elementary(parseLiteralOrIdentifier());

	switch (currentToken())
	{
	case Token::LParen:
	{
		Expression expr = parseCall(std::move(elementary));
		return ExpressionStatement{locationOf(expr), move(expr)};
	}
	case Token::Comma:
	case Token::AssemblyAssign:
	{
		Assignment assignment;
		assignment.location = locationOf(elementary);

		while (true)
		{
			if (!holds_alternative<Identifier>(elementary))
			{
				auto const token = currentToken() == Token::Comma ? "," : ":=";

				fatalParserError(
					2856_error,
					std::string("Variable name must precede \"") +
					token +
					"\"" +
					(currentToken() == Token::Comma ? " in multiple assignment." : " in assignment.")
				);
			}

			auto const& identifier = std::get<Identifier>(elementary);

			if (m_dialect.builtin(identifier.name))
				fatalParserError(6272_error, "Cannot assign to builtin function \"" + identifier.name.str() + "\".");

			assignment.variableNames.emplace_back(identifier);

			if (currentToken() != Token::Comma)
				break;

			expectToken(Token::Comma);

			elementary = parseLiteralOrIdentifier();
		}

		expectToken(Token::AssemblyAssign);

		assignment.value = make_unique<Expression>(parseExpression());
		assignment.location.end = locationOf(*assignment.value).end;

		return Statement{move(assignment)};
	}
	default:
		fatalParserError(6913_error, "Call or assignment expected.");
		break;
	}

	if (holds_alternative<Identifier>(elementary))
	{
		Identifier identifier = std::get<Identifier>(move(elementary));
		return ExpressionStatement{identifier.location, { move(identifier) }};
	}
	else if (holds_alternative<Literal>(elementary))
	{
		Literal literal = std::get<Literal>(move(elementary));
		return ExpressionStatement{literal.location, { move(literal) }};
	}
	else
	{
		yulAssert(false, "Invalid elementary operation.");
		return {};
	}
}

Case Parser::parseCase()
{
	RecursionGuard recursionGuard(*this);
	Case _case = createWithLocation<Case>();
	if (currentToken() == Token::Default)
		advance();
	else if (currentToken() == Token::Case)
	{
		advance();
		ElementaryOperation literal = parseLiteralOrIdentifier();
		if (!holds_alternative<Literal>(literal))
			fatalParserError(4805_error, "Literal expected.");
		_case.value = make_unique<Literal>(std::get<Literal>(std::move(literal)));
	}
	else
		yulAssert(false, "Case or default case expected.");
	_case.body = parseBlock();
	_case.location.end = _case.body.location.end;
	return _case;
}

ForLoop Parser::parseForLoop()
{
	RecursionGuard recursionGuard(*this);

	ForLoopComponent outerForLoopComponent = m_currentForLoopComponent;

	ForLoop forLoop = createWithLocation<ForLoop>();
	expectToken(Token::For);
	m_currentForLoopComponent = ForLoopComponent::ForLoopPre;
	forLoop.pre = parseBlock();
	m_currentForLoopComponent = ForLoopComponent::None;
	forLoop.condition = make_unique<Expression>(parseExpression());
	m_currentForLoopComponent = ForLoopComponent::ForLoopPost;
	forLoop.post = parseBlock();
	m_currentForLoopComponent = ForLoopComponent::ForLoopBody;
	forLoop.body = parseBlock();
	forLoop.location.end = forLoop.body.location.end;

	m_currentForLoopComponent = outerForLoopComponent;

	return forLoop;
}

Expression Parser::parseExpression()
{
	RecursionGuard recursionGuard(*this);

	ElementaryOperation operation = parseLiteralOrIdentifier();
	if (holds_alternative<Identifier>(operation))
	{
		if (currentToken() == Token::LParen)
			return parseCall(std::move(operation));
		return std::get<Identifier>(operation);
	}
	else
	{
		yulAssert(holds_alternative<Literal>(operation), "");
		return std::get<Literal>(operation);
	}
}

Parser::ElementaryOperation Parser::parseLiteralOrIdentifier()
{
	RecursionGuard recursionGuard(*this);
	ElementaryOperation ret;
	switch (currentToken())
	{
	case Token::Identifier:
	{
		ret = Identifier{currentLocation(), YulString{currentLiteral()}};
		advance();
		break;
	}
	case Token::StringLiteral:
	case Token::Number:
	case Token::TrueLiteral:
	case Token::FalseLiteral:
	{
		LiteralKind kind = LiteralKind::Number;
		switch (currentToken())
		{
		case Token::StringLiteral:
			kind = LiteralKind::String;
			break;
		case Token::Number:
			if (!isValidNumberLiteral(currentLiteral()))
				fatalParserError(4828_error, "Invalid number literal.");
			kind = LiteralKind::Number;
			break;
		case Token::TrueLiteral:
		case Token::FalseLiteral:
			kind = LiteralKind::Boolean;
			break;
		default:
			break;
		}

		Literal literal{
			currentLocation(),
			kind,
			YulString{currentLiteral()},
			kind == LiteralKind::Boolean ? m_dialect.boolType : m_dialect.defaultType
		};
		advance();
		if (currentToken() == Token::Colon)
		{
			expectToken(Token::Colon);
			literal.location.end = currentLocation().end;
			literal.type = expectAsmIdentifier();
		}

		ret = std::move(literal);
		break;
	}
	case Token::HexStringLiteral:
		fatalParserError(3772_error, "Hex literals are not valid in this context.");
		break;
	default:
		fatalParserError(1856_error, "Literal or identifier expected.");
	}
	return ret;
}

VariableDeclaration Parser::parseVariableDeclaration()
{
	RecursionGuard recursionGuard(*this);
	VariableDeclaration varDecl = createWithLocation<VariableDeclaration>();
	expectToken(Token::Let);
	while (true)
	{
		varDecl.variables.emplace_back(parseTypedName());
		if (currentToken() == Token::Comma)
			expectToken(Token::Comma);
		else
			break;
	}
	if (currentToken() == Token::AssemblyAssign)
	{
		expectToken(Token::AssemblyAssign);
		varDecl.value = make_unique<Expression>(parseExpression());
		varDecl.location.end = locationOf(*varDecl.value).end;
	}
	else
		varDecl.location.end = varDecl.variables.back().location.end;
	return varDecl;
}

FunctionDefinition Parser::parseFunctionDefinition()
{
	RecursionGuard recursionGuard(*this);

	if (m_currentForLoopComponent == ForLoopComponent::ForLoopPre)
		m_errorReporter.syntaxError(
			3441_error,
			currentLocation(),
			"Functions cannot be defined inside a for-loop init block."
		);

	ForLoopComponent outerForLoopComponent = m_currentForLoopComponent;
	m_currentForLoopComponent = ForLoopComponent::None;

	FunctionDefinition funDef = createWithLocation<FunctionDefinition>();
	expectToken(Token::Function);
	funDef.name = expectAsmIdentifier();
	expectToken(Token::LParen);
	while (currentToken() != Token::RParen)
	{
		funDef.parameters.emplace_back(parseTypedName());
		if (currentToken() == Token::RParen)
			break;
		expectToken(Token::Comma);
	}
	expectToken(Token::RParen);
	if (currentToken() == Token::RightArrow)
	{
		expectToken(Token::RightArrow);
		while (true)
		{
			funDef.returnVariables.emplace_back(parseTypedName());
			if (currentToken() == Token::LBrace)
				break;
			expectToken(Token::Comma);
		}
	}
	bool preInsideFunction = m_insideFunction;
	m_insideFunction = true;
	funDef.body = parseBlock();
	m_insideFunction = preInsideFunction;
	funDef.location.end = funDef.body.location.end;

	m_currentForLoopComponent = outerForLoopComponent;
	return funDef;
}

FunctionCall Parser::parseCall(Parser::ElementaryOperation&& _initialOp)
{
	RecursionGuard recursionGuard(*this);

	if (!holds_alternative<Identifier>(_initialOp))
		fatalParserError(9980_error, "Function name expected.");

	FunctionCall ret;
	ret.functionName = std::move(std::get<Identifier>(_initialOp));
	ret.location = ret.functionName.location;

	expectToken(Token::LParen);
	if (currentToken() != Token::RParen)
	{
		ret.arguments.emplace_back(parseExpression());
		while (currentToken() != Token::RParen)
		{
			expectToken(Token::Comma);
			ret.arguments.emplace_back(parseExpression());
		}
	}
	ret.location.end = currentLocation().end;
	expectToken(Token::RParen);
	return ret;
}

TypedName Parser::parseTypedName()
{
	RecursionGuard recursionGuard(*this);
	TypedName typedName = createWithLocation<TypedName>();
	typedName.name = expectAsmIdentifier();
	if (currentToken() == Token::Colon)
	{
		expectToken(Token::Colon);
		typedName.location.end = currentLocation().end;
		typedName.type = expectAsmIdentifier();
	}
	else
		typedName.type = m_dialect.defaultType;

	return typedName;
}

YulString Parser::expectAsmIdentifier()
{
	YulString name{currentLiteral()};
	if (currentToken() == Token::Identifier && m_dialect.builtin(name))
		fatalParserError(5568_error, "Cannot use builtin function name \"" + name.str() + "\" as identifier name.");
	// NOTE: We keep the expectation here to ensure the correct source location for the error above.
	expectToken(Token::Identifier);
	return name;
}

void Parser::checkBreakContinuePosition(string const& _which)
{
	switch (m_currentForLoopComponent)
	{
	case ForLoopComponent::None:
		m_errorReporter.syntaxError(2592_error, currentLocation(), "Keyword \"" + _which + "\" needs to be inside a for-loop body.");
		break;
	case ForLoopComponent::ForLoopPre:
		m_errorReporter.syntaxError(9615_error, currentLocation(), "Keyword \"" + _which + "\" in for-loop init block is not allowed.");
		break;
	case ForLoopComponent::ForLoopPost:
		m_errorReporter.syntaxError(2461_error, currentLocation(), "Keyword \"" + _which + "\" in for-loop post block is not allowed.");
		break;
	case ForLoopComponent::ForLoopBody:
		break;
	}
}

bool Parser::isValidNumberLiteral(string const& _literal)
{
	try
	{
		// Try to convert _literal to u256.
		[[maybe_unused]] auto tmp = u256(_literal);
	}
	catch (...)
	{
		return false;
	}
	if (boost::starts_with(_literal, "0x"))
		return true;
	else
		return _literal.find_first_not_of("0123456789") == string::npos;
}
