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
 * Encodes Solidity into SMT expressions without creating
 * any verification targets.
 * Also implements the SSA scheme for branches.
 */

#pragma once


#include <libsolidity/formal/EncodingContext.h>
#include <libsolidity/formal/SymbolicVariables.h>
#include <libsolidity/formal/VariableUsage.h>

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/interface/ReadFile.h>
#include <liblangutil/ErrorReporter.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace solidity::langutil
{
class ErrorReporter;
struct SourceLocation;
}

namespace solidity::frontend
{

class SMTEncoder: public ASTConstVisitor
{
public:
	SMTEncoder(smt::EncodingContext& _context);

	/// @returns the leftmost identifier in a multi-d IndexAccess.
	static Expression const* leftmostBase(IndexAccess const& _indexAccess);

	/// @returns the innermost element in a chain of 1-tuples if applicable,
	/// otherwise _expr.
	static Expression const* innermostTuple(Expression const& _expr);

	/// @returns the FunctionDefinition of a FunctionCall
	/// if possible or nullptr.
	static FunctionDefinition const* functionCallToDefinition(FunctionCall const& _funCall);

	static std::vector<VariableDeclaration const*> stateVariablesIncludingInheritedAndPrivate(ContractDefinition const& _contract);
	static std::vector<VariableDeclaration const*> stateVariablesIncludingInheritedAndPrivate(FunctionDefinition const& _function);

	/// @returns the SourceUnit that contains _scopable.
	static SourceUnit const* sourceUnitContaining(Scopable const& _scopable);

protected:
	// TODO: Check that we do not have concurrent reads and writes to a variable,
	// because the order of expression evaluation is undefined
	// TODO: or just force a certain order, but people might have a different idea about that.

	bool visit(ContractDefinition const& _node) override;
	void endVisit(ContractDefinition const& _node) override;
	void endVisit(VariableDeclaration const& _node) override;
	bool visit(ModifierDefinition const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	void endVisit(FunctionDefinition const& _node) override;
	bool visit(PlaceholderStatement const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const&) override { return false; }
	bool visit(ForStatement const&) override { return false; }
	void endVisit(VariableDeclarationStatement const& _node) override;
	void endVisit(Assignment const& _node) override;
	void endVisit(TupleExpression const& _node) override;
	bool visit(UnaryOperation const& _node) override;
	void endVisit(UnaryOperation const& _node) override;
	bool visit(BinaryOperation const& _node) override;
	void endVisit(BinaryOperation const& _node) override;
	bool visit(Conditional const& _node) override;
	void endVisit(FunctionCall const& _node) override;
	bool visit(ModifierInvocation const& _node) override;
	void endVisit(Identifier const& _node) override;
	void endVisit(ElementaryTypeNameExpression const& _node) override;
	void endVisit(Literal const& _node) override;
	void endVisit(Return const& _node) override;
	bool visit(MemberAccess const& _node) override;
	void endVisit(IndexAccess const& _node) override;
	void endVisit(IndexRangeAccess const& _node) override;
	bool visit(InlineAssembly const& _node) override;
	void endVisit(Break const&) override {}
	void endVisit(Continue const&) override {}
	bool visit(TryCatchClause const& _node) override;

	/// Do not visit subtree if node is a RationalNumber.
	/// Symbolic _expr is the rational literal.
	bool shortcutRationalNumber(Expression const& _expr);
	void arithmeticOperation(BinaryOperation const& _op);
	/// @returns _op(_left, _right) with and without modular arithmetic.
	/// Used by the function above, compound assignments and
	/// unary increment/decrement.
	virtual std::pair<smtutil::Expression, smtutil::Expression> arithmeticOperation(
		Token _op,
		smtutil::Expression const& _left,
		smtutil::Expression const& _right,
		TypePointer const& _commonType,
		Expression const& _expression
	);

	smtutil::Expression bitwiseOperation(
		Token _op,
		smtutil::Expression const& _left,
		smtutil::Expression const& _right,
		TypePointer const& _commonType
	);

	void compareOperation(BinaryOperation const& _op);
	void booleanOperation(BinaryOperation const& _op);
	void bitwiseOperation(BinaryOperation const& _op);
	void bitwiseNotOperation(UnaryOperation const& _op);

	void initContract(ContractDefinition const& _contract);
	void initFunction(FunctionDefinition const& _function);
	void visitAssert(FunctionCall const& _funCall);
	void visitRequire(FunctionCall const& _funCall);
	void visitCryptoFunction(FunctionCall const& _funCall);
	void visitGasLeft(FunctionCall const& _funCall);
	virtual void visitAddMulMod(FunctionCall const& _funCall);
	void visitObjectCreation(FunctionCall const& _funCall);
	void visitTypeConversion(FunctionCall const& _funCall);
	void visitFunctionIdentifier(Identifier const& _identifier);

	/// Encodes a modifier or function body according to the modifier
	/// visit depth.
	void visitFunctionOrModifier();

	/// Inlines a modifier or base constructor call.
	void inlineModifierInvocation(ModifierInvocation const* _invocation, CallableDeclaration const* _definition);

	/// Inlines the constructor hierarchy into a single constructor.
	void inlineConstructorHierarchy(ContractDefinition const& _contract);

	/// Defines a new global variable or function.
	void defineGlobalVariable(std::string const& _name, Expression const& _expr, bool _increaseIndex = false);

	/// Handles the side effects of assignment
	/// to variable of some SMT array type
	/// while aliasing is not supported.
	void arrayAssignment();
	/// Handles assignments to index or member access.
	void indexOrMemberAssignment(Expression const& _expr, smtutil::Expression const& _rightHandSide);

	void arrayPush(FunctionCall const& _funCall);
	void arrayPop(FunctionCall const& _funCall);
	void arrayPushPopAssign(Expression const& _expr, smtutil::Expression const& _array);
	/// Allows BMC and CHC to create verification targets for popping
	/// an empty array.
	virtual void makeArrayPopVerificationTarget(FunctionCall const&) {}

	void addArrayLiteralAssertions(
		smt::SymbolicArrayVariable& _symArray,
		std::vector<smtutil::Expression> const& _elementValues
	);

	/// Division expression in the given type. Requires special treatment because
	/// of rounding for signed division.
	smtutil::Expression division(smtutil::Expression _left, smtutil::Expression _right, IntegerType const& _type);

	void assignment(VariableDeclaration const& _variable, Expression const& _value);
	/// Handles assignments to variables of different types.
	void assignment(VariableDeclaration const& _variable, smtutil::Expression const& _value);
	/// Handles assignments between generic expressions.
	/// Will also be used for assignments of tuple components.
	void assignment(
		Expression const& _left,
		smtutil::Expression const& _right,
		TypePointer const& _type
	);
	/// Handle assignments between tuples.
	void tupleAssignment(Expression const& _left, Expression const& _right);
	/// Computes the right hand side of a compound assignment.
	smtutil::Expression compoundAssignment(Assignment const& _assignment);

	/// Maps a variable to an SSA index.
	using VariableIndices = std::unordered_map<VariableDeclaration const*, int>;

	/// Visits the branch given by the statement, pushes and pops the current path conditions.
	/// @param _condition if present, asserts that this condition is true within the branch.
	/// @returns the variable indices after visiting the branch.
	VariableIndices visitBranch(ASTNode const* _statement, smtutil::Expression const* _condition = nullptr);
	VariableIndices visitBranch(ASTNode const* _statement, smtutil::Expression _condition);

	using CallStackEntry = std::pair<CallableDeclaration const*, ASTNode const*>;

	void createStateVariables(ContractDefinition const& _contract);
	void initializeStateVariables(ContractDefinition const& _contract);
	void createLocalVariables(FunctionDefinition const& _function);
	void initializeLocalVariables(FunctionDefinition const& _function);
	void initializeFunctionCallParameters(CallableDeclaration const& _function, std::vector<smtutil::Expression> const& _callArgs);
	void resetStateVariables();
	/// Resets all references/pointers that have the same type or have
	/// a subexpression of the same type as _varDecl.
	void resetReferences(VariableDeclaration const& _varDecl);
	/// Resets all references/pointers that have type _type.
	void resetReferences(TypePointer _type);
	/// @returns the type without storage pointer information if it has it.
	TypePointer typeWithoutPointer(TypePointer const& _type);
	/// @returns whether _a or a subtype of _a is the same as _b.
	bool sameTypeOrSubtype(TypePointer _a, TypePointer _b);

	/// Given two different branches and the touched variables,
	/// merge the touched variables into after-branch ite variables
	/// using the branch condition as guard.
	void mergeVariables(std::set<VariableDeclaration const*> const& _variables, smtutil::Expression const& _condition, VariableIndices const& _indicesEndTrue, VariableIndices const& _indicesEndFalse);
	/// Tries to create an uninitialized variable and returns true on success.
	bool createVariable(VariableDeclaration const& _varDecl);

	/// @returns an expression denoting the value of the variable declared in @a _decl
	/// at the current point.
	smtutil::Expression currentValue(VariableDeclaration const& _decl);
	/// @returns an expression denoting the value of the variable declared in @a _decl
	/// at the given index. Does not ensure that this index exists.
	smtutil::Expression valueAtIndex(VariableDeclaration const& _decl, unsigned _index);
	/// Returns the expression corresponding to the AST node.
	/// If _targetType is not null apply conversion.
	/// Throws if the expression does not exist.
	smtutil::Expression expr(Expression const& _e, TypePointer _targetType = nullptr);
	/// Creates the expression (value can be arbitrary)
	void createExpr(Expression const& _e);
	/// Creates the expression and sets its value.
	void defineExpr(Expression const& _e, smtutil::Expression _value);

	/// Adds a new path condition
	void pushPathCondition(smtutil::Expression const& _e);
	/// Remove the last path condition
	void popPathCondition();
	/// Returns the conjunction of all path conditions or True if empty
	smtutil::Expression currentPathConditions();
	/// @returns a human-readable call stack. Used for models.
	langutil::SecondarySourceLocation callStackMessage(std::vector<CallStackEntry> const& _callStack);
	/// Copies and pops the last called node.
	CallStackEntry popCallStack();
	/// Adds (_definition, _node) to the callstack.
	void pushCallStack(CallStackEntry _entry);
	/// Add to the solver: the given expression implied by the current path conditions
	void addPathImpliedExpression(smtutil::Expression const& _e);

	/// Copy the SSA indices of m_variables.
	VariableIndices copyVariableIndices();
	/// Resets the variable indices.
	void resetVariableIndices(VariableIndices const& _indices);
	/// Used when starting a new block.
	virtual void clearIndices(ContractDefinition const* _contract, FunctionDefinition const* _function = nullptr);


	/// @returns variables that are touched in _node's subtree.
	std::set<VariableDeclaration const*> touchedVariables(ASTNode const& _node);

	/// @returns the VariableDeclaration referenced by an Identifier or nullptr.
	VariableDeclaration const* identifierToVariable(Expression const& _expr);

	/// Creates symbolic expressions for the returned values
	/// and set them as the components of the symbolic tuple.
	void createReturnedExpressions(FunctionCall const& _funCall);

	/// @returns the symbolic arguments for a function call,
	/// taking into account bound functions and
	/// type conversion.
	std::vector<smtutil::Expression> symbolicArguments(FunctionCall const& _funCall);

	/// @returns a note to be added to warnings.
	std::string extraComment();

	struct VerificationTarget
	{
		enum class Type { ConstantCondition, Underflow, Overflow, UnderOverflow, DivByZero, Balance, Assert, PopEmptyArray } type;
		smtutil::Expression value;
		smtutil::Expression constraints;
	};

	smt::VariableUsage m_variableUsage;
	bool m_arrayAssignmentHappened = false;
	// True if the "No SMT solver available" warning was already created.
	bool m_noSolverWarning = false;

	/// Stores the instances of an Uninterpreted Function applied to arguments.
	/// These may be direct application of UFs or Array index access.
	/// Used to retrieve models.
	std::set<Expression const*> m_uninterpretedTerms;
	std::vector<smtutil::Expression> m_pathConditions;
	/// Local SMTEncoder ErrorReporter.
	/// This is necessary to show the "No SMT solver available"
	/// warning before the others in case it's needed.
	langutil::ErrorReporter m_errorReporter;
	langutil::ErrorList m_smtErrors;

	/// Stores the current function/modifier call/invocation path.
	std::vector<CallStackEntry> m_callStack;
	/// Returns true if the current function was not visited by
	/// a function call.
	bool isRootFunction();
	/// Returns true if _funDef was already visited.
	bool visitedFunction(FunctionDefinition const* _funDef);

	/// Depth of visit to modifiers.
	/// When m_modifierDepth == #modifiers the function can be visited
	/// when placeholder is visited.
	/// Needs to be a stack because of function calls.
	std::vector<int> m_modifierDepthStack;

	std::map<ContractDefinition const*, ModifierInvocation const*> m_baseConstructorCalls;

	ContractDefinition const* m_currentContract = nullptr;

	/// Stores the context of the encoding.
	smt::EncodingContext& m_context;
};

}
