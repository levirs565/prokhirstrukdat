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
#include "winlamb/textbox.h"
#include "winlamb/label.h"

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

class FindBetweenWindow : public wl::window_control
{

    wl::textbox mFromTextBox;
    wl::textbox mToTextBox;
    wl::button mFind;
    wl::label mFromLabel;
    wl::label mToLabel;
    wl::listview mListView;

public:
    bool isModal = true;

    FindBetweenWindow() : wl::window_control()
    {
        setup.wndClassEx.lpszClassName = L"FindBetweenWindow";
        // setup.exStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_TOPMOST | WS_EX_NOPARENTNOTIFY;
        setup.style = WS_VISIBLE | WS_SYSMENU | WS_CAPTION;
        setup.title = L"Loading";

        on_message(WM_CREATE, [&](wl::params) -> LRESULT
                   {
                    Layouter layouter (hwnd());

                    POINT pos = layouter.pos(5);
                    SIZE size = layouter.component(50,21);

                    mFromLabel.create(this, 0, L"Dari :", pos, size);
                    layouter.nextColumn();

                    pos = layouter.pos(10);
                    size = layouter.edit();

                    mFromTextBox.create(this, 0, wl::textbox::type::NORMAL, pos, size.cx, size.cy);

                    layouter.nextColumn();

                    pos = layouter.pos(25);
                    size = layouter.component(55,21);

                    mToLabel.create(this, 0, L"Hingga :", pos, size);
                    layouter.nextColumn();
                    
                    pos = layouter.pos(10);
                    size = layouter.edit();

                    mToTextBox.create(this,0 , wl::textbox::type::NORMAL, pos, size.cx, size.cy);
                    layouter.nextColumn();

                    pos = layouter.pos(10);
                    size = layouter.edit();

                    mFind.create(this, 4002, L"Cari", pos, size);   

                    layouter.nextRow();
                    pos = layouter.pos();
                    size = layouter.fill();

                    mListView.create(this, 0, pos, size, LVS_REPORT | LVS_ALIGNLEFT | WS_VISIBLE | WS_CHILD, WS_EX_CLIENTEDGE);
                    mListView.columns.add(L"ISBN", 100);
                    mListView.columns.add(L"Title", 200);
            return true; });

        on_command(4002, [&](wl::params p) -> LRESULT
                   {
                    run_thread_detached(std::bind(& FindBetweenWindow::FindBetween,this));
                    return true; });
    }

    void FindBetween(){
        mListView.items.remove_all();
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

        std::string from = converter.to_bytes(mFromTextBox.get_text());
        std::string to = converter.to_bytes(mToTextBox.get_text());

                    size_t current = 0;
                    tree.findBetween(Book{".", from}, Book{"?", to}, [&](auto node)
                                { 
                        current++;
                        mListView.items.add(converter.from_bytes(node->value.isbn))
                            .set_text(converter.from_bytes(node->value.title), 1); });
    }
};

class MainWindow : public wl::window_main
{
    FindBetweenWindow mFindBetweenWindow;
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
                    mFindBetweenWindow.create(this, 0, {0,0}, {400,400}) ;
                    Layouter layouter(hwnd(), 25);

                    POINT pos = layouter.pos();
                    SIZE size = layouter.button();
                    
                    mButton.create(this, 4001, L"Test", pos, size);

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

        on_command(4001, [&](wl::params p) -> LRESULT
                   { return true; });
    }

    void beginLoadData()
    {
        mProgressBar.set_waiting(true);
        mStatusbar.set_text(L"Memuat data", 1);

        run_thread_detached(std::bind(&MainWindow::loadData, this));
    }

    void loadData()
    {
        size_t count = 0;
        mStatusbar.set_text(L"Memuat data", 1);
        Timer timer;
        timer.start();
        {
            CSVReader<CSVReaderIOBuffSync> reader("./data/books.csv", ';');
            reader.startRead();

            int isbnIndex = reader.findHeaderIndex("ISBN");
            int titleIndex = reader.findHeaderIndex("Book-Title");

            while (reader.readData())
            {
                count++;
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

        mProgressBar.set_waiting(false);
        timer.start();

        mListView.items.remove_all();

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

        size_t current = 0;
        tree.inorder(tree.root, [&](auto node)
                     { 
            current++;
            mProgressBar.set_pos((double) current * 100.0 / (double) count);
            mListView.items.add(converter.from_bytes(node->value.isbn))
                .set_text(converter.from_bytes(node->value.title), 1); });

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
