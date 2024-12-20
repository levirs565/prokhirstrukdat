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
    std::wstring author;
    std::wstring publisher;
    std::wstring year;
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

namespace AddWindow
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label labelk;
    UI::Label isbn;
    UI::Label judul;
    UI::Label penulis;
    UI::Label tahun;
    UI::Label penerbit;
    UI::TextBox isbnTextBox;
    UI::TextBox judulTextBox;
    UI::TextBox penulisTextBox;
    UI::TextBox tahunTextBox;
    UI::TextBox penerbitTextBox;
    UI::Button btnAdd;

    LRESULT OnAddClick(UI::CallbackParam param) {
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Masukkan Data Buku!");
        isbn.SetText(L"ISBN :");
        judul.SetText(L"Judul Buku :");
        penulis.SetText(L"Penulis Buku :");
        tahun.SetText(L"Tahun Terbit :");
        penerbit.SetText(L"Penerbit :");
        labelk.SetText(L" ");

        isbnTextBox._dwStyle |= WS_BORDER;
        judulTextBox._dwStyle |= WS_BORDER;
        penulisTextBox._dwStyle |= WS_BORDER;
        tahunTextBox._dwStyle |= WS_BORDER;
        penerbitTextBox._dwStyle |= WS_BORDER;

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &isbn)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &isbnTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &judul)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &judulTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &penulis)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &penulisTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &tahun)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &tahunTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &penerbit)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &penerbitTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &btnAdd)}};

        UI::LayoutControls(&window, true);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = false;
        window.title = L"Add WIndow";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

LRESULT OnAddClick(UI::CallbackParam param)
{
    AddWindow::Show();
    return 0;
}

void RemoveByListViewSelection(UI::ListView &listView)
{
    size_t shift = 0;
    for (int v : listView.GetSelectedIndex())
    {
        std::wstring isbn = listView.GetText(v, 0);
        Book *buku = hashTable.get(isbn);
        if (!tree.remove(*buku))
            std::cout << "Penghapusan di RBTree gagal" << std::endl;

        if (!hashTable.remove(isbn))
            std::cout << "Penghapusan di RobinHoodHashTable gagal" << std::endl;

        listView.RemoveRow(v - shift);
        shift++;
    }
}

namespace TabFindBooksRange
{
    UI::Window window;
    UI::Label fromLabel, toLabel;
    UI::TextBox fromTextBox, toTextBox;
    UI::Button btnFind;
    UI::Button btnAdd;
    UI::Button btnDelete;
    UI::ProgressBar progress;
    UI::Label label;
    UI::Label labelk;
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

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        RemoveByListViewSelection(listView);
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
        labelk.SetText(L" ");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Buku");
        btnDelete.commandListener = OnDeleteClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &fromLabel),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &fromTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &toLabel),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &toTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnFind)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

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
    UI::Button btnAdd;
    UI::Button btnDelete;
    UI::ProgressBar progress;
    UI::Label label;
    UI::Label labelk;
    UI::VListView listView;
    std::thread showThread;
    std::vector<Book*> booksList;

    wchar_t* OnGetItem(int row, int column) {
        Book* book = booksList[row];
        if (column == 0)
            return const_cast<wchar_t*>(book->isbn.c_str());
        else if (column == 1)
            return const_cast<wchar_t*>(book->title.c_str());
        return nullptr;
    }

    void DoShow()
    {
        std::wstring type = combobox.GetSelectedText();

        booksList.resize(tree.count);
        listView.SetRowCount(0);
        
        label.SetText(L"Memuat data");
        Timer timer;

        progress.SetWaiting(true);

        {
            timer.start();
            size_t current = 0;
            std::function<void(RBNode<Book> *)> visitor = [&](RBNode<Book> *node)
            {
                booksList[current] = &node->value;
                current++;
            };
            if (type == L"In-order")
                tree.inorder(tree.root, visitor);
            else if (type == L"Pre-order")
                tree.preorder(tree.root, visitor);
            else if (type == L"Post-order")
                tree.postorder(tree.root, visitor);
            timer.end();
        }

        progress.SetWaiting(false);
        listView.SetRowCount(booksList.size());

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

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        RemoveByListViewSelection(listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.SetText(L"Tampilkan");
        button.commandListener = OnShowClick;
        label.SetText(L"Data belum dimuat");
        listView._dwStyle |= LVS_REPORT | WS_BORDER;
        listView.itemGetter = OnGetItem;

        labelk.SetText(L" ");

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Buku");

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &button)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        combobox.AddItem(L"Pre-order");
        combobox.AddItem(L"In-order");
        combobox.AddItem(L"Post-order");
        combobox.SetSelectedIndex(1);

        listView.InsertColumn(L"ISBN", 100);
        listView.InsertColumn(L"Judul Buku", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"TabAllBooks";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabDetailsBooks
{
    UI::Window window;
    UI::Label label;
    UI::Label ISBNlabel;
    UI::Button btnSearch;
    UI::TextBox ISBNTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;

    void DoFind()
    {
        label.SetText(L"Mencari buku sesuai dengan ISBN");
        Timer timer;

        {
            std::wstring isbn = ISBNTextBox.getText();
            timer.start();
            Book *buku = hashTable.get(isbn);
            timer.end();
            if (buku == nullptr)
            {
                label.SetText(L"TIdak DiTemukan");
            }
            else
            {
                listView.SetText(0, 1, buku->isbn);
                listView.SetText(1, 1, buku->title);
                listView.SetText(2, 1, buku->author);
                listView.SetText(3, 1, buku->publisher);
                listView.SetText(4, 1, buku->year);
            }
        }
        btnSearch.SetEnable(true);
    }

    LRESULT OnFindClick(UI::CallbackParam)
    {
        btnSearch.SetEnable(false);
        DoFind();
        return 0;
    }

    LRESULT onCreate(UI::CallbackParam param)
    {
        ISBNlabel.SetText(L"Cari Berdasarkan ISBN :");
        ISBNTextBox._dwStyle |= WS_BORDER;

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        label.SetText(L"Data Belum Dimuat");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &ISBNlabel),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &ISBNTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"ISBN               :");
        listView.InsertRow(L"Judul Buku        :");
        listView.InsertRow(L"Penulis Buku      :");
        listView.InsertRow(L"Tahun Terbit     :");
        listView.InsertRow(L"Penerbit Buku  :");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"DetailsBooks";
        window.registerMessageListener(WM_CREATE, onCreate);
    }
}

namespace TabHistoryDelete
{
    UI::Window window;
    UI::Label label;
    UI::ComboBox combobox;
    UI::Button btnRestore;
    UI::Button btnTampil;
    UI::Label labelk;
    UI::ProgressBar progress;
    UI::ListView listView;
    std::thread showThread;

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        btnTampil.SetEnable(false);
        if (showThread.joinable())
        {
            showThread.join();
        }
        // showThread = std::thread(DoShow);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        btnTampil.SetText(L"Tampilkan");
        label.SetText(L"Data Belum Dimuat");
        labelk.SetText(L" ");
        btnRestore.SetText(L"Restore");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnTampil)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnRestore)}};

        UI::LayoutControls(&window, true);

        combobox.AddItem(L"PreOrder");
        combobox.AddItem(L"InOrder");
        combobox.AddItem(L"PostOrder");
        combobox.SetSelectedIndex(1);

        listView.InsertColumn(L"ISBN", 100);
        listView.InsertColumn(L"Judul Buku", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"TabHistoryDelete";
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
            int authorIndex = reader.findHeaderIndex("Book-Author");
            int publisherIndex = reader.findHeaderIndex("Publisher");
            int yearIndex= reader.findHeaderIndex("Year-Of-Publication");

            while (reader.readData())
            {
                Book book{
                    Utils::stringviewToWstring(reader.data[isbnIndex]),
                    Utils::stringviewToWstring(reader.data[titleIndex]),
                    Utils::stringviewToWstring(reader.data[authorIndex]),
                    Utils::stringviewToWstring(reader.data[publisherIndex]),
                    Utils::stringviewToWstring(reader.data[yearIndex])};
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

        size_t maxPsl = 0;
        std::wstring isbn = L"";
        for (size_t i = 0; i < hashTable.bucketSize; i++)
        {
            if (hashTable.buckets[i].filled)
            {
                if (hashTable.buckets[i].psl >= maxPsl)
                {
                    maxPsl = hashTable.buckets[i].psl;
                    isbn = hashTable.buckets[i].key;
                }
            }
        }

        std::cout << "Max PSL: " << maxPsl << std::endl;

        timer.start();
        std::wcout << hashTable.get(isbn)->isbn << std::endl;
        std::wcout << hashTable.get(L"0374157065")->title << std::endl;
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

        TabDetailsBooks::Init();
        tabs.AddPage(L"Details Buku", &TabDetailsBooks::window);

        TabHistoryDelete::Init();
        tabs.AddPage(L"Delete History", &TabHistoryDelete::window);

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