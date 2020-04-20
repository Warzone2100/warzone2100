/*
	PropertyMatcher - v1.0

	Copyright (c) 2020 past-due (https://github.com/past-due)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#pragma once

#include <string>

class PropertyMatcher
{
public:
	class PropertyProvider {
	public:
		virtual ~PropertyProvider();
		virtual bool getPropertyValue(const std::string& property, std::string& output_value) = 0;
	};
public:
	// Evaluate a conditional expression of property matches
	//
	// Supported Operators:
	// (...):	logical grouping
	// &&	:	logical AND
	// ||	:	logical OR
	// !	:	negation
	// =~	:	regex binding operator (applies the regex provided by its second operand to the value of the PROPERTY provided by its first operand)
	//
	// PROPERTY_NAME =~ "MATCH_REGEX"
	//
	// Generally, you should enclose the regex inside double-quotes (""). Literal double-quotes inside the regex string should be escaped as: \"
	// The PROPERTY_NAME should be in all-caps, not contain whitespace, and *not* be enclosed in quotes.
	//
	// For regex syntax, see: https://github.com/google/re2/wiki/Syntax
	//
	// The regex engine used is RE2.
	// > RE2 was designed and implemented with an explicit goal of being able to handle regular expressions from untrusted users without risk.
	// > https://github.com/google/re2/wiki/WhyRE2
	//
	// Expects a `PropertyProvider` subclass that implements the `getPropertyValue` method.
	//
	// Some examples, assuming a PropertyProvider that can query values for property names "GIT_BRANCH" and "PLATFORM"
	//	- GIT_BRANCH =~ "^master$" && PLATFORM =~ "^Windows"
	//	- !(GIT_BRANCH =~ "^master$") || (GIT_BRANCH =~ "^master$" && PLATFORM =~ "^Windows")
	static bool evaluateConditionString(const std::string& conditionString, PropertyProvider& propertyProvider);
};
