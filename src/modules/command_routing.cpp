/*
  Solum

  Shared command routing for common menu commands.
*/

#include "command_routing.h"

#include "background.h"
#include "commands.h"
#include "dialog.h"
#include "editor.h"
#include "file.h"
#include "theme.h"
#include "ui.h"
#include "core/globals.h"
#include "resource.h"

bool RouteStandardCommand(HWND hwnd, WORD cmd)
{
    switch (cmd)
    {
    case IDM_FILE_SAVE:
        FileSave();
        return true;
    case IDM_FILE_SAVEAS:
        FileSaveAs();
        return true;
    case IDM_FILE_PRINT:
        FilePrint();
        return true;
    case IDM_FILE_PAGESETUP:
        FilePageSetup();
        return true;
    case IDM_FILE_EXIT:
        SendMessageW(hwnd, WM_CLOSE, 0, 0);
        return true;

    case IDM_EDIT_UNDO:
        EditUndo();
        return true;
    case IDM_EDIT_REDO:
        EditRedo();
        return true;
    case IDM_EDIT_CUT:
        EditCut();
        return true;
    case IDM_EDIT_COPY:
        EditCopy();
        return true;
    case IDM_EDIT_PASTE:
        EditPaste();
        return true;
    case IDM_EDIT_DELETE:
        EditDelete();
        return true;
    case IDM_EDIT_FIND:
        EditFind();
        return true;
    case IDM_EDIT_FINDNEXT:
        EditFindNext();
        return true;
    case IDM_EDIT_FINDPREV:
        EditFindPrev();
        return true;
    case IDM_EDIT_REPLACE:
        EditReplace();
        return true;
    case IDM_EDIT_GOTO:
        EditGoto();
        return true;
    case IDM_EDIT_SELECTALL:
        EditSelectAll();
        return true;
    case IDM_EDIT_TIMEDATE:
        EditTimeDate();
        return true;

    case IDM_FORMAT_WORDWRAP:
        if (!g_state.largeFileMode)
            FormatWordWrap();
        return true;
    case IDM_FORMAT_FONT:
        FormatFont();
        return true;
    case IDM_FORMAT_BOLD:
        FormatBold();
        return true;
    case IDM_FORMAT_ITALIC:
        FormatItalic();
        return true;
    case IDM_FORMAT_STRIKETHROUGH:
        FormatStrikethrough();
        return true;

    case IDM_VIEW_ZOOMIN:
        ViewZoomIn();
        return true;
    case IDM_VIEW_ZOOMOUT:
        ViewZoomOut();
        return true;
    case IDM_VIEW_ZOOMDEFAULT:
        ViewZoomDefault();
        return true;
    case IDM_VIEW_STATUSBAR:
        ViewStatusBar();
        return true;
    case IDM_VIEW_DARKMODE:
        ToggleDarkMode();
        return true;
    case IDM_VIEW_TRANSPARENCY:
        ViewTransparency();
        return true;
    case IDM_VIEW_ALWAYSONTOP:
        ViewAlwaysOnTop();
        return true;

    case IDM_VIEW_BG_SELECT:
        ViewSelectBackground();
        return true;
    case IDM_VIEW_BG_CLEAR:
        ViewClearBackground();
        return true;
    case IDM_VIEW_BG_OPACITY:
        ViewBackgroundOpacity();
        return true;
    case IDM_VIEW_BG_POS_TOPLEFT:
        SetBackgroundPosition(BgPosition::TopLeft);
        return true;
    case IDM_VIEW_BG_POS_TOPCENTER:
        SetBackgroundPosition(BgPosition::TopCenter);
        return true;
    case IDM_VIEW_BG_POS_TOPRIGHT:
        SetBackgroundPosition(BgPosition::TopRight);
        return true;
    case IDM_VIEW_BG_POS_CENTERLEFT:
        SetBackgroundPosition(BgPosition::CenterLeft);
        return true;
    case IDM_VIEW_BG_POS_CENTER:
        SetBackgroundPosition(BgPosition::Center);
        return true;
    case IDM_VIEW_BG_POS_CENTERRIGHT:
        SetBackgroundPosition(BgPosition::CenterRight);
        return true;
    case IDM_VIEW_BG_POS_BOTTOMLEFT:
        SetBackgroundPosition(BgPosition::BottomLeft);
        return true;
    case IDM_VIEW_BG_POS_BOTTOMCENTER:
        SetBackgroundPosition(BgPosition::BottomCenter);
        return true;
    case IDM_VIEW_BG_POS_BOTTOMRIGHT:
        SetBackgroundPosition(BgPosition::BottomRight);
        return true;
    case IDM_VIEW_BG_POS_TILE:
        SetBackgroundPosition(BgPosition::Tile);
        return true;
    case IDM_VIEW_BG_POS_STRETCH:
        SetBackgroundPosition(BgPosition::Stretch);
        return true;
    case IDM_VIEW_BG_POS_FIT:
        SetBackgroundPosition(BgPosition::Fit);
        return true;
    case IDM_VIEW_BG_POS_FILL:
        SetBackgroundPosition(BgPosition::Fill);
        return true;

    case IDM_VIEW_ICON_CHANGE:
        ViewChangeIcon();
        return true;
    case IDM_VIEW_ICON_SYSTEM:
        ViewChooseSystemIcon();
        return true;
    case IDM_VIEW_ICON_RESET:
        ViewResetIcon();
        return true;

    case IDM_HELP_CHECKUPDATES:
        HelpCheckUpdates();
        return true;
    case IDM_HELP_PERF_BENCHMARK:
        HelpRunPerformanceBenchmark();
        return true;
    case IDM_HELP_ABOUT:
        HelpAbout();
        return true;

    default:
        return false;
    }
}
