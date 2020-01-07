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
 * Yul interpreter module that evaluates Ewasm builtins.
 */

#pragma once

#include <libyul/AsmDataForward.h>

#include <libsolutil/CommonData.h>

#include <vector>

namespace solidity::evmasm
{
enum class Instruction: uint8_t;
}

namespace solidity::yul
{
class YulString;
struct BuiltinFunctionForEVM;
}

namespace solidity::yul::test
{

struct InterpreterState;

/**
 * Interprets Ewasm builtins based on the current state and logs instructions with
 * side-effects.
 *
 * Since this is mainly meant to be used for differential fuzz testing, it is focused
 * on a single contract only, does not do any gas counting and differs from the correct
 * implementation in many ways:
 *
 * - If memory access to a "large" memory position is performed, a deterministic
 *   value is returned. Data that is stored in a "large" memory position is not
 *   retained.
 * - The blockhash instruction returns a fixed value if the argument is in range.
 * - Extcodesize returns a deterministic value depending on the address.
 * - Extcodecopy copies a deterministic value depending on the address.
 * - And many other things
 *
 * The main focus is that the generated execution trace is the same for equivalent executions
 * and likely to be different for non-equivalent executions.
 */
class EwasmBuiltinInterpreter
{
public:
	explicit EwasmBuiltinInterpreter(InterpreterState& _state):
		m_state(_state)
	{}
	/// Evaluate builtin function
	u256 evalBuiltin(YulString _fun, std::vector<u256> const& _arguments);

private:
	/// Checks if the memory access is not too large for the interpreter and adjusts
	/// msize accordingly.
	/// @returns false if the amount of bytes read is lager than 0xffff
	bool accessMemory(u256 const& _offset, u256 const& _size = 32);
	/// @returns the memory contents at the provided address.
	/// Does not adjust msize, use @a accessMemory for that
	bytes readMemory(uint64_t _offset, uint64_t _size = 32);
	/// @returns the memory contents at the provided address (little-endian).
	/// Does not adjust msize, use @a accessMemory for that
	uint64_t readMemoryWord(uint64_t _offset);
	/// Writes a word to memory (little-endian)
	/// Does not adjust msize, use @a accessMemory for that
	void writeMemoryWord(uint64_t _offset, uint64_t _value);
	/// Writes a byte to memory
	/// Does not adjust msize, use @a accessMemory for that
	void writeMemoryByte(uint64_t _offset, uint8_t _value);

	/// Helper for eth.* builtins. Writes to memory (big-endian) and always returns zero.
	void writeU256(uint64_t _offset, u256 _value, size_t _croppedTo = 32);
	void writeU128(uint64_t _offset, u256 _value) { writeU256(_offset, std::move(_value), 16); }
	void writeAddress(uint64_t _offset, u256 _value) { writeU256(_offset, std::move(_value), 20); }
	/// Helper for eth.* builtins. Reads from memory (big-endian) and returns the value;
	u256 readU256(uint64_t _offset, size_t _croppedTo = 32);
	u256 readU128(uint64_t _offset) { return readU256(_offset, 16); }
	u256 readAddress(uint64_t _offset) { return readU256(_offset, 20); }

	void logTrace(evmasm::Instruction _instruction, std::vector<u256> const& _arguments = {}, bytes const& _data = {});
	/// Appends a log to the trace representing an instruction or similar operation by string,
	/// with arguments and auxiliary data (if nonempty).
	void logTrace(std::string const& _pseudoInstruction, std::vector<u256> const& _arguments = {}, bytes const& _data = {});

	InterpreterState& m_state;
};

}
