#pragma once

#include <cstddef>
#include <string>

struct ListContinuationPlan
{
    bool matched = false;
    bool exitListMode = false;
    std::wstring continuationPrefix;
};

ListContinuationPlan BuildListContinuationPlan(const std::wstring &lineText, size_t caretOffsetInLine);
