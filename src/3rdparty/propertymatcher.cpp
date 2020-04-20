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

#include "propertymatcher.h"
#include <string>
#include <memory>
#include <sstream>
#include <re2/re2.h>

PropertyMatcher::PropertyProvider::~PropertyProvider() { }

static bool regexMatch(const std::string &inputString, const std::string &regexString)
{
	// Compile regular expression
	RE2 re(regexString, RE2::Quiet);  // don't write to stderr for parser failure
	if (!re.ok())
	{
		// Failed to compile regular expression
		std::stringstream errMsg;
		errMsg << "Failed to compile regular expression: \"" << regexString << "\"; with error: \"" <<  re.error() << "\"";
		throw std::runtime_error(errMsg.str());
		return false;
	}

	bool result = RE2::PartialMatch(inputString, re);
	return result;
}

#define MATCH_ANY_WHITESPACE "\\s*"
#define MATCH_ANY_NON_WHITESPACE "\\S+"
#define MATCH_ANY_OPERATOR "!|(\\(|\\)|=~|&&|\\|\\|)"
#define MATCH_ANY_QUOTED_OR_UNQUOTED_TOKEN_STRING "((\"(?:[^\"\\\\]|\\\\.)*\")|([^\"\\s][^\\s\\)]+))"
#define MATCH_REGEX_MATCH_EXPRESSION "(\\S+)\\s+=~\\s+" MATCH_ANY_QUOTED_OR_UNQUOTED_TOKEN_STRING

void str_replace(std::string& string, const std::string& find, const std::string& replace)
{
	if (find.empty()) { return; }
	std::string::size_type pos = 0;
	while((pos = string.find(find, pos)) != std::string::npos) {
		string.replace(pos, find.length(), replace);
		pos += replace.length();
	}
}

static std::string handleStringQuotes(const std::string& str)
{
	if (str.empty() || str.front() != '"' || str.back() != '"')
	{
		return str;
	}

	std::string result = str;
	// Remove escaping for " in string
	str_replace(result, "\\\"", "\"");
	// Remove beginning and ending quotes
	result.pop_back();
	if (!result.empty()) result.erase(result.begin());
	return result;
}

static std::vector<std::string> tokenizeMatchString(const std::string& str)
{
	std::vector<std::string> tokens;
	re2::StringPiece input(str);
	std::string token;

	// Tokenize the input string
	// - treat `PROPERTY_NAME =~ "MATCH_REGEX"` as a single token
	while (RE2::Consume(&input, MATCH_ANY_WHITESPACE "(" MATCH_ANY_OPERATOR "|(" MATCH_REGEX_MATCH_EXPRESSION ")|" MATCH_ANY_QUOTED_OR_UNQUOTED_TOKEN_STRING "|" MATCH_ANY_NON_WHITESPACE ")", &token))
	{
		tokens.push_back(token);
	}

	return tokens;
}

class PropertyMatchingRule
{
public:
	PropertyMatchingRule(const std::string& propertyName, const std::string& matchRegex)
	: propertyName(propertyName)
	, matchRegex(matchRegex)
	{ }
public:
	std::string propertyName;
	std::string matchRegex;
};

class Expression {
public:
	virtual ~Expression() { }
	virtual bool evaluate() = 0;
};

class IdentityExpression : public Expression {
public:
	IdentityExpression(bool value)
	: value(value)
	{ }
	virtual bool evaluate() override
	{
		return value;
	}
private:
bool value;
};

class PropertyMatchingRuleExpression : public Expression {
public:
	PropertyMatchingRuleExpression(const PropertyMatchingRule& rule, PropertyMatcher::PropertyProvider& propertyProvider)
	: rule(rule)
	, propertyProvider(propertyProvider)
	{ }
	virtual bool evaluate() override
	{
		std::string propertyValue;
		if (!propertyProvider.getPropertyValue(rule.propertyName, propertyValue))
		{
			// If the property provider failed to provide a value for this property, return false
			return false;
		}
		return regexMatch(propertyValue, rule.matchRegex);
	}
public:
	PropertyMatchingRule rule;
	PropertyMatcher::PropertyProvider& propertyProvider;
};

bool eval_negate(bool a1, bool a2)
{
	return !a1;
}
bool eval_or(bool a1, bool a2)
{
	return a1 || a2;
}
bool eval_and(bool a1, bool a2)
{
	return a1 && a2;
}

enum {ASSOC_NONE = 0, ASSOC_LEFT, ASSOC_RIGHT};

struct op_s {
	std::string op;
	int prec;
	int assoc;
	int unary;
	bool (*eval)(bool a1, bool a2);
} ops[]={
	{"!", 10, ASSOC_RIGHT, 1, eval_negate},
	{"||", 8, ASSOC_LEFT, 0, eval_or},
	{"&&", 5, ASSOC_LEFT, 0, eval_and},
	{"(", 0, ASSOC_NONE, 0, nullptr},
	{")", 0, ASSOC_NONE, 0, nullptr}
};

struct op_s* getop(const std::string& token)
{
	int i;
	for (i=0; i < sizeof ops / sizeof ops[0]; ++i) {
		if (ops[i].op == token) return ops+i;
	}
	return nullptr;
}

bool PropertyMatcher::evaluateConditionString(const std::string& conditionString, PropertyMatcher::PropertyProvider& propertyProvider)
{
	auto tokens = tokenizeMatchString(conditionString);
	if (tokens.empty())
	{
		throw std::runtime_error("Error parsing tokens");
	}

	std::vector<op_s *> opstack;
	std::vector<std::unique_ptr<Expression>> output;

	auto push_opstack = [&opstack](struct op_s *op) {
		opstack.push_back(op);
	};
	auto pop_opstack = [&opstack]() -> op_s * {
		op_s * result = nullptr;
		if (!opstack.empty())
		{
			result = opstack.back();
			opstack.pop_back();
		}
		return result;
	};
	auto push_output = [&output](std::unique_ptr<Expression>&& exp) {
		output.push_back(std::move(exp));
	};
	auto pop_output = [&output]() -> std::unique_ptr<Expression> {
		std::unique_ptr<Expression> result;
		if (!output.empty())
		{
			result = std::move(output.back());
			output.pop_back();
		}
		return result;
	};

	auto shunt_op = [&](struct op_s *op)
	{
	   struct op_s *pop;
		std::unique_ptr<Expression> exp1, exp2;
	   if (op->op == "(") {
		   push_opstack(op);
		   return;
	   } else if (op->op == ")") {
		   while (!opstack.empty() && opstack.back()->op != "(") {
			   pop = pop_opstack();
			   exp1 = pop_output();
			   if (pop->unary) push_output(std::unique_ptr<Expression>(new IdentityExpression(pop->eval(exp1->evaluate(), false))));
			   else {
				   exp2 = pop_output();
				   push_output(std::unique_ptr<Expression>(new IdentityExpression(pop->eval(exp2->evaluate(), exp1->evaluate()))));
			   }
		   }
		   if (!(pop = pop_opstack()) || pop->op != "(") {
			   throw std::runtime_error("ERROR: Stack error. No matching \'(\'");
		   }
		   return;
	   }

	   if (op->assoc == ASSOC_RIGHT) {
		   while (!opstack.empty() && op->prec < opstack.back()->prec) {
			   pop = pop_opstack();
			   exp1 = pop_output();
			   if (pop->unary) push_output(std::unique_ptr<Expression>(new IdentityExpression(pop->eval(exp1->evaluate(), false))));
			   else {
				   exp2 = pop_output();
				   push_output(std::unique_ptr<Expression>(new IdentityExpression(pop->eval(exp2->evaluate(), exp1->evaluate()))));
			   }
		   }
	   } else {
		   while (!opstack.empty() && op->prec <= opstack.back()->prec) {
			   pop = pop_opstack();
			   exp1 = pop_output();
			   if (pop->unary) push_output(std::unique_ptr<Expression>(new IdentityExpression(pop->eval(exp1->evaluate(), false))));
			   else {
				   exp2 = pop_output();
				   push_output(std::unique_ptr<Expression>(new IdentityExpression(pop->eval(exp2->evaluate(), exp1->evaluate()))));
			   }
		   }
	   }
	   push_opstack(op);
	};

	RE2 matchesRegexMatch(MATCH_REGEX_MATCH_EXPRESSION, RE2::Quiet);  // don't write to stderr for parser failure
	if (!matchesRegexMatch.ok())
	{
		// Should not happen
		std::stringstream errMsg;
		errMsg << "Failed to compile base match expression with error: \"" <<  matchesRegexMatch.error() << "\"";
		throw std::runtime_error(errMsg.str());
	}

	struct op_s startop = {"X", 0, ASSOC_NONE, 0, nullptr};	/* Dummy operator to mark start */
	struct op_s *op = nullptr;
	std::unique_ptr<Expression> exp1, exp2;
	struct op_s *lastop = &startop;

	// Use shunting-yard to process tokens + operations
	for (auto& token : tokens)
	{
		if ((op = getop(token))) {
			if (lastop && (lastop == &startop || lastop->op != ")")) {
				if (op->op != "(" && op->op != "!") {
					std::stringstream errMsg;
					errMsg << "ERROR: Illegal use of binary operator (" << op->op << ")";
					throw std::runtime_error(errMsg.str());
				}
			}
			shunt_op(op);
			lastop = op;
		}
		else
		{
			// Handle different types of tokens / expressions
			std::string property;
			std::string regexStr;
			if (RE2::FullMatch(token, matchesRegexMatch, &property, &regexStr))
			{
				property = handleStringQuotes(property);
				regexStr = handleStringQuotes(regexStr);
				PropertyMatchingRuleExpression* expression = new PropertyMatchingRuleExpression(PropertyMatchingRule(property, regexStr), propertyProvider);
				push_output(std::unique_ptr<Expression>(expression));
			}
			else
			{
				// Unknown / unexpected token
				std::stringstream errMsg;
				errMsg << "ERROR: Unknown, invalid, or unexpected token: \"" << token << "\"";
				throw std::runtime_error(errMsg.str());
			}
			lastop = nullptr;
		}
	}

	while (!opstack.empty()) {
		op = pop_opstack();
		exp1 = pop_output();
		if (op->unary) push_output(std::unique_ptr<Expression>(new IdentityExpression(op->eval(exp1->evaluate(), false))));
		else {
			exp2 = pop_output();
			push_output(std::unique_ptr<Expression>(new IdentityExpression(op->eval(exp2->evaluate(), exp1->evaluate()))));
		}
	}
	if (output.size() != 1) {
		std::stringstream errMsg;
		errMsg << "ERROR: Result stack has " << output.size() << " elements after evaluation. Should be 1.";
		throw std::runtime_error(errMsg.str());
	}
	bool result = output.front()->evaluate();
	return result;
}
