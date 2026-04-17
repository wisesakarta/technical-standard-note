#include "crash_diagnostics.h"

#include <windows.h>
#include <dbghelp.h>
#include <sstream>

namespace
{
bool g_diagEnabled = false;
bool g_dumpEnabled = false;
std::wstring g_logPath;
std::wstring g_dumpDir;

bool IsTruthyEnv(const wchar_t *name)
{
    wchar_t value[16] = {};
    const DWORD len = GetEnvironmentVariableW(name, value, static_cast<DWORD>(std::size(value)));
    if (len == 0 || len >= std::size(value))
        return false;
    return (_wcsicmp(value, L"1") == 0) ||
           (_wcsicmp(value, L"true") == 0) ||
           (_wcsicmp(value, L"yes") == 0) ||
           (_wcsicmp(value, L"on") == 0);
}

std::wstring BaseDiagnosticsDir()
{
    wchar_t localAppData[MAX_PATH] = {};
    const DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    std::wstring root;
    if (len > 0 && len < MAX_PATH)
    {
        root = localAppData;
    }
    else
    {
        root = L".";
    }
    root += L"\\Otso";
    CreateDirectoryW(root.c_str(), nullptr);
    return root;
}

std::wstring TimestampForFile()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t stamp[64] = {};
    wsprintfW(stamp, L"%04u%02u%02u-%02u%02u%02u",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
    return stamp;
}

void AppendLineUtf8(const std::wstring &path, const std::wstring &line)
{
    HANDLE hFile = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    const int utf8Len = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), static_cast<int>(line.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Len > 0)
    {
        std::string utf8(utf8Len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, line.c_str(), static_cast<int>(line.size()), utf8.data(), utf8Len, nullptr, nullptr);
        DWORD written = 0;
        WriteFile(hFile, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    const char newline[] = "\r\n";
    DWORD written = 0;
    WriteFile(hFile, newline, static_cast<DWORD>(sizeof(newline) - 1), &written, nullptr);
    CloseHandle(hFile);
}

LONG WINAPI CrashFilter(EXCEPTION_POINTERS *exceptionPointers)
{
    std::wstringstream ss;
    if (exceptionPointers && exceptionPointers->ExceptionRecord)
    {
        ss << L"Unhandled exception code=0x"
           << std::hex << exceptionPointers->ExceptionRecord->ExceptionCode
           << L" address=0x"
           << reinterpret_cast<UINT_PTR>(exceptionPointers->ExceptionRecord->ExceptionAddress);
    }
    else
    {
        ss << L"Unhandled exception (no record)";
    }
    CrashDiagnosticsLog(ss.str());

    if (!g_dumpEnabled || g_dumpDir.empty())
        return EXCEPTION_EXECUTE_HANDLER;

    const std::wstring dumpPath = g_dumpDir + L"\\crash-" + TimestampForFile() + L".dmp";
    HANDLE hDumpFile = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hDumpFile == INVALID_HANDLE_VALUE)
        return EXCEPTION_EXECUTE_HANDLER;

    HMODULE hDbgHelp = LoadLibraryW(L"dbghelp.dll");
    if (!hDbgHelp)
    {
        CloseHandle(hDumpFile);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    using fnMiniDumpWriteDump = BOOL(WINAPI *)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
                                               PMINIDUMP_EXCEPTION_INFORMATION,
                                               PMINIDUMP_USER_STREAM_INFORMATION,
                                               PMINIDUMP_CALLBACK_INFORMATION);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    auto miniDumpWriteDump = reinterpret_cast<fnMiniDumpWriteDump>(GetProcAddress(hDbgHelp, "MiniDumpWriteDump"));
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    if (miniDumpWriteDump)
    {
        MINIDUMP_EXCEPTION_INFORMATION exInfo{};
        exInfo.ThreadId = GetCurrentThreadId();
        exInfo.ExceptionPointers = exceptionPointers;
        exInfo.ClientPointers = FALSE;

        miniDumpWriteDump(GetCurrentProcess(),
                          GetCurrentProcessId(),
                          hDumpFile,
                          static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithThreadInfo),
                          exceptionPointers ? &exInfo : nullptr,
                          nullptr,
                          nullptr);
    }

    FreeLibrary(hDbgHelp);
    CloseHandle(hDumpFile);
    return EXCEPTION_EXECUTE_HANDLER;
}
}

void InitializeCrashDiagnostics()
{
    g_diagEnabled = IsTruthyEnv(L"TECHNICAL_STANDARD_NOTE_DIAGNOSTICS");
    g_dumpEnabled = IsTruthyEnv(L"TECHNICAL_STANDARD_NOTE_MINIDUMP");
    if (!g_diagEnabled && !g_dumpEnabled)
        return;

    const std::wstring baseDir = BaseDiagnosticsDir();
    g_logPath = baseDir + L"\\logs\\startup.log";
    g_dumpDir = baseDir + L"\\crashes";
    CreateDirectoryW((baseDir + L"\\logs").c_str(), nullptr);
    if (g_dumpEnabled)
        CreateDirectoryW(g_dumpDir.c_str(), nullptr);

    SetUnhandledExceptionFilter(CrashFilter);
    CrashDiagnosticsLog(L"Crash diagnostics initialized");
}

void CrashDiagnosticsLog(const std::wstring &message)
{
    if (!g_diagEnabled || g_logPath.empty())
        return;

    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t ts[64] = {};
    wsprintfW(ts, L"[%04u-%02u-%02u %02u:%02u:%02u] ",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
    AppendLineUtf8(g_logPath, std::wstring(ts) + message);
}

