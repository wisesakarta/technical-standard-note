#include "list_continuation.h"

#include <cwctype>
#include <limits>

namespace
{
bool IsListSpacingChar(wchar_t ch)
{
    return ch == L' ' || ch == L'\t';
}

std::wstring ContinuationSpacing(const std::wstring &body, size_t spacingStart, size_t spacingEnd)
{
    if (spacingEnd > spacingStart)
        return body.substr(spacingStart, spacingEnd - spacingStart);
    return L" ";
}

bool IsBulletMarker(wchar_t ch)
{
    return ch == L'-' || ch == L'*' || ch == L'+' || ch == L'\x2022';
}
}

ListContinuationPlan BuildListContinuationPlan(const std::wstring &lineText, size_t caretOffsetInLine)
{
    ListContinuationPlan plan{};
    if (caretOffsetInLine > lineText.size())
        return plan;

    size_t indentLength = 0;
    while (indentLength < lineText.size() && IsListSpacingChar(lineText[indentLength]))
        ++indentLength;

    if (indentLength >= lineText.size())
        return plan;

    const std::wstring body = lineText.substr(indentLength);
    if (body.empty())
        return plan;

    if (IsBulletMarker(body[0]))
    {
        size_t spacingStart = 1;
        size_t spacingEnd = spacingStart;
        while (spacingEnd < body.size() && IsListSpacingChar(body[spacingEnd]))
            ++spacingEnd;

        const std::wstring itemContent = body.substr(spacingEnd);
        if (itemContent.empty())
        {
            if (caretOffsetInLine != lineText.size())
                return plan;
            plan.matched = true;
            plan.exitListMode = true;
            return plan;
        }

        if (caretOffsetInLine < (indentLength + spacingEnd))
            return plan;

        plan.matched = true;
        plan.continuationPrefix = lineText.substr(0, indentLength);
        plan.continuationPrefix.push_back(body[0]);
        plan.continuationPrefix += ContinuationSpacing(body, spacingStart, spacingEnd);
        return plan;
    }

    size_t digitsEnd = 0;
    while (digitsEnd < body.size() && iswdigit(static_cast<wint_t>(body[digitsEnd])))
        ++digitsEnd;
    if (digitsEnd == 0 || digitsEnd >= body.size())
        return plan;

    const wchar_t delimiter = body[digitsEnd];
    if (delimiter != L'.' && delimiter != L')')
        return plan;

    size_t spacingStart = digitsEnd + 1;
    size_t spacingEnd = spacingStart;
    while (spacingEnd < body.size() && IsListSpacingChar(body[spacingEnd]))
        ++spacingEnd;

    unsigned long long number = 0;
    for (size_t i = 0; i < digitsEnd; ++i)
    {
        const unsigned long long digit = static_cast<unsigned long long>(body[i] - L'0');
        if (number > (std::numeric_limits<unsigned long long>::max() - digit) / 10)
            return plan;
        number = (number * 10) + digit;
    }
    if (number == std::numeric_limits<unsigned long long>::max())
        return plan;

    const std::wstring itemContent = body.substr(spacingEnd);
    if (itemContent.empty())
    {
        if (caretOffsetInLine != lineText.size())
            return plan;
        plan.matched = true;
        plan.exitListMode = true;
        return plan;
    }

    if (caretOffsetInLine < (indentLength + spacingEnd))
        return plan;

    std::wstring nextNumberText = std::to_wstring(number + 1);
    if (digitsEnd > nextNumberText.size() && body[0] == L'0')
        nextNumberText.insert(0, digitsEnd - nextNumberText.size(), L'0');

    plan.matched = true;
    plan.continuationPrefix = lineText.substr(0, indentLength);
    plan.continuationPrefix.append(nextNumberText);
    plan.continuationPrefix.push_back(delimiter);
    plan.continuationPrefix += ContinuationSpacing(body, spacingStart, spacingEnd);
    return plan;
}
