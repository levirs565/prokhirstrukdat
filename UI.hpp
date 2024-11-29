#pragma once

#include "Winapi.hpp"
#include <CommCtrl.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdexcept>
#include <algorithm>

namespace UI
{
    HINSTANCE _hInstance;
    int _nCmdShow;
    UINT_PTR _nextControlId = 4001;

    struct GDI
    {
        using CreateFontWT = HFONT(WINAPI *)(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic,
                                             DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                                             DWORD iQuality, DWORD iPitchAndFamily, LPCWSTR pszFaceName);
        CreateFontWT CreateFont;

        using GetTextExtentPoint32WT = BOOL(WINAPI *)(
            HDC hdc,
            LPCWSTR lpString,
            int c,
            LPSIZE psizl);
        GetTextExtentPoint32WT GetTextExtentPoint32W;

        using SelectObjectT = HGDIOBJ(WINAPI *)(HDC hdc, HGDIOBJ h);
        SelectObjectT SelectObject;

        using GetDeviceCapsT = int(WINAPI *)(HDC hdc, int index);
        GetDeviceCapsT GetDeviceCaps;
    } gdi;

    HINSTANCE _LoadLibrary(const char *library)
    {
        HINSTANCE inst = LoadLibraryA(library);

        if (inst == 0)
            throw Winapi::Error(std::string("Cannot LoadLibraryA ") + library);

        return inst;
    }

    template <typename T>
    T _GetProcAddress(HINSTANCE hInst, const char *name)
    {
        T func = (T)GetProcAddress(hInst, name);

        if (func == 0)
            throw Winapi::Error(std::string("Cannot GetProcAddress ") + name);

        return func;
    }

    void _LoadGDI()
    {
        HINSTANCE inst = _LoadLibrary("Gdi32.dll");
        gdi.CreateFontW = _GetProcAddress<GDI::CreateFontWT>(inst, "CreateFontW");
        gdi.GetTextExtentPoint32W = _GetProcAddress<GDI::GetTextExtentPoint32WT>(inst, "GetTextExtentPoint32W");
        gdi.SelectObject = _GetProcAddress<GDI::SelectObjectT>(inst, "SelectObject");
        gdi.GetDeviceCaps = _GetProcAddress<GDI::GetDeviceCapsT>(inst, "GetDeviceCaps");
    }

    HFONT hDefaultFont;
    void _InitFont()
    {
        HDC hDC = GetDC(0);
        int nHeight = -MulDiv(10, gdi.GetDeviceCaps(hDC, LOGPIXELSY), 72);
        ReleaseDC(0, hDC);
        hDefaultFont = gdi.CreateFontW(
            nHeight, 0, 0, 0, FW_DONTCARE,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, L"Segoe UI");

        if (hDefaultFont == 0)
        {
            throw Winapi::Error("Error loading font");
        }
    }

    void _CommCtlInitCommonControl()
    {

        HINSTANCE inst = _LoadLibrary("Comctl32.dll");

        using InitControlsT = void(WINAPI *)(void);
        InitControlsT func = _GetProcAddress<InitControlsT>(inst, "InitCommonControls");

        func();
    }

    void _LoadManifest() {
        ACTCTX actx{0};
        actx.cbSize = sizeof(ACTCTX);
        actx.dwFlags = 0;
        actx.lpSource = L"app.manifest";

        HANDLE hactx = CreateActCtxW(&actx);

        if (hactx == INVALID_HANDLE_VALUE)
            throw Winapi::Error("CreateActCtxW failed");

        ULONG_PTR actxCookie = 0;
        if (!ActivateActCtx(hactx, &actxCookie))
            throw Winapi::Error("ActivateActCtx");
    }

    void Setup(HINSTANCE instance, int nCmdShow)
    {   
        _LoadManifest();
        _CommCtlInitCommonControl();
        _LoadGDI();
        _InitFont();
        _hInstance = instance;
        _nCmdShow = nCmdShow;
    }

    struct Layouter
    {
        int x, y, w, h;
        int nextX, nextY;

        int maxW = 0, maxH = 0;
        int paddingBottom = 0;

        int spacing = 5;

        void init(HWND hwnd)
        {
            x = spacing;
            y = spacing;

            RECT rect;
            GetClientRect(hwnd, &rect);

            w = rect.right - rect.left;
            h = rect.bottom - rect.top - paddingBottom;

            nextX = 0;
            nextY = 0;
            maxW = 0;
            maxH = 0;
        }

        void nextRow()
        {
            if (maxH == 0)
                return;

            x = spacing;
            y += maxH + spacing;
            maxH = 0;
        }

        void nextColumn()
        {
            if (maxW == 0)
                return;

            x = nextX + spacing;
            maxW = 0;
        }

        POINT pos()
        {
            return {x, y};
        }

        SIZE component(int w, int h)
        {
            if (w == -1)
            {
                w = this->w - x - spacing;
            }
            if (h == -1)
            {
                h = this->h - y - spacing;
            }
            maxW = std::max(maxW, w);
            maxH = std::max(maxH, h);

            nextX = x + w;
            nextY = y + h;

            return {w, h};
        }
    };

    struct Window;
    struct Control;
    struct FixedControl;

    struct CallbackParam
    {
        Window *window;
        UINT message;
        WPARAM wParam;
        LPARAM lParam;
    };

    using CallbackType = std::function<LRESULT(CallbackParam)>;

    struct Window
    {
        std::wstring title;
        bool quitWhenClose = false;
        HWND parentHwnd = 0;

        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        Layouter layouter;
        std::map<UINT, CallbackType> _msgListeners;
        std::map<UINT_PTR, CallbackType> _cmdListeners;
        std::map<std::pair<UINT_PTR, UINT>, CallbackType> _notifyListeners;
        ATOM _atom = 0;
        HWND hwnd = 0;
        HBRUSH hBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);

        std::vector<std::vector<Control *>> controls;
        std::vector<FixedControl *> fixedControls;
        std::vector<Control *> _childs;

        void registerMessageListener(UINT message, CallbackType callback)
        {
            _msgListeners.emplace(message, callback);
        }

        void Clear()
        {
            controls.clear();
            fixedControls.clear();
            _msgListeners.clear();
            _cmdListeners.clear();
        }
    };

    HWND CreateWindowClass(Window &data);

    struct Control
    {
        int w = 0, h = 0;
        HWND hwnd;

        UINT_PTR _controlId = 0;
        std::wstring _className, _setupTitle;
        DWORD _dwStyle = WS_CHILD | WS_VISIBLE, _dwExStyle = 0;

        virtual SIZE GetDefaultSize()
        {
            return {0, 0};
        }

        virtual void Create(Window *window, HWND hParent, POINT pos, SIZE size)
        {
            if (_controlId == 0)
            {
                _controlId = _nextControlId++;
            }
            hwnd = CreateWindowExW(
                _dwExStyle,
                _className.c_str(),
                _setupTitle.c_str(),
                _dwStyle,
                pos.x, pos.y, size.cx, size.cy,
                hParent == 0 ? window->hwnd : hParent,
                reinterpret_cast<HMENU>(_controlId),
                _hInstance,
                0);
        }

        virtual void UpdatePosDefer(HDWP hDefer, POINT pos, SIZE size)
        {
            DeferWindowPos(hDefer, hwnd, nullptr, pos.x, pos.y, size.cx, size.cy, SWP_NOZORDER);
        }

        void ApplyDefaultFont()
        {
            SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<LPARAM>(hDefaultFont), TRUE);
        }

        void SetEnable(bool enable) {
            EnableWindow(hwnd, enable);
        }
    };

    struct TextBox : Control
    {
        SIZE GetDefaultSize() override
        {
            return {75, 23};
        }

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = WC_EDITW;
            Control::Create(window, hParent, pos, size);
            ApplyDefaultFont();
        }

        std::wstring getText()
        {
            std::wstring result;
            int size = GetWindowTextLengthW(hwnd);
            if (size != 0)
            {
                result.resize(size + 1, '\0');
                GetWindowTextW(hwnd, &result[0], size + 1);
                result.resize(size);
            }

            return result;
        }
    };

    struct Label : Control
    {
        std::wstring text;

        SIZE GetDefaultSize() override
        {
            SIZE size;
            HDC hDC = GetDC(0);
            gdi.SelectObject(hDC, hDefaultFont);
            gdi.GetTextExtentPoint32W(hDC, text.c_str(), text.length(), &size);
            ReleaseDC(0, hDC);

            return size;
        }

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _setupTitle = text;
            _className = WC_STATICW;
            Control::Create(window, hParent, pos, size);
            ApplyDefaultFont();
        }

        void SetText(const std::wstring& text) {
            SetWindowTextW(hwnd, text.c_str());
        }
    };

    struct Button : Control
    {
        std::wstring text;
        CallbackType commandListener;

        SIZE GetDefaultSize() override
        {
            return {75, 23};
        }

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = WC_BUTTONW;
            _setupTitle = text;
            Control::Create(window, hParent, pos, size);
            ApplyDefaultFont();

            if (commandListener)
                window->_cmdListeners.emplace(_controlId, commandListener);
        }
    };

    struct ListView : Control
    {
        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = WC_LISTVIEWW;

            Control::Create(window, hParent, pos, size);
        }

        void InsertColumn(const std::wstring &title, int width)
        {
            LVCOLUMNW lvc{};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = const_cast<wchar_t *>(title.c_str());
            lvc.cx = width;
            ListView_InsertColumn(hwnd, 0xFFFF, &lvc);
        }

        void DeleteAllRows() {
            ListView_DeleteAllItems(hwnd);
        }

        int InsertRow(const std::wstring &text)
        {
            LVITEMW lvi{};
            lvi.iItem = 0x0FFFFFFF;
            lvi.mask = LVIF_TEXT;
            lvi.pszText = const_cast<wchar_t *>(text.c_str());
            return ListView_InsertItem(hwnd, &lvi);
        }

        void SetText(int row, int column, const std::wstring &text)
        {
            ListView_SetItemText(hwnd, row, column, const_cast<wchar_t *>(text.c_str()));
        }
    };

    struct FixedControl
    {
        virtual void Repos() = 0;
    };

    struct StatusBar : Control, FixedControl
    {
        std::vector<int> _rightEdges;

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = STATUSCLASSNAMEW;

            Control::Create(window, hParent, pos, size);
        }

        void Create(Window *window)
        {
            Create(window, 0, {0, 0}, {0, 0});
        }

        void Repos() override
        {
            SendMessageW(hwnd, WM_SIZE, 0, 0);
        }

        void SetParts(const std::vector<int> &sizes)
        {
            _rightEdges.resize(sizes.size());

            int right = 0;
            for (size_t i = 0; i < sizes.size(); i++)
            {
                right += sizes[i];
                _rightEdges[i] = right;
            }

            SendMessageW(hwnd, SB_SETPARTS, _rightEdges.size(), reinterpret_cast<LPARAM>(&_rightEdges[0]));
        }

        void SetText(size_t index, const std::wstring &text)
        {
            SendMessageW(hwnd, SB_SETTEXT, MAKEWPARAM(MAKEWORD(index, 0), 0), reinterpret_cast<LPARAM>(text.c_str()));
        }
    };

    void ToggleWindowStyle(HWND hwnd, bool isEx, bool add, DWORD style)
    {
        LONG_PTR current = GetWindowLongPtrW(hwnd, isEx ? GWL_EXSTYLE : GWL_STYLE);
        if (add)
            current |= static_cast<LONG_PTR>(style);
        else
            current &= static_cast<LONG_PTR>(~style);
        SetWindowLongPtrW(hwnd, isEx ? GWL_EXSTYLE : GWL_STYLE, current);
    }

    struct ProgressBar : Control
    {
        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = PROGRESS_CLASSW;

            Control::Create(window, hParent, pos, size);
        }

        SIZE GetDefaultSize() override {
            return {75, 23};
        }

        void SetProgress(int pos)
        {
            SendMessageW(hwnd, PBM_SETPOS, pos, 0);
        }

        void SetWaiting(bool isWaiting)
        {
            SendMessageW(hwnd, PBM_SETMARQUEE, static_cast<WPARAM>(isWaiting), 0);
            ToggleWindowStyle(hwnd, false, isWaiting, PBS_MARQUEE);
        }
    };

    struct Tabs : Control
    {
        std::vector<HWND> _windows;

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _windows.clear();
            _className = WC_TABCONTROLW;

            Control::Create(window, hParent, pos, size);

            ApplyDefaultFont();

            window->_notifyListeners.emplace(
                std::make_pair(_controlId, TCN_SELCHANGE),
                std::bind(&Tabs::OnSelChange, this, std::placeholders::_1));
        }

        LRESULT OnSelChange(CallbackParam param)
        {
            size_t selected = static_cast<size_t>(TabCtrl_GetCurSel(hwnd));

            for (size_t i = 0; i < _windows.size(); i++)
            {
                ShowWindow(_windows[i], i == selected ? SW_SHOW : SW_HIDE);
            }

            return 0;
        }

        void AddPage(const std::wstring &text, Window *page)
        {
            TCITEMW tci{0};

            tci.mask = TCIF_TEXT;
            tci.pszText = const_cast<wchar_t *>(text.c_str());

            TabCtrl_InsertItem(hwnd, 0xFFFF, &tci);

            page->dwStyle = WS_CHILD | WS_VISIBLE;
            page->parentHwnd = hwnd;
            page->hBackground = 0;

            _windows.push_back(CreateWindowClass(*page));

            if (_windows.size() == 1)
                ShowWindow(_windows.back(), SW_SHOW);
            else
                ShowWindow(_windows.back(), SW_HIDE);
        }

        void UpdatePosDefer(HDWP hDefer, POINT pos, SIZE size) override
        {
            Control::UpdatePosDefer(hDefer, pos, size);

            int paddingTop = 25;
            for (HWND window : _windows)
            {
                SetWindowPos(window, NULL, 0, paddingTop, size.cx, size.cy - paddingTop, SWP_NOZORDER);
            }
        }
    };

    struct ComboBox : Control
    {
        SIZE GetDefaultSize() override
        {
            return {75, 23};
        }

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = WC_COMBOBOXW;
            _dwStyle |= CBS_DROPDOWNLIST;

            Control::Create(window, hParent, pos, size);

            ApplyDefaultFont();
        }

        void AddItem(const std::wstring &text)
        {
            SendMessageW(hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }

        int GetSelectedIndex()
        {
            return SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
        }

        void SetSelectedIndex(int index) {
            SendMessageW(hwnd, CB_SETCURSEL, index, 0);
        }

        std::wstring GetText(int index)
        {
            std::wstring res;
            size_t size = SendMessageW(hwnd, CB_GETLBTEXTLEN, index, 0);
            if (size)
            {
                res.resize(size + 1, '\0');
                SendMessageW(hwnd, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(&res[0]));
                res.resize(size);
            }

            return res;
        }

        std::wstring GetSelectedText()
        {
            return GetText(GetSelectedIndex());
        }
    };

    void LayoutControls(Window *window, bool create)
    {
        bool firstRow = true;
        window->layouter.init(window->hwnd);

        HDWP hDefer = 0;

        if (!create)
        {
            int count = 0;
            for (const auto &row : window->controls)
            {
                count += window->controls.size();
            }

            hDefer = BeginDeferWindowPos(count);
        }

        for (const auto &row : window->controls)
        {
            if (!firstRow)
            {
                window->layouter.nextRow();
            }
            firstRow = false;

            bool firstColumn = true;
            for (Control *control : row)
            {
                if (!firstColumn)
                {
                    window->layouter.nextColumn();
                }
                firstColumn = false;

                SIZE defaultSize = control->GetDefaultSize();
                POINT pos = window->layouter.pos();
                SIZE size = window->layouter.component(
                    control->w == 0 ? defaultSize.cx : control->w,
                    control->h == 0 ? defaultSize.cy : control->h);

                if (create)
                {
                    control->Create(window, 0, pos, size);
                }
                else
                {
                    control->UpdatePosDefer(hDefer, pos, size);
                }
            }
        }

        if (hDefer != 0)
        {
            EndDeferWindowPos(hDefer);
        }

        for (FixedControl *control : window->fixedControls)
        {
            control->Repos();
        }
    }

    LRESULT CALLBACK _WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Window *self = nullptr;
        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCTW *createParam = reinterpret_cast<CREATESTRUCTW *>(lParam);
            self = reinterpret_cast<Window *>(createParam->lpCreateParams);
            self->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        switch (uMsg)
        {
        case WM_DESTROY:
        {
            if (self->quitWhenClose)
            {
                PostQuitMessage(0);
                return 0;
            }
            self->Clear();
            break;
        }
        case WM_SIZE:
            LayoutControls(self, false);
            return 0;
        case WM_COMMAND:
        {
            if (self)
            {
                auto func = self->_cmdListeners.find(LOWORD(wParam));
                if (func != self->_cmdListeners.end())
                {
                    return func->second({self,
                                         uMsg,
                                         wParam,
                                         lParam});
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            if (self)
            {
                LPNMHDR param = reinterpret_cast<LPNMHDR>(lParam);
                auto func = self->_notifyListeners.find(std::make_pair(param->idFrom, param->code));
                if (func != self->_notifyListeners.end())
                {
                    return func->second({self,
                                         uMsg,
                                         wParam,
                                         lParam});
                }
            }
            break;
        }
        }

        if (self)
        {
            auto func = self->_msgListeners.find(uMsg);

            if (func != self->_msgListeners.end())
            {
                return func->second({self,
                                     uMsg,
                                     wParam,
                                     lParam});
            }
        }

        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    HWND CreateWindowClass(Window &data)
    {
        if (data._atom != 0)
        {
            if (!UnregisterClassW(reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(static_cast<WORD>(data._atom))), _hInstance))
            {
                throw Winapi::Error("Error at UnregisterClassW");
            }
        }

        WNDCLASSEXW wc = {};

        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = _WindowProc;
        wc.hInstance = _hInstance;
        wc.lpszClassName = data.title.c_str();
        wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
        wc.hbrBackground = data.hBackground;

        data._atom = RegisterClassExW(&wc);

        if (data._atom == 0)
        {
            throw Winapi::Error("Error at RegisterClassExW");
        }

        HWND hwnd = CreateWindowExW(
            0,
            reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(static_cast<WORD>(data._atom))),
            data.title.c_str(),
            data.dwStyle,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            data.parentHwnd,
            NULL,
            _hInstance,
            reinterpret_cast<LPVOID>(&data));

        if (hwnd == 0)
        {
            throw Winapi::Error("Error at CreateWindowExW");
        }

        return hwnd;
    }

    void ShowWindowClass(Window &data)
    {

        ShowWindow(CreateWindowClass(data), _nCmdShow);
    }

    int RunEventLoop()
    {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return 0;
    }
}