#ifndef UNICODE
#define UNICODE
#endif

#include <string>
#include <algorithm>
#include <sstream>
#include "CSVReader.hpp"
#include "RBTree.hpp"
#include "Timer.hpp"
#include "winlamb/window_main.h"
#include "winlamb/window_control.h"
#include "winlamb/progressbar.h"
#include "winlamb/button.h"
#include "winlamb/statusbar.h"

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

public:
    MainWindow()
    {
        setup.wndClassEx.lpszClassName = L"MainWindow";
        setup.title = L"Books";
        setup.size = {400, 400};
        setup.style = WS_OVERLAPPEDWINDOW;

        on_message(WM_CREATE, [&](wl::params) -> LRESULT
                   {
                    mButton.create(this, 0, L"Test", {0, 0});
                    mStatusbar.create(this);
                    mProgressBar.create(mStatusbar.hwnd(), 0, L"", {9, 2}, {100, 19});
                    mStatusbar.add_fixed_part(118);
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

    void loadData()
    {
        Timer timer;
        timer.start();
        CSVReader<CSVReaderIOBuffAsync> reader("./data/books.csv", ';');
        reader.startRead();

        int isbnIndex = reader.findHeaderIndex("ISBN");
        int titleIndex = reader.findHeaderIndex("Book-Title");
        
        while (reader.readData()) {
            // tree.insert(Book{std::string{reader.data[isbnIndex]}, std::string{reader.data[titleIndex]}});
        }

        timer.end();

        // std::cout << tree.root->value.title << std::endl;

        run_thread_ui([&] () {
            mProgressBar.set_waiting(false);
            std::wstringstream stream;
            stream << L"Data dimuat dalam " << timer.durationMs() << L"ms"; 
            mStatusbar.set_text(stream.str().c_str(), 1);
        });
    }
};

// int main()
// {
//     ;

//     std::string last = "";
//     tree.findBetween(Book{"", "A"}, Book{"", "B"}, [&](auto node)
//                      {
//                         assert(last <= node->value.title);
//                         std::cout << node->value.title << std::endl;
//                         last = node->value.title; });

//     std::cout << tree.maxLevel(tree.root, 0) << std::endl;

//     int a;
//     std::cin >> a;
// }

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    gnCmdShow = cmdShow;
    return wl::_wli::run_main<MainWindow>(hInst, cmdShow);
}
