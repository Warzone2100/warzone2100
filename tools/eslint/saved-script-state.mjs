// ESLint plugin for Warzone 2100 game scripts: state that a saved game would not keep.
//
// A GameState save keeps only a script's global variables
// (`var` declarations and properties assigned on the global object).
// Loading a game runs the script's top level again and then puts the saved globals back,
// so everything else starts over from what the top level sets up:
// - the contents of top-level `const` and `let` bindings
// - variables of an immediately invoked function expression (IIFE) wrapping a script file
// - properties set on function objects (ex. through debugGetCallerFuncObject())
// Writes to these from the top level itself are fine (a load repeats them).
// Writes from inside a function, at run time, are state that a loaded game loses.
//
// The rule follows bindings, not values: it doesn't see a write through another name that refers to the same object
// (ex. a const array that a global variable also refers to).
//
// A script is often split into several files that include each other and share one global scope,
// so a binding written in one file can be declared in another.
// The rule looks for such declarations in the other files of the script:
// those listed for it in the "wz.scriptGroups" setting
// (path prefixes relative to the working directory, where a prefix ending in "/" takes every .js file below it),
// or else the other .js files of the same directory.

import fs from "node:fs";
import path from "node:path";

const MUTATING_METHODS = new Set(["push", "pop", "shift", "unshift", "splice", "sort", "reverse", "fill", "copyWithin"]);

// The variable that `name` refers to in `scope`, or null for an undeclared (implicit global) name
function findVariable(scope, name) {
	for (let s = scope; s !== null; s = s.upper) {
		const v = s.set.get(name);
		if (v !== undefined) {
			return v;
		}
	}
	return null;
}

// The identifier a member expression chain starts from (a.b[c].d -> a), or null
function rootIdentifier(node) {
	let n = node;
	while (n.type === "MemberExpression") {
		n = n.object;
	}
	return n.type === "Identifier" ? n : null;
}

// The call at the root of a member expression chain (f().a.b -> f()), or null
function rootCall(node) {
	let n = node;
	while (n.type === "MemberExpression") {
		n = n.object;
	}
	return n.type === "CallExpression" ? n : null;
}

// True if `fn` is a function expression called right away at the top level of the script,
// as in (function(...){ ... })(...)
function isTopLevelIife(fn) {
	if (fn.type !== "FunctionExpression" && fn.type !== "ArrowFunctionExpression") {
		return false;
	}
	const call = fn.parent;
	if (call.type !== "CallExpression" || call.callee !== fn) {
		return false;
	}
	return call.parent.type === "ExpressionStatement" && call.parent.parent.type === "Program";
}

// True if the member expression writes to a function's prototype (ex. Foo.prototype.bar = ...)
function isPrototypeWrite(member) {
	for (let n = member; n.type === "MemberExpression"; n = n.object) {
		if (!n.computed && n.property.type === "Identifier" && n.property.name === "prototype") {
			return true;
		}
	}
	return false;
}

// The .js files below `dir`, recursively
function listScripts(dir, out) {
	let entries;
	try {
		entries = fs.readdirSync(dir, { withFileTypes: true });
	} catch {
		return out;
	}
	for (const e of entries) {
		const p = path.join(dir, e.name);
		if (e.isDirectory()) {
			listScripts(p, out);
		} else if (e.isFile() && e.name.endsWith(".js")) {
			out.push(p);
		}
	}
	return out;
}

// The files of the script `filename` belongs to (absolute paths)
function scriptFiles(filename, cwd, groups) {
	const rel = path.relative(cwd, filename).split(path.sep).join("/");
	for (const group of groups) {
		if (group.some((prefix) => (prefix.endsWith("/") ? rel.startsWith(prefix) : rel === prefix))) {
			const files = [];
			for (const prefix of group) {
				const abs = path.join(cwd, prefix);
				if (prefix.endsWith("/")) {
					listScripts(abs, files);
				} else {
					files.push(abs);
				}
			}
			return files;
		}
	}
	return listScripts(path.dirname(filename), []).filter((f) => path.dirname(f) === path.dirname(filename));
}

// Top-level declarations of a file: name -> "const" / "let" / "function"
const declarationCache = new Map();
function topLevelDeclarations(file, parser) {
	if (declarationCache.has(file)) {
		return declarationCache.get(file);
	}
	const result = new Map();
	declarationCache.set(file, result);
	let ast;
	try {
		const text = fs.readFileSync(file, "utf8");
		const options = { ecmaVersion: "latest", sourceType: "script", range: true, loc: true };
		ast = typeof parser.parseForESLint === "function" ? parser.parseForESLint(text, options).ast : parser.parse(text, options);
	} catch {
		return result;
	}
	const addFunctionAssignments = (statements) => {
		for (const st of statements) {
			const e = st.type === "ExpressionStatement" ? st.expression : null;
			if (e && e.type === "AssignmentExpression" && e.left.type === "MemberExpression" && !e.left.computed &&
				(e.right.type === "FunctionExpression" || e.right.type === "ArrowFunctionExpression") &&
				((e.left.object.type === "Identifier" && e.left.object.name === "_global") || e.left.object.type === "ThisExpression")) {
				result.set(e.left.property.name, "function");
			}
		}
	};
	for (const st of ast.body) {
		if (st.type === "VariableDeclaration" && st.kind !== "var") {
			for (const d of st.declarations) {
				if (d.id.type === "Identifier") {
					result.set(d.id.name, st.kind);
				}
			}
		} else if (st.type === "FunctionDeclaration" && st.id) {
			result.set(st.id.name, "function");
		} else if (st.type === "ExpressionStatement" && st.expression.type === "CallExpression" &&
			(st.expression.callee.type === "FunctionExpression" || st.expression.callee.type === "ArrowFunctionExpression") &&
			st.expression.callee.body.type === "BlockStatement") {
			// A wrapping IIFE exports its functions as _global.f = function ...
			addFunctionAssignments(st.expression.callee.body.body);
		}
	}
	addFunctionAssignments(ast.body);
	return result;
}

const savedScriptState = {
	meta: {
		type: "problem",
		docs: {
			description: "Disallow script state that a saved game would not keep (state outside global variables)",
		},
		schema: [],
		messages: {
			topLevelBinding:
				"'{{name}}' is a top-level {{kind}}: a saved game doesn't keep {{what}}, so a loaded game starts over from its initial value. Keep state that changes during the game in a var global.",
			closureVariable:
				"'{{name}}' belongs to the function wrapping this file: a saved game doesn't keep it, so a loaded game starts over from its initial value. Keep state that changes during the game in a global variable (ex. _global.{{name}}).",
			functionProperty:
				"Properties set on the function '{{name}}' aren't kept by a saved game. Keep state that changes during the game in a global variable.",
			callerFunctionObject:
				"Properties set on debugGetCallerFuncObject() aren't kept by a saved game. Keep state that changes during the game in a global variable (ex. keyed by the caller's name).",
		},
	},

	create(context) {
		const sourceCode = context.sourceCode;
		// Functions this file assigns as properties of the global object (_global.f = function ... in a wrapping IIFE,
		// or this.f = function ... at the top level), whose names aren't bindings
		const globalFunctions = new Set();

		// The kind of the top-level declaration of `name` in another file of this script, if any
		let otherDeclarations = null;
		function otherFileDeclaration(name) {
			if (otherDeclarations === null) {
				otherDeclarations = new Map();
				const groups = (context.settings.wz && context.settings.wz.scriptGroups) || [];
				const filename = context.filename;
				for (const file of scriptFiles(filename, context.cwd, groups)) {
					if (path.resolve(file) === path.resolve(filename)) {
						continue;
					}
					for (const [n, kind] of topLevelDeclarations(file, context.languageOptions.parser)) {
						if (!otherDeclarations.has(n)) {
							otherDeclarations.set(n, kind);
						}
					}
				}
			}
			return otherDeclarations.get(name);
		}

		// Report a write to `target` (the written expression) made by `node`
		function check(node, target, isMember) {
			const scope = sourceCode.getScope(node);
			// Writes made while the script's top level runs are repeated by a load
			if (scope.variableScope.type === "global") {
				return;
			}

			if (isMember && rootCall(target) !== null) {
				const call = rootCall(target);
				if (call.callee.type === "Identifier" && call.callee.name === "debugGetCallerFuncObject") {
					context.report({ node, messageId: "callerFunctionObject" });
				}
				return;
			}

			const id = rootIdentifier(target);
			if (id === null) {
				return;
			}
			const variable = findVariable(scope, id.name);
			if (variable === null || variable.defs.length === 0) {
				// Not declared in this file: declared in another file of the script, or an implicit global
				const kind = globalFunctions.has(id.name) ? "function" : otherFileDeclaration(id.name);
				if (kind === "function") {
					if (isMember && target !== id && !isPrototypeWrite(target)) {
						context.report({ node, messageId: "functionProperty", data: { name: id.name } });
					}
				} else if (kind !== undefined && (isMember || kind === "let")) {
					context.report({
						node,
						messageId: "topLevelBinding",
						data: { name: id.name, kind, what: isMember ? "changes made to its contents" : "what it is set to" },
					});
				}
				return;
			}
			const def = variable.defs[0];

			if (variable.scope.type === "global") {
				if (def.type === "Variable" && def.parent.kind !== "var" && (isMember || def.parent.kind === "let")) {
					context.report({
						node,
						messageId: "topLevelBinding",
						data: {
							name: id.name,
							kind: def.parent.kind,
							what: isMember ? "changes made to its contents" : "what it is set to",
						},
					});
				} else if (def.type === "FunctionName" && isMember && !isPrototypeWrite(target)) {
					context.report({ node, messageId: "functionProperty", data: { name: id.name } });
				}
				return;
			}

			// A variable of the function wrapping the file, written from a function inside it
			if (isTopLevelIife(variable.scope.block) && scope.variableScope !== variable.scope) {
				if (def.type === "Variable" || (def.type === "FunctionName" && isMember && !isPrototypeWrite(target))) {
					context.report({
						node,
						messageId: def.type === "FunctionName" ? "functionProperty" : "closureVariable",
						data: { name: id.name },
					});
				}
			}
		}

		function collectGlobalFunction(node) {
			const left = node.left;
			if (left.type === "MemberExpression" && !left.computed && left.property.type === "Identifier" &&
				(node.right.type === "FunctionExpression" || node.right.type === "ArrowFunctionExpression") &&
				((left.object.type === "Identifier" && left.object.name === "_global") || left.object.type === "ThisExpression")) {
				globalFunctions.add(left.property.name);
			}
		}

		return {
			Program(node) {
				// Collect before checking any write, as functions can be used before the assignment that defines them
				const visit = (n) => {
					if (n === null || typeof n.type !== "string") {
						return;
					}
					if (n.type === "AssignmentExpression") {
						collectGlobalFunction(n);
					}
					for (const key of sourceCode.visitorKeys[n.type] || []) {
						const child = n[key];
						if (Array.isArray(child)) {
							child.forEach(visit);
						} else if (child) {
							visit(child);
						}
					}
				};
				visit(node);
			},
			AssignmentExpression(node) {
				check(node, node.left, node.left.type === "MemberExpression");
			},
			UpdateExpression(node) {
				check(node, node.argument, node.argument.type === "MemberExpression");
			},
			UnaryExpression(node) {
				if (node.operator === "delete" && node.argument.type === "MemberExpression") {
					check(node, node.argument, true);
				}
			},
			CallExpression(node) {
				const callee = node.callee;
				if (callee.type === "MemberExpression" && !callee.computed && callee.property.type === "Identifier" &&
					MUTATING_METHODS.has(callee.property.name)) {
					check(node, callee.object, true);
				}
			},
		};
	},
};

export default {
	meta: { name: "wz-script-state" },
	rules: {
		"saved-script-state": savedScriptState,
	},
};
