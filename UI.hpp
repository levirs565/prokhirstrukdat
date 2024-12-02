#pragma once

/*
    Sebuah library UI sederhana yang menggunakan Winapi (Windows API)
*/

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

    /**
     * Struktur yang digunakan untuk menyimpan pointer ke fungsi di library Gdi32.dll
     */
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

    /**
     * Fungsi untuk memuat library DLL secara dinamis
     */
    HINSTANCE _LoadLibrary(const char *library)
    {
        HINSTANCE inst = LoadLibraryA(library);

        if (inst == 0)
            throw Winapi::Error(std::string("Cannot LoadLibraryA ") + library);

        return inst;
    }

    /**
     * Fungsi yang akan menghasilkan pointer ke fungsi yang menunjuk ke fungsi dengan nama `name` dari library yang ditunjuk `hInst`
     */
    template <typename T>
    T _GetProcAddress(HINSTANCE hInst, const char *name)
    {
        T func = (T)GetProcAddress(hInst, name);

        if (func == 0)
            throw Winapi::Error(std::string("Cannot GetProcAddress ") + name);

        return func;
    }

    /**
     * Memuat library Gdi32.dll dan mengisi struktur GDI dengan pointer ke fungsi yang sesuai
     */
    void _LoadGDI()
    {
        HINSTANCE inst = _LoadLibrary("Gdi32.dll");
        gdi.CreateFontW = _GetProcAddress<GDI::CreateFontWT>(inst, "CreateFontW");
        gdi.GetTextExtentPoint32W = _GetProcAddress<GDI::GetTextExtentPoint32WT>(inst, "GetTextExtentPoint32W");
        gdi.SelectObject = _GetProcAddress<GDI::SelectObjectT>(inst, "SelectObject");
        gdi.GetDeviceCaps = _GetProcAddress<GDI::GetDeviceCapsT>(inst, "GetDeviceCaps");
    }

    /**
     * Memuaat font yang akan digunakan sebagai font default
     */
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

    /**
     * Memuat library Comctl32.dll dan memanggil fungsi InitCommonControls
     * InitCommonControls akan menginisialisi kontrol umum seperti ListView agar bisa digunakan aplikasi
     */
    void _CommCtlInitCommonControl()
    {
        HINSTANCE inst = _LoadLibrary("Comctl32.dll");

        using InitControlsT = void(WINAPI *)(void);
        InitControlsT func = _GetProcAddress<InitControlsT>(inst, "InitCommonControls");

        func();
    }

    /**
     * Memuat file manifest `app.manifest`
     * Manifest dibutuhkan agar controk menggunakan Common Controls V6 yang mempunyai tampilan modern
     * Lihat file `app.manifest` untuk lebih lanjut
     */
    void _LoadManifest()
    {
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

    /**
     * Mensetup library UI
     * Melakukan semua prosedur yang harus dilakukan sebelum membuat window apapun
     * Harus dipanggil sebelum membuat Window
     */
    void Setup(HINSTANCE instance, int nCmdShow)
    {
        _LoadManifest();
        _CommCtlInitCommonControl();
        _LoadGDI();
        _InitFont();
        _hInstance = instance;
        _nCmdShow = nCmdShow;
    }

    /**
     * Sebuah layout engine berjenis flow
     * Layout hanya bisa berjalan dari atas ke bawah dan dari kiri ke nana
     */
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

    /**
     * Struktur yang akan diberikan ke callback
     */
    struct CallbackParam
    {
        Window *window;
        UINT message;
        WPARAM wParam;
        LPARAM lParam;
    };

    /**
     * Tipe data fungsi untuk callback
     */
    using CallbackType = std::function<LRESULT(CallbackParam)>;

    /**
     * Sebuah struktur yang menyimpan informasi window
     */
    struct Window
    {
        /**
         * Judul dan nama kelas dari window
         */
        std::wstring title;
        /**
         * Menentukan apakah jika window di close maka aplikasi harus dihentikan
         */
        bool quitWhenClose = false;
        /**
         * Menentukan handle HWND ke window parent
         * Dubutuhkan jika window dijasikan sebagai subwindow atau menjadi kontrol di dalam window
         */
        HWND parentHwnd = 0;

        /**
         * Menentukan dwStyle pada pemanggilan CreateWindowExW
         */
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        /**
         * Layouter yang digunakan untuk melayout kontrol
         */
        Layouter layouter;
        /**
         * Sebuah map/table yang memetqkan `messageCode` dengan `callback`
         * `callback` akan dipanggil saat window mendapatkan pesan dengan kode `messageKode`
         */
        std::map<UINT, CallbackType> _msgListeners;
        /**
         * Sebuah map/table yang memetqkan `controID` dengan `callback`
         * `callback` akan dipanggil saat kontrol dengan id `controlID` mendapatkan pesan WM_COMMAND
         */
        std::map<UINT_PTR, CallbackType> _cmdListeners;
        /**
         * Sebuah map/table yang memetqkan `controID`,`notifyCode` dengan `callback`
         * `callback` akan dipanggil saat kontrol dengan id `controlID` mendapatkan pesan WM_NOTIFY dengan kode pesa `notifyCode`
         */
        std::map<std::pair<UINT_PTR, UINT>, CallbackType> _notifyListeners;
        /**
         * Berisi hasil dari CreateWindowExW
         */
        ATOM _atom = 0;
        /**
         * Berisi handle HWND window sekarang
         */
        HWND hwnd = 0;
        /**
         * Berisi background dari window, akan digunakan sebagai parameter di CreateWindowExW
         */
        HBRUSH hBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);

        /**
         * Sebuah array dinamis 2D, yang berisi baris dan setiap baris berisi kontrol sebagai kolom
         * Kontrol akan ditata berdasarkan isi array ini
         * Jangan lupa memanggil LayoutControls(&window, true) setalah menyetel nilai ini
         */
        std::vector<std::vector<Control *>> controls;
        /**
         * Array dinamis yang berisi kontrol yang memiliki posisi tetap, contohnya statusbar
         */
        std::vector<FixedControl *> fixedControls;
        /**
         * Berisi semua kontrol yang menjadi child dari window ini
         * Belum berguna untuk saat ini
         */
        std::vector<Control *> _childs;

        /**
         * Mendaftarkan callback yang akan dipanggil saat window mendapatkan pesan dengan kode `message`
         */
        void registerMessageListener(UINT message, CallbackType callback)
        {
            _msgListeners.emplace(message, callback);
        }

        /**
         * Menreset isi dari windows
         * Note: Jangan memenggail fungsi ini
         * Fungsi ini dipanggil otomatis saat window mendapatkan pesan WM_DESTROY
         */
        void Clear()
        {
            controls.clear();
            fixedControls.clear();
            _msgListeners.clear();
            _cmdListeners.clear();
        }
    };

    HWND CreateWindowClass(Window &data);

    /**
     * Struktur untuk kontrol
     */
    struct Control
    {
        /**
         * Ukuran dari kontrol. Akan diberikan ke layouter.
         * Jika -1 maka ukuran kontrol akan memenuhi ruang yang ada
         */
        int w = 0, h = 0;
        /**
         * HWND kontrol saat ini
         */
        HWND hwnd;
        /**
         * Control id dari kontrol ini. Digenerate otomatis
         */
        UINT_PTR _controlId = 0;
        /**
         * Parameter title yang akan diberika kepada CreateWindowExW
         */
        std::wstring _className;
        /**
         * Parameter title yang akan diberika kepada CreateWindowExW
         */
        std::wstring _setupTitle;
        /**
         * Parameter dwStyle yang akan diberika kepada CreateWindowExW. Mengatur style dari kontrol
         */
        DWORD _dwStyle = WS_CHILD | WS_VISIBLE, _dwExStyle = 0;

        /**
         * Mendapatkan ukuran default kontrol.
         * Jika w = 0, maka akan diganti dengan cx hasil fungsi ini
         * Jika h = 0, maka akan diganti dengan cy hasil fungsi ini
         */
        virtual SIZE GetDefaultSize()
        {
            return {0, 0};
        }

        /**
         * Membuat kontrol dengan CreateWindowExW
         * Fungsi init secara otomatis mengisi controlId
         * @param window pointer ke window parent
         * @param hParent HWND dari parent, jika 0 maka hwnd dari window akan digunakan.
         * @param pos posisi dari kontrol
         * @param size ukuran dari kontrol
         */
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

        /**
         * Mengupdate posisi dan ukuran dari kontrol dengan defer
         * Posisi dan ukuran akan diterapkan setelah EndDeferWindowPos dipanggil
         * Digunakan untuk meningkatkan performa saat window perlu melakukan tata letak ulang
         */
        virtual void UpdatePosDefer(HDWP hDefer, POINT pos, SIZE size)
        {
            DeferWindowPos(hDefer, hwnd, nullptr, pos.x, pos.y, size.cx, size.cy, SWP_NOZORDER);
        }

        /**
         * Mennerapkan font default
         */
        void ApplyDefaultFont()
        {
            SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<LPARAM>(hDefaultFont), TRUE);
        }

        /**
         * Meengenable dan mendisable window
         */
        void SetEnable(bool enable)
        {
            EnableWindow(hwnd, enable);
        }
    };

    /**
     * Kontrol yang digunakan untuk input text
     */
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

        /**
         * Mendapatkan teks yang dimasukkan ke kontrol
         */
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

    /**
     * Berisi sebuah label/teks
     * Ukuran default akan dihitung berdasasarkan ukuran text
     */
    struct Label : Control
    {
        /**
         * Text yang akan ditampilkan
         * Harus diseteleh sebelum Create dipanggil
         * Setelah pemanggilan create, tidak akan memberikan dampak
         */
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

        /**
         * Mengubah text kontrol, hanya bisa dijalankan setelah Create dan kontrol belum didestroy
         */
        void SetText(const std::wstring &text)
        {
            SetWindowTextW(hwnd, text.c_str());
        }
    };

    /**
     * Kontrol berupa tombol
     */
    struct Button : Control
    {
        /**
         * Text yang akan ditampilkan
         * Harus diseteleh sebelum Create dipanggil
         * Setelah pemanggilan create, tidak akan memberikan dampak
         */
        std::wstring text;
        /**
         * Callback yang akan dipanggil saat tombol ini ditekan (saat tombo mendapatkan pesan WM_COMMAND)
         */
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
    
    /**
     * Kontrol berupa listView
     * Untuk membuat kontrol ini menjadi tabel setel _dwStyle |= LVS_REPORT;
     */
    struct ListView : Control
    {
        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = WC_LISTVIEWW;

            Control::Create(window, hParent, pos, size);
        }

        /**
         * Menambahkan kolom
         * @param title judul kolom
         * @param width panjang dari kolom
         */
        void InsertColumn(const std::wstring &title, int width)
        {
            LVCOLUMNW lvc{};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = const_cast<wchar_t *>(title.c_str());
            lvc.cx = width;
            ListView_InsertColumn(hwnd, 0xFFFF, &lvc);
        }

        /**
         * Menghapus semua baris
         */
        void DeleteAllRows()
        {
            ListView_DeleteAllItems(hwnd);
        }

        /**
         * Menambahkan baris baru
         * @param text berisi isi dari kolom pertama
         * @result index dari baris yang ditambahkan
         */
        int InsertRow(const std::wstring &text)
        {
            LVITEMW lvi{};
            lvi.iItem = 0x0FFFFFFF;
            lvi.mask = LVIF_TEXT;
            lvi.pszText = const_cast<wchar_t *>(text.c_str());
            return ListView_InsertItem(hwnd, &lvi);
        }

        /**
         * Mengganti teks pada baris dan kolom tertentu
         * @param row indeks dari baris, bisa berupa hasil InsertRow
         * @param column indeks dari kolom
         * @param text isi dari sel
         */
        void SetText(int row, int column, const std::wstring &text)
        {
            ListView_SetItemText(hwnd, row, column, const_cast<wchar_t *>(text.c_str()));
        }
    };

    /**
     * Kontrol yang dapat dimasukkan ke window.fixedControls
     */
    struct FixedControl
    {
        /**
         * Fungsi yang dipanggil saat kontrol perlu mengatur posisi dan ukuran setelah window di resize
         */
        virtual void Repos() = 0;
    };

    /**
     * Kontrol yang tampil di bagian bawah
     * Kontrol ini dibagi menjadi beberapa bagian
     */
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

        /**
         * Mengubah jumlah dan ukuran bagian statusbar
         * @param sizes array berisi ukuran bagian
         */
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

        /**
         * Mengubah teks pada bagian tertentu
         * @param index indeks bagian
         * @param text teks untuk bagian
         */
        void SetText(size_t index, const std::wstring &text)
        {
            SendMessageW(hwnd, SB_SETTEXT, MAKEWPARAM(MAKEWORD(index, 0), 0), reinterpret_cast<LPARAM>(text.c_str()));
        }
    };
    
    /**
     * Mengaktifkan dan menonaktifkan style dari kontrol
     * @param hwnd HWND dari kontrol
     * @param isEx jika true maka style termasuk di dwExStyle dan jika false style termasuk di dwStyle
     * @param add jika true maka menambahkan style jika false maka menghapus style
     * @param style kode style yang akan ditambahkan 
     */
    void ToggleWindowStyle(HWND hwnd, bool isEx, bool add, DWORD style)
    {
        LONG_PTR current = GetWindowLongPtrW(hwnd, isEx ? GWL_EXSTYLE : GWL_STYLE);
        if (add)
            current |= static_cast<LONG_PTR>(style);
        else
            current &= static_cast<LONG_PTR>(~style);
        SetWindowLongPtrW(hwnd, isEx ? GWL_EXSTYLE : GWL_STYLE, current);
    }

    /**
     * Kontrol yang akan menampailkan progress
     */
    struct ProgressBar : Control
    {
        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = PROGRESS_CLASSW;

            Control::Create(window, hParent, pos, size);
        }

        SIZE GetDefaultSize() override
        {
            return {75, 23};
        }

        /**
         * Menentukan progress dalam bentuk persen
         * @param pos nilai dalam persen, jika 10 berarti 10%
         */
        void SetProgress(int pos)
        {
            SendMessageW(hwnd, PBM_SETPOS, pos, 0);
        }

        /**
         * Menentukan apakah progressbar dalam keadaaan menunggu/intederminate
         * Gunakan ini jika progress tidak dapat ditentukan
         */
        void SetWaiting(bool isWaiting)
        {
            SendMessageW(hwnd, PBM_SETMARQUEE, static_cast<WPARAM>(isWaiting), 0);
            ToggleWindowStyle(hwnd, false, isWaiting, PBS_MARQUEE);
        }
    };

    /**
     * Kontrol yang akan menjadi tab
     * Tab terdiri dari judul dan body
     */
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

        /**
         * Callback yang akan dipanggil saat pilihan di tab diganti
         * Akan mengganti body yang aktif sesuai dengan pilihan di tab
         */
        LRESULT OnSelChange(CallbackParam param)
        {
            size_t selected = static_cast<size_t>(TabCtrl_GetCurSel(hwnd));

            for (size_t i = 0; i < _windows.size(); i++)
            {
                ShowWindow(_windows[i], i == selected ? SW_SHOW : SW_HIDE);
            }

            return 0;
        }

        /**
         * Menambahkan halaman baru
         * @param text berisi judul dari halaman
         * @param page sebuah window yang akan menjadi body, window harus belum di Create
         */
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

    /**
     * Kontrol berupa pilihan dropdown
     */
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

        /**
         * Menambahkan item ke kontrol
         * @param text teks untuk item
         */
        void AddItem(const std::wstring &text)
        {
            SendMessageW(hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }

        /**
         * @result Mendapatkan indedks item yang terpilih
         */
        int GetSelectedIndex()
        {
            return SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
        }

        /**
         * Mengganti item yang terpilih
         * @param index indeks item yang akan dipilih
         */
        void SetSelectedIndex(int index)
        {
            SendMessageW(hwnd, CB_SETCURSEL, index, 0);
        }

        /**
         * Mendapatkan teks item pada indeks tertentu
         */
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

        /**
         * Mendapatkan teks pada item yang terpilih
         */
        std::wstring GetSelectedText()
        {
            return GetText(GetSelectedIndex());
        }
    };

    /**
     * Melakukan proses tata letak kontrol pada window
     * @param window window yang akan ditata kontrolnya
     * @param create jika true, control akan dibuat, jika false maka kontrol hanya akan mendapatkan update posisi dan ukuran
     */
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

    /**
     * Fungsi utama yang akan dipanggil saat window mendapatkan pesan
     * Fungsi ini akan 
     * - memanggil callback yang tertaut dengan pesan yang sesaui
     * - Jika pesan berupa WM_COMMAND, maka akan memanggil callbak yang tertaut dengan control yang sesaui
     * - Jika pesan berupa WM_NOTIFY, maka akan memanggil callback yang tertantu dengan control dan kode notify yang sesuao
     * - Jika ukuran window berubah (pesan WM_SIZE), maka akan melakukan tata ulang kontrol pada window
     * - akan men-Clear window yang dihancurkan
     */
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

    /**
     * Membuat kelas dari Window
     * Jika kelas sudah pernah dibuat, maka kelas sebelumnya akan di unregister
     */
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

    /**
     * Membuat kelas dari window dan menampilkanya
     */
    void ShowWindowClass(Window &data)
    {

        ShowWindow(CreateWindowClass(data), _nCmdShow);
    }

    /**
     * Menjalankan event loop
     * Fungsi ini harus dijalankan pada fungsi main/WinMain
     */
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