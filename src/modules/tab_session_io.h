/*
  Technical Standard

  Session and disk I/O helpers for tab documents.
*/

#pragma once

#include <windows.h>
#include <string>
#include <vector>

#include "tab_document.h"

struct TabSessionSnapshot
{
    std::vector<DocumentTabState> documents;
    int activeDocument = -1;
};

bool SessionWriteTabDocumentRecord(HANDLE hFile, const DocumentTabState &doc, DWORD maxChars);
bool SessionReadTabDocumentRecord(HANDLE hFile, DocumentTabState &doc, DWORD maxChars);
bool SessionLoadDocumentTextFromDisk(DocumentTabState &doc);

std::wstring SessionRuntimeFilePath();
std::wstring SessionToWin32IoPath(const std::wstring &path);
bool SessionPathExists(const std::wstring &path);
std::wstring SessionNormalizePathForCompare(const std::wstring &path);

bool EstimateSessionSerializedBytes(const std::vector<DocumentTabState> &documents,
                                    DWORD docCount,
                                    DWORD maxFileBytes,
                                    ULONGLONG &outBytes);

bool SessionWriteSnapshot(const std::wstring &sessionFilePath,
                          const std::vector<DocumentTabState> &documents,
                          int activeDocument,
                          DWORD sessionMagic,
                          DWORD sessionVersion,
                          DWORD maxDocuments,
                          DWORD maxStringChars,
                          DWORD maxFileBytes);

bool SessionReadSnapshot(const std::wstring &sessionFilePath,
                         TabSessionSnapshot &outSnapshot,
                         DWORD expectedMagic,
                         DWORD expectedVersion,
                         DWORD maxDocuments,
                         DWORD maxStringChars,
                         DWORD maxFileBytes,
                         bool deleteOversizedFile);
