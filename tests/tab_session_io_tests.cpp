#include "modules/tab_session_io.h"

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

std::wstring TempPathForTest()
{
    wchar_t tempDir[MAX_PATH] = {};
    const DWORD len = GetTempPathW(MAX_PATH, tempDir);
    std::wstring root = (len > 0 && len < MAX_PATH) ? std::wstring(tempDir) : std::wstring(L".\\");
    wchar_t suffix[64] = {};
    wsprintfW(suffix, L"otso-tab-session-io-test-%lu.bin", GetCurrentProcessId());
    return root + suffix;
}
}

int RunTabSessionIoTests()
{
    bool ok = true;

    {
        std::vector<DocumentTabState> docs;
        DocumentTabState doc;
        doc.filePath = L"C:\\temp\\small.txt";
        doc.modified = true;
        doc.text = L"small text";
        docs.push_back(doc);

        ULONGLONG bytes = 0;
        ok = ExpectTrue(L"Estimate small session size",
                        EstimateSessionSerializedBytes(docs, 1, 64 * 1024 * 1024, bytes)) &&
             ok;
        ok = ExpectTrue(L"Estimated bytes non-zero", bytes > 0) && ok;
    }

    {
        std::vector<DocumentTabState> docs;
        DocumentTabState doc;
        doc.filePath = L"C:\\temp\\huge.txt";
        doc.modified = true;
        doc.text.assign(2 * 1024 * 1024, L'x');
        docs.push_back(doc);

        ULONGLONG bytes = 0;
        ok = ExpectTrue(L"Reject oversized session payload",
                        !EstimateSessionSerializedBytes(docs, 1, 1 * 1024 * 1024, bytes)) &&
             ok;
    }

    {
        const std::wstring path = TempPathForTest();

        DocumentTabState outDoc;
        outDoc.modified = true;
        outDoc.encoding = Encoding::UTF16LE;
        outDoc.lineEnding = LineEnding::LF;
        outDoc.filePath = L"C:\\temp\\roundtrip.txt";
        outDoc.text = L"line1\nline2\nline3";

        HANDLE hWrite = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        ok = ExpectTrue(L"Create file for roundtrip write", hWrite != INVALID_HANDLE_VALUE) && ok;
        if (hWrite != INVALID_HANDLE_VALUE)
        {
            ok = ExpectTrue(L"Write tab record", SessionWriteTabDocumentRecord(hWrite, outDoc, 1024 * 1024)) && ok;
            CloseHandle(hWrite);
        }

        HANDLE hRead = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        ok = ExpectTrue(L"Open file for roundtrip read", hRead != INVALID_HANDLE_VALUE) && ok;
        if (hRead != INVALID_HANDLE_VALUE)
        {
            DocumentTabState inDoc;
            ok = ExpectTrue(L"Read tab record", SessionReadTabDocumentRecord(hRead, inDoc, 1024 * 1024)) && ok;
            ok = ExpectTrue(L"Roundtrip modified", inDoc.modified == outDoc.modified) && ok;
            ok = ExpectTrue(L"Roundtrip encoding", inDoc.encoding == outDoc.encoding) && ok;
            ok = ExpectTrue(L"Roundtrip line ending", inDoc.lineEnding == outDoc.lineEnding) && ok;
            ok = ExpectTrue(L"Roundtrip file path", inDoc.filePath == outDoc.filePath) && ok;
            ok = ExpectTrue(L"Roundtrip text", inDoc.text == outDoc.text) && ok;
            CloseHandle(hRead);
        }

        DeleteFileW(path.c_str());
    }

    {
        const std::wstring path = TempPathForTest();
        constexpr DWORD kMagic = 0x4C4E5331u;
        constexpr DWORD kVersion = 1u;
        constexpr DWORD kMaxDocs = 64u;
        constexpr DWORD kMaxChars = 8u * 1024u * 1024u;
        constexpr DWORD kMaxFileBytes = 64u * 1024u * 1024u;

        std::vector<DocumentTabState> docs;
        DocumentTabState savedDoc;
        savedDoc.modified = true;
        savedDoc.filePath = L"C:\\temp\\saved.txt";
        savedDoc.text = L"keep me";
        docs.push_back(savedDoc);

        DocumentTabState cleanDoc;
        cleanDoc.modified = false;
        cleanDoc.filePath = L"C:\\temp\\clean.txt";
        cleanDoc.text = L"should not persist";
        docs.push_back(cleanDoc);

        const bool writeOk = SessionWriteSnapshot(path, docs, 1, kMagic, kVersion, kMaxDocs, kMaxChars, kMaxFileBytes);
        ok = ExpectTrue(L"Write snapshot file", writeOk) && ok;

        TabSessionSnapshot snapshot;
        const bool readOk = SessionReadSnapshot(path, snapshot, kMagic, kVersion, kMaxDocs, kMaxChars, kMaxFileBytes, true);
        ok = ExpectTrue(L"Read snapshot file", readOk) && ok;
        ok = ExpectTrue(L"Snapshot document count", snapshot.documents.size() == 2) && ok;
        ok = ExpectTrue(L"Snapshot active document index", snapshot.activeDocument == 1) && ok;
        if (snapshot.documents.size() == 2)
        {
            ok = ExpectTrue(L"Snapshot modified text restored", snapshot.documents[0].text == L"keep me") && ok;
            ok = ExpectTrue(L"Snapshot clean doc marked reload", snapshot.documents[1].needsReloadFromDisk) && ok;
            ok = ExpectTrue(L"Snapshot clean doc text omitted", snapshot.documents[1].text.empty()) && ok;
        }

        DeleteFileW(path.c_str());
    }

    if (!ok)
        return 1;
    std::wcout << L"[PASS] tab session io tests\n";
    return 0;
}

int main()
{
    return RunTabSessionIoTests();
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    return RunTabSessionIoTests();
}

