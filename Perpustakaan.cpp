#include "UI.hpp"
#include "CSVReader.hpp"
#include "RBTree.hpp"
#include <thread>
#include "Timer.hpp"
#include <sstream>
#include "Utils.hpp"
#include "RobinHoodHashMap.hpp"
#include "HalfSipHash.h"
#include "MurmurHash3.h"
#include "unordered_map"
#include <stdlib.h>

struct Book
{
    std::wstring isbn;
    std::wstring title;
};

bool BookTitleCompare(const Book &a, const Book &b)
{
    int compare = Utils::CompareWStringHalfInsensitive(a.title, b.title);

    if (compare == 0)
        return a.isbn < b.isbn;

    return compare == -1;
}

uint64_t seed = 0xe17a1465;

std::hash<std::wstring> hasher;

uint64_t fnv1a_hash(const uint8_t *data, size_t data_size)
{
    uint64_t hash = 0xcbf29ce484222325; // FNV offset basis

    for (size_t i = 0; i < data_size; i++)
    {
        hash ^= data[i];
        hash *= 0x100000001b3; // FNV prime
    }
    return hash;
}

uint64_t BookHash(const std::wstring &wstr)
{
    // std::cout << wcstoull(wstr.c_str(), 0, 10) << std::endl;
    return HalfSipHash_64(wstr.data(), sizeof(wchar_t) * wstr.size(), &seed);
    // return wcstoull(wstr.c_str(), 0, 10);
    // return hasher(wstr);
    // return MurmurHash_fmix64(wcstoull(wstr.c_str(), 0, 10));
    // return fnv1a_hash((const uint8_t*) wstr.data(), sizeof(wchar_t) * wstr.size());
}

// struct BookHasher {
//     uint64_t operator()(const std::wstring& wstr) {

//     }
// };

RBTree<Book, decltype(&BookTitleCompare)> tree(BookTitleCompare);
RobinHoodHashMap<std::wstring, Book, decltype(&BookHash)> hashTable(BookHash);
// std::unordered_map<std::wstring, Book> hashTable;

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
            tree.findBetween(Book{L".", fromTextBox.getText()}, {L":", toTextBox.getText()}, [&](RBNode<Book> *node)
                             {
            size_t index = listView.InsertRow(node->value.isbn);
            listView.SetText(index, 1, node->value.title); });

            timer.end();
        }

        std::wstringstream stream;
        stream << "Data ditemukan dalam dalam " << timer.durationStr();
        label.SetText(stream.str().c_str());
        progress.SetWaiting(false);
        btnFind.SetEnable(true);
    }

    LRESULT OnFindClick(UI::CallbackParam param)
    {
        btnFind.SetEnable(false);
        if (findThread.joinable())
        {
            findThread.join();
        }
        findThread = std::thread(DoFind);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        fromLabel.SetText(L"Dari");
        toLabel.SetText(L"Ke");

        fromTextBox._dwStyle |= WS_BORDER;
        toTextBox._dwStyle |= WS_BORDER;

        btnFind.SetText(L"Cari");
        btnFind.commandListener = OnFindClick;

        label.SetText(L"Data belum dimuat");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &fromLabel),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &fromTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &toLabel),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &toTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnFind)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)}};

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
            tree.inorder(tree.root, [&](RBNode<Book> *node)
                         {
            size_t index = listView.InsertRow(node->value.isbn);
            listView.SetText(index, 1, node->value.title); 
            current++;
            progress.SetProgress(static_cast<int>(current * 100.0 / tree.count)); });
            timer.end();
        }

        std::wstringstream stream;
        stream << "Data dimuat dalam " << timer.durationStr();
        label.SetText(stream.str().c_str());
        button.SetEnable(false);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        button.SetEnable(false);
        if (showThread.joinable())
        {
            showThread.join();
        }
        showThread = std::thread(DoShow);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.SetText(L"Tampilkan");
        button.commandListener = OnShowClick;
        label.SetText(L"Data belum dimuat");
        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &button)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)}};
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
                Book book{
                    Utils::stringviewToWstring(reader.data[isbnIndex]),
                    Utils::stringviewToWstring(reader.data[titleIndex])};
                hashTable.put(book.isbn, book);
                // hashTable.insert(std::make_pair(book.isbn, book));
                tree.insert(std::move(book));
            }
            timer.end();
        }
        progressBar.SetWaiting(false);

        std::wstringstream stream;
        stream << L"Data dimuat dari CSV dalam " << timer.durationStr();
        statusBar.SetText(1, stream.str());

        // size_t maxPsl = 0;
        // std::wstring isbn = L"";
        // for (size_t i = 0; i < hashTable.bucketSize; i++)
        // {
        //     if (hashTable.buckets[i].filled)
        //     {
        //         if (hashTable.buckets[i].psl >= maxPsl)
        //         {
        //             maxPsl = hashTable.buckets[i].psl;
        //             isbn = hashTable.buckets[i].key;
        //         }
        //     }
        // }

        // std::cout << "Max PSL: " << maxPsl << std::endl;

        // timer.start();
        // std::wcout << hashTable.get(isbn).value().isbn << std::endl;
        // timer.end();
        // std::wcout << timer.durationMs() << std::endl;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &tabs)}};
        window.layouter.paddingBottom = 20;
        UI::LayoutControls(&window, true);

        statusBar.Create(&window);
        statusBar.SetParts({118, 300, 300, 300});

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