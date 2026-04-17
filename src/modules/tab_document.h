/*
  Otso

  Shared tab document model used by tab/session controllers.
*/

#pragma once

#include <string>

#include "core/types.h"

struct DocumentTabState
{
    std::wstring text;
    std::string richText;
    bool hasRichText = false;
    std::wstring filePath;
    bool modified = false;
    Encoding encoding = Encoding::UTF8;
    LineEnding lineEnding = LineEnding::CRLF;
    bool largeFileMode = false;
    size_t sourceBytes = 0;
    bool needsReloadFromDisk = false;
};

inline size_t EstimateDocumentTextBytes(const std::wstring &text)
{
    return text.size() * sizeof(wchar_t);
}

inline bool ShouldUseLargeDocumentMode(size_t bytes)
{
    return bytes >= LARGE_FILE_MODE_THRESHOLD_BYTES;
}
