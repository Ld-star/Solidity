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
 * @date 2015
 * Converts the AST into json format
 */

#include <libsolidity/ast/ASTJsonConverter.h>
#include <boost/algorithm/string/join.hpp>
#include <libdevcore/UTF8.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/interface/Exceptions.h>
#include <libsolidity/inlineasm/AsmData.h>
#include <libsolidity/inlineasm/AsmPrinter.h>

using namespace std;

namespace dev
{
namespace solidity
{

ASTJsonConverter::ASTJsonConverter(bool _legacy, map<string, unsigned> _sourceIndices):
	m_legacy(_legacy),
	m_sourceIndices(_sourceIndices)
{
}


void ASTJsonConverter::setJsonNode(
	ASTNode const& _node,
	string const& _nodeName,
	initializer_list<pair<string, Json::Value>>&& _attributes
)
{
	ASTJsonConverter::setJsonNode(
		_node,
		_nodeName,
		std::vector<pair<string, Json::Value>>(std::move(_attributes))
	);
}

void ASTJsonConverter::setJsonNode(
	ASTNode const& _node,
	string const& _nodeType,
	std::vector<pair<string, Json::Value>>&& _attributes
)
{
	m_currentValue = Json::objectValue;
	m_currentValue["id"] = nodeId(_node);
	m_currentValue["src"] = sourceLocationToString(_node.location());
	if (!m_legacy)
	{
		m_currentValue["nodeType"] = _nodeType;
		for (auto& e: _attributes)
			m_currentValue[e.first] = std::move(e.second);
	}
	else
	{
		m_currentValue["name"] = _nodeType;
		Json::Value attrs(Json::objectValue);
		if (
			//these nodeTypes need to have a children-node even if it is empty
			(_nodeType == "VariableDeclaration") ||
			(_nodeType == "ParameterList") ||
			(_nodeType == "Block") ||
			(_nodeType == "InlineAssembly") ||
			(_nodeType == "Throw")
		)
		{
			Json::Value children(Json::arrayValue);
			m_currentValue["children"] = children;
		}

		for (auto& e: _attributes)
		{
			if (
				(!e.second.isNull()) &&
					(
					(e.second.isObject() && e.second.isMember("name")) ||
					(e.second.isArray() && e.second[0].isObject() && e.second[0].isMember("name")) ||
					(e.first == "declarations") // (in the case (_,x)= ... there's a nullpointer at [0]
					)
			)
			{
				if (e.second.isObject())
					m_currentValue["children"].append(std::move(e.second));
				if (e.second.isArray())
					for (auto& child : e.second)
						if (!child.isNull())
							m_currentValue["children"].append(std::move(child));
			}
			else
			{
				if (e.first == "typeDescriptions")
					attrs["type"] = Json::Value(e.second["typeString"]);
				else
					attrs[e.first] = std::move(e.second);
			}
		}
		if (!attrs.empty())
			m_currentValue["attributes"] = std::move(attrs);
	}
}

string ASTJsonConverter::sourceLocationToString(SourceLocation const& _location) const
{
	int sourceIndex{-1};
	if (_location.sourceName && m_sourceIndices.count(*_location.sourceName))
		sourceIndex = m_sourceIndices.at(*_location.sourceName);
	int length = -1;
	if (_location.start >= 0 && _location.end >= 0)
		length = _location.end - _location.start;
	return std::to_string(_location.start) + ":" + std::to_string(length) + ":" + std::to_string(sourceIndex);
}

string ASTJsonConverter::namePathToString(std::vector<ASTString> const& _namePath) const
{
	return boost::algorithm::join(_namePath, ".");
}

Json::Value ASTJsonConverter::typePointerToJson(TypePointer _tp)
{
	Json::Value typeDescriptions(Json::objectValue);
	typeDescriptions["typeString"] = _tp ? Json::Value(_tp->toString()) : Json::nullValue;
	typeDescriptions["typeIdentifier"] = _tp ? Json::Value(_tp->identifier()) : Json::nullValue;
	return typeDescriptions;

}
Json::Value ASTJsonConverter::typePointerToJson(std::shared_ptr<std::vector<TypePointer>> _tps)
{
	if (_tps)
	{
		Json::Value arguments(Json::arrayValue);
		for (auto const& tp : *_tps)
			arguments.append(typePointerToJson(tp));
		return arguments;
	}
	else
		return Json::nullValue;
}

void ASTJsonConverter::appendExpressionAttributes(
	std::vector<pair<string, Json::Value>> * _attributes,
	ExpressionAnnotation const& _annotation
)
{
	std::vector<pair<string, Json::Value>> exprAttributes = {
	make_pair("typeDescriptions", typePointerToJson(_annotation.type)),
	    make_pair("isConstant", _annotation.isConstant),
	    make_pair("isPure", _annotation.isPure),
	    make_pair("isLValue", _annotation.isLValue),
	    make_pair("lValueRequested", _annotation.lValueRequested),
	    make_pair("argumentTypes", typePointerToJson(_annotation.argumentTypes))
	};
	_attributes->insert(_attributes->end(), exprAttributes.begin(), exprAttributes.end());
}


void ASTJsonConverter::print(ostream& _stream, ASTNode const& _node)
{
	_stream << toJson(_node);
}

Json::Value ASTJsonConverter::toJson(ASTNode const& _node)
{
	_node.accept(*this);
	return std::move(m_currentValue);
}

bool ASTJsonConverter::visit(SourceUnit const& _node)
{
	Json::Value exportedSymbols = Json::objectValue;
	for (auto const& sym: _node.annotation().exportedSymbols)
	{
		exportedSymbols[sym.first] = Json::arrayValue;
		for (Declaration const* overload: sym.second)
			exportedSymbols[sym.first].append(nodeId(*overload));
	}
	setJsonNode(
		_node,
		"SourceUnit",
		{
			make_pair("absolutePath", _node.annotation().path),
			make_pair("exportedSymbols", move(exportedSymbols)),
			make_pair("nodes", toJson(_node.nodes()))
		}
	);
	return false;
}

bool ASTJsonConverter::visit(PragmaDirective const& _node)
{
	Json::Value literals(Json::arrayValue);
	for (auto const& literal: _node.literals())
		literals.append(literal);
	setJsonNode( _node, "PragmaDirective", {
		make_pair("literals", std::move(literals))
	});
	return false;
}

bool ASTJsonConverter::visit(ImportDirective const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("file", _node.path()),
		make_pair("absolutePath", _node.annotation().absolutePath),
		make_pair("SourceUnit", nodeId(*_node.annotation().sourceUnit)),
		make_pair("scope", idOrNull(_node.scope()))
	};
	attributes.push_back(make_pair("unitAlias", _node.name()));
	Json::Value symbolAliases(Json::arrayValue);
	for (auto const& symbolAlias: _node.symbolAliases())
	{
		Json::Value tuple(Json::objectValue);
		solAssert(symbolAlias.first, "");
		tuple["foreign"] = nodeId(*symbolAlias.first);
		tuple["local"] =  symbolAlias.second ? Json::Value(*symbolAlias.second) : Json::nullValue;
		symbolAliases.append(tuple);
	}
	attributes.push_back( make_pair("symbolAliases", std::move(symbolAliases)));
	setJsonNode(_node, "ImportDirective", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(ContractDefinition const& _node)
{
	Json::Value linearizedBaseContracts(Json::arrayValue);
	for (auto const& baseContract: _node.annotation().linearizedBaseContracts)
		linearizedBaseContracts.append(nodeId(*baseContract));
	Json::Value contractDependencies(Json::arrayValue);
	for (auto const& dependentContract: _node.annotation().contractDependencies)
		contractDependencies.append(nodeId(*dependentContract));
	setJsonNode(_node, "ContractDefinition", {
		make_pair("name", _node.name()),
		make_pair("isLibrary", _node.isLibrary()),
		make_pair("fullyImplemented", _node.annotation().isFullyImplemented),
		make_pair("linearizedBaseContracts", std::move(linearizedBaseContracts)),
		make_pair("baseContracts", toJson(_node.baseContracts())),
		make_pair("contractDependencies", std::move(contractDependencies)),
		make_pair("nodes", toJson(_node.subNodes())),
		make_pair("scope", idOrNull(_node.scope()))
	});
	return false;
}

bool ASTJsonConverter::visit(InheritanceSpecifier const& _node)
{
	setJsonNode(_node, "InheritanceSpecifier", {
		make_pair("baseName", toJson(_node.name())),
		make_pair("arguments", toJson(_node.arguments()))
	});
	return false;
}

bool ASTJsonConverter::visit(UsingForDirective const& _node)
{
	setJsonNode(_node, "UsingForDirective", {
		make_pair("libraryNames", toJson(_node.libraryName())),
		make_pair("typeName", _node.typeName() ? toJson(*_node.typeName()) : Json::nullValue)
	});
	return false;
}

bool ASTJsonConverter::visit(StructDefinition const& _node)
{
	setJsonNode(_node, "StructDefinition", {
		make_pair("name", _node.name()),
		make_pair("visibility", visibility(_node.visibility())),
		make_pair("canonicalName", _node.annotation().canonicalName),
		make_pair("members", toJson(_node.members())),
		make_pair("scope", idOrNull(_node.scope()))
	});
	return false;
}

bool ASTJsonConverter::visit(EnumDefinition const& _node)
{
	setJsonNode(_node, "EnumDefinition", {
		make_pair("name", _node.name()),
		make_pair("canonicalName", _node.annotation().canonicalName),
		make_pair("members", toJson(_node.members()))
	});
	return false;
}

bool ASTJsonConverter::visit(EnumValue const& _node)
{
	setJsonNode(_node, "EnumValue", {
		make_pair("name", _node.name())
	});
	return false;
}

bool ASTJsonConverter::visit(ParameterList const& _node)
{
	setJsonNode(_node, "ParameterList", {
		make_pair("parameters", toJson(_node.parameters()))
	});
	return false;
}

bool ASTJsonConverter::visit(FunctionDefinition const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("name", _node.name()),
		make_pair("constant", _node.isDeclaredConst()),
		make_pair("payable", _node.isPayable()),
		make_pair("visibility", visibility(_node.visibility())),
		make_pair("parameters",	toJson(_node.parameterList())),
		make_pair("isConstructor", _node.isConstructor()),
		make_pair("returnParameters", toJson((*_node.returnParameterList()))),
		make_pair("modifiers", toJson(_node.modifiers())),
		make_pair("body", _node.isImplemented() ? toJson(_node.body()) : Json::nullValue),
		make_pair("isImplemented", _node.isImplemented()),
		make_pair("scope", idOrNull(_node.scope()))
	};
	setJsonNode(_node, "FunctionDefinition", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(VariableDeclaration const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("name", _node.name()),
		make_pair("typeName", toJsonOrNull(_node.typeName())),
		make_pair("constant", _node.isConstant()),
		make_pair("storageLocation", location(_node.referenceLocation())),
		make_pair("visibility", visibility(_node.visibility())),
		make_pair("value", _node.value() ? toJson(*_node.value()) : Json::nullValue),
		make_pair("scope", idOrNull(_node.scope())),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type))
	};
	if (m_inEvent)
		attributes.push_back(make_pair("indexed", _node.isIndexed()));
	setJsonNode(_node, "VariableDeclaration", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(ModifierDefinition const& _node)
{
	setJsonNode(_node, "ModifierDefinition", {
		make_pair("name", _node.name()),
		make_pair("visibility", visibility(_node.visibility())),
		make_pair("parameters", toJson(_node.parameterList())),
		make_pair("body", toJson(_node.body()))
	});
	return false;
}

bool ASTJsonConverter::visit(ModifierInvocation const& _node)
{
	setJsonNode(_node, "ModifierInvocation", {
		make_pair("modifierName", toJson(*_node.name())),
		make_pair("arguments", toJson(_node.arguments()))
	});
	return false;
}

bool ASTJsonConverter::visit(TypeName const&)
{
	return false;
}

bool ASTJsonConverter::visit(EventDefinition const& _node)
{
	m_inEvent = true;
	setJsonNode(_node, "EventDefinition", {
		make_pair("name", _node.name()),
		make_pair("parameters", toJson(_node.parameterList())),
		make_pair("isAnonymous", _node.isAnonymous())
	});
	return false;
}

bool ASTJsonConverter::visit(ElementaryTypeName const& _node)
{
	setJsonNode(_node, "ElementaryTypeName", {
		make_pair("name", _node.typeName().toString()),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type))
	});
	return false;
}

bool ASTJsonConverter::visit(UserDefinedTypeName const& _node)
{
	setJsonNode(_node, "UserDefinedTypeName", {
		make_pair("name", namePathToString(_node.namePath())),
		make_pair("referencedDeclaration", idOrNull(_node.annotation().referencedDeclaration)),
		make_pair("contractScope", idOrNull(_node.annotation().contractScope)),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type))
	});
	return false;
}

bool ASTJsonConverter::visit(FunctionTypeName const& _node)
{
	setJsonNode(_node, "FunctionTypeName", {
		make_pair("payable", _node.isPayable()),
		make_pair("visibility", visibility(_node.visibility())),
		make_pair("constant", _node.isDeclaredConst()),
		make_pair("parameterTypes", toJson(_node.parameterTypes())),
		make_pair("returnParameterTypes", toJson(_node.returnParameterTypes())),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type))
	});
	return false;
}

bool ASTJsonConverter::visit(Mapping const& _node)
{
	setJsonNode(_node, "Mapping", {
		make_pair("keyType", toJson(_node.keyType())),
		make_pair("valueType", toJson(_node.valueType())),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type))
	});
	return false;
}

bool ASTJsonConverter::visit(ArrayTypeName const& _node)
{
	setJsonNode(_node, "ArrayTypeName", {
		make_pair("baseType", toJson(_node.baseType())),
		make_pair("length", toJsonOrNull(_node.length())),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type))
	});
	return false;
}

bool ASTJsonConverter::visit(InlineAssembly const& _node)
{
	std::map<assembly::Identifier const*, Declaration const*>::iterator it;
	Json::Value externalReferences(Json::arrayValue);
	for (
		it = _node.annotation().externalReferences.begin();
		it != _node.annotation().externalReferences.end();
		it++
	)
	{
		if (it->first && it->second)
		{
			Json::Value tuple(Json::objectValue);
			tuple[it->first->name] = nodeId(*it->second);
			externalReferences.append(tuple);
		}
	}
	setJsonNode(_node, "InlineAssembly", {
		make_pair("operations", Json::Value(assembly::AsmPrinter()(_node.operations()))),
		make_pair("externalReferences", std::move(externalReferences))
	});
	return false;
}

bool ASTJsonConverter::visit(Block const& _node)
{
	setJsonNode(_node, "Block", {
		make_pair("statements", toJson(_node.statements()))
	});
	return false;
}

bool ASTJsonConverter::visit(PlaceholderStatement const& _node)
{
	setJsonNode(_node, "PlaceholderStatement", {});
	return false;
}

bool ASTJsonConverter::visit(IfStatement const& _node)
{
	setJsonNode(_node, "IfStatement", {
		make_pair("condition", toJson(_node.condition())),
		make_pair("trueBody", toJson(_node.trueStatement())),
		make_pair("falseBody", toJsonOrNull(_node.falseStatement()))
	});
	return false;
}

bool ASTJsonConverter::visit(WhileStatement const& _node)
{
	setJsonNode(
		_node,
		_node.isDoWhile() ? "DoWhileStatement" : "WhileStatement",
		{
			make_pair("condition", toJson(_node.condition())),
			make_pair("body", toJson(_node.body()))
		}
	);
	return false;
}

bool ASTJsonConverter::visit(ForStatement const& _node)
{
	setJsonNode(_node, "ForStatement", {
		make_pair("initExpression", toJsonOrNull(_node.initializationExpression())),
		make_pair("condition", toJsonOrNull(_node.condition())),
		make_pair("loopExpression", toJsonOrNull(_node.loopExpression())),
		make_pair("body", toJson(_node.body()))
	});
	return false;
}

bool ASTJsonConverter::visit(Continue const& _node)
{
	setJsonNode(_node, "Continue", {});
	return false;
}

bool ASTJsonConverter::visit(Break const& _node)
{
	setJsonNode(_node, "Break", {});
	return false;
}

bool ASTJsonConverter::visit(Return const& _node)
{
	setJsonNode(_node, "Return", {
		make_pair("expression", toJsonOrNull(_node.expression())),
		make_pair("functionReturnParameters", idOrNull(_node.annotation().functionReturnParameters))
	});
	return false;
}

bool ASTJsonConverter::visit(Throw const& _node)
{
	setJsonNode(_node, "Throw", {});;
	return false;
}

bool ASTJsonConverter::visit(VariableDeclarationStatement const& _node)
{
	Json::Value varDecs(Json::arrayValue);
	for (auto const& v: _node.annotation().assignments)
	{
		varDecs.append(idOrNull(v));
	}
	setJsonNode(_node, "VariableDeclarationStatement", {
		make_pair("assignments", std::move(varDecs)),
		make_pair("declarations", toJson(_node.declarations())),
		make_pair("initialValue", toJsonOrNull(_node.initialValue()))
	});
	return false;
}

bool ASTJsonConverter::visit(ExpressionStatement const& _node)
{
	setJsonNode(_node, "ExpressionStatement", {
		make_pair("expression", toJson(_node.expression()))
	});
	return false;
}

bool ASTJsonConverter::visit(Conditional const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("condition", toJson(_node.condition())),
		make_pair("trueExpression", toJson(_node.trueExpression())),
		make_pair("falseExpression", toJson(_node.falseExpression()))
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "Conditional", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(Assignment const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("operator", Token::toString(_node.assignmentOperator())),
		make_pair("leftHandSide", toJson(_node.leftHandSide())),
		make_pair("rightHandSide", toJson(_node.rightHandSide()))
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode( _node, "Assignment", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(TupleExpression const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("isInlineArray", Json::Value(_node.isInlineArray())),
		make_pair("components", toJson(_node.components())),
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "TupleExpression", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(UnaryOperation const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("prefix", _node.isPrefixOperation()),
		make_pair("operator", Token::toString(_node.getOperator())),
		make_pair("subExpression", toJson(_node.subExpression()))
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "UnaryOperation", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(BinaryOperation const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("operator", Token::toString(_node.getOperator())),
		make_pair("leftExpression", toJson(_node.leftExpression())),
		make_pair("rightExpression", toJson(_node.rightExpression())),
		make_pair("commonType", typePointerToJson(_node.annotation().commonType)),
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "BinaryOperation", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(FunctionCall const& _node)
{
	Json::Value names(Json::arrayValue);
	for (auto const& name: _node.names())
		names.append(Json::Value(*name));
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("type_conversion", _node.annotation().isTypeConversion),
		make_pair("isStructContstructorCall", _node.annotation().isStructConstructorCall),
		make_pair("expression", toJson(_node.expression())),
		make_pair("names", std::move(names)),
		make_pair("arguments", toJson(_node.arguments()))
		};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "FunctionCall", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(NewExpression const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("typeName", toJson(_node.typeName()))
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "NewExpression", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(MemberAccess const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("member_name", _node.memberName()),
		make_pair("expression", toJson(_node.expression())),
		make_pair("referencedDeclaration", idOrNull(_node.annotation().referencedDeclaration)),
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "MemberAccess", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(IndexAccess const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("baseExpression", toJson(_node.baseExpression())),
		make_pair("indexExpression", toJsonOrNull(_node.indexExpression())),
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "IndexAccess", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(Identifier const& _node)
{
	Json::Value overloads(Json::arrayValue);
	for (auto const& dec: _node.annotation().overloadedDeclarations)
		overloads.append(nodeId(*dec));
	setJsonNode(_node, "Identifier", {
		make_pair("value", _node.name()),
		make_pair("referencedDeclaration", idOrNull(_node.annotation().referencedDeclaration)),
		make_pair("overloadedDeclarations", overloads),
		make_pair("typeDescriptions", typePointerToJson(_node.annotation().type)),
		make_pair("argumentTypes", typePointerToJson(_node.annotation().argumentTypes))
	});
	return false;
}

bool ASTJsonConverter::visit(ElementaryTypeNameExpression const& _node)
{
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("value", _node.typeName().toString())
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "ElementaryTypeNameExpression", std::move(attributes));
	return false;
}

bool ASTJsonConverter::visit(Literal const& _node)
{
	char const* tokenString = Token::toString(_node.token());
	Json::Value value{_node.value()};
	if (!dev::validateUTF8(_node.value()))
		value = Json::nullValue;
	Token::Value subdenomination = Token::Value(_node.subDenomination());
	std::vector<pair<string, Json::Value>> attributes = {
		make_pair("token", tokenString ? tokenString : Json::Value()),
		make_pair("value", value),
		make_pair("hexvalue", toHex(_node.value())),
		make_pair(
			"subdenomination",
			subdenomination == Token::Illegal ?
			Json::nullValue :
			Json::Value{Token::toString(subdenomination)}
		)
	};
	appendExpressionAttributes(&attributes, _node.annotation());
	setJsonNode(_node, "Literal", std::move(attributes));
	return false;
}


void ASTJsonConverter::endVisit(EventDefinition const&)
{
	m_inEvent = false;
}

string ASTJsonConverter::visibility(Declaration::Visibility const& _visibility)
{
	switch (_visibility)
	{
	case Declaration::Visibility::Private:
		return "private";
	case Declaration::Visibility::Internal:
		return "internal";
	case Declaration::Visibility::Public:
		return "public";
	case Declaration::Visibility::External:
		return "external";
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown declaration visibility."));
	}
}

string ASTJsonConverter::location(VariableDeclaration::Location _location)
{
	switch (_location)
	{
	case VariableDeclaration::Location::Default:
		return "default";
	case VariableDeclaration::Location::Storage:
		return "storage";
	case VariableDeclaration::Location::Memory:
		return "memory";
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown declaration location."));
	}
}

string ASTJsonConverter::type(Expression const& _expression)
{
	return _expression.annotation().type ? _expression.annotation().type->toString() : "Unknown";
}

string ASTJsonConverter::type(VariableDeclaration const& _varDecl)
{
	return _varDecl.annotation().type ? _varDecl.annotation().type->toString() : "Unknown";
}

}
}
