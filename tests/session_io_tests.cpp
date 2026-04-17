#include "core/session_io.h"

#include <windows.h>
#include <iostream>

namespace
{
bool ExpectTrue(const wchar_t *name, bool condition)
{
    if (condition)
        return true;
    std::wcerr << L"[FAIL] " << name << L"\n";
    return false;
}

std::wstring TempSessionPath()
{
    wchar_t tempDir[MAX_PATH] = {};
    DWORD len = GetTempPathW(MAX_PATH, tempDir);
    std::wstring root = (len > 0 && len < MAX_PATH) ? std::wstring(tempDir) : std::wstring(L".\\");
    wchar_t suffix[64] = {};
    wsprintfW(suffix, L"otso-session-io-test-%lu.bin", GetCurrentProcessId());
    return root + suffix;
}
}

int RunSessionIoTests()
{
    bool ok = true;
    const std::wstring path = TempSessionPath();

    SessionDocumentRecord outRecord;
    outRecord.modified = true;
    outRecord.encoding = Encoding::UTF8BOM;
    outRecord.lineEnding = LineEnding::LF;
    outRecord.filePath = L"C:\\temp\\example.txt";
    outRecord.text = L"line1\nline2";

    {
        HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        ok = ExpectTrue(L"Create temp file for write", hFile != INVALID_HANDLE_VALUE) && ok;
        if (hFile != INVALID_HANDLE_VALUE)
        {
            ok = ExpectTrue(L"Write session magic", SessionWriteUInt32(hFile, 0xABCD1234u)) && ok;
            ok = ExpectTrue(L"Write session record", SessionWriteDocumentRecord(hFile, outRecord, 1024)) && ok;
            CloseHandle(hFile);
        }
    }

    {
        HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        ok = ExpectTrue(L"Open temp file for read", hFile != INVALID_HANDLE_VALUE) && ok;
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD magic = 0;
            SessionDocumentRecord inRecord;
            ok = ExpectTrue(L"Read session magic", SessionReadUInt32(hFile, magic)) && ok;
            ok = ExpectTrue(L"Session magic equal", magic == 0xABCD1234u) && ok;
            ok = ExpectTrue(L"Read session record", SessionReadDocumentRecord(hFile, inRecord, 1024)) && ok;
            ok = ExpectTrue(L"Record modified equal", inRecord.modified == outRecord.modified) && ok;
            ok = ExpectTrue(L"Record encoding equal", inRecord.encoding == outRecord.encoding) && ok;
            ok = ExpectTrue(L"Record line ending equal", inRecord.lineEnding == outRecord.lineEnding) && ok;
            ok = ExpectTrue(L"Record file path equal", inRecord.filePath == outRecord.filePath) && ok;
            ok = ExpectTrue(L"Record text equal", inRecord.text == outRecord.text) && ok;
            CloseHandle(hFile);
        }
    }

    {
        HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        ok = ExpectTrue(L"Create temp file for max string test", hFile != INVALID_HANDLE_VALUE) && ok;
        if (hFile != INVALID_HANDLE_VALUE)
        {
            const std::wstring tooLong = L"0123456789";
            const bool writeOk = SessionWriteWideString(hFile, tooLong, 5);
            ok = ExpectTrue(L"Reject over-max string", writeOk == false) && ok;
            CloseHandle(hFile);
        }
    }

    DeleteFileW(path.c_str());

    if (!ok)
        return 1;
    std::wcout << L"[PASS] session io tests\n";
    return 0;
}

int main()
{
    return RunSessionIoTests();
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    return RunSessionIoTests();
}

