#include "UI.hpp"
#include "CSVReader.hpp"
#include "RBTree.hpp"
#include <thread>
#include "Timer.hpp"
#include <sstream>
#include "Utils.hpp"

struct Book
{
    std::wstring isbn;
    std::wstring title;
};

bool BookTitleCompare(const Book &a, const Book &b)
{
    return tie(a.title, a.isbn) < tie(b.title, b.isbn);
}

RBTree<Book, decltype(&BookTitleCompare)> tree(BookTitleCompare);

namespace TabFindBooksRange
{
    UI::Window window;
    UI::Label fromLabel, toLabel;
    UI::TextBox fromTextBox, toTextBox;
    UI::Button btnFind;
    UI::ProgressBar progress;
    UI::Label label;
    UI::ListView listView;
    std::thread findThread;

    void DoFind()
    {
        listView.DeleteAllRows();
        label.SetText(L"Menemukan data");
        Timer timer;
        progress.SetWaiting(true);

        {
            timer.start();
            tree.findBetween(Book{L".", fromTextBox.getText()}, {L":", toTextBox.getText()}, [&](RBNode<Book>* node)
                             {
            size_t index = listView.InsertRow(node->value.isbn);
            listView.SetText(index, 1, node->value.title); });

            timer.end();
        }

        std::wstringstream stream;
        stream << "Data ditemukan dalam dalam " << timer.durationMs() << "ms";
        label.SetText(stream.str().c_str());
        progress.SetWaiting(false);
        btnFind.SetEnable(true);
    }

    LRESULT OnFindClick(UI::CallbackParam param) {
        btnFind.SetEnable(false);
        if (findThread.joinable()) {
            findThread.join();
        }
        findThread = std::thread(DoFind);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        fromLabel.text = L"Dari";
        toLabel.text = L"Ke";

        fromTextBox._dwStyle |= WS_BORDER;
        toTextBox._dwStyle |= WS_BORDER;

        progress.w = -1;

        btnFind.text = L"Cari";
        btnFind.commandListener = OnFindClick;

        label.text = L"Data belum dimuat";
        label.w = -1;

        listView.w = -1;
        listView.h = -1;
        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controls = {
            {&fromLabel, &fromTextBox, &toLabel, &toTextBox, &btnFind},
            {&progress},
            {&label},
            {&listView}};

        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"ISBN", 100);
        listView.InsertColumn(L"Judul", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"TabFindBooksRange";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabAllBooks
{
    UI::Window window;
    UI::ComboBox combobox;
    UI::Button button;
    UI::ProgressBar progress;
    UI::Label label;
    UI::ListView listView;
    std::thread showThread;

    void DoShow()
    {
        listView.DeleteAllRows();
        label.SetText(L"Memuat data");
        Timer timer;

        {
            timer.start();
            size_t current = 0;
            tree.inorder(tree.root, [&](RBNode<Book>* node)
                         {
            size_t index = listView.InsertRow(node->value.isbn);
            listView.SetText(index, 1, node->value.title); 
                
            current++;
            progress.SetProgress(static_cast<int>(current * 100.0 / tree.count)); });
            timer.end();
        }

        std::wstringstream stream;
        stream << "Data dimuat dalam " << timer.durationMs() << "ms";
        label.SetText(stream.str().c_str());
        button.SetEnable(false);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        button.SetEnable(false);
        if (showThread.joinable()) {
            showThread.join();
        }
        showThread = std::thread(DoShow);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.text = L"Tampilkan";
        button.commandListener = OnShowClick;

        progress.w = -1;

        label.text = L"Data belum dimuat";
        label.w = -1;

        listView.w = -1;
        listView.h = -1;
        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controls = {
            {&combobox, &button},
            {&progress},
            {&label},
            {&listView}};
        UI::LayoutControls(&window, true);

        combobox.AddItem(L"Preorder");
        combobox.AddItem(L"Inorder");
        combobox.AddItem(L"Postorder");
        combobox.SetSelectedIndex(1);

        listView.InsertColumn(L"ISBN", 100);
        listView.InsertColumn(L"Judul", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"TabAllBooks";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace MainWindow
{
    UI::Window window;
    UI::Tabs tabs;
    UI::StatusBar statusBar;
    UI::ProgressBar progressBar;
    std::thread loadThread;

    void DoLoad()
    {

        progressBar.SetWaiting(true);

        Timer timer;
        statusBar.SetText(1, L"Memuat data dari CSV");

        {
            timer.start();
            CSVReader<CSVReaderIOBuffSync> reader("data/books.csv", ';');
            reader.startRead();

            int isbnIndex = reader.findHeaderIndex("ISBN");
            int titleIndex = reader.findHeaderIndex("Book-Title");

            while (reader.readData())
            {
                tree.insert(std::move(Book{
                    std::move(Utils::stringviewToWstring(reader.data[isbnIndex])),
                    std::move(Utils::stringviewToWstring(reader.data[titleIndex]))}));
            }
            timer.end();
        }

        progressBar.SetWaiting(false);

        std::wstringstream stream;
        stream << L"Data dimuat dari CSV dalam " << timer.durationMs() << L"ms";
        statusBar.SetText(1, stream.str());
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        tabs.w = -1;
        tabs.h = -1;

        window.controls = {
            {&tabs}};
        window.layouter.paddingBottom = 20;
        UI::LayoutControls(&window, true);

        statusBar.Create(&window);
        statusBar.SetParts({118, 200, 200, 200});

        progressBar.Create(&window, statusBar.hwnd, {9, 2}, {100, 19});

        window.fixedControls = {&statusBar};

        TabAllBooks::Init();
        tabs.AddPage(L"Semua Buku", &TabAllBooks::window);

        TabFindBooksRange::Init();
        tabs.AddPage(L"Temukan Buku dalam Rentang", &TabFindBooksRange::window);

        loadThread = std::thread(DoLoad);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = true;
        window.title = L"MainWindow";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    UI::Setup(hInst, cmdShow);
    MainWindow::Show();
    return UI::RunEventLoop();
}