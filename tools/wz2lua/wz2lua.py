#!/usr/bin/env python

from spark import *

from sys import stdin, stdout, stderr
import os.path

from optparse import OptionParser

usage = 'usage: wz2lua.py vlofile slofile lua_vlo lua_slo'

parser = OptionParser(usage=usage)
parser.add_option( '-v', dest='verbose', default = False, action='store_true',
				   help='add "event at line xxx" comments into generated code')

(options, args) = parser.parse_args()
## print 'options ', options, 'args ', args

# global flag
# for sticking those -- event at line xxx comments into the generated code
verbose = options.verbose


'''
			'CALL_DROID_SELECTED':'',
			'CALL_MANURUN':'',
			'CALL_MANULIST':'',
			'CALL_BUILDLIST':'',
			'CALL_BUILDGRID':'',
			'CALL_RESEARCHLIST':'',
			'CALL_BUTTON_PRESSED':'',
			'CALL_DESIGN_WEAPON':'',
			'CALL_DESIGN_BODY':'',
			'CALL_DESIGN_PROPULSION':'',
			'CALL_DELIVPOINTMOVED':'',
			'CALL_DESIGN_QUIT':'',
			'CALL_OBJ_SEEN':'',
			'CALL_GAMEINIT':'',
			'CALL_DROIDDESIGNED':'',
			'CALL_RESEARCHCOMPLETED':'',
			'CALL_NEWDROID':'',
			'CALL_DROIDBUILT':'',
			'CALL_POWERGEN_BUILT':'',
			'CALL_RESEX_BUILT':'',
			'CALL_RESEARCH_BUILT':'',
			'CALL_FACTORY_BUILT':'',
			'CALL_MISSION_START':'',
			'CALL_MISSION_END':'',
			'CALL_VIDEO_QUIT':'',
			'CALL_TRANSPORTER_OFFMAP':'',
			'CALL_TRANSPORTER_LANDED':'',
			'CALL_TRANSPORTER_LANDED_B':'',
			'CALL_LAUNCH_TRANSPORTER':'',
			'CALL_START_NEXT_LEVEL':'',
			'CALL_TRANSPORTER_REINFORCE':'',
			'CALL_MISSION_TIME':'',
			'CALL_PLAYERLEFT':'',
			'CALL_STRUCT_ATTACKED':'',
			'CALL_ATTACKED':'',
			'CALL_STRUCT_DESTROYED':'',
			'CALL_STRUCTBUILT':'',
			'CALL_DROID_ATTACKED':'',
			'CALL_DROID_DESTROYED':'',
			'CALL_DROID_SEEN':'',
			'CALL_OBJ_SEEN':'',
			'CALL_ALLIANCEOFFER':'',
			'CALL_AI_MSG':'',
			'CALL_BEACON':'',
			'CALL_CONSOLE':'',
			'CALL_ELECTRONIC_TAKEOVER':'',
			'CALL_UNITTAKEOVER':'',
			'CALL_CLUSTER_EMPTY':'',
			'CALL_NO_REINFORCEMENTS_LEFT':'',
			'CALL_VTOL_OFF_MAP':'',
			'CALL_KEY_PRESSED':'',
			'CALL_DROID_REACH_LOCATION':''
'''

cvars = ['trackTransporter', 'mapWidth', 'mapHeight', 'gameInitialised', 'selectedPlayer', 'gameTime', 'gameLevel', 'inTutorial', 'cursorType', 'intMode', 'targetedObjectType', 'extraVictoryFlag', 'extraFailFlag', 'multiPlayerGameType', 'multiPlayerMaxPlayers', 'multiPlayerBaseType', 'multiPlayerAlliancesType']
# functions that are to be renamed
rename = {
	'debug'          : 'debugFile',
	'EnumDroid'      : 'enumDroid',
	'InitEnumDroids' : 'initEnumDroids',
	'MsgBox'         : 'msgBox',
	'min'            : 'math.min',
	'fmin'           : 'math.min',
	'max'            : 'math.max',
	'fmax'           : 'math.max',
	'toPow'          : 'math.pow',
	}

unused_parameters = {
	'chooseValidLoc': [0, 1],
	'getPlayerStartPosition': [1, 2],
	}

# these functions do not get an extra return like UNUSED,... = function(...)
no_extra_return = ['circlePerimPoint', 'getPlayerStartPosition']

class Token:
	def __init__(self, type, attr='', constant=True):
		self.type = type
		self.attr = attr
		self.attr_backup = attr
		self.ref = []
		self.nonref = []
		self.line = current_line
		self.code = ''
		
		self.boolean = True # set to False to make conditional expressions truly boolean
		self.constant = constant

	def __cmp__(self, o):
		return cmp(self.type, o)
	def __repr__(self):
		return self.attr or self.type
	def __getitem__(self, i):
		raise IndexError

class AST:
	def __init__(self, t, kids):
		if type(t) == type(''):
			self.type = t
			self.attr = ''
		else:
			self.type = t.type
			self.attr = t.attr
		self.kids = kids
		self.ref = []
		self.nonref = []
		# the lowest line number
		linelist = [k.line for k in self.kids]
		linelist.append(current_line)
		linelist.sort()
		self.line = linelist[0]
		self.code = ''
		
		self.boolean = True # set to False to make conditional expressions truly boolean
		self.constant = True
		for k in self.kids:
			self.constant = self.constant and k.constant
		
	def __repr__(self):
		return self.attr or self.type
	def __getitem__(self, i):
		return self.kids[i]
	def __len__(self):
		return len(self.kids)

class CodeScanner(GenericScanner):
	def tokenize(self, input):
		self.rv = []
		GenericScanner.tokenize(self, input)
		return self.rv
	def t_whitespace(self, s):
		r" [ \t]+ "
		pass
	def t_number(self, s):
		r" ([0-9]+[.][0-9]+|[.][0-9]+|[0-9]+[.]?) "
		self.rv.append(Token('number', s))
	def t_operator(self, s):
		r" \|\||==|!=|~=|>=|<=|&&|[+*<>&/] "
		self.rv.append(Token('operator', s))
	def t_name(self, s):
		r" [a-zA-Z_][a-zA-Z0-9_]* "
		c = False
		if s == "TRUE" or s == "FALSE":
			c = True
		self.rv.append(Token('name', s, constant=c))
	def t_member(self, s):
		r" [.][a-zA-Z0-9_]+ "
		self.rv.append(Token('member', s))
	def t_open(self, s):
		r" [(] "
		self.rv.append(Token('open', ''))
	def t_close(self, s):
		r" [)] "
		self.rv.append(Token('close', ''))
	#def t_emptyline(self, s):
		#r' \n[ \t]*\n '
		#self.rv.append(Token('emptyline', ''))
	def t_newline(self, s):
		r' \n '
		global current_line
		current_line += 1
	def t_define(self,s):
		r' \#define '
		self.rv.append(Token('define'))
	def t_ignore(self,s):
		r' \#.*?\n '
		global current_line
		current_line += 1
	def t_string(self,s):
		r' "([^"]|\")*?" '
		self.rv.append(Token('string', s)) 
	def t_semicolon(self,s):
		r' ; '
		self.rv.append(Token('semicolon', '')) 
	def t_assign(self, s):
		r' = '
		self.rv.append(Token('assign', ''))
	def t_comma(self, s):
		r' , '
		self.rv.append(Token('comma', ''))
	def t_begin(self, s):
		r' { '
		self.rv.append(Token('begin', ''))
	def t_end(self, s):
		r' } '
		self.rv.append(Token('end', ''))
	def t_bracket_open(self,s):
		r' \[ '
		self.rv.append(Token('bracket_open', ''))
	def t_bracket_close(self,s):
		r' \] '
		self.rv.append(Token('bracket_close', ''))
	def t_pre_operator(self, s):
		r" ! "
		self.rv.append(Token('pre_operator',s))
	def t_min(self, s):
		r" - "
		self.rv.append(Token('min'))

	def t_default(self, s):
		r'( . | \n )+'
		print "Specification error: unmatched input:\n"+repr(s[:100])
		print 'last tokens:', self.rv[-25:]
		raise SystemExit

class CodeScanner2(CodeScanner):
	def t_pre_operator_2(self, s):
		r" \bnot\b "
		self.rv.append(Token('pre_operator',s))
	def t_andor(self, s):
		r" \b(and|or|AND|OR)\b "
		self.rv.append(Token('operator',s.lower()))
	def t_incdec(self, s):
		r" \+\+|-- "
		self.rv.append(Token('post_operator',s))
	def t_reserved(self, s):
		r" \b(exit|while|return|local|if|else|function|event|trigger|ref)\b "
		self.rv.append(Token(s,''))
	def t_access(self, s):
		r" public|private "
		self.rv.append(Token('access', s))
	def t_starcomment(self, s):
		r" /[*](.|\n)*?[*]/ "
		add_comment(s[2:-2])
	def t_linecomment(self, s):
		r" //[^\n]* "
		add_comment(s[2:])
	def t_emptyline(self, s):
		r' \n[ \t]*\n '
		global current_line
		current_line += 1
		insert_newline()

current_line = 1
comments = {}
def add_comment(c):
	global current_line
	current_line_inc = c.count('\n')
	c = c.strip()
	if not current_line in comments:
		comments[current_line] = ''
	if c.find('\n') > 0:
		comments[current_line] += '--[[' + c + ']]--\n'
	else:
		if c.replace('/','') == '':
			comments[current_line] += '--' + c.replace('/','-') + '\n'
		else:
			comments[current_line] += '-- ' + c + '\n'
	current_line += current_line_inc
def insert_newline():
	global current_line
	if not current_line in comments:
		comments[current_line] = ''
	if not comments[current_line].endswith('\n\n'):
		comments[current_line] += '\n'
	current_line += 1
last_asked = 0
def pop_comments(l):
	global last_asked, comments
	c = ''#'-- pop_comments('+str(l)+') last_asked='+str(last_asked)+'\n'
	if last_asked < l - 10:
		last_asked = l - 10
		c += '\n'
	while last_asked <= l:
		#c += '--'
		if last_asked in comments:
			c += comments[last_asked]
			del comments[last_asked]
		last_asked += 1
	return c

def reset_comments():
	global current_line, comments, last_asked
	current_line = 1
	last_asked = 0
	comments = {}
	

class CodeParser(GenericParser):
	def __init__(self, start='file'):
		GenericParser.__init__(self, start)
	
	def p_file(self, args):
		''' file ::= global_stuff
			 '''
		return args[0]
		
	def p_file2(self, args):
		''' file ::= file global_stuff
			 '''
		return AST('file', [args[0], args[1]])
		
	def p_global_stuff(self, args):
		''' global_stuff ::= emptyline
			global_stuff ::= macrodef
			global_stuff ::= globaldef
			global_stuff ::= trigger_declaration
			global_stuff ::= event_prototype
			global_stuff ::= function_prototype
			global_stuff ::= event_declaration
			global_stuff ::= function_declaration
			 '''
		return args[0]
		
	def p_macrodef(self, args):
		' macrodef ::= define name expression '
		return AST('macrodef', [args[1], args[2]])
		
	def p_expression_1(self, args):
		''' expression ::= number
			expression ::= negative
			expression ::= string
			expression ::= function_call
			expression ::= variable
			 '''
		return args[0]
	 
	def p_expression_2(self, args):
		''' expression ::= open expression close '''
		return AST('braced', [args[1]])
	def p_expression_3(self, args):
		''' expression ::= cast expression '''
		return args[1]
	def p_negative(self, args):
		''' negative ::= min number '''
		return AST('negative', [args[1]])
	def p_boolean_expression(self, args):
		''' boolean_expression ::= expression '''
		return AST('boolean_expression', [args[0]])
	def p_cast(self, args):
		''' cast ::= open name close '''
		return args[1]
		
	def p_operation_1_1(self, args):
		' expression ::= expression operator expression '
		return AST(args[1], [args[0], args[2]])
	def p_operation_1_2(self, args):
		' expression ::= expression min expression '
		return AST(args[1], [args[0], args[2]])
		
	def p_operation_2_1(self, args):
		' expression ::= pre_operator expression '
		return AST(args[0], [args[1]])
	def p_operation_2_2(self, args):
		' expression ::= min expression '
		return AST(args[0], [args[1]])
	def p_operation_3(self, args):
		' expression ::= expression post_operator'
		return AST(args[1], [args[0]])		
		
	def p_globaldef(self, args):
		' globaldef ::= access name definitionlist semicolon '
		return AST('globaldef', [args[0], args[1], args[2]])
	def p_localdef(self, args):
		''' localdef ::= local name definitionlist semicolon'''
		return AST('localdef', [args[0], args[1], args[2]])

	def p_definitionlist_1(self, args):
		' definitionlist ::= variable '
		return AST('definitionlist', [args[0]])
	def p_definitionlist_2(self, args):
		' definitionlist ::= definitionlist comma variable '
		args[0].kids.append(args[2])
		return args[0]
		
	def p_variable_1(self, args):
		''' variable ::= name '''
		return AST('variable', [args[0]])	
	def p_variable_2(self, args):
		''' variable ::= expression member '''
		return AST('member_reference', [args[0], args[1]])   
	def p_variable_3(self, args):
		''' variable ::= variable bracket_open expression bracket_close '''
		return AST('arrayref', [args[0], args[2]])
		
	def p_trigger(self, args):
		''' trigger_declaration ::= trigger name arglist semicolon '''
		return AST('trigger_declaration', [args[1], args[2]])
	def p_arglist_0(self, args):
		''' arglist ::= open close '''
		return AST('nothing', [])
	def p_arglist_1(self, args):
		''' arglist ::= open _arglist close '''
		return args[1]
	def p_arglist_2(self, args):
		''' _arglist ::= parameter '''
		return args[0]
	def p_arglist_3(self, args):
		''' _arglist ::= _arglist comma parameter'''
		return AST('arglist', [args[0], args[2]])
	
	def p_parameter_1(self, args):
		''' parameter ::= expression '''
		return AST('parameter', [args[0]])
	def p_parameter_2(self, args):
		''' parameter ::= ref name '''
		return AST(args[0], [args[1]])
		
	def p_event_prototype(self, args):
		''' event_prototype ::= event name semicolon '''
		return AST('event_prototype', [args[1]])
		
	def p_function_prototype(self, args):
		''' function_prototype ::= function name name argdeflist semicolon '''
		return AST('function_prototype', [args[1], args[2], args[3]])
	def p_argdeflist_0(self, args):
		''' argdeflist ::= open close '''
		return Token('nothing')
	def p_argdeflist_1(self, args):
		''' argdeflist ::= open _argdeflist close '''
		return args[1]
	def p_argdeflist_2(self, args):
		''' _argdeflist ::= argdef '''
		return args[0]
	def p_argdeflist_3(self, args):
		''' _argdeflist ::= _argdeflist comma argdef'''
		return AST('argdeflist', [args[0], args[2]])
	def p_argdef(self, args):
		''' argdef ::= name name'''
		return AST('argdef', [args[0], args[1]])
		
	def p_event_declaration(self, args):
		''' event_declaration ::= event name arglist block '''
		return AST('event_declaration', [args[1], args[2], args[3]])
	def p_block(self, args):
		''' block ::= begin statementlist end '''
		return args[1]
	def p_statementlist_1(self, args):
		''' statementlist ::= statement '''
		return AST('statementlist', [args[0]])
	def p_statementlist_2(self, args):
		''' statementlist ::= statementlist statement '''
		args[0].kids.append(args[1])
		return args[0]
	def p_statement(self, args):
		''' statement ::= assignment
			statement ::= expression_statement
			statement ::= if_statement
			statement ::= block
			statement ::= localdef
			statement ::= return_statement
			statement ::= while_statement
			'''
		return args[0]
	def p_expression_statement(self, args):
		' expression_statement ::= expression semicolon '
		return AST('expression_statement', [args[0]])
	
	def p_assignment(self, args):
		''' assignment ::= variable assign expression semicolon '''
		return AST('assignment', [args[0], args[2]])
	def p_function_call(self, args):
		''' function_call ::= name arglist '''
		return AST('function_call', [args[0], args[1]])
		
	def p_if(self, args):
		''' if_statement ::= if open boolean_expression close statement '''
		return AST('if_statement', [args[2], args[4]])
	def p_ifelse(self, args):
		''' if_statement ::= if open boolean_expression close statement else statement'''
		return AST('ifelse_statement', [args[2], args[4], args[6]])

	def p_function_declaration(self, args):
		''' function_declaration ::= function name name argdeflist block '''
		return AST('function_declaration', [args[2], args[3], args[4]])		
	def p_return_value_statement(self, args):
		''' return_statement ::= return expression semicolon'''
		return AST('return_value_statement', [args[1]])
	def p_return_statement(self, args):
		''' return_statement ::= return semicolon'''
		t = Token('return_statement')
		t.line = args[0].line
		return t
	def p_exit_statement(self, args):
		''' return_statement ::= exit semicolon'''
		t = Token('return_statement')
		t.line = args[0].line
		return t
		
	def p_while_statement(self, args):
		''' while_statement ::= while open boolean_expression close statement '''
		return AST('while_statement', [args[2], args[4]])
		
	def resolve(self, list):
		return list[-1]
		
	def error(self, tokens, pos):
		print "Previous tokens:"
		print_tokens(tokens[:pos][-5:])
		print "Syntax error at or near `%s' token" % tokens[pos]
		print "Next tokens:"
		print_tokens(tokens[pos:pos+30])
		raise SystemExit
		
class CodeCommenter(GenericASTTraversal):
	def __init__(self, ast):
		GenericASTTraversal.__init__(self, ast)
		self.preorder()
	def n_macrodef(self, node):
		node.code = pop_comments(node.line)
	def n_function_declaration(self, node):
		node.code = pop_comments(node.line)
	def n_if_statement(self, node):
		node.code = pop_comments(node.line)
	def n_ifelse_statement(self, node):
		node.code = pop_comments(node.line)
	def n_while_statement(self, node):
		node.code = pop_comments(node.line)
	def n_return_statement(self, node):
		node.code = pop_comments(node.line)
	def n_return_value_statement(self, node):
		node.code = pop_comments(node.line)
	def n_globaldef(self, node):
		node.code = pop_comments(node.line)
	def n_event_declaration(self, node):
		node.code = pop_comments(node.line)
	def n_assignment(self, node):
		node.code = pop_comments(node.line)
	def n_expression_statement(self, node):
		node.code = pop_comments(node.line)
	def n_localdef(self, node):
		node.code = pop_comments(node.line)
	def n_file(self, node):
		node.code = pop_comments(node.line)
	def n_statementlist(self, node):
		node.code = pop_comments(node.line)
		
class TreeCleaner(GenericASTTraversal):
	def __init__(self, ast):
		GenericASTTraversal.__init__(self, ast)
		self.preorder()
	def n_default(self, node):
		node.attr = node.attr_backup
		node.ref = []
		node.nonref = []
		node.code = ''
		node.boolean = True
		node.constant = constant
		node.arglist = None
		
class CodeGenerator(GenericASTTraversal):
	def __init__(self, ast):
		GenericASTTraversal.__init__(self, ast)
		self.postorder()
	
	def default(self, node):
		node.code = self.typestring(node)+'{'+','.join([k.code for k in node])+'}'
	
	def n_file(self, node):
		node.code = ''.join([k.code for k in node])
		
	def n_macrodef(self, node):
		if node[1].constant:
			node.code += node[0].code + ' = ' + node[1].code + '\n'
		else:
			node.code += node[0].code + ' = function () return ' + node[1].code + ' end\n'
			macros.append(node[0].code)
	def n_number(self, node):
		node.code = node.attr
	def n_negative(self, node):
		node.code = '-' + node[0].code
	def n_string(self, node):
		node.code = node.attr
	def n_name(self, node):
		if node.attr in ['TRUE', 'FALSE']:
			node.code = node.attr.lower()
		elif node.attr in ['NULLOBJECT', 'NULLSTAT', 'NULLTEMPLATE']:
			node.code = 'nil'
		elif node.attr in cvars:
			node.code = 'C.'+node.attr
		elif node.attr in rename:
			node.code = rename[node.attr]
		else:
			node.code = node.attr
		
	def n_braced(self, node):
		node.code = '(' + node[0].code + ')'
		
	def n_operator(self, node):
		op = node.attr
		if op in operator_convert:
			op = operator_convert[op]
		if op == '==' and node[0].code in can_be_destroyed and node[1].code == 'nil':
			node.code = 'destroyed('+node[0].code+')'
		elif op == '!=' and node[0].code in can_be_destroyed and node[1].code == 'nil':
			node.code = 'not destroyed('+node[0].code+')'
		else:
			if op in ['and', 'or']:
				if not node[0].boolean:
					node[0].code = '(' + node[0].code + ' == true)'
				if not node[1].boolean:
					node[1].code = '(' + node[1].code + ' == true)'
				node.boolean = True
			node.code = node[0].code+' '+op+' '+node[1].code
			if op in ['==', '<=', '>=', '<', '>', '~=']:
				node.boolean = True
	def n_boolean_expression(self, node):
		if node[0].boolean:
			node.code = node[0].code
		else:
			node.code = node[0].code + ' == true'
		node.ref = node[0].ref
	def n_assignment(self, node):
		# let's check if this is an assigment of the type
		# x = f(ref b)
		if node[1].ref:
			# add the multiple returns
			node.code += node[0].code + ', ' + ', '.join(node[1].ref) + ' = ' + node[1].code + '\n'
		else:
			node.code += node[0].code + ' = ' + node[1].code + '\n'
	def n_function_declaration(self, node):
		global verbose
		node.code += 'function ' + node[0].code + '(' + node[1].code + ')' + (' -- at line '+str(node.line) if verbose else '') +'\n' + indent(node[2].code) + 'end\n'
	def n_argdef(self, node):
		node.code = node[1].code
	def n_argdeflist(self,node):
		node.code = node[0].code + ', ' + node[1].code
	def n_nothing(self,node):
		node.code = ''
	def n_statementlist(self, node):
		node.code = ''
		for n in node:
			if n.code.startswith('return ') or n.code == 'return\n':
				node.code += n.code
				# do not output any more code for this statementlist
				# as Lua will rightfully complain it is unreachable
				break
			else:
				node.code += n.code
	def n_if_statement(self, node):
		if node[0].ref:
			# we're in trouble, it is: if function_call(ref x, ref y)
			node.code += '__result, ' + ', '.join(node[0].ref) + ' = ' + node[0].code + '\n'
			node.code += 'if __result then\n'
		else:
			node.code += 'if ' + node[0].code + ' then\n'
		node.code += indent(node[1].code) + 'end\n'
	def n_ifelse_statement(self, node):
		node.code += 'if ' + node[0].code + ' then\n' + indent(node[1].code)
		node.code += 'else\n' + indent(node[2].code) + 'end\n'
	def n_while_statement(self, node):
		node.code += 'while ' + node[0].code + ' do\n' + indent(node[1].code) + 'end\n'
	def n_return_statement(self, node):
		node.code += 'return\n'
	def n_return_value_statement(self, node):
		node.code += 'return ' + node[0].code + '\n'
	def n_member_reference(self, node):
		if node[1].code == 'members':
			node.code = 'groupCountMembers('+node[0].code+')'
		else:
			node.code = node[0].code+'.'+node[1].code
	def n_member(self, node):
		node.code = node.attr[1:]
	def n_arrayref(self, node):
		node.code = node[0].code + '[' + node[1].code + ']'
		node.arraydef = ''
		node.arrayinit = ''
		if node[0].type == 'arrayref':
			if not node[0][0].code+'[]' in defined_arrays:
				# a[8][2]
				node.arraydef += node[0][0].code + ' = Array(%s, %s)\n' % (node[0][1].code, node[1].code)
				defined_arrays.add(node[0][0].code+'[]')
		else:
			if not node[0].code in defined_arrays:
				node.arraydef += node[0].code + ' = Array(%s)\n' % node[1].code
				defined_arrays.add(node[0].code)
	def n_event_declaration(self, node):
		# originally, the events were only called if certain conditions were met
		# these conditions were specific to each event
		# we now add a piece of code to each event that will check the condition
		# itself. This simplifies calling events and adds flexibility.
		
		# these have an unneccesary player parameter
		# format: 'CALL_':(<pos of player>,<pos of object with player property>)
		check_player = {
			'CALL_NEWDROID':(0,1),
			'CALL_STRUCT_ATTACKED':(0,1),
			'CALL_DROID_ATTACKED':(0,1),
			'CALL_ATTACKED':(0,1),
			'CALL_VTOL_OFF_MAP':(0,1),
			'CALL_DROID_SEEN':(0,2),
			'CALL_OBJ_SEEN':(0,2),
			'CALL_STRUCTBUILT':(0,1),
			'CALL_DROID_DESTROYED':(0,1),
			'CALL_STRUCT_DESTROYED':(0,1),
			'CALL_DROID_REACH_LOCATION':(0,1),
			}
		# these have a real player parameter
		check_player_parameter = {
			'CALL_RESEARCHCOMPLETED':2,
			'CALL_TRANSPORTER_LANDED':1,
			'CALL_TRANSPORTER_OFFMAP':0,
			'CALL_AI_MSG':0,
			'CALL_BEACON':0,
			}
		trigger_inactive = False
		if node[1].code == "inactive":
			trigger_inactive = True
			# houston, we have a problem
			# we need to figure out the arguments to this function
			# FIXME: the assigned_triggers map needs to be populated in a separate pass
			if node[0].code in assigned_triggers:
				node[1].code = assigned_triggers[node[0].code]
			elif pass_number == 2:
				stderr.write('WARNING: could not find a trigger for deactivated event '+node[0].code+'\n')
		if node[1].code in triggers:
			trigger_name = node[1].code
			trigger = triggers[trigger_name]
		else:
			trigger = self.parse_trigger(node[1])
			trigger_name = node[0].code + 'Tr'
			while trigger_name in triggers:
				trigger_name += '_'
			triggers[trigger_name] = trigger
		# prefix arguments with an underscore
		# this is so we can assign them to the globals later
		# probably not neccesary but who knows?
		args = []
		for a in trigger['arglist']:
			args.append('_'+a)
		# check if the callback has arguments that need to be checked
		check_code = ''
		if trigger['nonref']:
			# arguments to the callback function we now need to check ourselves
			if trigger['expression'] in check_player:
				pos = check_player[trigger['expression']][0]
				obj = check_player[trigger['expression']][1]
				if trigger['arglist'][pos] != '-1': # -1 means for all players
					check_code = 'if %s.player ~= %s then return end\n' % (args[obj], trigger['arglist'][pos])
				args.pop(pos)
			elif trigger['expression'] in check_player_parameter:
				pos = check_player_parameter[trigger['expression']]
				if trigger['arglist'][pos] != '-1':
					check_code = 'if _player ~= %s then return end\n' % (trigger['arglist'][pos])
				args[pos] = '_player'
			elif trigger['expression'] == 'CALL_BUTTON_PRESSED':
				# this gets tiring quickly, all those exceptions to the rules
				button = trigger['arglist'][0]
				# fix up the parameters
				trigger['arglist'][0] = 'button'
				args[0] = '_button'
				# add a check
				check_code = 'if _button ~= %s then return end\n' % (button)
			else:
				stderr.write('error: add checks for trigger '+trigger['expression']+'\n')
		argsstr = ', '.join(args)
		# write the function declaration
		node.code += 'function ' + node[0].code + '('+argsstr+')' + (' -- event at line '+str(node.line) if verbose else '') +'\n'
		node.code += indent(check_code)
		# check if we need to reassign the renamed parameters
		if trigger['ref']:
			ref_args = []
			for a in trigger['ref']:
				ref_args.append('_'+a)
			ref_argsstr = ', '.join(ref_args)
			orgargsstr = ', '.join(trigger['ref'])
			node.code += indent(orgargsstr + ' = ' + ref_argsstr + ' -- wz2lua: probably these can be used as function arguments directly\n')
		node.code += indent(node[2].code) + 'end\n'
		if not trigger_inactive:
			node.code += self.setEventTrigger_call(node[0].code,trigger) + '\n'
	def setEventTrigger_call(self, function, trigger):
		if trigger['expression'] == 'nil':
			# this is a simple trigger
			if trigger['repeating']:
				return 'repeatingEvent(' + function + ', ' + trigger['wait'] + ')'
			return 'delayedEvent(' + function + ', ' + trigger['wait'] + ')'
		if trigger['type'] == 'Callback':
			return 'callbackEvent(' + function + ', ' + trigger['expression'] + ')'
		code = 'conditionalEvent('+function+', '
		code += trigger['expression'] + ', '
		code += trigger['wait']
		if not trigger['repeating']:
			code += ', false'
		code += ')'
		return code
	def n_function_call(self, node):
		if node[0].code == 'setEventTrigger':
			# fix up the setEventTrigger call
			trigger_name = node[1].arglist[1]
			if trigger_name == 'inactive':
				node.code = 'deactivateEvent(' + node[1].arglist[0] + ')'
			else:
				node.code = self.setEventTrigger_call(node[1].arglist[0], triggers[trigger_name])
				assigned_triggers[node[1].arglist[0]] = trigger_name
		elif node[0].code in ['setReinforcementTime', 'setMissionTime', 'pause']:
			node.code = node[0].code + '(' + node[1].code + '/10.0)'
		elif node[0].code in ['buildingDestroyed']:
			node.code = 'destroyed(' + node[1][0].code + ')'
		elif node[0].code in ['addPower']:
			# reverse the arguments
			node.code = 'addPower(' + node[1].arglist[1] + ', ' + node[1].arglist[0] + ')'
		else:
			if node[0].code in unused_parameters:
				# this function has some unused parameters that will need to be removed
				new_arglist = []
				for i in xrange(0, len(node[1].arglist)):
					if not i in unused_parameters[node[0].code]:
						new_arglist.append(node[1].arglist[i])
				node[1].arglist = new_arglist
				node[1].code = ', '.join(node[1].arglist)
			node.code = node[0].code + '(' + node[1].code + ')'
			node.ref = node[1].ref
		called_functions.add(node[0].code)
		node.boolean = True
	def n_expression_statement(self, node):
		if node[0].ref:
			# this is just a lone function call(ref a, ref b)
			# as all of these also return a value, we need to add it as UNUSED
			# the ref parameters are extra returns
			if not node[0][0].code in no_extra_return:
				node.code += 'UNUSED, ' + ', '.join(node[0].ref) + ' = ' + node[0].code + '\n'
			else:
				node.code += ', '.join(node[0].ref) + ' = ' + node[0].code + '\n'
		else:
			node.code += node[0].code + '\n'
	def n_arglist(self, node):
		node.ref = node[0].ref + node[1].ref
		node.nonref = node[0].nonref + node[1].nonref
		if node[0].type == 'arglist':
			node.arglist = node[0].arglist
		else:
			node.arglist = [node[0].code]
		node.arglist.append(node[1].code)
		node.code = node[0].code + ', ' + node[1].code
	def n_parameter(self, node):
		node.code = node[0].code
		node.nonref = [node[0].code]
	def n_ref(self, node):
		node.code = node[0].code
		node.ref = [node[0].code]
	def n_definitionlist(self, node):
		node.code = ', '.join([k.code for k in node])
	def n_min(self, node):
		if len(node) > 1:
			node.code = node[0].code + ' - ' + node[1].code
		else:
			node.code = '-' + node[0].code
	def n_post_operator(self, node):
		if node.attr == '++':
			op = '+'
		if node.attr == '--':
			op = '-'
		node.code = node[0].code + ' = ' + node[0].code + ' ' + op + ' 1'
	def n_pre_operator(self, node):
		op = node.attr
		if op in operator_convert:
			op = operator_convert[op]
		if op == 'not':
			if not node[0].boolean:
				node.code = node[0].code + ' == false'
				node.boolean = True
				return
			else:
				op += ' '
		node.code = op+node[0].code
	def n_function_prototype(self, node):
		# not neccesary for lua
		node.code = ''
	def n_event_prototype(self, node):
		# not neccesary for lua
		node.code = ''
	def n_globaldef(self, node):
		for d in node[2]:
			if node[1].code in ['STRUCTURE']:
				can_be_destroyed.append(d.attr)
			if d.type == 'arrayref':
				if not d[0].code in defined_globals:
					node.code += d.arraydef
					node.code += d.arrayinit
					defined_globals.append(d[0].code)
			else:
				if not d.code in defined_globals:
					node.code += d.code + ' = '
					node[1].code = node[1].code.lower()
					if node[1].code == 'bool':
						node.code += 'false'
					elif node[1].code == 'group':
						node.code += 'Group()'
					elif node[1].code in ['int', 'float']:
						node.code += '0'
					else:
						node.code += 'nil'
					node.code += '\n'
					defined_globals.append(d.code)
	def n_localdef(self, node):
		for d in node[2]:
			node.code += 'local ' + d.code + ' = '
			node[1].code = node[1].code.lower()
			if node[1].code == 'bool':
				node.code += 'false'
			elif node[1].code == 'group':
				node.code += 'Group()'
			elif node[1].code in ['int', 'float']:
				node.code += '0'
			else:
				node.code += 'nil'
			node.code += '\n'
	def n_variable(self, node):
		if node[0].code in macros:
			node.code = node[0].code + '()'
		else:
			node.code = node[0].code
	def n_trigger_declaration(self, node):
		trigger = self.parse_trigger(node[1])

		node.code = '' # self.trigger_declaration(trigger, node[0].code)

		triggers[node[0].code] = trigger
		
	def parse_trigger(self, node):
		# 0: name 1:args
		conv = {
			'every':'Repeating',
			'wait':'Delayed',
			'init':'Init',
			'inactive' : 'Inactive'
			}
		trigger = {}
		if 'arglist' in dir(node) and node.arglist:
				expression = node.arglist[0]
				node.arglist = node.arglist[1:]
				node.nonref = node.nonref[1:]
		else:
			expression = node.code
			node.arglist = []
			node.nonref = []
		if expression in conv:
			trigger['type'] = conv[expression]
		elif expression.startswith('CALL_'):
			trigger['type'] = 'Callback'
			trigger['expression'] = expression
		else:
			trigger['type'] = 'Bool'
			trigger['expression'] = '"' + convert(expression) + '"'
		trigger['wait'] = '1'
		trigger['repeating'] = True
		if trigger['type'] in ['Delayed', 'Init']:
			trigger['repeating'] = False
		if trigger['type'] in ['Init']:
			trigger['wait'] = '0'
			trigger['expression'] = 'nil'
		if trigger['type'] in ['Repeating','Delayed', 'Bool']:
			# the second argument is the time, and is a parameter to the
			# trigger and not to the function
			#print trigger['type'], expression
			trigger['wait'] = str(int(node.arglist[0])/10.0)
			node.arglist = node.arglist[1:]
			node.nonref = node.nonref[1:]
		if trigger['type'] in ['Repeating', 'Delayed']:
			trigger['expression'] = 'nil'
		trigger['nonref'] = node.nonref
		trigger['ref'] = node.ref
		trigger['arglist'] = node.arglist
		return trigger
			
def indent(code):
	l = code.split('\n')
	c2 = ['\t'+c for c in l[:-1]]+[l[-1]]
	return '\n'.join(c2)

def print_tokens(tokens):
	for t in tokens:
		print t.type, t.attr

def printtree(p, indent=''):
	if type(p) == type(''):
		print indent+p
	else:
		#print type(p), p, dir(p)
		print indent+p.type+' '+p.attr 
		indent += '	'
		try:
			for c in p.kids:
				printtree(c, indent)
		except:
			pass
			
def convert(text, type='expression'):
	tokens = CodeScanner2().tokenize(text)
	p = CodeParser(start=type)
	parsed = p.parse(tokens)
	#printtree(parsed)
	CodeGenerator(parsed)
	return parsed.code  

class VloScanner(CodeScanner2):
	def tokenize(self, input):
		return CodeScanner2.tokenize(self, input)
		
	def t_reserved(self, s):
		r" \b(script|run)\b "
		self.rv.append(Token(s,''))	
			
class VloParser(GenericParser):
	def __init__(self, start='file'):
		GenericParser.__init__(self, start)
		
	def p_file(self, args):
		''' file ::= deflist
			file ::= runfile '''
		return args[0]
	
	def p_runfile_1(self, args):
		''' runfile ::= scriptdef run begin deflist end
			 '''
		return AST('runfile', [args[0], args[3]])
	def p_runfile_2(self, args):
		''' runfile ::= scriptdef run begin end
			 '''
		return AST('runfile_empty', [args[0]])
		
	def p_scriptdef(self, args):
		' scriptdef ::= script string '
		return args[1]
	def p_definitions(self, args):
		' deflist ::= definition '
		return AST('deflist', [args[0]])
	def p_definitions_2(self, args):
		' deflist ::= deflist definition '
		args[0].kids.append(args[1])
		return args[0]
	def p_definition(self, args):
		' definition ::= variable name value '
		return AST('definition', [args[0], args[1], args[2]])
		
	def p_variable_1(self, args):
		''' variable ::= name '''
		return args[0]	
	def p_variable_2(self, args):
		''' variable ::= variable bracket_open number bracket_close '''
		return AST('arrayref', [args[0], args[2]])
		
	def p_value(self, args):
		''' value ::= string
			value ::= number
			value ::= name '''
		return args[0]
	def p_value_2(self, args):
		''' value ::= min number '''
		return AST('negative', [args[1]])


		
class VloCodeGenerator(GenericASTTraversal):
	def __init__(self, ast):
		GenericASTTraversal.__init__(self, ast)
		self.postorder()
	def default(self, node):
		node.code = self.typestring(node)+'{'+','.join([k.code for k in node])+'}'
	def n_file(self, node):
		node.code += node[0].code
	def n_runfile(self, node):
		node.code = '-- slo: ' + node[0].code + '\n\n' + node[1].code
	def n_runfile_empty(self, node):
		node.code = '-- slo: ' + node[0].code + '\n\n'
	def n_deflist(self, node):
		node.code = ''
		node.code += ''.join([k.code for k in node])
	def n_name(self, node):
		if node.attr in ['TRUE', 'FALSE']:
			node.attr = node.attr.lower()
		#if node.attr == 'NULLOBJECT':
		#	node.attr = 'nil'
		node.code = node.attr
	def n_number(self, node):
		node.code = node.attr
	def n_negative(self, node):
		node.code = '-' + node[0].code
	def n_string(self, node):
		node.code = node.attr
	def n_arrayref(self, node):
		node.code = node[0].code + '[' + node[1].code + ']'
		node.arraydef = ''
		if node[0].type == 'arrayref':
			node.arraydef = node[0].arraydef
			if not node[0][0].code+'[]' in defined_arrays:
				defined_arrays.add(node[0][0].code+'[]')
		if not node[0].code in defined_arrays:
			node.arraydef += node[0].code + ' = {}\n'
			defined_arrays.add(node[0].code)
	def n_definition(self, node):
		node.code = pop_comments(node.line)
		if node[0].type == 'arrayref':
			node.code += node[0].arraydef
			defined_globals.append(node[0][0].code)
		if node[1].code == 'STRUCTURE':
			can_be_destroyed.append(node[0].code)
			node.code += node[0].code + ' = getStructureByID(' + node[2].code + ')\n'
		elif node[1].code == 'WEAPON':
			can_be_destroyed.append(node[0].code)
			node.code += node[0].code + ' = getWeapon(' + node[2].code + ')\n'
		else:
			node.code += node[0].code + ' = ' + node[2].code + '\n'
		defined_globals.append(node[0].code)
			
operator_convert = { '!=':'~=', '!':'not', '&&':'and', '||':'or',  '&':'..' }

triggers = {}
assigned_triggers = {}
called_functions = set()

can_be_destroyed = []
macros = []
defined_globals = []
defined_arrays = set()

def load_file(filename):
	f = file(filename)
	text = f.read()
	f.close()
	text = text.replace('\r\n', '\n')
	text = text.replace('\r', '\n')
	return text

if len(args) < 4:
	raise "not enough arguments"

vlotext = load_file(args[0])

settings_output = file(args[2], "wt")

tokens = VloScanner().tokenize(vlotext)
#print tokens
p = VloParser()
#stderr.write('Vlo: Parsing...\n')
parsed = p.parse(tokens)
#printtree(parsed)
#stderr.write('Vlo: Generating code...\n')
VloCodeGenerator(parsed)

settings_output.write("-- Generated by wz2lua (data file)\nversion(0) -- version of the script API this script is written for\n")
settings_output.write(parsed.code)
settings_output.write("\n--run the code\ndofile('%s')\n" % (os.path.basename(sys.argv[4])))
settings_output.close()

reset_comments()

if not os.path.exists(args[3]):
	slotext = load_file(args[1])
	code_output = file(args[3], "wt")

	#stderr.write('Slo: Tokenizing...\n')
	tokens = CodeScanner2().tokenize(slotext)
	#print tokens
	p = CodeParser()
	#stderr.write('Slo: Parsing...\n')
	parsed = p.parse(tokens)
	#printtree(parsed)
	#stderr.write('Slo: Generating code...\n')

	# first pass
	#stderr.write('first pass\n')
	pass_number = 1
	defined_globals_backup = defined_globals[:]
	defined_arrays_backup = set(defined_arrays)
	macros_backup = macros[:]
	CodeGenerator(parsed)

	# second pass
	#stderr.write('second pass\n')
	pass_number = 2
	TreeCleaner(parsed)
	called_functions = set()

	macros = macros_backup
	defined_globals = defined_globals_backup
	defined_arrays = defined_arrays_backup

	CodeCommenter(parsed)
	CodeGenerator(parsed)

	code_output.write("-- Generated by wz2lua (implementation file)\nversion(0) --- version of the script API this script is written for\n")
	code_output.write(parsed.code)

	code_output.write('\n\n---------- stubs ----------\n\n')

	for f in called_functions:
		code_output.write('if '+f+' == nil then '+f+' = function() print("stub: '+f+'"); return 0 end end\n')
	code_output.close()
