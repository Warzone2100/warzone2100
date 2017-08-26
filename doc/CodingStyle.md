# Coding Style

## Introduction

We are trying to follow the coding style that is predominant in the old Warzone code.
Please use this style in your patches.

This means your code should look like this (note spaces, braces and line endings):

```
void functionName(const char *variableName, int *anotherVar, STRUCTURE *psStruct)
{
	if (variableName == NULL)
	{
		*anotherVar = 1 + 2 * (5 / 2);
	}
	else
	{
		*anotherVar = 1 + 2 * (atoi(variableName) / 2);
	}
}
```

This is not considered "good" c++ style anymore, but we think consistency is good
for making the code easy to read, so we want to stick to the old style even for new
code and new patches.

Indentation for scope should use tabs, while indentation for lining up wrapped lines
should use tabs to align with the start of previous line, then spaces. This way the
code will look readable with any tab size. For example, when a line with a two-line
printf begins, use tab to indent it, then use tab then spaces to indent the following
line, like this (\t for tabs):

```
\t\tprintf("some text on this line that got long, "
\t\t       "some more text on the next line");
```

## Guidelines

 * Comment always why you do something if this is not obvious.
 * Comment how you do it if it may be difficult to follow.
 * Do not comment out code you want to remove. Just remove it. Git can always get it back.
 * Always add braces following conditionals (if).
 * Put curly braces on lines of their own
 * Use stdint.h fixed size types (uint32_t etc.) when necessary, otherwise usual C types.
 * Don't use Pumpkin's legacy basic types (UDWORD etc.) in new code.
 * Try to avoid changing a ton of code when porting something C-ish to something more c++-ish
 * Do not rewrite code to C++ just to rewrite stuff to C++
 * Avoid templates unless they have clear benefit.
 * We do not use exceptions.
 * Inheritance is nice, but try to avoid unnecessary levels of abstraction.
 * Make sure your patch does not add any new compiler warnings.

We are implementing a doxygen documentation on the code, which can be accessed
[here](http://buildbot.wz2100.net/files/doxygen/).

We usually prefer to put doxygen function comments in the header files, rather than
in the source files.

## Artistic Style

For astyle, save the following as "astyle":

```
suffix=none
indent=tab
indent-switches
pad-oper
unpad-paren
min-conditional-indent=0
mode=c
indent-preprocessor
```

and call astyle like:

```
astyle --options=astyle <files>
```

## Coding techniques

We want the code to be easily readable and accessible to newcomers. That means
keeping it simple, rather than writing the coolest possible code. Don't use
fancy c++ features if basic features can do the job just as well. Don't write
clever optimizations if it makes the code hard to read, unless you are in a
performance critical section of the code.

As a wise man once said: Debugging code is harder than writing code. So if you
are writing code as cleverly as possible, you are by definition too stupid to
debug it.
