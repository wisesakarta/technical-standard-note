/*
  Otso

  Session and disk I/O helpers for tab documents.
*/

#include "tab_session_io.h"

#include "core/session_io.h"
#include "core/text_codec.h"

#include <shlwapi.h>
#include <algorithm>

namespace
{
ULONGLONG EstimateSessionWideStringBytes(size_t charCount)
{
    return static_cast<ULONGLONG>(sizeof(DWORD)) +
           static_cast<ULONGLONG>(charCount) * static_cast<ULONGLONG>(sizeof(wchar_t));
}
}

bool SessionWriteTabDocumentRecord(HANDLE hFile, const DocumentTabState &doc, DWORD maxChars)
{
    SessionDocumentRecord payload;
    payload.modified = doc.modified;
    payload.encoding = doc.encoding;
    payload.lineEnding = doc.lineEnding;
    payload.filePath = doc.filePath;
    const bool persistText = doc.filePath.empty() || doc.modified;
    if (persistText)
        payload.text = doc.text;
    return SessionWriteDocumentRecord(hFile, payload, maxChars);
}

bool SessionReadTabDocumentRecord(HANDLE hFile, DocumentTabState &doc, DWORD maxChars)
{
    SessionDocumentRecord payload;
    if (!SessionReadDocumentRecord(hFile, payload, maxChars))
        return false;

    doc.modified = payload.modified;
    doc.encoding = payload.encoding;
    doc.lineEnding = payload.lineEnding;
    doc.filePath = payload.filePath;
    doc.text = payload.text;
    doc.richText.clear();
    doc.hasRichText = false;

    doc.needsReloadFromDisk = false;
    if (!doc.modified && doc.text.empty() && !doc.filePath.empty())
    {
        doc.sourceBytes = 0;
        doc.largeFileMode = false;
        doc.needsReloadFromDisk = true;
        return true;
    }

    doc.sourceBytes = EstimateDocumentTextBytes(doc.text);
    doc.largeFileMode = ShouldUseLargeDocumentMode(doc.sourceBytes);
    return true;
}

bool SessionLoadDocumentTextFromDisk(DocumentTabState &doc)
{
    if (doc.filePath.empty())
        return false;

    const std::wstring ioPath = SessionToWin32IoPath(doc.filePath);
    HANDLE hFile = CreateFileW(ioPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER fileSize = {};
    if (!GetFileSizeEx(hFile, &fileSize) || fileSize.QuadPart < 0 || fileSize.QuadPart > static_cast<LONGLONG>(MAXDWORD))
    {
        CloseHandle(hFile);
        return false;
    }

    const DWORD size = static_cast<DWORD>(fileSize.QuadPart);
    std::vector<BYTE> data(size);
    if (size > 0 && !SessionReadAllBytes(hFile, data.data(), size))
    {
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);

    const auto detected = DetectEncoding(data);
    doc.text = DecodeText(data, detected.first);
    doc.richText.clear();
    doc.hasRichText = false;
    doc.encoding = detected.first;
    doc.lineEnding = detected.second;
    doc.sourceBytes = static_cast<size_t>(size);
    doc.largeFileMode = ShouldUseLargeDocumentMode(doc.sourceBytes);
    doc.needsReloadFromDisk = false;
    doc.modified = false;
    return true;
}

std::wstring SessionRuntimeFilePath()
{
    wchar_t localAppData[MAX_PATH] = {};
    const DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);

    std::wstring dirPath;
    if (len > 0 && len < MAX_PATH)
    {
        dirPath = localAppData;
    }
    else
    {
        wchar_t modulePath[MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) == 0)
            return L"session.dat";
        PathRemoveFileSpecW(modulePath);
        dirPath = modulePath;
    }

    dirPath += L"\\Otso";
    CreateDirectoryW(dirPath.c_str(), nullptr);
    return dirPath + L"\\session.dat";
}

std::wstring SessionToWin32IoPath(const std::wstring &path)
{
    if (path.empty())
        return path;
    if (path.rfind(L"\\\\?\\", 0) == 0)
        return path;

    if (path.rfind(L"\\\\", 0) == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);

    if (path.size() >= MAX_PATH && path.size() > 2 && path[1] == L':' &&
        (path[2] == L'\\' || path[2] == L'/'))
        return L"\\\\?\\" + path;

    return path;
}

bool SessionPathExists(const std::wstring &path)
{
    if (path.empty())
        return false;
    const std::wstring ioPath = SessionToWin32IoPath(path);
    const DWORD attrs = GetFileAttributesW(ioPath.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

std::wstring SessionNormalizePathForCompare(const std::wstring &path)
{
    if (path.empty())
        return {};

    std::wstring normalized = path;
    std::wstring ioPath = SessionToWin32IoPath(path);
    wchar_t fullPath[MAX_PATH] = {};
    DWORD len = GetFullPathNameW(ioPath.c_str(), MAX_PATH, fullPath, nullptr);
    if (len > 0 && len < MAX_PATH)
        normalized.assign(fullPath);

    if (normalized.rfind(L"\\\\?\\UNC\\", 0) == 0)
        normalized = L"\\" + normalized.substr(7);
    else if (normalized.rfind(L"\\\\?\\", 0) == 0)
        normalized.erase(0, 4);

    CharLowerBuffW(normalized.data(), static_cast<DWORD>(normalized.size()));
    return normalized;
}

bool EstimateSessionSerializedBytes(const std::vector<DocumentTabState> &documents,
                                    DWORD docCount,
                                    DWORD maxFileBytes,
                                    ULONGLONG &outBytes)
{
    ULONGLONG total = static_cast<ULONGLONG>(sizeof(DWORD)) * 4u;
    const ULONGLONG maxBytes = static_cast<ULONGLONG>(maxFileBytes);
    if (total > maxBytes)
        return false;

    for (DWORD i = 0; i < docCount; ++i)
    {
        const DocumentTabState &doc = documents[i];
        const bool persistText = doc.filePath.empty() || doc.modified;
        const ULONGLONG recordBytes = static_cast<ULONGLONG>(sizeof(DWORD)) * 3u +
                                      EstimateSessionWideStringBytes(doc.filePath.size()) +
                                      EstimateSessionWideStringBytes(persistText ? doc.text.size() : 0u);
        if (recordBytes > (maxBytes - total))
            return false;
        total += recordBytes;
    }

    outBytes = total;
    return true;
}

bool SessionWriteSnapshot(const std::wstring &sessionFilePath,
                          const std::vector<DocumentTabState> &documents,
                          int activeDocument,
                          DWORD sessionMagic,
                          DWORD sessionVersion,
                          DWORD maxDocuments,
                          DWORD maxStringChars,
                          DWORD maxFileBytes)
{
    const DWORD docCount = static_cast<DWORD>((std::min)(documents.size(), static_cast<size_t>(maxDocuments)));
    ULONGLONG estimatedBytes = 0;
    if (!EstimateSessionSerializedBytes(documents, docCount, maxFileBytes, estimatedBytes))
    {
        DeleteFileW(sessionFilePath.c_str());
        return false;
    }
    (void)estimatedBytes;

    HANDLE hFile = CreateFileW(sessionFilePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD activeDocIndex = 0xFFFFFFFFu;
    if (activeDocument >= 0 && activeDocument < static_cast<int>(docCount))
        activeDocIndex = static_cast<DWORD>(activeDocument);

    bool ok = SessionWriteUInt32(hFile, sessionMagic) &&
              SessionWriteUInt32(hFile, sessionVersion) &&
              SessionWriteUInt32(hFile, docCount) &&
              SessionWriteUInt32(hFile, activeDocIndex);
    for (DWORD i = 0; ok && i < docCount; ++i)
        ok = SessionWriteTabDocumentRecord(hFile, documents[i], maxStringChars);

    CloseHandle(hFile);
    if (!ok)
        DeleteFileW(sessionFilePath.c_str());
    return ok;
}

bool SessionReadSnapshot(const std::wstring &sessionFilePath,
                         TabSessionSnapshot &outSnapshot,
                         DWORD expectedMagic,
                         DWORD expectedVersion,
                         DWORD maxDocuments,
                         DWORD maxStringChars,
                         DWORD maxFileBytes,
                         bool deleteOversizedFile)
{
    HANDLE hFile = CreateFileW(sessionFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER sessionSize = {};
    if (!GetFileSizeEx(hFile, &sessionSize) || sessionSize.QuadPart <= 0 || sessionSize.QuadPart > static_cast<LONGLONG>(maxFileBytes))
    {
        CloseHandle(hFile);
        if (deleteOversizedFile && sessionSize.QuadPart > static_cast<LONGLONG>(maxFileBytes))
            DeleteFileW(sessionFilePath.c_str());
        return false;
    }

    DWORD magic = 0;
    DWORD version = 0;
    DWORD docCount = 0;
    DWORD activeDocIndex = 0xFFFFFFFFu;
    bool ok = SessionReadUInt32(hFile, magic) &&
              SessionReadUInt32(hFile, version) &&
              SessionReadUInt32(hFile, docCount) &&
              SessionReadUInt32(hFile, activeDocIndex) &&
              magic == expectedMagic &&
              version == expectedVersion &&
              docCount > 0 &&
              docCount <= maxDocuments;

    std::vector<DocumentTabState> restoredDocs;
    restoredDocs.reserve(docCount);
    for (DWORD i = 0; ok && i < docCount; ++i)
    {
        DocumentTabState doc;
        ok = SessionReadTabDocumentRecord(hFile, doc, maxStringChars);
        if (ok)
            restoredDocs.push_back(std::move(doc));
    }
    CloseHandle(hFile);

    if (!ok || restoredDocs.empty())
        return false;

    outSnapshot.documents = std::move(restoredDocs);
    outSnapshot.activeDocument = (activeDocIndex < outSnapshot.documents.size()) ? static_cast<int>(activeDocIndex) : 0;
    return true;
}

