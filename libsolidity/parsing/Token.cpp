// Copyright 2006-2012, the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
//      copyright notice, this list of conditions and the following
//      disclaimer in the documentation and/or other materials provided
//      with the distribution.
//    * Neither the name of Google Inc. nor the names of its
//      contributors may be used to endorse or promote products derived
//      from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Modifications as part of cpp-ethereum under the following license:
//
// cpp-ethereum is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// cpp-ethereum is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.

#include <map>
#include <libsolidity/parsing/Token.h>

using namespace std;

namespace dev
{
namespace solidity
{

bool ElementaryTypeNameToken::isElementaryTypeName(Token::Value _baseType, string const& _info)
{
	if (!Token::isElementaryTypeName(_baseType))
		return false;
	string baseType = Token::toString(_baseType);
	if (baseType.find('M') == string::npos)
		return true;
	short m;
	m = stoi(_info.substr(_info.find_first_of("0123456789")));

	if (baseType == "bytesM")
		return (0 < m && m <= 32) ? true : false;
	else if (baseType == "uintM" || baseType == "intM")
		return (0 < m && m <= 256 && m % 8 == 0) ? true : false;
	else if (baseType == "ufixedMxN" || baseType == "fixedMxN")
	{
		short n;
		m = stoi(_info.substr(_info.find_first_of("0123456789"), _info.find_last_of("x") - 1));
		n = stoi(_info.substr(_info.find_last_of("x") + 1));
		return (0 < n + m && n + m <= 256 && ((n % 8 == 0) && (m % 8 == 0)));
	}
	return false;
}

tuple<string, unsigned int, unsigned int> ElementaryTypeNameToken::setTypes(Token::Value _baseType, string const& _toSet)
{
	string baseType = Token::toString(_baseType);
	if (_toSet.find_first_of("0123456789") == string::npos)
		return make_tuple(baseType, 0, 0);
	baseType = baseType.substr(0, baseType.find('M') - 1) + _toSet;
	size_t index = _toSet.find('x') == string::npos ? string::npos : _toSet.find('x') - 1;
	unsigned int m = stoi(_toSet.substr(0, index));
	unsigned int n = 0;
	if (baseType == "fixed" || baseType == "ufixed")
		n = stoi(_toSet.substr(_toSet.find('x') + 1));
	return make_tuple(baseType, m, n);
}

#define T(name, string, precedence) #name,
char const* const Token::m_name[NUM_TOKENS] =
{
	TOKEN_LIST(T, T)
};
#undef T


#define T(name, string, precedence) string,
char const* const Token::m_string[NUM_TOKENS] =
{
	TOKEN_LIST(T, T)
};
#undef T


#define T(name, string, precedence) precedence,
int8_t const Token::m_precedence[NUM_TOKENS] =
{
	TOKEN_LIST(T, T)
};
#undef T


#define KT(a, b, c) 'T',
#define KK(a, b, c) 'K',
char const Token::m_tokenType[] =
{
	TOKEN_LIST(KT, KK)
};
Token::Value Token::fromIdentifierOrKeyword(const string& _name)
{
	// The following macros are used inside TOKEN_LIST and cause non-keyword tokens to be ignored
	// and keywords to be put inside the keywords variable.
#define KEYWORD(name, string, precedence) {string, Token::name},
#define TOKEN(name, string, precedence)
	static const map<string, Token::Value> keywords({TOKEN_LIST(TOKEN, KEYWORD)});
#undef KEYWORD
#undef TOKEN
	auto it = keywords.find(_name);
	return it == keywords.end() ? Token::Identifier : it->second;
}

#undef KT
#undef KK

}
}
