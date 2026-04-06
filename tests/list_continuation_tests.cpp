#include "core/list_continuation.h"

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

bool ExpectEq(const wchar_t *name, const std::wstring &actual, const std::wstring &expected)
{
    if (actual == expected)
        return true;
    std::wcerr << L"[FAIL] " << name << L" | expected='" << expected << L"' actual='" << actual << L"'\n";
    return false;
}
}

int RunListContinuationTests()
{
    bool ok = true;

    {
        const auto plan = BuildListContinuationPlan(L"1. item", 7);
        ok = ExpectTrue(L"Numeric list recognized", plan.matched) && ok;
        ok = ExpectTrue(L"Numeric list continues", !plan.exitListMode) && ok;
        ok = ExpectEq(L"Numeric continuation prefix", plan.continuationPrefix, L"2. ") && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"009) task", 9);
        ok = ExpectTrue(L"Numeric padding recognized", plan.matched) && ok;
        ok = ExpectEq(L"Numeric padding continuation", plan.continuationPrefix, L"010) ") && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"  - item", 8);
        ok = ExpectTrue(L"Bullet list recognized", plan.matched) && ok;
        ok = ExpectTrue(L"Bullet list continues", !plan.exitListMode) && ok;
        ok = ExpectEq(L"Bullet continuation prefix", plan.continuationPrefix, L"  - ") && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"- ", 2);
        ok = ExpectTrue(L"Empty bullet exits list", plan.matched && plan.exitListMode) && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"2. ", 3);
        ok = ExpectTrue(L"Empty number exits list", plan.matched && plan.exitListMode) && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"2. ", 1);
        ok = ExpectTrue(L"Caret before marker spacing should not match", !plan.matched) && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"plain text", 10);
        ok = ExpectTrue(L"Plain text should not match", !plan.matched) && ok;
    }

    {
        const auto plan = BuildListContinuationPlan(L"1.item", 6);
        ok = ExpectTrue(L"No-spacing number list recognized", plan.matched) && ok;
        ok = ExpectEq(L"No-spacing number continuation normalizes spacing", plan.continuationPrefix, L"2. ") && ok;
    }

    if (!ok)
        return 1;
    std::wcout << L"[PASS] list continuation tests\n";
    return 0;
}

int main()
{
    return RunListContinuationTests();
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    return RunListContinuationTests();
}
