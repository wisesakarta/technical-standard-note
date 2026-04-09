/*
  Technical Standard

  Update check flow for querying GitHub releases and presenting user prompts.
*/

#include "commands.h"

#include "core/globals.h"
#include "core/versioning.h"
#include "lang/lang.h"

#include <shellapi.h>
#include <winhttp.h>
#include <cstdint>
#include <string>

namespace
{
constexpr DWORD kHttpResolveTimeoutMs = 4000;
constexpr DWORD kHttpConnectTimeoutMs = 4000;
constexpr DWORD kHttpSendTimeoutMs = 5000;
constexpr DWORD kHttpReceiveTimeoutMs = 5000;
constexpr size_t kUpdatePayloadMaxBytes = 1024 * 1024;

class ScopedWinHttpHandle
{
public:
    ScopedWinHttpHandle() = default;
    explicit ScopedWinHttpHandle(HINTERNET handle)
        : handle_(handle)
    {
    }

    ~ScopedWinHttpHandle()
    {
        if (handle_)
            WinHttpCloseHandle(handle_);
    }

    ScopedWinHttpHandle(const ScopedWinHttpHandle &) = delete;
    ScopedWinHttpHandle &operator=(const ScopedWinHttpHandle &) = delete;

    ScopedWinHttpHandle(ScopedWinHttpHandle &&other) noexcept
        : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }

    ScopedWinHttpHandle &operator=(ScopedWinHttpHandle &&other) noexcept
    {
        if (this != &other)
        {
            if (handle_)
                WinHttpCloseHandle(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    HINTERNET get() const
    {
        return handle_;
    }

    bool valid() const
    {
        return handle_ != nullptr;
    }

private:
    HINTERNET handle_ = nullptr;
};

std::wstring Utf8ToWide(const std::string &text)
{
    if (text.empty())
        return {};

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0)
        return {};

    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

bool TryParseHex4(const std::string &json, size_t pos, uint32_t &value)
{
    if (pos + 4 > json.size())
        return false;

    uint32_t out = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        const unsigned char ch = static_cast<unsigned char>(json[pos + i]);
        out <<= 4;
        if (ch >= '0' && ch <= '9')
            out |= static_cast<uint32_t>(ch - '0');
        else if (ch >= 'a' && ch <= 'f')
            out |= static_cast<uint32_t>(10 + (ch - 'a'));
        else if (ch >= 'A' && ch <= 'F')
            out |= static_cast<uint32_t>(10 + (ch - 'A'));
        else
            return false;
    }

    value = out;
    return true;
}

void AppendUtf8CodePoint(std::string &out, uint32_t codePoint)
{
    if (codePoint <= 0x7Fu)
    {
        out.push_back(static_cast<char>(codePoint));
        return;
    }
    if (codePoint <= 0x7FFu)
    {
        out.push_back(static_cast<char>(0xC0u | ((codePoint >> 6) & 0x1Fu)));
        out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
        return;
    }
    if (codePoint <= 0xFFFFu)
    {
        out.push_back(static_cast<char>(0xE0u | ((codePoint >> 12) & 0x0Fu)));
        out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
        return;
    }

    out.push_back(static_cast<char>(0xF0u | ((codePoint >> 18) & 0x07u)));
    out.push_back(static_cast<char>(0x80u | ((codePoint >> 12) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3Fu)));
    out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
}

bool ExtractJsonStringField(const std::string &json, const char *field, std::string &value)
{
    const std::string key = "\"" + std::string(field) + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos)
        return false;

    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos)
        return false;

    pos = json.find('"', pos + 1);
    if (pos == std::string::npos)
        return false;
    ++pos;

    std::string out;
    while (pos < json.size())
    {
        char ch = json[pos++];
        if (ch == '"')
        {
            value = out;
            return true;
        }
        if (ch == '\\')
        {
            if (pos >= json.size())
                break;
            char esc = json[pos++];
            switch (esc)
            {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u':
            {
                uint32_t codePoint = 0;
                if (!TryParseHex4(json, pos, codePoint))
                    return false;
                pos += 4;

                if (codePoint >= 0xD800u && codePoint <= 0xDBFFu)
                {
                    if (pos + 6 <= json.size() && json[pos] == '\\' && json[pos + 1] == 'u')
                    {
                        uint32_t low = 0;
                        if (TryParseHex4(json, pos + 2, low) && low >= 0xDC00u && low <= 0xDFFFu)
                        {
                            codePoint = 0x10000u + (((codePoint - 0xD800u) << 10) | (low - 0xDC00u));
                            pos += 6;
                        }
                    }
                }

                AppendUtf8CodePoint(out, codePoint);
                break;
            }
            default:
                out.push_back(esc);
                break;
            }
            continue;
        }
        out.push_back(ch);
    }
    return false;
}

bool IsTrustedReleaseUrl(const std::wstring &url)
{
    if (url.rfind(L"https://github.com/", 0) == 0)
        return true;
    if (url.rfind(L"https://www.github.com/", 0) == 0)
        return true;
    return false;
}

bool IsLikelyVersionTag(const std::wstring &version)
{
    if (version.empty() || version.size() > 64)
        return false;
    for (wchar_t ch : version)
    {
        if (ch < 0x20 || ch == 0x7F)
            return false;
    }
    return true;
}

bool FetchLatestReleaseMetadata(std::string &tagName, std::string &releaseUrl)
{
    const std::wstring releaseApiPath = std::wstring(L"/repos/") + APP_GITHUB_OWNER + L"/" + APP_GITHUB_REPO + L"/releases/latest";

    ScopedWinHttpHandle hSession(WinHttpOpen((std::wstring(L"TechnicalStandardNote/") + APP_VERSION).c_str(),
                                             WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS,
                                             0));
    if (!hSession.valid())
        return false;

    if (!WinHttpSetTimeouts(hSession.get(), kHttpResolveTimeoutMs, kHttpConnectTimeoutMs, kHttpSendTimeoutMs, kHttpReceiveTimeoutMs))
        return false;

    ScopedWinHttpHandle hConnect(WinHttpConnect(hSession.get(), L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!hConnect.valid())
        return false;

    ScopedWinHttpHandle hRequest(WinHttpOpenRequest(hConnect.get(), L"GET", releaseApiPath.c_str(),
                                                    nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    WINHTTP_FLAG_SECURE));
    if (!hRequest.valid())
        return false;

    const wchar_t *headers = L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
    const BOOL sent = WinHttpSendRequest(hRequest.get(), headers, static_cast<DWORD>(-1L),
                                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    const BOOL received = sent ? WinHttpReceiveResponse(hRequest.get(), nullptr) : FALSE;
    if (!received)
        return false;

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        statusCode != 200)
        return false;

    std::string json;
    for (;;)
    {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(hRequest.get(), &available))
            return false;
        if (available == 0)
            break;
        if (json.size() + static_cast<size_t>(available) > kUpdatePayloadMaxBytes)
            return false;

        std::string chunk(available, '\0');
        DWORD bytesRead = 0;
        if (!WinHttpReadData(hRequest.get(), chunk.data(), available, &bytesRead))
            return false;
        if (bytesRead == 0)
            return false;

        chunk.resize(bytesRead);
        if (json.size() + chunk.size() > kUpdatePayloadMaxBytes)
            return false;
        json += chunk;
    }

    return ExtractJsonStringField(json, "tag_name", tagName) &&
           ExtractJsonStringField(json, "html_url", releaseUrl);
}
}

void HelpCheckUpdates()
{
    const auto &lang = GetLangStrings();
    const std::wstring fallbackUrl = std::wstring(APP_REPOSITORY_URL) + L"/releases/latest";

    std::string tagNameUtf8;
    std::string releaseUrlUtf8;
    if (!FetchLatestReleaseMetadata(tagNameUtf8, releaseUrlUtf8))
    {
        int choice = MessageBoxW(g_hwndMain,
                                 lang.msgUpdateCheckFailed.c_str(),
                                 lang.appName.c_str(),
                                 MB_YESNO | MB_ICONINFORMATION);
        if (choice == IDYES)
            ShellExecuteW(nullptr, L"open", fallbackUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    const std::wstring latestVersion = NormalizeVersionTag(Utf8ToWide(tagNameUtf8));
    const std::wstring currentVersion = APP_VERSION;
    std::wstring releaseUrl = Utf8ToWide(releaseUrlUtf8);
    if (releaseUrl.empty() || !IsTrustedReleaseUrl(releaseUrl))
        releaseUrl = fallbackUrl;

    if (!IsLikelyVersionTag(latestVersion))
    {
        int choice = MessageBoxW(g_hwndMain,
                                 lang.msgUpdateParseVersionFailed.c_str(),
                                 lang.appName.c_str(),
                                 MB_YESNO | MB_ICONINFORMATION);
        if (choice == IDYES)
            ShellExecuteW(nullptr, L"open", releaseUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    const int compare = CompareVersions(currentVersion, latestVersion);
    if (compare < 0)
    {
        std::wstring message = lang.msgUpdateAvailable + L"\n\n" +
                               lang.msgCurrentVersion + currentVersion +
                               L"\n" + lang.msgLatestVersion + latestVersion +
                               L"\n\n" + lang.msgOpenDownloadPage;
        int choice = MessageBoxW(g_hwndMain, message.c_str(), lang.appName.c_str(), MB_YESNO | MB_ICONINFORMATION);
        if (choice == IDYES)
            ShellExecuteW(nullptr, L"open", releaseUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    std::wstring message = lang.msgUpToDate + L"\n\n" +
                           lang.msgCurrentVersion + currentVersion +
                           L"\n" + lang.msgLatestVersion + latestVersion;
    MessageBoxW(g_hwndMain, message.c_str(), lang.appName.c_str(), MB_OK | MB_ICONINFORMATION);
}

