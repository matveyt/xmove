/*
 *  Utility to bypass Windows(R) file protection
 *  Vim example: :w !xmove %
 */

#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif // UNICODE

#include <stdarg.h>
#include <tchar.h>
#include <windef.h>
#include <winbase.h>
#include <shellapi.h>
#include <winuser.h>

#define MAGIC       TEXT("--no-admin")
#define COUNT(a)    (sizeof(a) / sizeof(*a))
#define STR(a)      (a), (sizeof(a) - sizeof(*a))

// temporary file name pointer
static LPCTSTR lpTempFile;

// exit on error, clean temporary file
__declspec(noreturn)
void error_exit(LPCSTR lpMsg, DWORD dwLen)
{
    HANDLE hError = GetStdHandle(STD_ERROR_HANDLE);
    DWORD cb;
    WriteFile(hError, STR("xmove error: "), &cb, NULL);
    WriteFile(hError, lpMsg, dwLen, &cb, NULL);
    WriteFile(hError, STR("\r\n"), &cb, NULL);

    if (lpTempFile != NULL)
        DeleteFile(lpTempFile);

    ExitProcess(1);
}

// make temporary file name
LPTSTR make_tempname(LPTSTR lpBuffer)
{
    TCHAR achTempDir[MAX_PATH];

    if (GetTempPath(COUNT(achTempDir), achTempDir) == 0 ||
        GetTempFileName(achTempDir, TEXT("xmove"), 0, lpBuffer) == 0)
        return NULL;

    return lpBuffer;
}

// move file even if target is read only
BOOL move_file(LPCTSTR lpOld, LPCTSTR lpNew)
{
    const DWORD RO = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
        FILE_ATTRIBUTE_SYSTEM;
    DWORD dwAttr = GetFileAttributes(lpNew);

    if (dwAttr != INVALID_FILE_ATTRIBUTES && dwAttr & RO)
        SetFileAttributes(lpNew, dwAttr & ~RO);
    BOOL bSuccess = MoveFileEx(lpOld, lpNew, MOVEFILE_REPLACE_EXISTING |
        MOVEFILE_COPY_ALLOWED);
    if (dwAttr != INVALID_FILE_ATTRIBUTES && dwAttr & RO)
        SetFileAttributes(lpNew, dwAttr);

    return bSuccess;
}

// ShellExecute helper
BOOL runas(LPCTSTR lpFile, LPCTSTR lpFormat, ...)
{
    va_list args;
    va_start(args, lpFormat);

    TCHAR lpParam[1024 + 1];
    wvsprintf(lpParam, lpFormat, args);

    DWORD dwExitCode = 1;
    SHELLEXECUTEINFO shxi = {
        .cbSize = sizeof(shxi),
        .fMask = SEE_MASK_NOCLOSEPROCESS,
        .lpVerb = TEXT("runas"),
        .lpFile = lpFile,
        .lpParameters = lpParam
    };
    if (ShellExecuteEx(&shxi)) {
        WaitForSingleObject(shxi.hProcess, INFINITE);
        GetExitCodeProcess(shxi.hProcess, &dwExitCode);
    }

    return dwExitCode ? FALSE : TRUE;
}

// save stream to new file
BOOL saveas(HANDLE hIn, LPCTSTR lpNew)
{
    HANDLE hOut = CreateFile(lpNew, GENERIC_WRITE, FILE_SHARE_DELETE, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE)
        return FALSE;

    BYTE buffer[4096];
    DWORD cb;
    while (ReadFile(hIn, buffer, sizeof(buffer), &cb, NULL)) {
        if (!WriteFile(hOut, buffer, cb, &cb, NULL))
            return FALSE;
    }

    return CloseHandle(hOut);
}

int _tmain(int argc, _TCHAR* argv[])
{
    BOOL bElevate = TRUE;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    TCHAR achSrcFile[MAX_PATH], achDestFile[MAX_PATH];

    if (argc > 1 && lstrcmp(argv[1], MAGIC) == 0) {
        bElevate = FALSE;
        --argc;
        ++argv;
    }

    if (argc == 2 && GetFileType(hStdin) != FILE_TYPE_CHAR) {
        // :w !xmove %
        lpTempFile = make_tempname(achSrcFile);
        if (!saveas(hStdin, lpTempFile))
            error_exit(STR("cannot create temporary file"));
        GetFullPathName(argv[1], COUNT(achDestFile), achDestFile, NULL);
    } else if (argc == 3) {
        // xmove SRC DEST
        GetFullPathName(argv[1], COUNT(achSrcFile), achSrcFile, NULL);
        GetFullPathName(argv[2], COUNT(achDestFile), achDestFile, NULL);
    } else {
        error_exit(STR("invoke as 'xmove SRC DEST' or ':w !xmove DEST' (in Vim)"));
    }

    BOOL bSuccess = bElevate ? runas(argv[0], TEXT("%s \"%s\" \"%s\""), MAGIC,
        achSrcFile, achDestFile) : move_file(achSrcFile, achDestFile);
    if (!bSuccess)
        error_exit(STR("cannot move file"));

    return 0;
}

// micro CRT startup code
#define ARGV shell32
#include "nocrt0c.c"
