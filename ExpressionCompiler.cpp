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
 * Solidity AST to EVM bytecode compiler for expressions.
 */

#include <utility>
#include <numeric>
#include <boost/range/adaptor/reversed.hpp>
#include <libdevcore/Common.h>
#include <libdevcrypto/SHA3.h>
#include <libsolidity/AST.h>
#include <libsolidity/ExpressionCompiler.h>
#include <libsolidity/CompilerContext.h>
#include <libsolidity/CompilerUtils.h>
#include <libsolidity/LValue.h>

using namespace std;

namespace dev
{
namespace solidity
{

void ExpressionCompiler::compile(Expression const& _expression)
{
	_expression.accept(*this);
}

void ExpressionCompiler::appendStateVariableInitialization(VariableDeclaration const& _varDecl)
{
	if (!_varDecl.getValue())
		return;
	solAssert(!!_varDecl.getValue()->getType(), "Type information not available.");
	CompilerContext::LocationSetter locationSetter(m_context, _varDecl);
	_varDecl.getValue()->accept(*this);
	appendTypeConversion(*_varDecl.getValue()->getType(), *_varDecl.getType(), true);

	StorageItem(m_context, _varDecl).storeValue(*_varDecl.getType(), _varDecl.getLocation(), true);
}

void ExpressionCompiler::appendStateVariableAccessor(VariableDeclaration const& _varDecl)
{
	CompilerContext::LocationSetter locationSetter(m_context, _varDecl);
	FunctionType accessorType(_varDecl);

	unsigned length = 0;
	TypePointers const& paramTypes = accessorType.getParameterTypes();
	// move arguments to memory
	for (TypePointer const& paramType: boost::adaptors::reverse(paramTypes))
		length += CompilerUtils(m_context).storeInMemory(length, *paramType, true);

	// retrieve the position of the variable
	m_context << m_context.getStorageLocationOfVariable(_varDecl);
	TypePointer returnType = _varDecl.getType();

	for (TypePointer const& paramType: paramTypes)
	{
		// move offset to memory
		CompilerUtils(m_context).storeInMemory(length);
		unsigned argLen = paramType->getCalldataEncodedSize();
		length -= argLen;
		m_context << u256(argLen + 32) << u256(length) << eth::Instruction::SHA3;

		returnType = dynamic_cast<MappingType const&>(*returnType).getValueType();
	}

	unsigned retSizeOnStack = 0;
	solAssert(accessorType.getReturnParameterTypes().size() >= 1, "");
	if (StructType const* structType = dynamic_cast<StructType const*>(returnType.get()))
	{
		auto const& names = accessorType.getReturnParameterNames();
		auto const& types = accessorType.getReturnParameterTypes();
		// struct
		for (size_t i = 0; i < names.size(); ++i)
		{
			m_context << eth::Instruction::DUP1
					  << structType->getStorageOffsetOfMember(names[i])
					  << eth::Instruction::ADD;
			StorageItem(m_context, *types[i]).retrieveValue(SourceLocation(), true);
			solAssert(types[i]->getSizeOnStack() == 1, "Returning struct elements with stack size != 1 not yet implemented.");
			m_context << eth::Instruction::SWAP1;
			retSizeOnStack += types[i]->getSizeOnStack();
		}
		m_context << eth::Instruction::POP;
	}
	else
	{
		// simple value
		solAssert(accessorType.getReturnParameterTypes().size() == 1, "");
		StorageItem(m_context, *returnType).retrieveValue(SourceLocation(), true);
		retSizeOnStack = returnType->getSizeOnStack();
	}
	solAssert(retSizeOnStack <= 15, "Stack too deep.");
	m_context << eth::dupInstruction(retSizeOnStack + 1);
	m_context.appendJump(eth::AssemblyItem::JumpType::OutOfFunction);
}

void ExpressionCompiler::appendTypeConversion(Type const& _typeOnStack, Type const& _targetType, bool _cleanupNeeded)
{
	// For a type extension, we need to remove all higher-order bits that we might have ignored in
	// previous operations.
	// @todo: store in the AST whether the operand might have "dirty" higher order bits

	if (_typeOnStack == _targetType && !_cleanupNeeded)
		return;
	Type::Category stackTypeCategory = _typeOnStack.getCategory();
	Type::Category targetTypeCategory = _targetType.getCategory();

	switch (stackTypeCategory)
	{
	case Type::Category::FixedBytes:
	{
		FixedBytesType const& typeOnStack = dynamic_cast<FixedBytesType const&>(_typeOnStack);
		if (targetTypeCategory == Type::Category::Integer)
		{
			// conversion from bytes to integer. no need to clean the high bit
			// only to shift right because of opposite alignment
			IntegerType const& targetIntegerType = dynamic_cast<IntegerType const&>(_targetType);
			m_context << (u256(1) << (256 - typeOnStack.getNumBytes() * 8)) << eth::Instruction::SWAP1 << eth::Instruction::DIV;
			if (targetIntegerType.getNumBits() < typeOnStack.getNumBytes() * 8)
				appendTypeConversion(IntegerType(typeOnStack.getNumBytes() * 8), _targetType, _cleanupNeeded); 
		}
		else
		{
			// clear lower-order bytes for conversion to shorter bytes - we always clean
			solAssert(targetTypeCategory == Type::Category::FixedBytes, "Invalid type conversion requested.");
			FixedBytesType const& targetType = dynamic_cast<FixedBytesType const&>(_targetType);
			if (targetType.getNumBytes() < typeOnStack.getNumBytes())
			{
				if (targetType.getNumBytes() == 0)
					m_context << eth::Instruction::DUP1 << eth::Instruction::XOR;
				else
					m_context << (u256(1) << (256 - targetType.getNumBytes() * 8))
							  << eth::Instruction::DUP1 << eth::Instruction::SWAP2
							  << eth::Instruction::DIV << eth::Instruction::MUL;
			}
		}
	}
		break;
	case Type::Category::Enum:
		solAssert(targetTypeCategory == Type::Category::Integer || targetTypeCategory == Type::Category::Enum, "");
		break;
	case Type::Category::Integer:
	case Type::Category::Contract:
	case Type::Category::IntegerConstant:
		if (targetTypeCategory == Type::Category::FixedBytes)
		{
			solAssert(stackTypeCategory == Type::Category::Integer || stackTypeCategory == Type::Category::IntegerConstant,
				"Invalid conversion to FixedBytesType requested.");
			// conversion from bytes to string. no need to clean the high bit
			// only to shift left because of opposite alignment
			FixedBytesType const& targetBytesType = dynamic_cast<FixedBytesType const&>(_targetType);
			if (auto typeOnStack = dynamic_cast<IntegerType const*>(&_typeOnStack))
				if (targetBytesType.getNumBytes() * 8 > typeOnStack->getNumBits())
					appendHighBitsCleanup(*typeOnStack);
			m_context << (u256(1) << (256 - targetBytesType.getNumBytes() * 8)) << eth::Instruction::MUL;
		}
		else if (targetTypeCategory == Type::Category::Enum)
			// just clean
			appendTypeConversion(_typeOnStack, *_typeOnStack.getRealType(), true);
		else
		{
			solAssert(targetTypeCategory == Type::Category::Integer || targetTypeCategory == Type::Category::Contract, "");
			IntegerType addressType(0, IntegerType::Modifier::Address);
			IntegerType const& targetType = targetTypeCategory == Type::Category::Integer
				? dynamic_cast<IntegerType const&>(_targetType) : addressType;
			if (stackTypeCategory == Type::Category::IntegerConstant)
			{
				IntegerConstantType const& constType = dynamic_cast<IntegerConstantType const&>(_typeOnStack);
				// We know that the stack is clean, we only have to clean for a narrowing conversion
				// where cleanup is forced.
				if (targetType.getNumBits() < constType.getIntegerType()->getNumBits() && _cleanupNeeded)
					appendHighBitsCleanup(targetType);
			}
			else
			{
				IntegerType const& typeOnStack = stackTypeCategory == Type::Category::Integer
					? dynamic_cast<IntegerType const&>(_typeOnStack) : addressType;
				// Widening: clean up according to source type width
				// Non-widening and force: clean up according to target type bits
				if (targetType.getNumBits() > typeOnStack.getNumBits())
					appendHighBitsCleanup(typeOnStack);
				else if (_cleanupNeeded)
					appendHighBitsCleanup(targetType);
			}
		}
		break;
	default:
		// All other types should not be convertible to non-equal types.
		solAssert(_typeOnStack == _targetType, "Invalid type conversion requested.");
		break;
	}
}

bool ExpressionCompiler::visit(Assignment const& _assignment)
{
	CompilerContext::LocationSetter locationSetter(m_context, _assignment);
	_assignment.getRightHandSide().accept(*this);
	if (_assignment.getType()->isValueType())
		appendTypeConversion(*_assignment.getRightHandSide().getType(), *_assignment.getType());
	_assignment.getLeftHandSide().accept(*this);
	solAssert(!!m_currentLValue, "LValue not retrieved.");

	Token::Value op = _assignment.getAssignmentOperator();
	if (op != Token::Assign) // compound assignment
	{
		solAssert(_assignment.getType()->isValueType(), "Compound operators not implemented for non-value types.");
		unsigned lvalueSize = m_currentLValue->sizeOnStack();
		unsigned itemSize = _assignment.getType()->getSizeOnStack();
		if (lvalueSize > 0)
		{
			CompilerUtils(m_context).copyToStackTop(lvalueSize + itemSize, itemSize);
			CompilerUtils(m_context).copyToStackTop(itemSize + lvalueSize, lvalueSize);
			// value lvalue_ref value lvalue_ref
		}
		m_currentLValue->retrieveValue(_assignment.getLocation(), true);
		appendOrdinaryBinaryOperatorCode(Token::AssignmentToBinaryOp(op), *_assignment.getType());
		if (lvalueSize > 0)
		{
			solAssert(itemSize + lvalueSize <= 16, "Stack too deep.");
			// value [lvalue_ref] updated_value
			for (unsigned i = 0; i < itemSize; ++i)
				m_context << eth::swapInstruction(itemSize + lvalueSize) << eth::Instruction::POP;
		}
	}
	m_currentLValue->storeValue(*_assignment.getRightHandSide().getType(), _assignment.getLocation());
	m_currentLValue.reset();
	return false;
}

bool ExpressionCompiler::visit(UnaryOperation const& _unaryOperation)
{
	CompilerContext::LocationSetter locationSetter(m_context, _unaryOperation);
	//@todo type checking and creating code for an operator should be in the same place:
	// the operator should know how to convert itself and to which types it applies, so
	// put this code together with "Type::acceptsBinary/UnaryOperator" into a class that
	// represents the operator
	if (_unaryOperation.getType()->getCategory() == Type::Category::IntegerConstant)
	{
		m_context << _unaryOperation.getType()->literalValue(nullptr);
		return false;
	}

	_unaryOperation.getSubExpression().accept(*this);

	switch (_unaryOperation.getOperator())
	{
	case Token::Not: // !
		m_context << eth::Instruction::ISZERO;
		break;
	case Token::BitNot: // ~
		m_context << eth::Instruction::NOT;
		break;
	case Token::After: // after
		m_context << eth::Instruction::TIMESTAMP << eth::Instruction::ADD;
		break;
	case Token::Delete: // delete
		solAssert(!!m_currentLValue, "LValue not retrieved.");
		m_currentLValue->setToZero(_unaryOperation.getLocation());
		m_currentLValue.reset();
		break;
	case Token::Inc: // ++ (pre- or postfix)
	case Token::Dec: // -- (pre- or postfix)
		solAssert(!!m_currentLValue, "LValue not retrieved.");
		m_currentLValue->retrieveValue(_unaryOperation.getLocation());
		solAssert(m_currentLValue->sizeOnStack() <= 1, "Not implemented.");
		if (!_unaryOperation.isPrefixOperation())
		{
			if (m_currentLValue->sizeOnStack() == 1)
				m_context << eth::Instruction::SWAP1 << eth::Instruction::DUP2;
			else
				m_context << eth::Instruction::DUP1;
		}
		m_context << u256(1);
		if (_unaryOperation.getOperator() == Token::Inc)
			m_context << eth::Instruction::ADD;
		else
			m_context << eth::Instruction::SWAP1 << eth::Instruction::SUB; // @todo avoid the swap
		// Stack for prefix: [ref] (*ref)+-1
		// Stack for postfix: *ref [ref] (*ref)+-1
		if (m_currentLValue->sizeOnStack() == 1)
			m_context << eth::Instruction::SWAP1;
		m_currentLValue->storeValue(
			*_unaryOperation.getType(), _unaryOperation.getLocation(),
			!_unaryOperation.isPrefixOperation());
		m_currentLValue.reset();
		break;
	case Token::Add: // +
		// unary add, so basically no-op
		break;
	case Token::Sub: // -
		m_context << u256(0) << eth::Instruction::SUB;
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Invalid unary operator: " +
																		 string(Token::toString(_unaryOperation.getOperator()))));
	}
	return false;
}

bool ExpressionCompiler::visit(BinaryOperation const& _binaryOperation)
{
	CompilerContext::LocationSetter locationSetter(m_context, _binaryOperation);
	Expression const& leftExpression = _binaryOperation.getLeftExpression();
	Expression const& rightExpression = _binaryOperation.getRightExpression();
	Type const& commonType = _binaryOperation.getCommonType();
	Token::Value const c_op = _binaryOperation.getOperator();

	if (c_op == Token::And || c_op == Token::Or) // special case: short-circuiting
		appendAndOrOperatorCode(_binaryOperation);
	else if (commonType.getCategory() == Type::Category::IntegerConstant)
		m_context << commonType.literalValue(nullptr);
	else
	{
		bool cleanupNeeded = commonType.getCategory() == Type::Category::Integer &&
			(Token::isCompareOp(c_op) || c_op == Token::Div || c_op == Token::Mod);

		// for commutative operators, push the literal as late as possible to allow improved optimization
		auto isLiteral = [](Expression const& _e)
		{
			return dynamic_cast<Literal const*>(&_e) || _e.getType()->getCategory() == Type::Category::IntegerConstant;
		};
		bool swap = m_optimize && Token::isCommutativeOp(c_op) && isLiteral(rightExpression) && !isLiteral(leftExpression);
		if (swap)
		{
			leftExpression.accept(*this);
			appendTypeConversion(*leftExpression.getType(), commonType, cleanupNeeded);
			rightExpression.accept(*this);
			appendTypeConversion(*rightExpression.getType(), commonType, cleanupNeeded);
		}
		else
		{
			rightExpression.accept(*this);
			appendTypeConversion(*rightExpression.getType(), commonType, cleanupNeeded);
			leftExpression.accept(*this);
			appendTypeConversion(*leftExpression.getType(), commonType, cleanupNeeded);
		}
		if (Token::isCompareOp(c_op))
			appendCompareOperatorCode(c_op, commonType);
		else
			appendOrdinaryBinaryOperatorCode(c_op, commonType);
	}

	// do not visit the child nodes, we already did that explicitly
	return false;
}

bool ExpressionCompiler::visit(FunctionCall const& _functionCall)
{
	CompilerContext::LocationSetter locationSetter(m_context, _functionCall);
	using Location = FunctionType::Location;
	if (_functionCall.isTypeConversion())
	{
		//@todo struct construction
		solAssert(_functionCall.getArguments().size() == 1, "");
		solAssert(_functionCall.getNames().empty(), "");
		Expression const& firstArgument = *_functionCall.getArguments().front();
		firstArgument.accept(*this);
		appendTypeConversion(*firstArgument.getType(), *_functionCall.getType());
	}
	else
	{
		FunctionType const& function = dynamic_cast<FunctionType const&>(*_functionCall.getExpression().getType());
		TypePointers const& parameterTypes = function.getParameterTypes();
		vector<ASTPointer<Expression const>> const& callArguments = _functionCall.getArguments();
		vector<ASTPointer<ASTString>> const& callArgumentNames = _functionCall.getNames();
		if (!function.takesArbitraryParameters())
			solAssert(callArguments.size() == parameterTypes.size(), "");

		vector<ASTPointer<Expression const>> arguments;
		if (callArgumentNames.empty())
			// normal arguments
			arguments = callArguments;
		else
			// named arguments
			for (auto const& parameterName: function.getParameterNames())
			{
				bool found = false;
				for (size_t j = 0; j < callArgumentNames.size() && !found; j++)
					if ((found = (parameterName == *callArgumentNames[j])))
						// we found the actual parameter position
						arguments.push_back(callArguments[j]);
				solAssert(found, "");
			}

		switch (function.getLocation())
		{
		case Location::Internal:
		{
			// Calling convention: Caller pushes return address and arguments
			// Callee removes them and pushes return values

			eth::AssemblyItem returnLabel = m_context.pushNewTag();
			for (unsigned i = 0; i < arguments.size(); ++i)
			{
				arguments[i]->accept(*this);
				appendTypeConversion(*arguments[i]->getType(), *function.getParameterTypes()[i]);
			}
			_functionCall.getExpression().accept(*this);

			m_context.appendJump(eth::AssemblyItem::JumpType::IntoFunction);
			m_context << returnLabel;

			unsigned returnParametersSize = CompilerUtils::getSizeOnStack(function.getReturnParameterTypes());
			// callee adds return parameters, but removes arguments and return label
			m_context.adjustStackOffset(returnParametersSize - CompilerUtils::getSizeOnStack(function.getParameterTypes()) - 1);

			// @todo for now, the return value of a function is its first return value, so remove
			// all others
			for (unsigned i = 1; i < function.getReturnParameterTypes().size(); ++i)
				CompilerUtils(m_context).popStackElement(*function.getReturnParameterTypes()[i]);
			break;
		}
		case Location::External:
		case Location::Bare:
			_functionCall.getExpression().accept(*this);
			appendExternalFunctionCall(function, arguments, function.getLocation() == Location::Bare);
			break;
		case Location::Creation:
		{
			_functionCall.getExpression().accept(*this);
			solAssert(!function.gasSet(), "Gas limit set for contract creation.");
			solAssert(function.getReturnParameterTypes().size() == 1, "");
			ContractDefinition const& contract = dynamic_cast<ContractType const&>(
							*function.getReturnParameterTypes().front()).getContractDefinition();
			// copy the contract's code into memory
			bytes const& bytecode = m_context.getCompiledContract(contract);
			m_context << u256(bytecode.size());
			//@todo could be done by actually appending the Assembly, but then we probably need to compile
			// multiple times. Will revisit once external fuctions are inlined.
			m_context.appendData(bytecode);
			//@todo copy to memory position 0, shift as soon as we use memory
			m_context << u256(0) << eth::Instruction::CODECOPY;

			m_context << u256(bytecode.size());
			appendArgumentsCopyToMemory(arguments, function.getParameterTypes());
			// size, offset, endowment
			m_context << u256(0);
			if (function.valueSet())
				m_context << eth::dupInstruction(3);
			else
				m_context << u256(0);
			m_context << eth::Instruction::CREATE;
			if (function.valueSet())
				m_context << eth::swapInstruction(1) << eth::Instruction::POP;
			break;
		}
		case Location::SetGas:
		{
			// stack layout: contract_address function_id [gas] [value]
			_functionCall.getExpression().accept(*this);
			arguments.front()->accept(*this);
			appendTypeConversion(*arguments.front()->getType(), IntegerType(256), true);
			// Note that function is not the original function, but the ".gas" function.
			// Its values of gasSet and valueSet is equal to the original function's though.
			unsigned stackDepth = (function.gasSet() ? 1 : 0) + (function.valueSet() ? 1 : 0);
			if (stackDepth > 0)
				m_context << eth::swapInstruction(stackDepth);
			if (function.gasSet())
				m_context << eth::Instruction::POP;
			break;
		}
		case Location::SetValue:
			// stack layout: contract_address function_id [gas] [value]
			_functionCall.getExpression().accept(*this);
			// Note that function is not the original function, but the ".value" function.
			// Its values of gasSet and valueSet is equal to the original function's though.
			if (function.valueSet())
				m_context << eth::Instruction::POP;
			arguments.front()->accept(*this);
			break;
		case Location::Send:
			_functionCall.getExpression().accept(*this);
			m_context << u256(0); // 0 gas, we do not want to execute code
			arguments.front()->accept(*this);
			appendTypeConversion(*arguments.front()->getType(),
								 *function.getParameterTypes().front(), true);
			appendExternalFunctionCall(FunctionType(TypePointers{}, TypePointers{},
													Location::External, false, true, true), {}, true);
			break;
		case Location::Suicide:
			arguments.front()->accept(*this);
			appendTypeConversion(*arguments.front()->getType(), *function.getParameterTypes().front(), true);
			m_context << eth::Instruction::SUICIDE;
			break;
		case Location::SHA3:
		{
			m_context << u256(0);
			appendArgumentsCopyToMemory(arguments, TypePointers(), function.padArguments());
			m_context << u256(0) << eth::Instruction::SHA3;
			break;
		}
		case Location::Log0:
		case Location::Log1:
		case Location::Log2:
		case Location::Log3:
		case Location::Log4:
		{
			unsigned logNumber = int(function.getLocation()) - int(Location::Log0);
			for (unsigned arg = logNumber; arg > 0; --arg)
			{
				arguments[arg]->accept(*this);
				appendTypeConversion(*arguments[arg]->getType(), *function.getParameterTypes()[arg], true);
			}
			m_context << u256(0);
			appendExpressionCopyToMemory(*function.getParameterTypes().front(), *arguments.front());
			m_context << u256(0) << eth::logInstruction(logNumber);
			break;
		}
		case Location::Event:
		{
			_functionCall.getExpression().accept(*this);
			auto const& event = dynamic_cast<EventDefinition const&>(function.getDeclaration());
			unsigned numIndexed = 0;
			// All indexed arguments go to the stack
			for (unsigned arg = arguments.size(); arg > 0; --arg)
				if (event.getParameters()[arg - 1]->isIndexed())
				{
					++numIndexed;
					arguments[arg - 1]->accept(*this);
					appendTypeConversion(*arguments[arg - 1]->getType(),
										 *function.getParameterTypes()[arg - 1], true);
				}
			m_context << u256(h256::Arith(dev::sha3(function.getCanonicalSignature(event.getName()))));
			++numIndexed;
			solAssert(numIndexed <= 4, "Too many indexed arguments.");
			// Copy all non-indexed arguments to memory (data)
			m_context << u256(0);
			for (unsigned arg = 0; arg < arguments.size(); ++arg)
				if (!event.getParameters()[arg]->isIndexed())
					appendExpressionCopyToMemory(*function.getParameterTypes()[arg], *arguments[arg]);
			m_context << u256(0) << eth::logInstruction(numIndexed);
			break;
		}
		case Location::BlockHash:
		{
			arguments[0]->accept(*this);
			appendTypeConversion(*arguments[0]->getType(), *function.getParameterTypes()[0], true);
			m_context << eth::Instruction::BLOCKHASH;
			break;
		}
		case Location::ECRecover:
		case Location::SHA256:
		case Location::RIPEMD160:
		{
			_functionCall.getExpression().accept(*this);
			static const map<Location, u256> contractAddresses{{Location::ECRecover, 1},
															   {Location::SHA256, 2},
															   {Location::RIPEMD160, 3}};
			m_context << contractAddresses.find(function.getLocation())->second;
			for (unsigned i = function.getSizeOnStack(); i > 0; --i)
				m_context << eth::swapInstruction(i);
			appendExternalFunctionCall(function, arguments, true);
			break;
		}
		default:
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Invalid function type."));
		}
	}
	return false;
}

bool ExpressionCompiler::visit(NewExpression const&)
{
	// code is created for the function call (CREATION) only
	return false;
}

void ExpressionCompiler::endVisit(MemberAccess const& _memberAccess)
{
	CompilerContext::LocationSetter locationSetter(m_context, _memberAccess);
	ASTString const& member = _memberAccess.getMemberName();
	switch (_memberAccess.getExpression().getType()->getCategory())
	{
	case Type::Category::Contract:
	{
		bool alsoSearchInteger = false;
		ContractType const& type = dynamic_cast<ContractType const&>(*_memberAccess.getExpression().getType());
		if (type.isSuper())
			m_context << m_context.getSuperFunctionEntryLabel(member, type.getContractDefinition()).pushTag();
		else
		{
			// ordinary contract type
			u256 identifier = type.getFunctionIdentifier(member);
			if (identifier != Invalid256)
			{
				appendTypeConversion(type, IntegerType(0, IntegerType::Modifier::Address), true);
				m_context << identifier;
			}
			else
				// not found in contract, search in members inherited from address
				alsoSearchInteger = true;
		}
		if (!alsoSearchInteger)
			break;
	}
	case Type::Category::Integer:
		if (member == "balance")
		{
			appendTypeConversion(*_memberAccess.getExpression().getType(),
								 IntegerType(0, IntegerType::Modifier::Address), true);
			m_context << eth::Instruction::BALANCE;
		}
		else if (member == "send" || member.substr(0, min<size_t>(member.size(), 4)) == "call")
			appendTypeConversion(*_memberAccess.getExpression().getType(),
								 IntegerType(0, IntegerType::Modifier::Address), true);
		else
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Invalid member access to integer."));
		break;
	case Type::Category::Function:
		solAssert(!!_memberAccess.getExpression().getType()->getMemberType(member),
				 "Invalid member access to function.");
		break;
	case Type::Category::Magic:
		// we can ignore the kind of magic and only look at the name of the member
		if (member == "coinbase")
			m_context << eth::Instruction::COINBASE;
		else if (member == "timestamp")
			m_context << eth::Instruction::TIMESTAMP;
		else if (member == "difficulty")
			m_context << eth::Instruction::DIFFICULTY;
		else if (member == "number")
			m_context << eth::Instruction::NUMBER;
		else if (member == "gaslimit")
			m_context << eth::Instruction::GASLIMIT;
		else if (member == "sender")
			m_context << eth::Instruction::CALLER;
		else if (member == "value")
			m_context << eth::Instruction::CALLVALUE;
		else if (member == "origin")
			m_context << eth::Instruction::ORIGIN;
		else if (member == "gas")
			m_context << eth::Instruction::GAS;
		else if (member == "gasprice")
			m_context << eth::Instruction::GASPRICE;
		else if (member == "data")
			m_context << u256(0) << eth::Instruction::CALLDATASIZE;
		else if (member == "sig")
			m_context << u256(0) << eth::Instruction::CALLDATALOAD
				<< (u256(0xffffffff) << (256 - 32)) << eth::Instruction::AND;
		else
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown magic member."));
		break;
	case Type::Category::Struct:
	{
		StructType const& type = dynamic_cast<StructType const&>(*_memberAccess.getExpression().getType());
		m_context << type.getStorageOffsetOfMember(member) << eth::Instruction::ADD;
		setLValueToStorageItem(_memberAccess);
		break;
	}
	case Type::Category::Enum:
	{
		EnumType const& type = dynamic_cast<EnumType const&>(*_memberAccess.getExpression().getType());
		m_context << type.getMemberValue(_memberAccess.getMemberName());
		break;
	}
	case Type::Category::TypeType:
	{
		TypeType const& type = dynamic_cast<TypeType const&>(*_memberAccess.getExpression().getType());
		if (!type.getMembers().getMemberType(member))
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Invalid member access to " + type.toString()));

		if (auto contractType = dynamic_cast<ContractType const*>(type.getActualType().get()))
		{
			ContractDefinition const& contract = contractType->getContractDefinition();
			for (ASTPointer<FunctionDefinition> const& function: contract.getDefinedFunctions())
				if (function->getName() == member)
				{
					m_context << m_context.getFunctionEntryLabel(*function).pushTag();
					return;
				}
			solAssert(false, "Function not found in member access.");
		}
		else if (auto enumType = dynamic_cast<EnumType const*>(type.getActualType().get()))
			m_context << enumType->getMemberValue(_memberAccess.getMemberName());
		break;
	}
	case Type::Category::Array:
	{
		solAssert(member == "length", "Illegal array member.");
		auto const& type = dynamic_cast<ArrayType const&>(*_memberAccess.getExpression().getType());
		if (!type.isDynamicallySized())
		{
			CompilerUtils(m_context).popStackElement(type);
			m_context << type.getLength();
		}
		else
			switch (type.getLocation())
			{
			case ArrayType::Location::CallData:
				m_context << eth::Instruction::SWAP1 << eth::Instruction::POP;
				break;
			case ArrayType::Location::Storage:
				setLValue<StorageArrayLength>(_memberAccess, type);
				break;
			default:
				solAssert(false, "Unsupported array location.");
				break;
			}
		break;
	}
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Member access to unknown type."));
	}
}

bool ExpressionCompiler::visit(IndexAccess const& _indexAccess)
{
	CompilerContext::LocationSetter locationSetter(m_context, _indexAccess);
	_indexAccess.getBaseExpression().accept(*this);

	Type const& baseType = *_indexAccess.getBaseExpression().getType();
	if (baseType.getCategory() == Type::Category::Mapping)
	{
		Type const& keyType = *dynamic_cast<MappingType const&>(baseType).getKeyType();
		m_context << u256(0);
		solAssert(_indexAccess.getIndexExpression(), "Index expression expected.");
		appendExpressionCopyToMemory(keyType, *_indexAccess.getIndexExpression());
		solAssert(baseType.getSizeOnStack() == 1,
				  "Unexpected: Not exactly one stack slot taken by subscriptable expression.");
		m_context << eth::Instruction::SWAP1;
		appendTypeMoveToMemory(IntegerType(256));
		m_context << u256(0) << eth::Instruction::SHA3;
		setLValueToStorageItem( _indexAccess);
	}
	else if (baseType.getCategory() == Type::Category::Array)
	{
		// stack layout: <base_ref> [<length>] <index>
		ArrayType const& arrayType = dynamic_cast<ArrayType const&>(baseType);
		solAssert(_indexAccess.getIndexExpression(), "Index expression expected.");
		ArrayType::Location location = arrayType.getLocation();
		eth::Instruction load =
			location == ArrayType::Location::Storage ? eth::Instruction::SLOAD :
			location == ArrayType::Location::Memory ? eth::Instruction::MLOAD :
			eth::Instruction::CALLDATALOAD;

		_indexAccess.getIndexExpression()->accept(*this);
		// retrieve length
		if (!arrayType.isDynamicallySized())
			m_context << arrayType.getLength();
		else if (location == ArrayType::Location::CallData)
			// length is stored on the stack
			m_context << eth::Instruction::SWAP1;
		else
			m_context << eth::Instruction::DUP2 << load;
		// stack: <base_ref> <index> <length>
		// check out-of-bounds access
		m_context << eth::Instruction::DUP2 << eth::Instruction::LT;
		eth::AssemblyItem legalAccess = m_context.appendConditionalJump();
		// out-of-bounds access throws exception (just STOP for now)
		m_context << eth::Instruction::STOP;

		m_context << legalAccess;
		// stack: <base_ref> <index>
		if (arrayType.isByteArray())
			// byte array is packed differently, especially in storage
			switch (location)
			{
			case ArrayType::Location::Storage:
				// byte array index storage lvalue on stack (goal):
				// <ref> <byte_number> = <base_ref + index / 32> <index % 32>
				m_context << u256(32) << eth::Instruction::SWAP2;
				CompilerUtils(m_context).computeHashStatic();
				// stack: 32 index data_ref
				m_context
					<< eth::Instruction::DUP3 << eth::Instruction::DUP3
					<< eth::Instruction::DIV << eth::Instruction::ADD
				// stack: 32 index (data_ref + index / 32)
					<< eth::Instruction::SWAP2 << eth::Instruction::SWAP1 << eth::Instruction::MOD;
				setLValue<StorageByteArrayElement>(_indexAccess);
				break;
			case ArrayType::Location::CallData:
				// no lvalue, just retrieve the value
				m_context
					<< eth::Instruction::ADD << eth::Instruction::CALLDATALOAD
					<< ((u256(0xff) << (256 - 8)))  << eth::Instruction::AND;
				break;
			case ArrayType::Location::Memory:
				solAssert(false, "Memory lvalues not yet implemented.");
			}
		else
		{
			u256 elementSize =
				location == ArrayType::Location::Storage ?
					arrayType.getBaseType()->getStorageSize() :
					arrayType.getBaseType()->getCalldataEncodedSize();
			solAssert(elementSize != 0, "Invalid element size.");
			if (elementSize > 1)
				m_context << elementSize << eth::Instruction::MUL;
			if (arrayType.isDynamicallySized())
			{
				if (location == ArrayType::Location::Storage)
				{
					m_context << eth::Instruction::SWAP1;
					CompilerUtils(m_context).computeHashStatic();
				}
				else if (location == ArrayType::Location::Memory)
					m_context << u256(32) << eth::Instruction::ADD;
			}
			m_context << eth::Instruction::ADD;
			switch (location)
			{
			case ArrayType::Location::CallData:
				if (arrayType.getBaseType()->isValueType())
					CompilerUtils(m_context).loadFromMemoryDynamic(*arrayType.getBaseType(), true, true, false);
				break;
			case ArrayType::Location::Storage:
				setLValueToStorageItem(_indexAccess);
				break;
			case ArrayType::Location::Memory:
				solAssert(false, "Memory lvalues not yet implemented.");
			}
		}
	}
	else
		solAssert(false, "Index access only allowed for mappings or arrays.");

	return false;
}

void ExpressionCompiler::endVisit(Identifier const& _identifier)
{
	CompilerContext::LocationSetter locationSetter(m_context, _identifier);
	Declaration const* declaration = _identifier.getReferencedDeclaration();
	if (MagicVariableDeclaration const* magicVar = dynamic_cast<MagicVariableDeclaration const*>(declaration))
	{
		switch (magicVar->getType()->getCategory())
		{
		case Type::Category::Contract:
			// "this" or "super"
			if (!dynamic_cast<ContractType const&>(*magicVar->getType()).isSuper())
				m_context << eth::Instruction::ADDRESS;
			break;
		case Type::Category::Integer:
			// "now"
			m_context << eth::Instruction::TIMESTAMP;
			break;
		default:
			break;
		}
	}
	else if (FunctionDefinition const* functionDef = dynamic_cast<FunctionDefinition const*>(declaration))
		m_context << m_context.getVirtualFunctionEntryLabel(*functionDef).pushTag();
	else if (auto variable = dynamic_cast<VariableDeclaration const*>(declaration))
	{
		if (!variable->isConstant())
			setLValueFromDeclaration(*declaration, _identifier);
		else
			variable->getValue()->accept(*this);
	}
	else if (dynamic_cast<ContractDefinition const*>(declaration))
	{
		// no-op
	}
	else if (dynamic_cast<EventDefinition const*>(declaration))
	{
		// no-op
	}
	else if (dynamic_cast<EnumDefinition const*>(declaration))
	{
		// no-op
	}
	else
	{
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Identifier type not expected in expression context."));
	}
}

void ExpressionCompiler::endVisit(Literal const& _literal)
{
	CompilerContext::LocationSetter locationSetter(m_context, _literal);
	switch (_literal.getType()->getCategory())
	{
	case Type::Category::IntegerConstant:
	case Type::Category::Bool:
	case Type::Category::FixedBytes:
		m_context << _literal.getType()->literalValue(&_literal);
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Only integer, boolean and string literals implemented for now."));
	}
}

void ExpressionCompiler::appendAndOrOperatorCode(BinaryOperation const& _binaryOperation)
{
	Token::Value const c_op = _binaryOperation.getOperator();
	solAssert(c_op == Token::Or || c_op == Token::And, "");

	_binaryOperation.getLeftExpression().accept(*this);
	m_context << eth::Instruction::DUP1;
	if (c_op == Token::And)
		m_context << eth::Instruction::ISZERO;
	eth::AssemblyItem endLabel = m_context.appendConditionalJump();
	m_context << eth::Instruction::POP;
	_binaryOperation.getRightExpression().accept(*this);
	m_context << endLabel;
}

void ExpressionCompiler::appendCompareOperatorCode(Token::Value _operator, Type const& _type)
{
	if (_operator == Token::Equal || _operator == Token::NotEqual)
	{
		m_context << eth::Instruction::EQ;
		if (_operator == Token::NotEqual)
			m_context << eth::Instruction::ISZERO;
	}
	else
	{
		IntegerType const& type = dynamic_cast<IntegerType const&>(_type);
		bool const c_isSigned = type.isSigned();

		switch (_operator)
		{
		case Token::GreaterThanOrEqual:
			m_context << (c_isSigned ? eth::Instruction::SLT : eth::Instruction::LT)
					  << eth::Instruction::ISZERO;
			break;
		case Token::LessThanOrEqual:
			m_context << (c_isSigned ? eth::Instruction::SGT : eth::Instruction::GT)
					  << eth::Instruction::ISZERO;
			break;
		case Token::GreaterThan:
			m_context << (c_isSigned ? eth::Instruction::SGT : eth::Instruction::GT);
			break;
		case Token::LessThan:
			m_context << (c_isSigned ? eth::Instruction::SLT : eth::Instruction::LT);
			break;
		default:
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown comparison operator."));
		}
	}
}

void ExpressionCompiler::appendOrdinaryBinaryOperatorCode(Token::Value _operator, Type const& _type)
{
	if (Token::isArithmeticOp(_operator))
		appendArithmeticOperatorCode(_operator, _type);
	else if (Token::isBitOp(_operator))
		appendBitOperatorCode(_operator);
	else if (Token::isShiftOp(_operator))
		appendShiftOperatorCode(_operator);
	else
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown binary operator."));
}

void ExpressionCompiler::appendArithmeticOperatorCode(Token::Value _operator, Type const& _type)
{
	IntegerType const& type = dynamic_cast<IntegerType const&>(_type);
	bool const c_isSigned = type.isSigned();

	switch (_operator)
	{
	case Token::Add:
		m_context << eth::Instruction::ADD;
		break;
	case Token::Sub:
		m_context << eth::Instruction::SUB;
		break;
	case Token::Mul:
		m_context << eth::Instruction::MUL;
		break;
	case Token::Div:
		m_context  << (c_isSigned ? eth::Instruction::SDIV : eth::Instruction::DIV);
		break;
	case Token::Mod:
		m_context << (c_isSigned ? eth::Instruction::SMOD : eth::Instruction::MOD);
		break;
	case Token::Exp:
		m_context << eth::Instruction::EXP;
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown arithmetic operator."));
	}
}

void ExpressionCompiler::appendBitOperatorCode(Token::Value _operator)
{
	switch (_operator)
	{
	case Token::BitOr:
		m_context << eth::Instruction::OR;
		break;
	case Token::BitAnd:
		m_context << eth::Instruction::AND;
		break;
	case Token::BitXor:
		m_context << eth::Instruction::XOR;
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown bit operator."));
	}
}

void ExpressionCompiler::appendShiftOperatorCode(Token::Value _operator)
{
	BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Shift operators not yet implemented."));
	switch (_operator)
	{
	case Token::SHL:
		break;
	case Token::SAR:
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown shift operator."));
	}
}

void ExpressionCompiler::appendHighBitsCleanup(IntegerType const& _typeOnStack)
{
	if (_typeOnStack.getNumBits() == 256)
		return;
	else if (_typeOnStack.isSigned())
		m_context << u256(_typeOnStack.getNumBits() / 8 - 1) << eth::Instruction::SIGNEXTEND;
	else
		m_context << ((u256(1) << _typeOnStack.getNumBits()) - 1) << eth::Instruction::AND;
}

void ExpressionCompiler::appendExternalFunctionCall(FunctionType const& _functionType,
													vector<ASTPointer<Expression const>> const& _arguments,
													bool bare)
{
	solAssert(_functionType.takesArbitraryParameters() ||
			  _arguments.size() == _functionType.getParameterTypes().size(), "");

	// Assumed stack content here:
	// <stack top>
	// value [if _functionType.valueSet()]
	// gas [if _functionType.gasSet()]
	// function identifier [unless bare]
	// contract address

	unsigned gasValueSize = (_functionType.gasSet() ? 1 : 0) + (_functionType.valueSet() ? 1 : 0);

	unsigned contractStackPos = m_context.currentToBaseStackOffset(1 + gasValueSize + (bare ? 0 : 1));
	unsigned gasStackPos = m_context.currentToBaseStackOffset(gasValueSize);
	unsigned valueStackPos = m_context.currentToBaseStackOffset(1);

	//@todo only return the first return value for now
	Type const* firstType = _functionType.getReturnParameterTypes().empty() ? nullptr :
							_functionType.getReturnParameterTypes().front().get();
	unsigned retSize = firstType ? firstType->getCalldataEncodedSize() : 0;
	m_context << u256(retSize) << u256(0);

	if (bare)
		m_context << u256(0);
	else
	{
		// copy function identifier
		m_context << eth::dupInstruction(gasValueSize + 3);
		CompilerUtils(m_context).storeInMemory(0, IntegerType(CompilerUtils::dataStartOffset * 8));
		m_context << u256(CompilerUtils::dataStartOffset);
	}

	// For bare call, activate "4 byte pad exception": If the first argument has exactly 4 bytes,
	// do not pad it to 32 bytes.
	appendArgumentsCopyToMemory(_arguments, _functionType.getParameterTypes(),
								_functionType.padArguments(), bare);

	// CALL arguments: outSize, outOff, inSize, (already present up to here)
	// inOff, value, addr, gas (stack top)
	m_context << u256(0);
	if (_functionType.valueSet())
		m_context << eth::dupInstruction(m_context.baseToCurrentStackOffset(valueStackPos));
	else
		m_context << u256(0);
	m_context << eth::dupInstruction(m_context.baseToCurrentStackOffset(contractStackPos));

	if (_functionType.gasSet())
		m_context << eth::dupInstruction(m_context.baseToCurrentStackOffset(gasStackPos));
	else
		// send all gas except the amount needed to execute "SUB" and "CALL"
		// @todo this retains too much gas for now, needs to be fine-tuned.
		m_context << u256(50 + (_functionType.valueSet() ? 9000 : 0)) << eth::Instruction::GAS << eth::Instruction::SUB;
	m_context << eth::Instruction::CALL;
	auto tag = m_context.appendConditionalJump();
	m_context << eth::Instruction::STOP << tag;	// STOP if CALL leaves 0.
	if (_functionType.valueSet())
		m_context << eth::Instruction::POP;
	if (_functionType.gasSet())
		m_context << eth::Instruction::POP;
	if (!bare)
		m_context << eth::Instruction::POP;
	m_context << eth::Instruction::POP; // pop contract address

	if (firstType)
		CompilerUtils(m_context).loadFromMemory(0, *firstType, false, true);
}

void ExpressionCompiler::appendArgumentsCopyToMemory(
	vector<ASTPointer<Expression const>> const& _arguments,
	TypePointers const& _types,
	bool _padToWordBoundaries,
	bool _padExceptionIfFourBytes
)
{
	solAssert(_types.empty() || _types.size() == _arguments.size(), "");
	for (size_t i = 0; i < _arguments.size(); ++i)
	{
		_arguments[i]->accept(*this);
		TypePointer const& expectedType = _types.empty() ? _arguments[i]->getType()->getRealType() : _types[i];
		appendTypeConversion(*_arguments[i]->getType(), *expectedType, true);
		bool pad = _padToWordBoundaries;
		// Do not pad if the first argument has exactly four bytes
		if (i == 0 && pad && _padExceptionIfFourBytes && expectedType->getCalldataEncodedSize(false) == 4)
			pad = false;
		appendTypeMoveToMemory(*expectedType, pad);
	}
}

void ExpressionCompiler::appendTypeMoveToMemory(Type const& _type, bool _padToWordBoundaries)
{
	CompilerUtils(m_context).storeInMemoryDynamic(_type, _padToWordBoundaries);
}

void ExpressionCompiler::appendExpressionCopyToMemory(Type const& _expectedType, Expression const& _expression)
{
	_expression.accept(*this);
	appendTypeConversion(*_expression.getType(), _expectedType, true);
	appendTypeMoveToMemory(_expectedType);
}

void ExpressionCompiler::setLValueFromDeclaration(Declaration const& _declaration, Expression const& _expression)
{
	if (m_context.isLocalVariable(&_declaration))
		setLValue<StackVariable>(_expression, _declaration);
	else if (m_context.isStateVariable(&_declaration))
			setLValue<StorageItem>(_expression, _declaration);
	else
		BOOST_THROW_EXCEPTION(InternalCompilerError()
			<< errinfo_sourceLocation(_expression.getLocation())
			<< errinfo_comment("Identifier type not supported or identifier not found."));
}

void ExpressionCompiler::setLValueToStorageItem(Expression const& _expression)
{
	setLValue<StorageItem>(_expression, *_expression.getType());
}

}
}
