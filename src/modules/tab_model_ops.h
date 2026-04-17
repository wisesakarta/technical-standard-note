/*
  Otso

  Pure tab model operations that do not depend on Win32 UI objects.
*/

#pragma once

#include <string>
#include <vector>

#include "tab_document.h"

using TabNormalizePathFn = std::wstring (*)(const std::wstring &path);
using TabPathExistsFn = bool (*)(const std::wstring &path);

int TabFindDocumentByPath(const std::vector<DocumentTabState> &documents,
                          const std::wstring &path,
                          TabNormalizePathFn normalizePath);

bool TabIsEmptyUntitled(const DocumentTabState &doc);

bool TabCompactDocumentTextIfEligible(std::vector<DocumentTabState> &documents,
                                      int index,
                                      int activeIndex,
                                      size_t compactThresholdBytes,
                                      TabPathExistsFn pathExists);

void TabPushClosedDocument(std::vector<DocumentTabState> &closedDocuments,
                           const DocumentTabState &doc,
                           size_t maxClosedDocuments);

void TabBuildPathSessionFallback(const std::vector<DocumentTabState> &documents,
                                 int activeIndex,
                                 std::vector<std::wstring> &outSessionPaths,
                                 int &outActivePathIndex,
                                 TabNormalizePathFn normalizePath);
