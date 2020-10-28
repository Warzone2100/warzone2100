// This file is directly included in quickjs.c.

/*
 * QuickJS Debugging Extensions
 *
 * Copyright (c) 2020 Koushik Dutta
 * Copyright (c) 2020 Warzone 2100 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

JSValue js_debugger_get_current_funcObject(JSContext *ctx)
{
	JSStackFrame *sf = ctx->rt->current_stack_frame;
	return JS_DupValue(ctx, sf->cur_func);
}

JSValue js_debugger_get_caller_funcObject(JSContext *ctx)
{
	JSStackFrame *sf = ctx->rt->current_stack_frame;
	size_t levels = 0;
	for(sf = ctx->rt->current_stack_frame; levels < 2; sf = sf->prev_frame) {
		if (sf == NULL)
		{
			return JS_ThrowInternalError(ctx, "Unable to get caller function object");
		}
		levels++;
	}
	return JS_DupValue(ctx, sf->cur_func);
}

JSValue js_debugger_get_caller_name(JSContext *ctx)
{
	JSStackFrame *sf = ctx->rt->current_stack_frame;
	size_t levels = 0;
	for(sf = ctx->rt->current_stack_frame; levels < 2; sf = sf->prev_frame) {
		if (sf == NULL)
		{
			return JS_NewString(ctx, "<unknown>");
		}
		levels++;
	}
	JSValue result;
	const char *func_name_str = get_func_name(ctx, sf->cur_func);
	if (!func_name_str || func_name_str[0] == '\0')
		result = JS_NewString(ctx, "<anonymous>");
	else
		result = JS_NewString(ctx, func_name_str);
	JS_FreeCString(ctx, func_name_str);
	return result;
}

JSValue js_debugger_build_backtrace(JSContext *ctx, const uint8_t *cur_pc)
{
    JSStackFrame *sf;
    const char *func_name_str;
    JSObject *p;
    JSValue ret = JS_NewArray(ctx);
    uint32_t stack_index = 0;

    for(sf = ctx->rt->current_stack_frame; sf != NULL; sf = sf->prev_frame) {
        JSValue current_frame = JS_NewObject(ctx);

        uint32_t id = stack_index++;
        JS_SetPropertyStr(ctx, current_frame, "id", JS_NewUint32(ctx, id));

        func_name_str = get_func_name(ctx, sf->cur_func);
        if (!func_name_str || func_name_str[0] == '\0')
            JS_SetPropertyStr(ctx, current_frame, "name", JS_NewString(ctx, "<anonymous>"));
        else
            JS_SetPropertyStr(ctx, current_frame, "name", JS_NewString(ctx, func_name_str));
        JS_FreeCString(ctx, func_name_str);

        p = JS_VALUE_GET_OBJ(sf->cur_func);
        if (p && js_class_has_bytecode(p->class_id)) {
            JSFunctionBytecode *b;
            int line_num1;

            b = p->u.func.function_bytecode;
            if (b->has_debug) {
                const uint8_t *pc = sf != ctx->rt->current_stack_frame || !cur_pc ? sf->cur_pc : cur_pc;
                line_num1 = find_line_num(ctx, b, pc - b->byte_code_buf - 1);
                JS_SetPropertyStr(ctx, current_frame, "filename", JS_AtomToString(ctx, b->debug.filename));
                if (line_num1 != -1)
                    JS_SetPropertyStr(ctx, current_frame, "line", JS_NewUint32(ctx, line_num1));
            }
        } else {
            JS_SetPropertyStr(ctx, current_frame, "name", JS_NewString(ctx, "(native)"));
        }
        JS_SetPropertyUint32(ctx, ret, id, current_frame);
    }
    return ret;
}
