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
 * Solidity data types
 */

#pragma once

#include <memory>
#include <string>
#include <map>
#include <boost/noncopyable.hpp>
#include <libdevcore/Common.h>
#include <libsolidity/interface/Exceptions.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/parsing/Token.h>
#include <libdevcore/UndefMacros.h>

namespace dev
{
namespace solidity
{

class Type; // forward
class FunctionType; // forward
using TypePointer = std::shared_ptr<Type const>;
using FunctionTypePointer = std::shared_ptr<FunctionType const>;
using TypePointers = std::vector<TypePointer>;


enum class DataLocation { Storage, CallData, Memory };

/**
 * Helper class to compute storage offsets of members of structs and contracts.
 */
class StorageOffsets
{
public:
	/// Resets the StorageOffsets objects and determines the position in storage for each
	/// of the elements of @a _types.
	void computeOffsets(TypePointers const& _types);
	/// @returns the offset of the given member, might be null if the member is not part of storage.
	std::pair<u256, unsigned> const* offset(size_t _index) const;
	/// @returns the total number of slots occupied by all members.
	u256 const& storageSize() const { return m_storageSize; }

private:
	u256 m_storageSize;
	std::map<size_t, std::pair<u256, unsigned>> m_offsets;
};

/**
 * List of members of a type.
 */
class MemberList
{
public:
	struct Member
	{
		Member(std::string const& _name, TypePointer const& _type, Declaration const* _declaration = nullptr):
			name(_name),
			type(_type),
			declaration(_declaration)
		{
		}

		std::string name;
		TypePointer type;
		Declaration const* declaration = nullptr;
	};

	using MemberMap = std::vector<Member>;

	MemberList() {}
	explicit MemberList(MemberMap const& _members): m_memberTypes(_members) {}
	MemberList& operator=(MemberList&& _other);
	TypePointer memberType(std::string const& _name) const
	{
		TypePointer type;
		for (auto const& it: m_memberTypes)
			if (it.name == _name)
			{
				solAssert(!type, "Requested member type by non-unique name.");
				type = it.type;
			}
		return type;
	}
	MemberMap membersByName(std::string const& _name) const
	{
		MemberMap members;
		for (auto const& it: m_memberTypes)
			if (it.name == _name)
				members.push_back(it);
		return members;
	}
	/// @returns the offset of the given member in storage slots and bytes inside a slot or
	/// a nullptr if the member is not part of storage.
	std::pair<u256, unsigned> const* memberStorageOffset(std::string const& _name) const;
	/// @returns the number of storage slots occupied by the members.
	u256 const& storageSize() const;

	MemberMap::const_iterator begin() const { return m_memberTypes.begin(); }
	MemberMap::const_iterator end() const { return m_memberTypes.end(); }

private:
	MemberMap m_memberTypes;
	mutable std::unique_ptr<StorageOffsets> m_storageOffsets;
};

/**
 * Abstract base class that forms the root of the type hierarchy.
 */
class Type: private boost::noncopyable, public std::enable_shared_from_this<Type>
{
public:
	enum class Category
	{
		Integer, IntegerConstant, StringLiteral, Bool, Real, Array,
		FixedBytes, Contract, Struct, Function, Enum, Tuple,
		Mapping, TypeType, Modifier, Magic
	};

	/// @{
	/// @name Factory functions
	/// Factory functions that convert an AST @ref TypeName to a Type.
	static TypePointer fromElementaryTypeName(Token::Value _typeToken);
	static TypePointer fromElementaryTypeName(std::string const& _name);
	/// @}

	/// Auto-detect the proper type for a literal. @returns an empty pointer if the literal does
	/// not fit any type.
	static TypePointer forLiteral(Literal const& _literal);
	/// @returns a pointer to _a or _b if the other is implicitly convertible to it or nullptr otherwise
	static TypePointer commonType(TypePointer const& _a, TypePointer const& _b);

	/// Calculates the

	virtual Category category() const = 0;
	virtual bool isImplicitlyConvertibleTo(Type const& _other) const { return *this == _other; }
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const
	{
		return isImplicitlyConvertibleTo(_convertTo);
	}
	/// @returns the resulting type of applying the given unary operator or an empty pointer if
	/// this is not possible.
	/// The default implementation does not allow any unary operator.
	virtual TypePointer unaryOperatorResult(Token::Value) const { return TypePointer(); }
	/// @returns the resulting type of applying the given binary operator or an empty pointer if
	/// this is not possible.
	/// The default implementation allows comparison operators if a common type exists
	virtual TypePointer binaryOperatorResult(Token::Value _operator, TypePointer const& _other) const
	{
		return Token::isCompareOp(_operator) ? commonType(shared_from_this(), _other) : TypePointer();
	}

	virtual bool operator==(Type const& _other) const { return category() == _other.category(); }
	virtual bool operator!=(Type const& _other) const { return !this->operator ==(_other); }

	/// @returns number of bytes used by this type when encoded for CALL. If it is a dynamic type,
	/// returns the size of the pointer (usually 32). Returns 0 if the type cannot be encoded
	/// in calldata.
	/// If @a _padded then it is assumed that each element is padded to a multiple of 32 bytes.
	virtual unsigned calldataEncodedSize(bool _padded) const { (void)_padded; return 0; }
	/// @returns the size of this data type in bytes when stored in memory. For memory-reference
	/// types, this is the size of the memory pointer.
	virtual unsigned memoryHeadSize() const { return calldataEncodedSize(); }
	/// Convenience version of @see calldataEncodedSize(bool)
	unsigned calldataEncodedSize() const { return calldataEncodedSize(true); }
	/// @returns true if the type is dynamically encoded in calldata
	virtual bool isDynamicallySized() const { return false; }
	/// @returns the number of storage slots required to hold this value in storage.
	/// For dynamically "allocated" types, it returns the size of the statically allocated head,
	virtual u256 storageSize() const { return 1; }
	/// Multiple small types can be packed into a single storage slot. If such a packing is possible
	/// this function @returns the size in bytes smaller than 32. Data is moved to the next slot if
	/// it does not fit.
	/// In order to avoid computation at runtime of whether such moving is necessary, structs and
	/// array data (not each element) always start a new slot.
	virtual unsigned storageBytes() const { return 32; }
	/// Returns true if the type can be stored in storage.
	virtual bool canBeStored() const { return true; }
	/// Returns false if the type cannot live outside the storage, i.e. if it includes some mapping.
	virtual bool canLiveOutsideStorage() const { return true; }
	/// Returns true if the type can be stored as a value (as opposed to a reference) on the stack,
	/// i.e. it behaves differently in lvalue context and in value context.
	virtual bool isValueType() const { return false; }
	virtual unsigned sizeOnStack() const { return 1; }
	/// @returns the mobile (in contrast to static) type corresponding to the given type.
	/// This returns the corresponding integer type for IntegerConstantTypes and the pointer type
	/// for storage reference types.
	virtual TypePointer mobileType() const { return shared_from_this(); }
	/// @returns true if this is a non-value type and the data of this type is stored at the
	/// given location.
	virtual bool dataStoredIn(DataLocation) const { return false; }
	/// @returns the type of a temporary during assignment to a variable of the given type.
	/// Specifically, returns the requested itself if it can be dynamically allocated (or is a value type)
	/// and the mobile type otherwise.
	virtual TypePointer closestTemporaryType(TypePointer const& _targetType) const
	{
		return _targetType->dataStoredIn(DataLocation::Storage) ? mobileType() : _targetType;
	}

	/// Returns the list of all members of this type. Default implementation: no members.
	/// @param _currentScope scope in which the members are accessed.
	virtual MemberList const& members(ContractDefinition const* /*_currentScope*/) const { return EmptyMemberList; }
	/// Convenience method, returns the type of the given named member or an empty pointer if no such member exists.
	TypePointer memberType(std::string const& _name, ContractDefinition const* _currentScope = nullptr) const
	{
		return members(_currentScope).memberType(_name);
	}

	virtual std::string toString(bool _short) const = 0;
	std::string toString() const { return toString(false); }
	/// @returns the canonical name of this type for use in function signatures.
	/// @param _addDataLocation if true, includes data location for reference types if it is "storage".
	virtual std::string canonicalName(bool /*_addDataLocation*/) const { return toString(true); }
	virtual u256 literalValue(Literal const*) const
	{
		BOOST_THROW_EXCEPTION(
			InternalCompilerError() <<
			errinfo_comment("Literal value requested for type without literals.")
		);
	}

	/// @returns a (simpler) type that is encoded in the same way for external function calls.
	/// This for example returns address for contract types.
	/// If there is no such type, returns an empty shared pointer.
	virtual TypePointer encodingType() const { return TypePointer(); }
	/// @returns a (simpler) type that is used when decoding this type in calldata.
	virtual TypePointer decodingType() const { return encodingType(); }
	/// @returns a type that will be used outside of Solidity for e.g. function signatures.
	/// This for example returns address for contract types.
	/// If there is no such type, returns an empty shared pointer.
	/// @param _inLibrary if set, returns types as used in a library, e.g. struct and contract types
	/// are returned without modification.
	virtual TypePointer interfaceType(bool /*_inLibrary*/) const { return TypePointer(); }

protected:
	/// Convenience object used when returning an empty member list.
	static const MemberList EmptyMemberList;
};

/**
 * Any kind of integer type (signed, unsigned, address).
 */
class IntegerType: public Type
{
public:
	enum class Modifier
	{
		Unsigned, Signed, Address
	};
	virtual Category category() const override { return Category::Integer; }

	explicit IntegerType(int _bits, Modifier _modifier = Modifier::Unsigned);

	virtual bool isImplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual TypePointer binaryOperatorResult(Token::Value _operator, TypePointer const& _other) const override;

	virtual bool operator==(Type const& _other) const override;

	virtual unsigned calldataEncodedSize(bool _padded = true) const override { return _padded ? 32 : m_bits / 8; }
	virtual unsigned storageBytes() const override { return m_bits / 8; }
	virtual bool isValueType() const override { return true; }

	virtual MemberList const& members(ContractDefinition const* /*_currentScope*/) const override
	{
		return isAddress() ? AddressMemberList : EmptyMemberList;
	}

	virtual std::string toString(bool _short) const override;

	virtual TypePointer encodingType() const override { return shared_from_this(); }
	virtual TypePointer interfaceType(bool) const override { return shared_from_this(); }

	int numBits() const { return m_bits; }
	bool isAddress() const { return m_modifier == Modifier::Address; }
	bool isSigned() const { return m_modifier == Modifier::Signed; }

	static const MemberList AddressMemberList;

private:
	int m_bits;
	Modifier m_modifier;
};

/**
 * Integer constants either literals or computed. Example expressions: 2, 2+10, ~10.
 * There is one distinct type per value.
 */
class IntegerConstantType: public Type
{
public:
	virtual Category category() const override { return Category::IntegerConstant; }

	/// @returns true if the literal is a valid integer.
	static bool isValidLiteral(Literal const& _literal);

	explicit IntegerConstantType(Literal const& _literal);
	explicit IntegerConstantType(bigint _value): m_value(_value) {}

	virtual bool isImplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual TypePointer binaryOperatorResult(Token::Value _operator, TypePointer const& _other) const override;

	virtual bool operator==(Type const& _other) const override;

	virtual bool canBeStored() const override { return false; }
	virtual bool canLiveOutsideStorage() const override { return false; }

	virtual std::string toString(bool _short) const override;
	virtual u256 literalValue(Literal const* _literal) const override;
	virtual TypePointer mobileType() const override;

	/// @returns the smallest integer type that can hold the value or an empty pointer if not possible.
	std::shared_ptr<IntegerType const> integerType() const;

private:
	bigint m_value;
};

/**
 * Literal string, can be converted to bytes, bytesX or string.
 */
class StringLiteralType: public Type
{
public:
	virtual Category category() const override { return Category::StringLiteral; }

	explicit StringLiteralType(Literal const& _literal);

	virtual bool isImplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual TypePointer binaryOperatorResult(Token::Value, TypePointer const&) const override
	{
		return TypePointer();
	}

	virtual bool operator==(Type const& _other) const override;

	virtual bool canBeStored() const override { return false; }
	virtual bool canLiveOutsideStorage() const override { return false; }
	virtual unsigned sizeOnStack() const override { return 0; }

	virtual std::string toString(bool) const override { return "literal_string \"" + m_value + "\""; }
	virtual TypePointer mobileType() const override;

	std::string const& value() const { return m_value; }

private:
	std::string m_value;
};

/**
 * Bytes type with fixed length of up to 32 bytes.
 */
class FixedBytesType: public Type
{
public:
	virtual Category category() const override { return Category::FixedBytes; }

	/// @returns the smallest bytes type for the given literal or an empty pointer
	/// if no type fits.
	static std::shared_ptr<FixedBytesType> smallestTypeForLiteral(std::string const& _literal);

	explicit FixedBytesType(int _bytes);

	virtual bool isImplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual bool operator==(Type const& _other) const override;
	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual TypePointer binaryOperatorResult(Token::Value _operator, TypePointer const& _other) const override;

	virtual unsigned calldataEncodedSize(bool _padded) const override { return _padded && m_bytes > 0 ? 32 : m_bytes; }
	virtual unsigned storageBytes() const override { return m_bytes; }
	virtual bool isValueType() const override { return true; }

	virtual std::string toString(bool) const override { return "bytes" + dev::toString(m_bytes); }
	virtual TypePointer encodingType() const override { return shared_from_this(); }
	virtual TypePointer interfaceType(bool) const override { return shared_from_this(); }

	int numBytes() const { return m_bytes; }

private:
	int m_bytes;
};

/**
 * The boolean type.
 */
class BoolType: public Type
{
public:
	BoolType() {}
	virtual Category category() const override { return Category::Bool; }
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual TypePointer binaryOperatorResult(Token::Value _operator, TypePointer const& _other) const override;

	virtual unsigned calldataEncodedSize(bool _padded) const override{ return _padded ? 32 : 1; }
	virtual unsigned storageBytes() const override { return 1; }
	virtual bool isValueType() const override { return true; }

	virtual std::string toString(bool) const override { return "bool"; }
	virtual u256 literalValue(Literal const* _literal) const override;
	virtual TypePointer encodingType() const override { return shared_from_this(); }
	virtual TypePointer interfaceType(bool) const override { return shared_from_this(); }
};

/**
 * Base class used by types which are not value types and can be stored either in storage, memory
 * or calldata. This is currently used by arrays and structs.
 */
class ReferenceType: public Type
{
public:
	explicit ReferenceType(DataLocation _location): m_location(_location) {}
	DataLocation location() const { return m_location; }

	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual TypePointer binaryOperatorResult(Token::Value, TypePointer const&) const override
	{
		return TypePointer();
	}
	virtual unsigned memoryHeadSize() const override { return 32; }

	/// @returns a copy of this type with location (recursively) changed to @a _location,
	/// whereas isPointer is only shallowly changed - the deep copy is always a bound reference.
	virtual TypePointer copyForLocation(DataLocation _location, bool _isPointer) const = 0;

	virtual TypePointer mobileType() const override { return copyForLocation(m_location, true); }
	virtual bool dataStoredIn(DataLocation _location) const override { return m_location == _location; }

	/// Storage references can be pointers or bound references. In general, local variables are of
	/// pointer type, state variables are bound references. Assignments to pointers or deleting
	/// them will not modify storage (that will only change the pointer). Assignment from
	/// non-storage objects to a variable of storage pointer type is not possible.
	bool isPointer() const { return m_isPointer; }

	bool operator==(ReferenceType const& _other) const
	{
		return location() == _other.location() && isPointer() == _other.isPointer();
	}

	/// @returns a copy of @a _type having the same location as this (and is not a pointer type)
	/// if _type is a reference type and an unmodified copy of _type otherwise.
	/// This function is mostly useful to modify inner types appropriately.
	static TypePointer copyForLocationIfReference(DataLocation _location, TypePointer const& _type);

protected:
	TypePointer copyForLocationIfReference(TypePointer const& _type) const;
	/// @returns a human-readable description of the reference part of the type.
	std::string stringForReferencePart() const;

	DataLocation m_location = DataLocation::Storage;
	bool m_isPointer = true;
};

/**
 * The type of an array. The flavours are byte array (bytes), statically- (<type>[<length>])
 * and dynamically-sized array (<type>[]).
 * In storage, all arrays are packed tightly (as long as more than one elementary type fits in
 * one slot). Dynamically sized arrays (including byte arrays) start with their size as a uint and
 * thus start on their own slot.
 */
class ArrayType: public ReferenceType
{
public:
	virtual Category category() const override { return Category::Array; }

	/// Constructor for a byte array ("bytes") and string.
	explicit ArrayType(DataLocation _location, bool _isString = false):
		ReferenceType(_location),
		m_arrayKind(_isString ? ArrayKind::String : ArrayKind::Bytes),
		m_baseType(std::make_shared<FixedBytesType>(1))
	{
	}
	/// Constructor for a dynamically sized array type ("type[]")
	ArrayType(DataLocation _location, TypePointer const& _baseType):
		ReferenceType(_location),
		m_baseType(copyForLocationIfReference(_baseType))
	{
	}
	/// Constructor for a fixed-size array type ("type[20]")
	ArrayType(DataLocation _location, TypePointer const& _baseType, u256 const& _length):
		ReferenceType(_location),
		m_baseType(copyForLocationIfReference(_baseType)),
		m_hasDynamicLength(false),
		m_length(_length)
	{}

	virtual bool isImplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual bool operator==(const Type& _other) const override;
	virtual unsigned calldataEncodedSize(bool _padded) const override;
	virtual bool isDynamicallySized() const override { return m_hasDynamicLength; }
	virtual u256 storageSize() const override;
	virtual bool canLiveOutsideStorage() const override { return m_baseType->canLiveOutsideStorage(); }
	virtual unsigned sizeOnStack() const override;
	virtual std::string toString(bool _short) const override;
	virtual std::string canonicalName(bool _addDataLocation) const override;
	virtual MemberList const& members(ContractDefinition const* _currentScope) const override;
	virtual TypePointer encodingType() const override;
	virtual TypePointer decodingType() const override;
	virtual TypePointer interfaceType(bool _inLibrary) const override;

	/// @returns true if this is a byte array or a string
	bool isByteArray() const { return m_arrayKind != ArrayKind::Ordinary; }
	/// @returns true if this is a string
	bool isString() const { return m_arrayKind == ArrayKind::String; }
	TypePointer const& baseType() const { solAssert(!!m_baseType, ""); return m_baseType;}
	u256 const& length() const { return m_length; }
	u256 memorySize() const;

	TypePointer copyForLocation(DataLocation _location, bool _isPointer) const override;

private:
	/// String is interpreted as a subtype of Bytes.
	enum class ArrayKind { Ordinary, Bytes, String };

	///< Byte arrays ("bytes") and strings have different semantics from ordinary arrays.
	ArrayKind m_arrayKind = ArrayKind::Ordinary;
	TypePointer m_baseType;
	bool m_hasDynamicLength = true;
	u256 m_length;
	/// List of member types, will be lazy-initialized because of recursive references.
	mutable std::unique_ptr<MemberList> m_members;
};

/**
 * The type of a contract instance or library, there is one distinct type for each contract definition.
 */
class ContractType: public Type
{
public:
	virtual Category category() const override { return Category::Contract; }
	explicit ContractType(ContractDefinition const& _contract, bool _super = false):
		m_contract(_contract), m_super(_super) {}
	/// Contracts can be implicitly converted to super classes and to addresses.
	virtual bool isImplicitlyConvertibleTo(Type const& _convertTo) const override;
	/// Contracts can be converted to themselves and to integers.
	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual bool operator==(Type const& _other) const override;
	virtual unsigned calldataEncodedSize(bool _padded ) const override
	{
		return encodingType()->calldataEncodedSize(_padded);
	}
	virtual unsigned storageBytes() const override { return 20; }
	virtual bool canLiveOutsideStorage() const override { return true; }
	virtual bool isValueType() const override { return true; }
	virtual std::string toString(bool _short) const override;
	virtual std::string canonicalName(bool _addDataLocation) const override;

	virtual MemberList const& members(ContractDefinition const* _currentScope) const override;
	virtual TypePointer encodingType() const override
	{
		return std::make_shared<IntegerType>(160, IntegerType::Modifier::Address);
	}
	virtual TypePointer interfaceType(bool _inLibrary) const override
	{
		return _inLibrary ? shared_from_this() : encodingType();
	}

	bool isSuper() const { return m_super; }
	ContractDefinition const& contractDefinition() const { return m_contract; }

	/// Returns the function type of the constructor. Note that the location part of the function type
	/// is not used, as this type cannot be the type of a variable or expression.
	FunctionTypePointer const& constructorType() const;

	/// @returns the identifier of the function with the given name or Invalid256 if such a name does
	/// not exist.
	u256 functionIdentifier(std::string const& _functionName) const;

	/// @returns a list of all state variables (including inherited) of the contract and their
	/// offsets in storage.
	std::vector<std::tuple<VariableDeclaration const*, u256, unsigned>> stateVariables() const;

private:
	ContractDefinition const& m_contract;
	/// If true, it is the "super" type of the current contract, i.e. it contains only inherited
	/// members.
	bool m_super = false;
	/// Type of the constructor, @see constructorType. Lazily initialized.
	mutable FunctionTypePointer m_constructorType;
	/// List of member types, will be lazy-initialized because of recursive references.
	mutable std::unique_ptr<MemberList> m_members;
};

/**
 * The type of a struct instance, there is one distinct type per struct definition.
 */
class StructType: public ReferenceType
{
public:
	virtual Category category() const override { return Category::Struct; }
	explicit StructType(StructDefinition const& _struct, DataLocation _location = DataLocation::Storage):
		ReferenceType(_location), m_struct(_struct) {}
	virtual bool isImplicitlyConvertibleTo(const Type& _convertTo) const override;
	virtual bool operator==(Type const& _other) const override;
	virtual unsigned calldataEncodedSize(bool _padded) const override;
	u256 memorySize() const;
	virtual u256 storageSize() const override;
	virtual bool canLiveOutsideStorage() const override { return true; }
	virtual std::string toString(bool _short) const override;

	virtual MemberList const& members(ContractDefinition const* _currentScope) const override;
	virtual TypePointer encodingType() const override
	{
		return location() == DataLocation::Storage ? std::make_shared<IntegerType>(256) : TypePointer();
	}
	virtual TypePointer interfaceType(bool _inLibrary) const override;

	TypePointer copyForLocation(DataLocation _location, bool _isPointer) const override;

	virtual std::string canonicalName(bool _addDataLocation) const override;

	/// @returns a function that peforms the type conversion between a list of struct members
	/// and a memory struct of this type.
	FunctionTypePointer constructorType() const;

	std::pair<u256, unsigned> const& storageOffsetsOfMember(std::string const& _name) const;
	u256 memoryOffsetOfMember(std::string const& _name) const;

	StructDefinition const& structDefinition() const { return m_struct; }

	/// @returns the set of all members that are removed in the memory version (typically mappings).
	std::set<std::string> membersMissingInMemory() const;

private:
	StructDefinition const& m_struct;
	/// List of member types, will be lazy-initialized because of recursive references.
	mutable std::unique_ptr<MemberList> m_members;
};

/**
 * The type of an enum instance, there is one distinct type per enum definition.
 */
class EnumType: public Type
{
public:
	virtual Category category() const override { return Category::Enum; }
	explicit EnumType(EnumDefinition const& _enum): m_enum(_enum) {}
	virtual TypePointer unaryOperatorResult(Token::Value _operator) const override;
	virtual bool operator==(Type const& _other) const override;
	virtual unsigned calldataEncodedSize(bool _padded) const override
	{
		return encodingType()->calldataEncodedSize(_padded);
	}
	virtual unsigned storageBytes() const override;
	virtual bool canLiveOutsideStorage() const override { return true; }
	virtual std::string toString(bool _short) const override;
	virtual std::string canonicalName(bool _addDataLocation) const override;
	virtual bool isValueType() const override { return true; }

	virtual bool isExplicitlyConvertibleTo(Type const& _convertTo) const override;
	virtual TypePointer encodingType() const override
	{
		return std::make_shared<IntegerType>(8 * int(storageBytes()));
	}
	virtual TypePointer interfaceType(bool _inLibrary) const override
	{
		return _inLibrary ? shared_from_this() : encodingType();
	}

	EnumDefinition const& enumDefinition() const { return m_enum; }
	/// @returns the value that the string has in the Enum
	unsigned int memberValue(ASTString const& _member) const;

private:
	EnumDefinition const& m_enum;
	/// List of member types, will be lazy-initialized because of recursive references.
	mutable std::unique_ptr<MemberList> m_members;
};

/**
 * Type that can hold a finite sequence of values of different types.
 * In some cases, the components are empty pointers (when used as placeholders).
 */
class TupleType: public Type
{
public:
	virtual Category category() const override { return Category::Tuple; }
	explicit TupleType(std::vector<TypePointer> const& _types = std::vector<TypePointer>()): m_components(_types) {}
	virtual bool isImplicitlyConvertibleTo(Type const& _other) const override;
	virtual bool operator==(Type const& _other) const override;
	virtual TypePointer binaryOperatorResult(Token::Value, TypePointer const&) const override { return TypePointer(); }
	virtual std::string toString(bool) const override;
	virtual bool canBeStored() const override { return false; }
	virtual u256 storageSize() const override;
	virtual bool canLiveOutsideStorage() const override { return false; }
	virtual unsigned sizeOnStack() const override;
	virtual TypePointer mobileType() const override;
	/// Converts components to their temporary types and performs some wildcard matching.
	virtual TypePointer closestTemporaryType(TypePointer const& _targetType) const override;

	std::vector<TypePointer> const& components() const { return m_components; }

private:
	std::vector<TypePointer> const m_components;
};

/**
 * The type of a function, identified by its (return) parameter types.
 * @todo the return parameters should also have names, i.e. return parameters should be a struct
 * type.
 */
class FunctionType: public Type
{
public:
	/// How this function is invoked on the EVM.
	/// @todo This documentation is outdated, and Location should rather be named "Type"
	enum class Location
	{
		Internal, ///< stack-call using plain JUMP
		External, ///< external call using CALL
		CallCode, ///< extercnal call using CALLCODE, i.e. not exchanging the storage
		Bare, ///< CALL without function hash
		BareCallCode, ///< CALLCODE without function hash
		Creation, ///< external call using CREATE
		Send, ///< CALL, but without data and gas
		SHA3, ///< SHA3
		Selfdestruct, ///< SELFDESTRUCT
		ECRecover, ///< CALL to special contract for ecrecover
		SHA256, ///< CALL to special contract for sha256
		RIPEMD160, ///< CALL to special contract for ripemd160
		Log0,
		Log1,
		Log2,
		Log3,
		Log4,
		Event, ///< syntactic sugar for LOG*
		SetGas, ///< modify the default gas value for the function call
		SetValue, ///< modify the default value transfer for the function call
		BlockHash, ///< BLOCKHASH
		AddMod, ///< ADDMOD
		MulMod, ///< MULMOD
		ArrayPush, ///< .push() to a dynamically sized array in storage
		ByteArrayPush, ///< .push() to a dynamically sized byte array in storage
		ObjectCreation ///< array creation using new
	};

	virtual Category category() const override { return Category::Function; }

	/// Creates the type of a function.
	explicit FunctionType(FunctionDefinition const& _function, bool _isInternal = true);
	/// Creates the accessor function type of a state variable.
	explicit FunctionType(VariableDeclaration const& _varDecl);
	/// Creates the function type of an event.
	explicit FunctionType(EventDefinition const& _event);
	FunctionType(
		strings const& _parameterTypes,
		strings const& _returnParameterTypes,
		Location _location = Location::Internal,
		bool _arbitraryParameters = false
	): FunctionType(
		parseElementaryTypeVector(_parameterTypes),
		parseElementaryTypeVector(_returnParameterTypes),
		strings(),
		strings(),
		_location,
		_arbitraryParameters
	)
	{
	}
	FunctionType(
		TypePointers const& _parameterTypes,
		TypePointers const& _returnParameterTypes,
		strings _parameterNames = strings(),
		strings _returnParameterNames = strings(),
		Location _location = Location::Internal,
		bool _arbitraryParameters = false,
		Declaration const* _declaration = nullptr,
		bool _gasSet = false,
		bool _valueSet = false
	):
		m_parameterTypes(_parameterTypes),
		m_returnParameterTypes(_returnParameterTypes),
		m_parameterNames(_parameterNames),
		m_returnParameterNames(_returnParameterNames),
		m_location(_location),
		m_arbitraryParameters(_arbitraryParameters),
		m_gasSet(_gasSet),
		m_valueSet(_valueSet),
		m_declaration(_declaration)
	{}

	TypePointers const& parameterTypes() const { return m_parameterTypes; }
	std::vector<std::string> const& parameterNames() const { return m_parameterNames; }
	std::vector<std::string> const parameterTypeNames(bool _addDataLocation) const;
	TypePointers const& returnParameterTypes() const { return m_returnParameterTypes; }
	std::vector<std::string> const& returnParameterNames() const { return m_returnParameterNames; }
	std::vector<std::string> const returnParameterTypeNames(bool _addDataLocation) const;

	virtual bool operator==(Type const& _other) const override;
	virtual std::string toString(bool _short) const override;
	virtual bool canBeStored() const override { return false; }
	virtual u256 storageSize() const override;
	virtual bool canLiveOutsideStorage() const override { return false; }
	virtual unsigned sizeOnStack() const override;
	virtual MemberList const& members(ContractDefinition const* _currentScope) const override;

	/// @returns TypePointer of a new FunctionType object. All input/return parameters are an
	/// appropriate external types (i.e. the interfaceType()s) of input/return parameters of
	/// current function.
	/// Returns an empty shared pointer if one of the input/return parameters does not have an
	/// external type.
	FunctionTypePointer interfaceFunctionType() const;

	/// @returns true if this function can take the given argument types (possibly
	/// after implicit conversion).
	bool canTakeArguments(TypePointers const& _arguments) const;
	/// @returns true if the types of parameters are equal (does't check return parameter types)
	bool hasEqualArgumentTypes(FunctionType const& _other) const;

	/// @returns true if the ABI is used for this call (only meaningful for external calls)
	bool isBareCall() const;
	Location const& location() const { return m_location; }
	/// @returns the external signature of this function type given the function name
	std::string externalSignature() const;
	/// @returns the external identifier of this function (the hash of the signature).
	u256 externalIdentifier() const;
	Declaration const& declaration() const
	{
		solAssert(m_declaration, "Requested declaration from a FunctionType that has none");
		return *m_declaration;
	}
	bool hasDeclaration() const { return !!m_declaration; }
	bool isConstant() const { return m_isConstant; }
	/// @return A shared pointer of an ASTString.
	/// Can contain a nullptr in which case indicates absence of documentation
	ASTPointer<ASTString> documentation() const;

	/// true iff arguments are to be padded to multiples of 32 bytes for external calls
	bool padArguments() const { return !(m_location == Location::SHA3 || m_location == Location::SHA256 || m_location == Location::RIPEMD160); }
	bool takesArbitraryParameters() const { return m_arbitraryParameters; }
	bool gasSet() const { return m_gasSet; }
	bool valueSet() const { return m_valueSet; }

	/// @returns a copy of this type, where gas or value are set manually. This will never set one
	/// of the parameters to fals.
	TypePointer copyAndSetGasOrValue(bool _setGas, bool _setValue) const;

	/// @returns a copy of this function type where all return parameters of dynamic size are
	/// removed and the location of reference types is changed from CallData to Memory.
	/// This is needed if external functions are called on other contracts, as they cannot return
	/// dynamic values.
	/// @param _inLibrary if true, uses CallCode as location.
	FunctionTypePointer asMemberFunction(bool _inLibrary) const;

private:
	static TypePointers parseElementaryTypeVector(strings const& _types);

	TypePointers m_parameterTypes;
	TypePointers m_returnParameterTypes;
	std::vector<std::string> m_parameterNames;
	std::vector<std::string> m_returnParameterNames;
	Location const m_location;
	/// true if the function takes an arbitrary number of arguments of arbitrary types
	bool const m_arbitraryParameters = false;
	bool const m_gasSet = false; ///< true iff the gas value to be used is on the stack
	bool const m_valueSet = false; ///< true iff the value to be sent is on the stack
	bool m_isConstant = false;
	mutable std::unique_ptr<MemberList> m_members;
	Declaration const* m_declaration = nullptr;
};

/**
 * The type of a mapping, there is one distinct type per key/value type pair.
 * Mappings always occupy their own storage slot, but do not actually use it.
 */
class MappingType: public Type
{
public:
	virtual Category category() const override { return Category::Mapping; }
	MappingType(TypePointer const& _keyType, TypePointer const& _valueType):
		m_keyType(_keyType), m_valueType(_valueType) {}

	virtual bool operator==(Type const& _other) const override;
	virtual std::string toString(bool _short) const override;
	virtual std::string canonicalName(bool _addDataLocation) const override;
	virtual bool canLiveOutsideStorage() const override { return false; }
	virtual TypePointer encodingType() const override
	{
		return std::make_shared<IntegerType>(256);
	}
	virtual TypePointer interfaceType(bool _inLibrary) const override
	{
		return _inLibrary ? shared_from_this() : TypePointer();
	}

	TypePointer const& keyType() const { return m_keyType; }
	TypePointer const& valueType() const { return m_valueType; }

private:
	TypePointer m_keyType;
	TypePointer m_valueType;
};

/**
 * The type of a type reference. The type of "uint32" when used in "a = uint32(2)" is an example
 * of a TypeType.
 * For super contracts or libraries, this has members directly.
 */
class TypeType: public Type
{
public:
	virtual Category category() const override { return Category::TypeType; }
	explicit TypeType(TypePointer const& _actualType): m_actualType(_actualType) {}
	TypePointer const& actualType() const { return m_actualType; }

	virtual TypePointer binaryOperatorResult(Token::Value, TypePointer const&) const override { return TypePointer(); }
	virtual bool operator==(Type const& _other) const override;
	virtual bool canBeStored() const override { return false; }
	virtual u256 storageSize() const override;
	virtual bool canLiveOutsideStorage() const override { return false; }
	virtual unsigned sizeOnStack() const override;
	virtual std::string toString(bool _short) const override { return "type(" + m_actualType->toString(_short) + ")"; }
	virtual MemberList const& members(ContractDefinition const* _currentScope) const override;

private:
	TypePointer m_actualType;
	/// List of member types, will be lazy-initialized because of recursive references.
	mutable std::unique_ptr<MemberList> m_members;
	mutable ContractDefinition const* m_cachedScope = nullptr;
};


/**
 * The type of a function modifier. Not used for anything for now.
 */
class ModifierType: public Type
{
public:
	virtual Category category() const override { return Category::Modifier; }
	explicit ModifierType(ModifierDefinition const& _modifier);

	virtual TypePointer binaryOperatorResult(Token::Value, TypePointer const&) const override { return TypePointer(); }
	virtual bool canBeStored() const override { return false; }
	virtual u256 storageSize() const override;
	virtual bool canLiveOutsideStorage() const override { return false; }
	virtual unsigned sizeOnStack() const override { return 0; }
	virtual bool operator==(Type const& _other) const override;
	virtual std::string toString(bool _short) const override;

private:
	TypePointers m_parameterTypes;
};


/**
 * Special type for magic variables (block, msg, tx), similar to a struct but without any reference
 * (it always references a global singleton by name).
 */
class MagicType: public Type
{
public:
	enum class Kind { Block, Message, Transaction };
	virtual Category category() const override { return Category::Magic; }

	explicit MagicType(Kind _kind);

	virtual TypePointer binaryOperatorResult(Token::Value, TypePointer const&) const override
	{
		return TypePointer();
	}

	virtual bool operator==(Type const& _other) const override;
	virtual bool canBeStored() const override { return false; }
	virtual bool canLiveOutsideStorage() const override { return true; }
	virtual unsigned sizeOnStack() const override { return 0; }
	virtual MemberList const& members(ContractDefinition const*) const override { return m_members; }

	virtual std::string toString(bool _short) const override;

private:
	Kind m_kind;

	MemberList m_members;
};

}
}
