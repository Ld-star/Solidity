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
 * Utilities for the solidity compiler.
 */

#pragma once

#include <ostream>
#include <stack>
#include <utility>
#include <libevmcore/Instruction.h>
#include <libevmcore/Assembly.h>
#include <libsolidity/ASTForward.h>
#include <libsolidity/Types.h>
#include <libdevcore/Common.h>

namespace dev {
namespace solidity {


/**
 * Context to be shared by all units that compile the same contract.
 * It stores the generated bytecode and the position of identifiers in memory and on the stack.
 */
class CompilerContext
{
public:
	void addMagicGlobal(MagicVariableDeclaration const& _declaration);
	void addStateVariable(VariableDeclaration const& _declaration, u256 const& _storageOffset, unsigned _byteOffset);
	void addVariable(VariableDeclaration const& _declaration, unsigned _offsetToCurrent = 0);
	void removeVariable(VariableDeclaration const& _declaration);
	void addAndInitializeVariable(VariableDeclaration const& _declaration);

	void setCompiledContracts(std::map<ContractDefinition const*, bytes const*> const& _contracts) { m_compiledContracts = _contracts; }
	bytes const& getCompiledContract(ContractDefinition const& _contract) const;

	void setStackOffset(int _offset) { m_asm.setDeposit(_offset); }
	void adjustStackOffset(int _adjustment) { m_asm.adjustDeposit(_adjustment); }
	unsigned getStackHeight() const { solAssert(m_asm.deposit() >= 0, ""); return unsigned(m_asm.deposit()); }

	bool isMagicGlobal(Declaration const* _declaration) const { return m_magicGlobals.count(_declaration) != 0; }
	bool isLocalVariable(Declaration const* _declaration) const;
	bool isStateVariable(Declaration const* _declaration) const { return m_stateVariables.count(_declaration) != 0; }

	eth::AssemblyItem getFunctionEntryLabel(Declaration const& _declaration);
	void setInheritanceHierarchy(std::vector<ContractDefinition const*> const& _hierarchy) { m_inheritanceHierarchy = _hierarchy; }
	/// @returns the entry label of the given function and takes overrides into account.
	eth::AssemblyItem getVirtualFunctionEntryLabel(FunctionDefinition const& _function);
	/// @returns the entry label of a function that overrides the given declaration from the most derived class just
	/// above _base in the current inheritance hierarchy.
	eth::AssemblyItem getSuperFunctionEntryLabel(FunctionDefinition const& _function, ContractDefinition const& _base);
	FunctionDefinition const* getNextConstructor(ContractDefinition const& _contract) const;

	/// @returns the set of functions for which we still need to generate code
	std::set<Declaration const*> getFunctionsWithoutCode();
	/// Resets function specific members, inserts the function entry label and marks the function
	/// as "having code".
	void startFunction(Declaration const& _function);

	ModifierDefinition const& getFunctionModifier(std::string const& _name) const;
	/// Returns the distance of the given local variable from the bottom of the stack (of the current function).
	unsigned getBaseStackOffsetOfVariable(Declaration const& _declaration) const;
	/// If supplied by a value returned by @ref getBaseStackOffsetOfVariable(variable), returns
	/// the distance of that variable from the current top of the stack.
	unsigned baseToCurrentStackOffset(unsigned _baseOffset) const;
	/// Converts an offset relative to the current stack height to a value that can be used later
	/// with baseToCurrentStackOffset to point to the same stack element.
	unsigned currentToBaseStackOffset(unsigned _offset) const;
	/// @returns pair of slot and byte offset of the value inside this slot.
	std::pair<u256, unsigned> getStorageLocationOfVariable(Declaration const& _declaration) const;

	/// Appends a JUMPI instruction to a new tag and @returns the tag
	eth::AssemblyItem appendConditionalJump() { return m_asm.appendJumpI().tag(); }
	/// Appends a JUMPI instruction to @a _tag
	CompilerContext& appendConditionalJumpTo(eth::AssemblyItem const& _tag) { m_asm.appendJumpI(_tag); return *this; }
	/// Appends a JUMP to a new tag and @returns the tag
	eth::AssemblyItem appendJumpToNew() { return m_asm.appendJump().tag(); }
	/// Appends a JUMP to a tag already on the stack
	CompilerContext&  appendJump(eth::AssemblyItem::JumpType _jumpType = eth::AssemblyItem::JumpType::Ordinary);
	/// Appends a JUMP to a specific tag
	CompilerContext& appendJumpTo(eth::AssemblyItem const& _tag) { m_asm.appendJump(_tag); return *this; }
	/// Appends pushing of a new tag and @returns the new tag.
	eth::AssemblyItem pushNewTag() { return m_asm.append(m_asm.newPushTag()).tag(); }
	/// @returns a new tag without pushing any opcodes or data
	eth::AssemblyItem newTag() { return m_asm.newTag(); }
	/// Adds a subroutine to the code (in the data section) and pushes its size (via a tag)
	/// on the stack. @returns the assembly item corresponding to the pushed subroutine, i.e. its offset.
	eth::AssemblyItem addSubroutine(eth::Assembly const& _assembly) { return m_asm.appendSubSize(_assembly); }
	/// Pushes the size of the final program
	void appendProgramSize() { return m_asm.appendProgramSize(); }
	/// Adds data to the data section, pushes a reference to the stack
	eth::AssemblyItem appendData(bytes const& _data) { return m_asm.append(_data); }
	/// Resets the stack of visited nodes with a new stack having only @c _node
	void resetVisitedNodes(ASTNode const* _node);
	/// Pops the stack of visited nodes
	void popVisitedNodes() { m_visitedNodes.pop(); updateSourceLocation(); }
	/// Pushes an ASTNode to the stack of visited nodes
	void pushVisitedNodes(ASTNode const* _node) { m_visitedNodes.push(_node); updateSourceLocation(); }

	/// Append elements to the current instruction list and adjust @a m_stackOffset.
	CompilerContext& operator<<(eth::AssemblyItem const& _item) { m_asm.append(_item); return *this; }
	CompilerContext& operator<<(eth::Instruction _instruction) { m_asm.append(_instruction); return *this; }
	CompilerContext& operator<<(u256 const& _value) { m_asm.append(_value); return *this; }
	CompilerContext& operator<<(bytes const& _data) { m_asm.append(_data); return *this; }

	eth::Assembly const& getAssembly() const { return m_asm; }
	/// @arg _sourceCodes is the map of input files to source code strings
	void streamAssembly(std::ostream& _stream, StringMap const& _sourceCodes = StringMap()) const { m_asm.stream(_stream, "", _sourceCodes); }

	bytes getAssembledBytecode(bool _optimize = false) { return m_asm.optimise(_optimize).assemble(); }

	/**
	 * Helper class to pop the visited nodes stack when a scope closes
	 */
	class LocationSetter: public ScopeGuard
	{
	public:
		LocationSetter(CompilerContext& _compilerContext, ASTNode const& _node):
			ScopeGuard([&]{ _compilerContext.popVisitedNodes(); }) { _compilerContext.pushVisitedNodes(&_node); }
	};

private:
	/// @returns the entry label of the given function - searches the inheritance hierarchy
	/// startig from the given point towards the base.
	eth::AssemblyItem getVirtualFunctionEntryLabel(
		FunctionDefinition const& _function,
		std::vector<ContractDefinition const*>::const_iterator _searchStart
	);
	/// @returns an iterator to the contract directly above the given contract.
	std::vector<ContractDefinition const*>::const_iterator getSuperContract(const ContractDefinition &_contract) const;
	/// Updates source location set in the assembly.
	void updateSourceLocation();

	eth::Assembly m_asm;
	/// Magic global variables like msg, tx or this, distinguished by type.
	std::set<Declaration const*> m_magicGlobals;
	/// Other already compiled contracts to be used in contract creation calls.
	std::map<ContractDefinition const*, bytes const*> m_compiledContracts;
	/// Storage offsets of state variables
	std::map<Declaration const*, std::pair<u256, unsigned>> m_stateVariables;
	/// Offsets of local variables on the stack (relative to stack base).
	std::map<Declaration const*, unsigned> m_localVariables;
	/// Labels pointing to the entry points of functions.
	std::map<Declaration const*, eth::AssemblyItem> m_functionEntryLabels;
	/// Set of functions for which we did not yet generate code.
	std::set<Declaration const*> m_functionsWithCode;
	/// List of current inheritance hierarchy from derived to base.
	std::vector<ContractDefinition const*> m_inheritanceHierarchy;
	/// Stack of current visited AST nodes, used for location attachment
	std::stack<ASTNode const*> m_visitedNodes;
};

}
}
