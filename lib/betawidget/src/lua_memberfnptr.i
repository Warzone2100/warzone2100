%{
typedef struct
{
	lua_State* L;
	int table;
} SWIGLUA_MEMBER_FN;

static void swiglua_member_fn_get(SWIGLUA_MEMBER_FN const * const fn)
{
	// memberfn.function(memberfn.weak.self, ...
	lua_getref(fn->L, fn->table);
	lua_getfield(fn->L, -1, "function");
	lua_getfield(fn->L, -2, "weak");
	lua_getfield(fn->L, -1, "self");
	lua_replace(fn->L, -2);
	lua_remove(fn->L, -3);
}

static void swiglua_member_fn_clear(SWIGLUA_MEMBER_FN* const fn)
{
	lua_unref(fn->L, fn->table);
	free(fn);
}
%}

%typemap (in, checkfn="lua_isfunction") SWIGLUA_MEMBER_FN*
%{
	// Allocate a chunk of memory to place a reference to the function in
	$1 = (SWIGLUA_MEMBER_FN*)malloc(sizeof(*$1));
        if ($1 == NULL)
        {
                lua_pushstring(L, "Error in $symname: Out of memory");
                goto fail;
        }

	/* Create a table to hold the function and a weak table with the self object in it */
	// callback = { function = function }
	lua_newtable(L); // callback = { function = function }
	lua_pushvalue(L, $input);
	lua_setfield(L, -2, "function");

	// weak = {}
	lua_newtable(L);

	// setmetatable(weak, { __mode = 'v' })
	lua_newtable(L);
	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);

	// weak.widget = self
	lua_pushvalue(L, 1);
	lua_setfield(L, -2, "self");

	// callback.weak = weak
	lua_setfield(L, -2, "weak");

	// Retrieve a reference to the callback table
	$1->L     = L;
	$1->table = lua_ref(L, true);
%}
