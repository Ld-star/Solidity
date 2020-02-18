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
 * Yul code and data object container.
 */

#pragma once

#include <libyul/AsmDataForward.h>
#include <libyul/YulString.h>

#include <libsolutil/Common.h>

#include <memory>
#include <set>

namespace solidity::yul
{
struct Dialect;
struct AsmAnalysisInfo;


/**
 * Generic base class for both Yul objects and Yul data.
 */
struct ObjectNode
{
	virtual ~ObjectNode() = default;
	virtual std::string toString(Dialect const* _dialect) const = 0;
	std::string toString() { return toString(nullptr); }

	YulString name;
};

/**
 * Named data in Yul objects.
 */
struct Data: ObjectNode
{
	Data(YulString _name, bytes _data): data(std::move(_data)) { name = _name; }
	std::string toString(Dialect const* _dialect) const override;

	bytes data;
};

/**
 * Yul code and data object container.
 */
struct Object: ObjectNode
{
public:
	/// @returns a (parseable) string representation. Includes types if @a _yul is set.
	std::string toString(Dialect const* _dialect) const override;

	/// @returns the set of names of data objects accessible from within the code of
	/// this object.
	std::set<YulString> dataNames() const;

	std::shared_ptr<Block> code;
	std::vector<std::shared_ptr<ObjectNode>> subObjects;
	std::map<YulString, size_t> subIndexByName;
	std::shared_ptr<yul::AsmAnalysisInfo> analysisInfo;
};

}
