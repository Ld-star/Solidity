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

#include <libsolidity/analysis/ImmutableValidator.h>

#include <libsolutil/CommonData.h>

#include <boost/range/adaptor/reversed.hpp>

using namespace solidity::frontend;

void ImmutableValidator::analyze()
{
	m_inConstructionContext = true;

	auto linearizedContracts = m_currentContract.annotation().linearizedBaseContracts | boost::adaptors::reversed;

	for (ContractDefinition const* contract: linearizedContracts)
		for (VariableDeclaration const* stateVar: contract->stateVariables())
			if (stateVar->value())
			{
				stateVar->value()->accept(*this);
				solAssert(m_initializedStateVariables.emplace(stateVar).second, "");
			}

	for (ContractDefinition const* contract: linearizedContracts)
		if (contract->constructor())
			visitCallableIfNew(*contract->constructor());

	for (ContractDefinition const* contract: linearizedContracts)
		for (std::shared_ptr<InheritanceSpecifier> const inheritSpec: contract->baseContracts())
			if (auto args = inheritSpec->arguments())
				ASTNode::listAccept(*args, *this);

	m_inConstructionContext = false;

	for (ContractDefinition const* contract: linearizedContracts)
	{
		for (auto funcDef: contract->definedFunctions())
			visitCallableIfNew(*funcDef);

		for (auto modDef: contract->functionModifiers())
			visitCallableIfNew(*modDef);
	}

	checkAllVariablesInitialized(m_currentContract.location());
}

bool ImmutableValidator::visit(FunctionDefinition const& _functionDefinition)
{
	return analyseCallable(_functionDefinition);
}

bool ImmutableValidator::visit(ModifierDefinition const& _modifierDefinition)
{
	return analyseCallable(_modifierDefinition);
}

bool ImmutableValidator::visit(MemberAccess const& _memberAccess)
{
	_memberAccess.expression().accept(*this);

	if (auto varDecl = dynamic_cast<VariableDeclaration const*>(_memberAccess.annotation().referencedDeclaration))
		analyseVariableReference(*varDecl, _memberAccess);
	else if (auto funcType = dynamic_cast<FunctionType const*>(_memberAccess.annotation().type))
		if (funcType->kind() == FunctionType::Kind::Internal && funcType->hasDeclaration())
			visitCallableIfNew(funcType->declaration());

	return false;
}

bool ImmutableValidator::visit(IfStatement const& _ifStatement)
{
	bool prevInBranch = m_inBranch;

	_ifStatement.condition().accept(*this);

	m_inBranch = true;
	_ifStatement.trueStatement().accept(*this);

	if (auto falseStatement = _ifStatement.falseStatement())
		falseStatement->accept(*this);

	m_inBranch = prevInBranch;

	return false;
}

bool ImmutableValidator::visit(WhileStatement const& _whileStatement)
{
	bool prevInLoop = m_inLoop;
	m_inLoop = true;

	_whileStatement.condition().accept(*this);
	_whileStatement.body().accept(*this);

	m_inLoop = prevInLoop;

	return false;
}

void ImmutableValidator::endVisit(Identifier const& _identifier)
{
	if (auto const callableDef = dynamic_cast<CallableDeclaration const*>(_identifier.annotation().referencedDeclaration))
		visitCallableIfNew(callableDef->resolveVirtual(m_currentContract));
	if (auto const varDecl = dynamic_cast<VariableDeclaration const*>(_identifier.annotation().referencedDeclaration))
		analyseVariableReference(*varDecl, _identifier);
}

void ImmutableValidator::endVisit(Return const& _return)
{
	if (m_currentConstructor != nullptr)
		checkAllVariablesInitialized(_return.location());
}

bool ImmutableValidator::analyseCallable(CallableDeclaration const& _callableDeclaration)
{
	FunctionDefinition const* prevConstructor = m_currentConstructor;
	m_currentConstructor = nullptr;

	if (FunctionDefinition const* funcDef = dynamic_cast<decltype(funcDef)>(&_callableDeclaration))
	{
		ASTNode::listAccept(funcDef->modifiers(), *this);

		if (funcDef->isConstructor())
			m_currentConstructor = funcDef;

		if (funcDef->isImplemented())
			funcDef->body().accept(*this);
	}
	else if (ModifierDefinition const* modDef = dynamic_cast<decltype(modDef)>(&_callableDeclaration))
		modDef->body().accept(*this);

	m_currentConstructor = prevConstructor;

	return false;
}

void ImmutableValidator::analyseVariableReference(VariableDeclaration const& _variableReference, Expression const& _expression)
{
	if (!_variableReference.isStateVariable() || !_variableReference.immutable())
		return;

	if (_expression.annotation().lValueRequested && _expression.annotation().lValueOfOrdinaryAssignment)
	{
		if (!m_currentConstructor)
			m_errorReporter.typeError(
				_expression.location(),
				"Immutable variables can only be initialized inline or assigned directly in the constructor."
			);
		else if (m_currentConstructor->annotation().contract->id() != _variableReference.annotation().contract->id())
			m_errorReporter.typeError(
				_expression.location(),
				"Immutable variables must be initialized in the constructor of the contract they are defined in."
			);
		else if (m_inLoop)
			m_errorReporter.typeError(
				_expression.location(),
				"Immutable variables can only be initialized once, not in a while statement."
			);
		else if (m_inBranch)
			m_errorReporter.typeError(
				_expression.location(),
				"Immutable variables must be initialized unconditionally, not in an if statement."
			);

		if (!m_initializedStateVariables.emplace(&_variableReference).second)
			m_errorReporter.typeError(
				_expression.location(),
				"Immutable state variable already initialized."
			);
	}
	else if (m_inConstructionContext)
		m_errorReporter.typeError(
			_expression.location(),
			"Immutable variables cannot be read during contract creation time, which means they cannot be read in the constructor or any function or modifier called from it."
		);
}

void ImmutableValidator::checkAllVariablesInitialized(solidity::langutil::SourceLocation const& _location)
{
	for (ContractDefinition const* contract: m_currentContract.annotation().linearizedBaseContracts)
		for (VariableDeclaration const* varDecl: contract->stateVariables())
			if (varDecl->immutable())
				if (!util::contains(m_initializedStateVariables, varDecl))
					m_errorReporter.typeError(
						_location,
						solidity::langutil::SecondarySourceLocation().append("Not initialized: ", varDecl->location()),
						"Construction control flow ends without initializing all immutable state variables."
					);
}

void ImmutableValidator::visitCallableIfNew(Declaration const& _declaration)
{
	CallableDeclaration const* _callable = dynamic_cast<CallableDeclaration const*>(&_declaration);
	solAssert(_callable != nullptr, "");

	if (m_visitedCallables.emplace(_callable).second)
		_declaration.accept(*this);
}
