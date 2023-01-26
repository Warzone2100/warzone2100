#include "sentry_windows_dbghelp.h"

#include <dbghelp.h>

#if _WIN32_WINNT && _WIN32_WINNT < 0x0600
typedef WORD(NTAPI *RtlCaptureStackBackTraceProc)(DWORD FramesToSkip,
    DWORD FramesToCapture, PVOID *BackTrace, PDWORD BackTraceHash);
#endif

size_t
sentry__unwind_stack_dbghelp(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (!uctx && !addr) {
#if _WIN32_WINNT && _WIN32_WINNT < 0x0600
        HMODULE ntdll = NULL;
        RtlCaptureStackBackTraceProc proc = NULL;

        if (!(ntdll = LoadLibraryW(L"ntdll.dll"))) {
            return 0;
        }
        if (!(proc = (RtlCaptureStackBackTraceProc)GetProcAddress(
                  ntdll, "RtlCaptureStackBackTrace"))) {
            return 0;
        }

        // sum of frames to skip and frames to captures must be less than 63 for
        // XP/2003
        if (max_frames > 61) {
            max_frames = 61;
        }

        return (size_t)proc(1, (DWORD)max_frames, ptrs, 0);
#else
        return (size_t)CaptureStackBackTrace(1, (ULONG)max_frames, ptrs, 0);
#endif
    }

    sentry__init_dbghelp();

    CONTEXT ctx = *uctx->exception_ptrs.ContextRecord;
    STACKFRAME64 stack_frame;
    memset(&stack_frame, 0, sizeof(stack_frame));

    size_t size = 0;
#if defined(_M_X64)
    int machine_type = IMAGE_FILE_MACHINE_AMD64;
    stack_frame.AddrPC.Offset = ctx.Rip;
    stack_frame.AddrFrame.Offset = ctx.Rbp;
    stack_frame.AddrStack.Offset = ctx.Rsp;
#elif defined(_M_IX86)
    int machine_type = IMAGE_FILE_MACHINE_I386;
    stack_frame.AddrPC.Offset = ctx.Eip;
    stack_frame.AddrFrame.Offset = ctx.Ebp;
    stack_frame.AddrStack.Offset = ctx.Esp;
#elif defined(_M_ARM64)
    int machine_type = IMAGE_FILE_MACHINE_ARM64;
    stack_frame.AddrPC.Offset = ctx.Pc;
# if defined (NONAMELESSUNION)
    stack_frame.AddrFrame.Offset = ctx.DUMMYUNIONNAME.DUMMYSTRUCTNAME.Fp;
# else
    stack_frame.AddrFrame.Offset = ctx.Fp;
# endif
    stack_frame.AddrStack.Offset = ctx.Sp;
#elif defined(_M_ARM)
    int machine_type = IMAGE_FILE_MACHINE_ARM;
    stack_frame.AddrPC.Offset = ctx.Pc;
    stack_frame.AddrFrame.Offset = ctx.R11;
    stack_frame.AddrStack.Offset = ctx.Sp;
#else
# error "Platform not supported!"
#endif
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    if (addr) {
        stack_frame.AddrPC.Offset = (DWORD64)addr;
    }

    while (StackWalk64(machine_type, GetCurrentProcess(), GetCurrentThread(),
               &stack_frame, &ctx, NULL, SymFunctionTableAccess64,
               SymGetModuleBase64, NULL)
        && size < max_frames) {
        ptrs[size++] = (void *)(size_t)stack_frame.AddrPC.Offset;
    }

    return size;
}
