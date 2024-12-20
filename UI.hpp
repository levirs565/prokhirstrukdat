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
#include <cassert>
#include <numeric>
#include <limits>
#include <stdexcept>

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

    const LONG SIZE_FILL = -1;
    const LONG SIZE_DEFAULT = 0;

    struct LayouterCell
    {
        SIZE size;
        SIZE defaultSize;
        POINT pos;
    };

    /**
     * Sebuah layout engine berjenis flow
     * Layout hanya bisa berjalan dari atas ke bawah dan dari kiri ke nana
     */
    struct Layouter
    {
        LONG paddingBottom = 0;
        LONG spacing = 5;
        SIZE size = {};
        std::vector<std::vector<LayouterCell>> _cells;
        std::vector<LONG> _rowsSize;

        void Build()
        {
            LONG availWidth = size.cx;
            LONG availHeight = size.cy - paddingBottom;

            for (std::vector<LayouterCell> &row : _cells)
            {
                for (LayouterCell &cell : row)
                {
                    if (cell.size.cx == SIZE_DEFAULT)
                        cell.size.cx = cell.defaultSize.cx;
                    if (cell.size.cy == SIZE_DEFAULT)
                        cell.size.cy = cell.defaultSize.cy;
                }
            }

            _rowsSize.resize(_cells.size());
            LONG vertFillCount = 0;
            LONG vertFixedWidth = 0;

            for (size_t i = 0; i < _cells.size(); i++)
            {
                std::vector<LayouterCell> &row = _cells[i];
                LONG horzFillCount = 0;
                LONG horzFixedWith = 0;

                LONG maxHeight = 0;

                for (LayouterCell &cell : row)
                {
                    if (cell.size.cx == SIZE_FILL)
                        horzFillCount++;
                    else
                        horzFixedWith += cell.size.cx;

                    if (cell.size.cy == SIZE_FILL || maxHeight == SIZE_FILL)
                        maxHeight = SIZE_FILL;
                    else
                        maxHeight = std::max(maxHeight, cell.size.cy);
                }

                LONG horzFillWidth = availWidth - horzFixedWith - LONG(row.size() + 1) * spacing;
                if (horzFillCount > 0)
                    horzFillWidth /= horzFillCount;
                LONG x = spacing;

                for (LayouterCell &cell : row)
                {
                    cell.pos.x = x;
                    if (cell.size.cx == SIZE_FILL)
                        cell.size.cx = horzFillWidth;

                    x += cell.size.cx + spacing;
                }

                _rowsSize[i] = maxHeight;

                if (maxHeight == SIZE_FILL)
                    vertFillCount++;
                else
                    vertFixedWidth += maxHeight;
            }

            LONG vertFillHeight = availHeight - vertFixedWidth - LONG(_cells.size() + 1) * spacing;
            if (vertFillCount > 0)
                vertFillHeight /= vertFillCount;
            LONG y = spacing;

            for (size_t i = 0; i < _cells.size(); i++)
            {
                std::vector<LayouterCell> &row = _cells[i];
                LONG maxHeight = _rowsSize[i];

                if (maxHeight == SIZE_FILL)
                    maxHeight = vertFillHeight;

                for (LayouterCell &cell : row)
                {
                    if (cell.size.cy == SIZE_FILL)
                        cell.size.cy = maxHeight;

                    LONG offsetY = (maxHeight - cell.size.cy) / 2;
                    cell.pos.y = y + offsetY;
                }

                y += maxHeight + spacing;
            }
        }
    };

    struct Window;
    struct Control;
    struct FixedControl;

    struct ControlCell
    {
        LONG w;
        LONG h;
        Control *control;
        /**
         * Membuat sebuah sel baru
         *
         * @param w, h Ukuran dari kontrol. Akan diberikan ke layouter.
         *             Jika SIZE_FILL maka ukuran kontrol akan memenuhi ruang yang ada
         *             Jika SIZE_DEFAULT maka ukuran kontrol akan diisi nilai default
         */
        ControlCell(LONG w, LONG h, Control *control)
        {
            this->w = w;
            this->h = h;
            this->control = control;
        }
    };

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

    void _ClearWindowControls(Window *window);

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
         * Sebuah array dinamis 2D, yang berisi baris dan setiap baris berisi sel/kolom
         * Kontrol akan ditata berdasarkan isi array ini
         * Jangan lupa memanggil LayoutControls(&window, true) setalah menyetel nilai ini
         */
        std::vector<std::vector<ControlCell>> controlsLayout;
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
            controlsLayout.clear();
            fixedControls.clear();
            _msgListeners.clear();
            _cmdListeners.clear();

            _ClearWindowControls(this);
        }

        void Destroy() 
        {
            DestroyWindow(hwnd);
        }
    };

    HWND CreateWindowClass(Window &data);

    /**
     * Struktur untuk kontrol
     */
    struct Control
    {
        /**
         * HWND kontrol saat ini
         */
        HWND hwnd = 0;
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
            window->_childs.push_back(this);
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
            assert(hwnd != 0);
            DeferWindowPos(hDefer, hwnd, nullptr, pos.x, pos.y, size.cx, size.cy, SWP_NOZORDER);
        }

        /**
         * Mennerapkan font default
         */
        void ApplyDefaultFont()
        {
            assert(hwnd != 0);
            SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<LPARAM>(hDefaultFont), TRUE);
        }

        /**
         * Meengenable dan mendisable window
         */
        void SetEnable(bool enable)
        {
            assert(hwnd != 0);
            EnableWindow(hwnd, enable);
        }

        /**
         * Mengosongkan data di Control, dipanggil otomatis saat window di Clear
         */
        void Clear()
        {
            hwnd = 0;
        }
    };

    void _ClearWindowControls(Window *window)
    {
        for (Control *control : window->_childs)
            control->Clear();

        window->_childs.clear();
    }

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
            _dwStyle |= ES_AUTOHSCROLL;
            _className = WC_EDITW;
            Control::Create(window, hParent, pos, size);
            ApplyDefaultFont();
        }

        /**
         * Mendapatkan teks yang dimasukkan ke kontrol
         */
        std::wstring getText()
        {
            assert(hwnd != 0);
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
        SIZE GetDefaultSize() override
        {
            SIZE size;
            HDC hDC = GetDC(0);
            gdi.SelectObject(hDC, hDefaultFont);
            gdi.GetTextExtentPoint32W(hDC, _setupTitle.c_str(), _setupTitle.length(), &size);
            ReleaseDC(0, hDC);

            return size;
        }

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _className = WC_STATICW;
            Control::Create(window, hParent, pos, size);
            ApplyDefaultFont();
        }

        /**
         * Mengubah text kontrol
         */
        void SetText(const std::wstring &text)
        {
            _setupTitle = text;
            if (hwnd != 0)
                SetWindowTextW(hwnd, text.c_str());
        }
    };

    /**
     * Kontrol berupa tombol
     */
    struct Button : Control
    {
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
            Control::Create(window, hParent, pos, size);
            ApplyDefaultFont();

            if (commandListener)
                window->_cmdListeners.emplace(_controlId, commandListener);
        }

        /**
         * Mengubah text kontrol
         */
        void SetText(const std::wstring &text)
        {
            _setupTitle = text;
            if (hwnd != 0)
                SetWindowTextW(hwnd, text.c_str());
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
            assert(hwnd != 0);
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
            assert(hwnd != 0);
            ListView_DeleteAllItems(hwnd);
        }

        /**
         * Menambahkan baris baru
         * @param text berisi isi dari kolom pertama
         * @result index dari baris yang ditambahkan
         */
        int InsertRow(const std::wstring &text)
        {
            assert(hwnd != 0);
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
            assert(hwnd != 0);
            ListView_SetItemText(hwnd, row, column, const_cast<wchar_t *>(text.c_str()));
        }

        std::vector<int> GetSelectedIndex()
        {
            int iPos = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
            std::vector<int> res;
            while (iPos != -1)
            {
                res.push_back(iPos);
                iPos = ListView_GetNextItem(hwnd, iPos, LVNI_SELECTED);
            }

            return res;
        }

        std::wstring GetText(int row, int column)
        {
            LVITEMW lvi = {};
            lvi.iItem = row;
            lvi.iSubItem = column;

            std::wstring buffer(64, 0);
            int bufferSize = 0;
            int charsWrittenWithoutNull = 0;
            do
            {
                bufferSize += 64;
                buffer.resize(bufferSize);
                lvi.cchTextMax = bufferSize;
                lvi.pszText = &buffer[0];
                charsWrittenWithoutNull = static_cast<int>(
                    SendMessageW(hwnd, LVM_GETITEMTEXTW, row, reinterpret_cast<LPARAM>(&lvi)));
            } while (charsWrittenWithoutNull == bufferSize - 1);

            buffer.resize(lstrlenW(buffer.c_str()));
            return buffer;
        }

        void RemoveRow(int row) {
            ListView_DeleteItem(hwnd, row);
        }
    };

    struct VListView : ListView {
        size_t _columnCount = 0;
        std::function<wchar_t*(int, int)> itemGetter;

        void Create(Window *window, HWND hParent, POINT pos, SIZE size) override
        {
            _dwStyle |= LVS_OWNERDATA;

            ListView::Create(window, hParent, pos, size);

            window->_notifyListeners.emplace(
                std::make_pair(_controlId, LVN_GETDISPINFOW),
                std::bind(&VListView::_OnLVNGetDispInfo, this, std::placeholders::_1)
            );
            _columnCount = 0;
        }

        LRESULT _OnLVNGetDispInfo(UI::CallbackParam param) {
            NMLVDISPINFOW* info = reinterpret_cast<NMLVDISPINFOW*>(param.lParam); 

            // if (info->item.mask & LVIF_STATE) {
                // info->item.state |= LVIS_
            // }
            if (info->item.mask & LVIF_TEXT) {
                if (info->item.iSubItem < _columnCount) {
                    info->item.pszText = itemGetter(info->item.iItem, info->item.iSubItem);
                }
            }

            return 0;
        }

        void InsertColumn(const std::wstring &title, int width) {
            _columnCount++;
            ListView::InsertColumn(title, width);     
        }

        void DeleteAllRows() {
            throw std::invalid_argument("Cannot use DeleteAllRows in VListView");
        }

        void RemoveRow() {
            throw std::invalid_argument("Cannot use RemoveRow in VListView");
        }

        int InsertRow(const std::wstring &text) {
            throw std::invalid_argument("Cannot use InsertRow in VListView");
        }

        void SetRowCount(size_t count) {
            ListView_SetItemCount(hwnd, static_cast<int>(count));
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
            assert(hwnd != 0);
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
            assert(hwnd != 0);
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
            assert(hwnd != 0);
            SendMessageW(hwnd, PBM_SETPOS, pos, 0);
        }

        /**
         * Menentukan apakah progressbar dalam keadaaan menunggu/intederminate
         * Gunakan ini jika progress tidak dapat ditentukan
         */
        void SetWaiting(bool isWaiting)
        {
            assert(hwnd != 0);
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
            assert(hwnd != 0);
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
            assert(hwnd != 0);
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
            assert(hwnd != 0);
            SendMessageW(hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }

        /**
         * @result Mendapatkan indedks item yang terpilih
         */
        int GetSelectedIndex()
        {
            assert(hwnd != 0);
            return SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
        }

        /**
         * Mengganti item yang terpilih
         * @param index indeks item yang akan dipilih
         */
        void SetSelectedIndex(int index)
        {
            assert(hwnd != 0);
            SendMessageW(hwnd, CB_SETCURSEL, index, 0);
        }

        /**
         * Mendapatkan teks item pada indeks tertentu
         */
        std::wstring GetText(int index)
        {
            assert(hwnd != 0);
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
            assert(hwnd != 0);
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
        HDWP hDefer = 0;

        if (!create)
        {
            int count = 0;
            for (const std::vector<ControlCell> &row : window->controlsLayout)
            {
                count += window->controlsLayout.size();
            }

            hDefer = BeginDeferWindowPos(count);
        }

        RECT rect;
        GetClientRect(window->hwnd, &rect);
        window->layouter.size.cx = rect.right - rect.left;
        window->layouter.size.cy = rect.bottom - rect.top;

        window->layouter._cells.resize(window->controlsLayout.size());

        for (size_t i = 0; i < window->controlsLayout.size(); i++)
        {
            const std::vector<ControlCell> &row = window->controlsLayout[i];
            std::vector<LayouterCell> &cells = window->layouter._cells[i];
            cells.resize(row.size());

            for (size_t j = 0; j < row.size(); j++)
            {
                const ControlCell &controlCell = row[j];
                LayouterCell &cell = cells[j];
                cell.defaultSize = controlCell.control->GetDefaultSize();
                cell.size = {controlCell.w, controlCell.h};
            }
        }

        window->layouter.Build();

        for (size_t i = 0; i < window->controlsLayout.size(); i++)
        {
            const std::vector<ControlCell> &row = window->controlsLayout[i];
            std::vector<LayouterCell> &cells = window->layouter._cells[i];
            cells.resize(row.size());

            for (size_t j = 0; j < row.size(); j++)
            {
                const ControlCell &controlCell = row[j];
                LayouterCell &cell = cells[j];
                if (create)
                {
                    controlCell.control->Create(window, 0, cell.pos, cell.size);
                }
                else
                {
                    controlCell.control->UpdatePosDefer(hDefer, cell.pos, cell.size);
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