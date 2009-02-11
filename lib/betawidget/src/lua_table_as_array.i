%{
#include <vector>
%}

%define %table_as_array(T, COUNT, ARRAY)
%typemap (in, checkfn="lua_istable") (size_t COUNT, const T* ARRAY) (std::vector<T> array)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "ipairs");
	lua_pushvalue(L, $input);

	if (lua_pcall(L, 1, 3, 0) != 0)
		goto fail;

	for (;;)
	{
		T* element = 0;

		lua_pushvalue(L, -3);
		lua_pushvalue(L, -3);
		lua_pushvalue(L, -3);
		if (lua_pcall(L, 2, 2, 0) != 0)
		{
			goto fail;
		}

		if (!SWIG_isptrtype(L, -1))
		{
			SWIG_fail_arg("$symname", -1, "T *");
		}

		if (!SWIG_IsOK(SWIG_ConvertPtr(L, -1, (void**)&element, SWIGTYPE_p_##T, 0)))
		{
			SWIG_fail_ptr("$symname", -1, SWIGTYPE_p_##T);
		}
		lua_pop(L, 1);
		lua_replace(L, -2);

		if (!element)
			break;

		array.push_back(*element);
	}

	lua_pop(L, 3);

	$1 = array.size();
	$2 = &array[0];
}
%enddef
