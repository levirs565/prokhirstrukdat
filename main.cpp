#ifndef UNICODE
#define UNICODE
#endif

#include <string>
#include <algorithm>
#include <sstream>
#include <locale>
#include <codecvt>
#include "CSVReader.hpp"
#include "RBTree.hpp"
#include "Timer.hpp"
#include "winlamb/window_main.h"
#include "winlamb/window_control.h"
#include "winlamb/progressbar.h"
#include "winlamb/button.h"
#include "winlamb/statusbar.h"
#include "winlamb/listview.h"
#include "Layouter.hpp"

int gnCmdShow;

struct Book
{
    std::string isbn;
    std::string title;
};

bool BookTitleCompare(const Book &a, const Book &b)
{
    return tie(a.title, a.isbn) < tie(b.title, b.isbn);
}

RBTree<Book, decltype(&BookTitleCompare)> tree(BookTitleCompare);

class LoadingWindow : public wl::window_control
{
    wl::progressbar mProgressBar;

public:
    bool isModal = true;

    LoadingWindow() : wl::window_control()
    {
        setup.wndClassEx.lpszClassName = L"LoadingDialog";
        setup.exStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_TOPMOST | WS_EX_NOPARENTNOTIFY;
        setup.style = WS_VISIBLE | WS_SYSMENU | WS_CAPTION;
        setup.title = L"Loading";

        on_message(WM_CREATE, [&](wl::params) -> LRESULT
                   {
            mProgressBar.create(this, 4002, L"Loading", {100, 100}, {200, 25});
             
            mProgressBar.set_waiting(true);
            return true; });
    }
};

class MainWindow : public wl::window_main
{
    LoadingWindow mLoadingDialog;
    wl::button mButton;
    wl::statusbar mStatusbar;
    wl::progressbar mProgressBar;
    wl::listview mListView;

public:
    MainWindow()
    {
        setup.wndClassEx.lpszClassName = L"MainWindow";
        setup.title = L"Books";
        setup.size = {400, 400};
        setup.style = WS_OVERLAPPEDWINDOW;

        on_message(WM_CREATE, [&](wl::params) -> LRESULT
                   {
                    Layouter layouter(hwnd(), 25);

                    POINT pos = layouter.pos();
                    SIZE size = layouter.button();
                    
                    mButton.create(this, 0, L"Test", pos, size);

                    layouter.nextRow();

                    pos = layouter.pos();
                    size = layouter.fill();

                    mListView.create(this, 0, pos, size, LVS_REPORT | LVS_ALIGNLEFT | WS_VISIBLE | WS_CHILD, WS_EX_CLIENTEDGE);
                    mListView.columns.add(L"ISBN", 100);
                    mListView.columns.add(L"Title", 200);

                    mStatusbar.create(this);
                    mProgressBar.create(mStatusbar.hwnd(), 0, L"", {9, 2}, {100, 19});
                    mStatusbar.add_fixed_part(118);
                    mStatusbar.add_resizable_part(1);
                    mStatusbar.add_resizable_part(1);

                    beginLoadData();
                    
                    return true; });

        on_message(WM_SIZE, [&](wl::params p) -> LRESULT
                   {
            mStatusbar.adjust(p);
            return true; });
    }

    void beginLoadData()
    {
        mProgressBar.set_waiting(true);
        mStatusbar.set_text(L"Memuat data", 1);

        run_thread_detached(std::bind(&MainWindow::loadData, this));
    }

    void showItems()
    {
        mListView.items.remove_all();

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

        tree.inorder(tree.root, [&](auto node)
                     { mListView.items.add(converter.from_bytes(node->value.isbn))
                           .set_text(converter.from_bytes(node->value.title), 1); });
    }

    void loadData()
    {
        mStatusbar.set_text(L"Memuat data", 1);
        Timer timer;
        timer.start();
        {
            CSVReader<CSVReaderIOWinFMAP> reader("./data/books.csv", ';');
            reader.startRead();

            int isbnIndex = reader.findHeaderIndex("ISBN");
            int titleIndex = reader.findHeaderIndex("Book-Title");

            while (reader.readData())
            {
                tree.insert(std::move(Book{
                    std::move(std::string{reader.data[isbnIndex]}),
                    std::move(std::string{reader.data[titleIndex]})}));
            }
        }

        timer.end();

        std::wstringstream stream;
        stream << L"Data dimuat dalam " << timer.durationMs() << L"ms";
        mStatusbar.set_text(stream.str().c_str(), 1);

        mStatusbar.set_text(L"Menampilkan data", 2);

        timer.start();
        showItems();
        std::cout << "Selesai" << std::endl;
        mProgressBar.set_waiting(false);
        timer.end();

        stream = std::wstringstream();
        stream << L"Data ditampilkan dalam " << timer.durationMs() << L"ms";

        mStatusbar.set_text(stream.str().c_str(), 2);
    }
};

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    gnCmdShow = cmdShow;
    return wl::_wli::run_main<MainWindow>(hInst, cmdShow);
}
