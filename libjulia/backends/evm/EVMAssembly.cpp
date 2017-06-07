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
 * Assembly interface for EVM and EVM1.5.
 */

#include <libjulia/backends/evm/EVMAssembly.h>

#include <libevmasm/Instruction.h>

#include <libsolidity/interface/Utils.h>

using namespace std;
using namespace dev;
using namespace julia;

namespace
{
size_t constexpr labelReferenceSize = 4;
}


void EVMAssembly::setSourceLocation(SourceLocation const&)
{
	// Ignored for now;
}

void EVMAssembly::appendInstruction(solidity::Instruction _instr)
{
	m_bytecode.push_back(byte(_instr));
	m_stackHeight += solidity::instructionInfo(_instr).ret - solidity::instructionInfo(_instr).args;
}

void EVMAssembly::appendConstant(u256 const& _constant)
{
	bytes data = toCompactBigEndian(_constant, 1);
	appendInstruction(solidity::pushInstruction(data.size()));
	m_bytecode += data;
}

void EVMAssembly::appendLabel(LabelID _labelId)
{
	setLabelToCurrentPosition(_labelId);
	appendInstruction(solidity::Instruction::JUMPDEST);
}

void EVMAssembly::appendLabelReference(LabelID _labelId)
{
	solAssert(!m_evm15, "Cannot use plain label references in EMV1.5 mode.");
	// @TODO we now always use labelReferenceSize for all labels, it could be shortened
	// for some of them.
	appendInstruction(solidity::pushInstruction(labelReferenceSize));
	m_labelReferences[m_bytecode.size()] = _labelId;
	m_bytecode += bytes(labelReferenceSize);
}

EVMAssembly::LabelID EVMAssembly::newLabelId()
{
	m_labelPositions[m_nextLabelID] = size_t(-1);
	return m_nextLabelID++;
}

void EVMAssembly::appendLinkerSymbol(string const&)
{
	solAssert(false, "Linker symbols not yet implemented.");
}

void EVMAssembly::appendJump(int _stackDiffAfter)
{
	solAssert(!m_evm15, "Plain JUMP used for EVM 1.5");
	appendInstruction(solidity::Instruction::JUMP);
	m_stackHeight += _stackDiffAfter;
}

void EVMAssembly::appendJumpTo(LabelID _labelId, int _stackDiffAfter)
{
	if (m_evm15)
	{
		m_bytecode.push_back(byte(solidity::Instruction::JUMPTO));
		appendLabelReferenceInternal(_labelId);
		m_stackHeight += _stackDiffAfter;
	}
	else
	{
		appendLabelReference(_labelId);
		appendJump(_stackDiffAfter);
	}
}

void EVMAssembly::appendJumpToIf(LabelID _labelId)
{
	if (m_evm15)
	{
		m_bytecode.push_back(byte(solidity::Instruction::JUMPIF));
		appendLabelReferenceInternal(_labelId);
		m_stackHeight--;
	}
	else
	{
		appendLabelReference(_labelId);
		appendInstruction(solidity::Instruction::JUMPI);
	}
}

void EVMAssembly::appendBeginsub(LabelID _labelId, int _arguments)
{
	solAssert(m_evm15, "BEGINSUB used for EVM 1.0");
	solAssert(_arguments >= 0, "");
	setLabelToCurrentPosition(_labelId);
	m_bytecode.push_back(byte(solidity::Instruction::BEGINSUB));
	m_stackHeight += _arguments;
}

void EVMAssembly::appendJumpsub(LabelID _labelId, int _arguments, int _returns)
{
	solAssert(m_evm15, "JUMPSUB used for EVM 1.0");
	solAssert(_arguments >= 0 && _returns >= 0, "");
	m_bytecode.push_back(byte(solidity::Instruction::JUMPSUB));
	appendLabelReferenceInternal(_labelId);
	m_stackHeight += _returns - _arguments;
}

void EVMAssembly::appendReturnsub(int _returns, int _stackDiffAfter)
{
	solAssert(m_evm15, "RETURNSUB used for EVM 1.0");
	solAssert(_returns >= 0, "");
	m_bytecode.push_back(byte(solidity::Instruction::RETURNSUB));
	m_stackHeight += _stackDiffAfter - _returns;
}

eth::LinkerObject EVMAssembly::finalize()
{
	for (auto const& ref: m_labelReferences)
	{
		size_t referencePos = ref.first;
		solAssert(m_labelPositions.count(ref.second), "");
		size_t labelPos = m_labelPositions.at(ref.second);
		solAssert(labelPos != size_t(-1), "Undefined but allocated label used.");
		solAssert(m_bytecode.size() >= 4 && referencePos <= m_bytecode.size() - 4, "");
		solAssert(uint64_t(labelPos) < (uint64_t(1) << (8 * labelReferenceSize)), "");
		for (size_t i = 0; i < labelReferenceSize; i++)
			m_bytecode[referencePos + i] = byte((labelPos >> (8 * (labelReferenceSize - i - 1))) & 0xff);
	}
	eth::LinkerObject obj;
	obj.bytecode = m_bytecode;
	return obj;
}

void EVMAssembly::setLabelToCurrentPosition(LabelID _labelId)
{
	solAssert(m_labelPositions.count(_labelId), "Label not found.");
	solAssert(m_labelPositions[_labelId] == size_t(-1), "Label already set.");
	m_labelPositions[_labelId] = m_bytecode.size();
}

void EVMAssembly::appendLabelReferenceInternal(LabelID _labelId)
{
	m_labelReferences[m_bytecode.size()] = _labelId;
	m_bytecode += bytes(labelReferenceSize);
}
