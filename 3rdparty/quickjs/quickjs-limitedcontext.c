// This file is directly included in quickjs.c.

/*
 * QuickJS Limited Context Extensions
 *
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

#include "quickjs-limitedcontext.h"

JSContext *JS_NewLimitedContext(JSRuntime *rt, const JSLimitedContextOptions* options)
{
	JSContext *ctx;

	ctx = JS_NewContextRaw(rt);
	if (!ctx)
		return NULL;

	if (options->baseObjects)
		JS_AddIntrinsicBaseObjects(ctx);
	if (options->dateObject)
		JS_AddIntrinsicDate(ctx);
	if (options->eval)
		JS_AddIntrinsicEval(ctx);	// required for JS_Eval (etc) to work
	if (options->stringNormalize)
		JS_AddIntrinsicStringNormalize(ctx);
	if (options->regExp)
		JS_AddIntrinsicRegExp(ctx);
	if (options->json)
		JS_AddIntrinsicJSON(ctx);
	if (options->proxy)
		JS_AddIntrinsicProxy(ctx);
	if (options->mapSet)
		JS_AddIntrinsicMapSet(ctx);
	if (options->typedArrays)
		JS_AddIntrinsicTypedArrays(ctx);
	if (options->promise)
		JS_AddIntrinsicPromise(ctx);
//#ifdef CONFIG_BIGNUM
//	JS_AddIntrinsicBigInt(ctx);
//#endif
	return ctx;
}
