#ifndef MACROS_H
#define MACROS_H

/**
 * @def WZ_DEPRECATED
 *
 * The WZ_DEPRECATED macro can be used to trigger compile-time warnings
 * with newer compilers when deprecated functions are used.
 *
 * For non-inline functions, the macro gets inserted at front of the
 * function declaration, right before the return type:
 *
 * \code
 * WZ_DEPRECATED void deprecatedFunctionA();
 * WZ_DEPRECATED int deprecatedFunctionB() const;
 * \endcode
 *
 * For functions which are implemented inline,
 * the WZ_DEPRECATED macro is inserted at the front, right before the return
 * type, but after "static", "inline" or "virtual":
 *
 * \code
 * WZ_DEPRECATED void deprecatedInlineFunctionA() { .. }
 * virtual WZ_DEPRECATED int deprecatedInlineFunctionB() { .. }
 * static WZ_DEPRECATED bool deprecatedInlineFunctionC() { .. }
 * inline WZ_DEPRECATED bool deprecatedInlineFunctionD() { .. }
 * \endcode
 *
 * You can also mark whole structs or classes as deprecated, by inserting the
 * WZ_DEPRECATED macro after the struct/class keyword, but before the
 * name of the struct/class:
 *
 * \code
 * class WZ_DEPRECATED DeprecatedClass { };
 * struct WZ_DEPRECATED DeprecatedStruct { };
 * \endcode
 *
 * \note
 * Description copied from KDE4.
 *
 */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#  define WZ_DEPRECATED __attribute__ ((__deprecated__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#  define WZ_DEPRECATED __declspec(deprecated)
#else
#  define WZ_DEPRECATED
#endif

#endif // MACROS_H
