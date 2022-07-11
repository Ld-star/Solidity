/*(
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
 * Base class to perform data flow analysis during AST walks.
 * Tracks assignments and is used as base class for both Rematerialiser and
 * Common Subexpression Eliminator.
 */

#include <libyul/optimiser/DataFlowAnalyzer.h>

#include <libyul/optimiser/NameCollector.h>
#include <libyul/optimiser/Semantics.h>
#include <libyul/optimiser/KnowledgeBase.h>
#include <libyul/AST.h>
#include <libyul/Dialect.h>
#include <libyul/Exceptions.h>
#include <libyul/Utilities.h>

#include <libsolutil/CommonData.h>
#include <libsolutil/cxx20.h>

#include <variant>

#include <range/v3/view/reverse.hpp>

using namespace std;
using namespace solidity;
using namespace solidity::util;
using namespace solidity::yul;

DataFlowAnalyzer::DataFlowAnalyzer(
	Dialect const& _dialect,
	MemoryAndStorage _analyzeStores,
	map<YulString, SideEffects> _functionSideEffects
):
	m_dialect(_dialect),
	m_functionSideEffects(std::move(_functionSideEffects)),
	m_knowledgeBase(_dialect, [this](YulString _var) { return variableValue(_var); }),
	m_analyzeStores(_analyzeStores == MemoryAndStorage::Analyze)
{
	if (m_analyzeStores)
	{
		if (auto const* builtin = _dialect.memoryStoreFunction(YulString{}))
			m_storeFunctionName[static_cast<unsigned>(StoreLoadLocation::Memory)] = builtin->name;
		if (auto const* builtin = _dialect.memoryLoadFunction(YulString{}))
			m_loadFunctionName[static_cast<unsigned>(StoreLoadLocation::Memory)] = builtin->name;
		if (auto const* builtin = _dialect.storageStoreFunction(YulString{}))
			m_storeFunctionName[static_cast<unsigned>(StoreLoadLocation::Storage)] = builtin->name;
		if (auto const* builtin = _dialect.storageLoadFunction(YulString{}))
			m_loadFunctionName[static_cast<unsigned>(StoreLoadLocation::Storage)] = builtin->name;
	}
}

void DataFlowAnalyzer::operator()(ExpressionStatement& _statement)
{
	if (m_analyzeStores)
	{
		if (auto vars = isSimpleStore(StoreLoadLocation::Storage, _statement))
		{
			ASTModifier::operator()(_statement);
			cxx20::erase_if(m_state.storage, mapTuple([&](auto&& key, auto&& value) {
				return
					!m_knowledgeBase.knownToBeDifferent(vars->first, key) &&
					!m_knowledgeBase.knownToBeEqual(vars->second, value);
			}));
			m_state.storage[vars->first] = vars->second;
			return;
		}
		else if (auto vars = isSimpleStore(StoreLoadLocation::Memory, _statement))
		{
			ASTModifier::operator()(_statement);
			cxx20::erase_if(m_state.memory, mapTuple([&](auto&& key, auto&& /* value */) {
				return !m_knowledgeBase.knownToBeDifferentByAtLeast32(vars->first, key);
			}));
			m_state.memory[vars->first] = vars->second;
			return;
		}
	}
	clearKnowledgeIfInvalidated(_statement.expression);
	ASTModifier::operator()(_statement);
}

void DataFlowAnalyzer::operator()(Assignment& _assignment)
{
	set<YulString> names;
	for (auto const& var: _assignment.variableNames)
		names.emplace(var.name);
	assertThrow(_assignment.value, OptimizerException, "");
	clearKnowledgeIfInvalidated(*_assignment.value);
	visit(*_assignment.value);
	handleAssignment(names, _assignment.value.get(), false);
}

void DataFlowAnalyzer::operator()(VariableDeclaration& _varDecl)
{
	set<YulString> names;
	for (auto const& var: _varDecl.variables)
		names.emplace(var.name);
	m_variableScopes.back().variables += names;

	if (_varDecl.value)
	{
		clearKnowledgeIfInvalidated(*_varDecl.value);
		visit(*_varDecl.value);
	}

	handleAssignment(names, _varDecl.value.get(), true);
}

void DataFlowAnalyzer::operator()(If& _if)
{
	clearKnowledgeIfInvalidated(*_if.condition);
	unordered_map<YulString, YulString> storage = m_state.storage;
	unordered_map<YulString, YulString> memory = m_state.memory;

	ASTModifier::operator()(_if);

	joinKnowledge(storage, memory);

	clearValues(assignedVariableNames(_if.body));
}

void DataFlowAnalyzer::operator()(Switch& _switch)
{
	clearKnowledgeIfInvalidated(*_switch.expression);
	visit(*_switch.expression);
	set<YulString> assignedVariables;
	for (auto& _case: _switch.cases)
	{
		unordered_map<YulString, YulString> storage = m_state.storage;
		unordered_map<YulString, YulString> memory = m_state.memory;
		(*this)(_case.body);
		joinKnowledge(storage, memory);

		set<YulString> variables = assignedVariableNames(_case.body);
		assignedVariables += variables;
		// This is a little too destructive, we could retain the old values.
		clearValues(variables);
		clearKnowledgeIfInvalidated(_case.body);
	}
	for (auto& _case: _switch.cases)
		clearKnowledgeIfInvalidated(_case.body);
	clearValues(assignedVariables);
}

void DataFlowAnalyzer::operator()(FunctionDefinition& _fun)
{
	// Save all information. We might rather reinstantiate this class,
	// but this could be difficult if it is subclassed.
	ScopedSaveAndRestore stateResetter(m_state, {});
	ScopedSaveAndRestore loopDepthResetter(m_loopDepth, 0u);
	pushScope(true);

	for (auto const& parameter: _fun.parameters)
		m_variableScopes.back().variables.emplace(parameter.name);
	for (auto const& var: _fun.returnVariables)
	{
		m_variableScopes.back().variables.emplace(var.name);
		handleAssignment({var.name}, nullptr, true);
	}
	ASTModifier::operator()(_fun);

	// Note that the contents of return variables, storage and memory at this point
	// might be incorrect due to the fact that the DataFlowAnalyzer ignores the ``leave``
	// statement.

	popScope();
}

void DataFlowAnalyzer::operator()(ForLoop& _for)
{
	// If the pre block was not empty,
	// we would have to deal with more complicated scoping rules.
	assertThrow(_for.pre.statements.empty(), OptimizerException, "");

	++m_loopDepth;

	AssignmentsSinceContinue assignmentsSinceCont;
	assignmentsSinceCont(_for.body);

	set<YulString> assignedVariables =
		assignedVariableNames(_for.body) + assignedVariableNames(_for.post);
	clearValues(assignedVariables);

	// break/continue are tricky for storage and thus we almost always clear here.
	clearKnowledgeIfInvalidated(*_for.condition);
	clearKnowledgeIfInvalidated(_for.post);
	clearKnowledgeIfInvalidated(_for.body);

	visit(*_for.condition);
	(*this)(_for.body);
	clearValues(assignmentsSinceCont.names());
	clearKnowledgeIfInvalidated(_for.body);
	(*this)(_for.post);
	clearValues(assignedVariables);
	clearKnowledgeIfInvalidated(*_for.condition);
	clearKnowledgeIfInvalidated(_for.post);
	clearKnowledgeIfInvalidated(_for.body);

	--m_loopDepth;
}

void DataFlowAnalyzer::operator()(Block& _block)
{
	size_t numScopes = m_variableScopes.size();
	pushScope(false);
	ASTModifier::operator()(_block);
	popScope();
	assertThrow(numScopes == m_variableScopes.size(), OptimizerException, "");
}

optional<YulString> DataFlowAnalyzer::storageValue(YulString _key) const
{
	if (YulString const* value = util::valueOrNullptr(m_state.storage, _key))
		return *value;
	else
		return nullopt;
}

optional<YulString> DataFlowAnalyzer::memoryValue(YulString _key) const
{
	if (YulString const* value = util::valueOrNullptr(m_state.memory, _key))
		return *value;
	else
		return nullopt;
}

void DataFlowAnalyzer::handleAssignment(set<YulString> const& _variables, Expression* _value, bool _isDeclaration)
{
	if (!_isDeclaration)
		clearValues(_variables);

	MovableChecker movableChecker{m_dialect, &m_functionSideEffects};
	if (_value)
		movableChecker.visit(*_value);
	else
		for (auto const& var: _variables)
			assignValue(var, &m_zero);

	if (_value && _variables.size() == 1)
	{
		YulString name = *_variables.begin();
		// Expression has to be movable and cannot contain a reference
		// to the variable that will be assigned to.
		if (movableChecker.movable() && !movableChecker.referencedVariables().count(name))
			assignValue(name, _value);
	}

	auto const& referencedVariables = movableChecker.referencedVariables();
	for (auto const& name: _variables)
	{
		m_state.references[name] = referencedVariables;
		if (!_isDeclaration)
		{
			// assignment to slot denoted by "name"
			m_state.storage.erase(name);
			// assignment to slot contents denoted by "name"
			cxx20::erase_if(m_state.storage, mapTuple([&name](auto&& /* key */, auto&& value) { return value == name; }));
			// assignment to slot denoted by "name"
			m_state.memory.erase(name);
			// assignment to slot contents denoted by "name"
			cxx20::erase_if(m_state.memory, mapTuple([&name](auto&& /* key */, auto&& value) { return value == name; }));
		}
	}

	if (_value && _variables.size() == 1)
	{
		YulString variable = *_variables.begin();
		if (!movableChecker.referencedVariables().count(variable))
		{
			// This might erase additional knowledge about the slot.
			// On the other hand, if we knew the value in the slot
			// already, then the sload() / mload() would have been replaced by a variable anyway.
			if (auto key = isSimpleLoad(StoreLoadLocation::Memory, *_value))
				m_state.memory[*key] = variable;
			else if (auto key = isSimpleLoad(StoreLoadLocation::Storage, *_value))
				m_state.storage[*key] = variable;
		}
	}
}

void DataFlowAnalyzer::pushScope(bool _functionScope)
{
	m_variableScopes.emplace_back(_functionScope);
}

void DataFlowAnalyzer::popScope()
{
	for (auto const& name: m_variableScopes.back().variables)
	{
		m_state.value.erase(name);
		m_state.references.erase(name);
	}
	m_variableScopes.pop_back();
}

void DataFlowAnalyzer::clearValues(set<YulString> _variables)
{
	// All variables that reference variables to be cleared also have to be
	// cleared, but not recursively, since only the value of the original
	// variables changes. Example:
	// let a := 1
	// let b := a
	// let c := b
	// let a := 2
	// add(b, c)
	// In the last line, we can replace c by b, but not b by a.
	//
	// This cannot be easily tested since the substitutions will be done
	// one by one on the fly, and the last line will just be add(1, 1)

	// First clear storage knowledge, because we do not have to clear
	// storage knowledge of variables whose expression has changed,
	// since the value is still unchanged.
	auto eraseCondition = mapTuple([&_variables](auto&& key, auto&& value) {
		return _variables.count(key) || _variables.count(value);
	});
	cxx20::erase_if(m_state.storage, eraseCondition);
	cxx20::erase_if(m_state.memory, eraseCondition);

	// Also clear variables that reference variables to be cleared.
	for (auto const& variableToClear: _variables)
		for (auto const& [ref, names]: m_state.references)
			if (names.count(variableToClear))
				_variables.emplace(ref);

	// Clear the value and update the reference relation.
	for (auto const& name: _variables)
	{
		m_state.value.erase(name);
		m_state.references.erase(name);
	}
}

void DataFlowAnalyzer::assignValue(YulString _variable, Expression const* _value)
{
	m_state.value[_variable] = {_value, m_loopDepth};
}

void DataFlowAnalyzer::clearKnowledgeIfInvalidated(Block const& _block)
{
	if (!m_analyzeStores)
		return;
	SideEffectsCollector sideEffects(m_dialect, _block, &m_functionSideEffects);
	if (sideEffects.invalidatesStorage())
		m_state.storage.clear();
	if (sideEffects.invalidatesMemory())
		m_state.memory.clear();
}

void DataFlowAnalyzer::clearKnowledgeIfInvalidated(Expression const& _expr)
{
	if (!m_analyzeStores)
		return;
	SideEffectsCollector sideEffects(m_dialect, _expr, &m_functionSideEffects);
	if (sideEffects.invalidatesStorage())
		m_state.storage.clear();
	if (sideEffects.invalidatesMemory())
		m_state.memory.clear();
}

void DataFlowAnalyzer::joinKnowledge(
	unordered_map<YulString, YulString> const& _olderStorage,
	unordered_map<YulString, YulString> const& _olderMemory
)
{
	if (!m_analyzeStores)
		return;
	joinKnowledgeHelper(m_state.storage, _olderStorage);
	joinKnowledgeHelper(m_state.memory, _olderMemory);
}

void DataFlowAnalyzer::joinKnowledgeHelper(
	std::unordered_map<YulString, YulString>& _this,
	std::unordered_map<YulString, YulString> const& _older
)
{
	// We clear if the key does not exist in the older map or if the value is different.
	// This also works for memory because _older is an "older version"
	// of m_state.memory and thus any overlapping write would have cleared the keys
	// that are not known to be different inside m_state.memory already.
	cxx20::erase_if(_this, mapTuple([&_older](auto&& key, auto&& currentValue){
		YulString const* oldValue = util::valueOrNullptr(_older, key);
		return !oldValue || *oldValue != currentValue;
	}));
}

bool DataFlowAnalyzer::inScope(YulString _variableName) const
{
	for (auto const& scope: m_variableScopes | ranges::views::reverse)
	{
		if (scope.variables.count(_variableName))
			return true;
		if (scope.isFunction)
			return false;
	}
	return false;
}

optional<u256> DataFlowAnalyzer::valueOfIdentifier(YulString const& _name)
{
	if (AssignedValue const* value = variableValue(_name))
		if (Literal const* literal = get_if<Literal>(value->value))
			return valueOfLiteral(*literal);
	return nullopt;
}

std::optional<pair<YulString, YulString>> DataFlowAnalyzer::isSimpleStore(
	StoreLoadLocation _location,
	ExpressionStatement const& _statement
) const
{
	if (FunctionCall const* funCall = get_if<FunctionCall>(&_statement.expression))
		if (funCall->functionName.name == m_storeFunctionName[static_cast<unsigned>(_location)])
			if (Identifier const* key = std::get_if<Identifier>(&funCall->arguments.front()))
				if (Identifier const* value = std::get_if<Identifier>(&funCall->arguments.back()))
					return make_pair(key->name, value->name);
	return {};
}

std::optional<YulString> DataFlowAnalyzer::isSimpleLoad(
	StoreLoadLocation _location,
	Expression const& _expression
) const
{
	if (FunctionCall const* funCall = get_if<FunctionCall>(&_expression))
		if (funCall->functionName.name == m_loadFunctionName[static_cast<unsigned>(_location)])
			if (Identifier const* key = std::get_if<Identifier>(&funCall->arguments.front()))
				return key->name;
	return {};
}
