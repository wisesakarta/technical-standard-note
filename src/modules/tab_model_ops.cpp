/*
  Otso

  Pure tab model operations that do not depend on Win32 UI objects.
*/

#include "tab_model_ops.h"

int TabFindDocumentByPath(const std::vector<DocumentTabState> &documents,
                          const std::wstring &path,
                          TabNormalizePathFn normalizePath)
{
    if (!normalizePath || path.empty())
        return -1;

    const std::wstring needle = normalizePath(path);
    if (needle.empty())
        return -1;

    for (int i = 0; i < static_cast<int>(documents.size()); ++i)
    {
        if (normalizePath(documents[i].filePath) == needle)
            return i;
    }
    return -1;
}

bool TabIsEmptyUntitled(const DocumentTabState &doc)
{
    return doc.filePath.empty() && !doc.modified && doc.text.empty();
}

bool TabCompactDocumentTextIfEligible(std::vector<DocumentTabState> &documents,
                                      int index,
                                      int activeIndex,
                                      size_t compactThresholdBytes,
                                      TabPathExistsFn pathExists)
{
    if (!pathExists)
        return false;
    if (index < 0 || index >= static_cast<int>(documents.size()))
        return false;
    if (index == activeIndex)
        return false;

    DocumentTabState &doc = documents[index];
    if (doc.modified || doc.filePath.empty() || doc.needsReloadFromDisk)
        return false;

    const size_t bytes = (doc.sourceBytes > 0) ? doc.sourceBytes : EstimateDocumentTextBytes(doc.text);
    if (bytes < compactThresholdBytes)
        return false;
    if (!pathExists(doc.filePath))
        return false;

    doc.sourceBytes = bytes;
    doc.largeFileMode = doc.largeFileMode || ShouldUseLargeDocumentMode(doc.sourceBytes);
    std::wstring().swap(doc.text);
    std::string().swap(doc.richText);
    doc.hasRichText = false;
    doc.needsReloadFromDisk = true;
    return true;
}

void TabPushClosedDocument(std::vector<DocumentTabState> &closedDocuments,
                           const DocumentTabState &doc,
                           size_t maxClosedDocuments)
{
    if (doc.filePath.empty() && doc.text.empty() && !doc.modified)
        return;

    closedDocuments.push_back(doc);
    if (closedDocuments.size() > maxClosedDocuments)
        closedDocuments.erase(closedDocuments.begin());
}

void TabBuildPathSessionFallback(const std::vector<DocumentTabState> &documents,
                                 int activeIndex,
                                 std::vector<std::wstring> &outSessionPaths,
                                 int &outActivePathIndex,
                                 TabNormalizePathFn normalizePath)
{
    outSessionPaths.clear();
    outActivePathIndex = -1;
    if (!normalizePath)
        return;

    std::vector<std::wstring> normalizedPaths;
    outSessionPaths.reserve(documents.size());
    normalizedPaths.reserve(documents.size());

    for (int i = 0; i < static_cast<int>(documents.size()); ++i)
    {
        const std::wstring &path = documents[i].filePath;
        if (path.empty())
            continue;

        const std::wstring normalized = normalizePath(path);
        if (normalized.empty())
            continue;

        int existingIndex = -1;
        for (int j = 0; j < static_cast<int>(normalizedPaths.size()); ++j)
        {
            if (normalizedPaths[j] == normalized)
            {
                existingIndex = j;
                break;
            }
        }

        if (existingIndex >= 0)
        {
            if (i == activeIndex)
                outActivePathIndex = existingIndex;
            continue;
        }

        normalizedPaths.push_back(normalized);
        outSessionPaths.push_back(path);
        if (i == activeIndex)
            outActivePathIndex = static_cast<int>(outSessionPaths.size()) - 1;
    }
}
