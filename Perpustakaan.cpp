#include "UI.hpp"
#include "CSVReader.hpp"
#include "RBTree.hpp"
#include "Timer.hpp"
#include "Utils.hpp"
#include "RobinHoodHashMap.hpp"
#include "HalfSipHash.h"
#include "MurmurHash3.h"
#include "unordered_map"
#include "WorkerThread.hpp"
#include <stdlib.h>

struct Book
{
    std::wstring isbn;
    std::wstring title;
    std::wstring author;
    std::wstring publisher;
    std::wstring year;
};

struct BookTitleComparer
{
    int compare(const Book &a, const Book &b)
    {
        int compare = Utils::CompareWStringHalfInsensitive(a.title, b.title);

        if (compare != 0)
            return compare;

        return Utils::CompareWStringHalfInsensitive(a.isbn, b.isbn);
    }
};

struct BookTitleHasher
{
    uint64_t seed = 0xe17a1465;

    uint64_t hash(const std::wstring &wstr)
    {
        return HalfSipHash_64(wstr.data(), sizeof(wchar_t) * wstr.size(), &seed);
    }
};

RBTree<Book, BookTitleComparer> tree;
RobinHoodHashMap<std::wstring, Book, BookTitleHasher> hashTable;
RBTree<Book, BookTitleComparer> removeHistoryTree;

void EnqueueRefreshAllList();

namespace AddWindow
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
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
    Book book;

    void DoAdd() {
        Timer t;

        t.start();
        hashTable.put(book.isbn, book);
        tree.insert(std::move(book));
        t.end();

        std::wstring message = L"Buku Telah berhasil Ditambahkan dalam Waktu " + t.durationStr();
        MessageBoxW(window.hwnd, message.c_str(), L"Success", MB_OK);
        window.CloseModal();
    }

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        book = Book{
            isbnTextBox.getText(),
            judulTextBox.getText(),
            penulisTextBox.getText(),
            penerbitTextBox.getText(),
            tahunTextBox.getText()};

        book.year.erase(book.year.find_last_not_of(' ') + 1);
        book.year.erase(0, book.year.find_first_not_of(' '));

        try
        {
            if (book.isbn.size() == 0)
            {
                throw std::domain_error("ISBN Tidak Boleh Kosong");
            }

            if (book.isbn.size() != 10)
            {
                throw std::domain_error("ISBN Harus 10 Angka");
            }
            int Xcount = 0;
            for (wchar_t ch : book.isbn)
            {
                if (ch == L'X')
                {
                    Xcount++;
                    continue;
                }
                if (!iswdigit(ch))
                {
                    throw std::domain_error("ISBN harus berupa angka!");
                }
            }

            if ((Xcount == 1 && book.isbn[book.isbn.size() - 1] != L'X') || (Xcount > 1))
            {
                throw std::domain_error("X hanya Bisa Di akhir ISBN");
            }
            if (hashTable.get(book.isbn) != nullptr)
            {
                throw std::domain_error("Buku dengan ISBN sama telah ada");
            }

            if (book.title.size() == 0)
            {
                throw std::domain_error("Judul Tidak Boleh Kosong");
            }
            if (book.author.size() == 0)
            {
                throw std::domain_error("Penulis Tidak Boleh Kosong");
            }
            if (book.year.size() == 0)
            {
                throw std::domain_error("Tahun Terbit Tidak Boleh Kosong");
            }

            try
            {
                size_t pos;
                int year = std::stoi(book.year, &pos);
                if (pos != book.year.size())
                {
                    throw std::invalid_argument(" ");
                }
                if (year < 1000)
                    throw std::domain_error("Tahun Minimal 1000");
                if (year > 2500)
                    throw std::domain_error("Tahun Maximal 2500");
            }
            catch (std::out_of_range const &)
            {
                throw std::domain_error("Angka Melampaui Batas");
            }
            catch (std::invalid_argument const &)
            {
                throw std::domain_error("Tahun Harus Berupa Angka");
            }

            if (book.publisher.size() == 0)
            {
                throw std::domain_error("Penerbit Tidak Boleh Kosong");
            }
        }

        catch (std::domain_error const &error)
        {
            MessageBoxA(window.hwnd, error.what(), "Error", MB_OK);
            return 0;
        }

        btnAdd.SetEnable(false);
        WorkerThread::EnqueueWork(DoAdd);
        EnqueueRefreshAllList();

        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        window.InitModal();

        label.SetText(L"Masukkan Data Buku!");
        isbn.SetText(L"ISBN :");
        judul.SetText(L"Judul Buku :");
        penulis.SetText(L"Penulis Buku :");
        tahun.SetText(L"Tahun Terbit :");
        penerbit.SetText(L"Penerbit :");

        isbnTextBox._dwStyle |= WS_BORDER;
        judulTextBox._dwStyle |= WS_BORDER;
        penulisTextBox._dwStyle |= WS_BORDER;
        tahunTextBox._dwStyle |= WS_BORDER;
        penerbitTextBox._dwStyle |= WS_BORDER;

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
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
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
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

struct BookListView : UI::VListView
{
    std::vector<Book *> items;

    void Create(UI::Window *window, HWND hParent, POINT pos, SIZE size) override
    {
        _dwStyle |= LVS_REPORT | WS_BORDER | LVS_SHOWSELALWAYS;
        UI::VListView::Create(window, hParent, pos, size);

        SetExtendedStyle(LVS_EX_FULLROWSELECT);
        InsertColumn(L"ISBN", 100);
        InsertColumn(L"Judul", 200);
        InsertColumn(L"Penulis", 100);
        InsertColumn(L"Penerbit", 100);
        InsertColumn(L"Tahun", 100);
    }

    wchar_t *OnGetItem(int row, int column) override
    {
        Book *book = items[row];
        if (column == 0)
            return const_cast<wchar_t *>(book->isbn.c_str());
        else if (column == 1)
            return const_cast<wchar_t *>(book->title.c_str());
        else if (column == 2)
            return const_cast<wchar_t *>(book->author.c_str());
        else if (column == 3)
            return const_cast<wchar_t *>(book->publisher.c_str());
        else if (column == 4)
            return const_cast<wchar_t *>(book->year.c_str());
        return nullptr;
    }
};

namespace TabHistoryDelete
{
    UI::Window window;
    UI::Label label;
    UI::ComboBox combobox;
    UI::Button btnRestore;
    UI::Button btnTampil;
    UI::ProgressBar progress;
    BookListView listView;
    std::wstring restoreTime;

    void DoRefresh()
    {
        std::wstring type = combobox.GetSelectedText();
        listView.items.resize(removeHistoryTree.count);
        listView.SetRowCount(0);
        Timer timer;
        progress.SetWaiting(true);

        {
            timer.start();

            size_t current = 0;
            std::function<void(RBNode<Book> *)> visitor = [&](RBNode<Book> *node)
            {
                listView.items[current] = &node->value;
                current++;
            };

            if (type == L"Pre-order")
                removeHistoryTree.preorder(removeHistoryTree.root, visitor);
            else if (type == L"In-order")
                removeHistoryTree.inorder(removeHistoryTree.root, visitor);
            else if (type == L"Post-order")
                removeHistoryTree.postorder(removeHistoryTree.root, visitor);

            timer.end();
        }

        progress.SetWaiting(false);
        listView.SetRowCount(removeHistoryTree.count);

        std::wstring message = L"Data dimuat dalam " + timer.durationStr(); 
        if (!restoreTime.empty()) {
            message = L"Data di restore dalam " + restoreTime + L". " + message;
            restoreTime = L"";
        }
        label.SetText(message);

        btnTampil.SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        btnTampil.SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoRestore()
    {
        Timer t;
        t.start();
        for (int v : listView.GetSelectedIndex())
        {
            Book book;
            book.isbn = listView.GetText(v, 0);
            book.title = listView.GetText(v, 1);

            RBNode<Book> *node = removeHistoryTree.findNode(book);
            book = node->value;

            hashTable.put(book.isbn, book);
            tree.insert(std::move(book));

            removeHistoryTree.removeNode(node);
        }
        t.end();

        restoreTime = t.durationStr();
        DoRefresh();
    }

    LRESULT OnRestoreClick(UI::CallbackParam param)
    {
        btnTampil.SetEnable(false);

        WorkerThread::EnqueueWork(DoRestore);
        EnqueueRefreshAllList();

        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        btnTampil.SetText(L"Tampilkan");
        btnTampil.commandListener = OnShowClick;

        label.SetText(L"Data Belum Dimuat");

        btnRestore.SetText(L"Restore");
        btnRestore.commandListener = OnRestoreClick;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnTampil)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnRestore)}};

        UI::LayoutControls(&window, true);

        combobox.AddItem(L"Pre-order");
        combobox.AddItem(L"In-order");
        combobox.AddItem(L"Post-order");
        combobox.SetSelectedIndex(1);

        return 0;
    }

    void Init()
    {
        window.title = L"TabHistoryDelete";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

LRESULT OnAddClick(UI::CallbackParam param)
{
    AddWindow::Show();
    return 0;
}

void DoRemoveByListViewSelection(BookListView *listView, UI::ProgressBar *progress, std::wstring *message)
{
    progress->SetWaiting(true);

    std::vector<Book> selectedBook;
    for (int v : listView->GetSelectedIndex()) {
        selectedBook.push_back(*listView->items[v]);
    }

    Timer t;

    t.start();
    for (Book& buku : selectedBook)
    {
        if (!tree.remove(buku))
            MessageBoxA(listView->_window->hwnd, "Penghapusan di RBTree gagal", "Gagal", MB_OK);

        if (!hashTable.remove(buku.isbn))
            MessageBoxA(listView->_window->hwnd, "Penghapusan di RobinHoodHashTable gagal", "Gagal", MB_OK);

        removeHistoryTree.insert(std::move(buku));
    }
    t.end();

    *message = L"Penghapusan selesai dalam " + t.durationStr() + L". Penghapusan mungkin terlihat lama karena proses mendapatkan pilihan dari UI";
    progress->SetWaiting(false);
}

void CopyISBNByListViewSelection(UI::ListView *listView)
{
    try
    {
        std::vector<int> focusedIndexs = listView->GetSelectedIndex();

        if (focusedIndexs.size() == 0)
            throw std::domain_error("Tidak ada item yang dipilih");
        if (focusedIndexs.size() > 1)
            throw std::domain_error("Item yang dipilih lebih dari 1");

        std::wstring isbn = listView->GetText(focusedIndexs[0], 0);
        Utils::CopyToClipboard(isbn);
    }
    catch (std::domain_error const &e)
    {
        MessageBoxA(listView->hwnd, e.what(), "Gagal", MB_OK);
    }
}

namespace TabFindBooksRange
{
    UI::Window window;
    UI::Label fromLabel, toLabel;
    UI::TextBox fromTextBox, toTextBox;
    UI::Button btnFind;
    UI::Button btnAdd, btnDelete, btnCopyISBN;
    UI::ProgressBar progress;
    UI::Label label;
    std::wstring deleteMessage;
    BookListView listView;

    void DoRefresh()
    {
        listView.items.clear();
        listView.SetRowCount(0);

        label.SetText(L"Menemukan data");
        Timer timer;

        progress.SetWaiting(true);

        {
            timer.start();
            tree.findBetween(Book{L".", fromTextBox.getText()}, {L":", toTextBox.getText()}, [&](RBNode<Book> *node)
                             { listView.items.push_back(&node->value); });
            timer.end();
        }

        progress.SetWaiting(false);
        listView.SetRowCount(listView.items.size());

        std::wstring message = L"Data ditemukan dalam dalam " + timer.durationStr();
        if (!deleteMessage.empty()) {
            message = deleteMessage + L". " + message;
            deleteMessage = L"";
        }
        label.SetText(message);
        progress.SetWaiting(false);
        btnFind.SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        btnFind.SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnFindClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoDelete()
    {
        DoRemoveByListViewSelection(&listView, &progress, &deleteMessage);
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        WorkerThread::EnqueueWork(DoDelete);
        EnqueueRefreshAllList();
        TabHistoryDelete::EnqueueRefreshList();
        return 0;
    }

    LRESULT OnCopyISBNClick(UI::CallbackParam param)
    {
        CopyISBNByListViewSelection(&listView);
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

        btnCopyISBN.SetText(L"Salin ISBN");
        btnCopyISBN.commandListener = OnCopyISBNClick;

        label.SetText(L"Data belum dimuat");

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
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyISBN),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

        UI::LayoutControls(&window, true);

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
    UI::Button btnCopyISBN;
    UI::ProgressBar progress;
    UI::Label label;
    std::wstring deleteMessage;
    BookListView listView;

    void DoRefresh()
    {
        std::wstring type = combobox.GetSelectedText();

        listView.items.resize(tree.count);
        listView.SetRowCount(0);

        label.SetText(L"Memuat data");
        Timer timer;

        progress.SetWaiting(true);

        {
            timer.start();
            size_t current = 0;
            std::function<void(RBNode<Book> *)> visitor = [&](RBNode<Book> *node)
            {
                listView.items[current] = &node->value;
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
        listView.SetRowCount(listView.items.size());

        std::wstring message = L"Data dimuat dalam " + timer.durationStr();
        if (!deleteMessage.empty()) {
            message = deleteMessage + L". " + message;
            deleteMessage = L"";
        }

        label.SetText(message);

        button.SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        button.SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoDelete()
    {
        DoRemoveByListViewSelection(&listView, &progress, &deleteMessage);
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        WorkerThread::EnqueueWork(DoDelete);
        EnqueueRefreshAllList();
        TabHistoryDelete::EnqueueRefreshList();
        return 0;
    }

    LRESULT OnCopyISBNClick(UI::CallbackParam param)
    {
        CopyISBNByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.SetText(L"Tampilkan");
        button.commandListener = OnShowClick;
        label.SetText(L"Data belum dimuat");

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Buku");
        btnDelete.commandListener = OnDeleteClick;

        btnCopyISBN.SetText(L"Salin ISBN");
        btnCopyISBN.commandListener = OnCopyISBNClick;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &button)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyISBN),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        combobox.AddItem(L"Pre-order");
        combobox.AddItem(L"In-order");
        combobox.AddItem(L"Post-order");
        combobox.SetSelectedIndex(1);

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
    UI::Button btnAdd;
    UI::Button btnDelete;

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
                label.SetText(L"TIdak Di Temukan");
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

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Buku");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &ISBNlabel),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &ISBNTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"ISBN");
        listView.InsertRow(L"Judul Buku");
        listView.InsertRow(L"Penulis Buku");
        listView.InsertRow(L"Penerbit Buku");
        listView.InsertRow(L"Tahun Terbit");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"DetailsBooks";
        window.registerMessageListener(WM_CREATE, onCreate);
    }
}

namespace MainWindow
{
    UI::Window window;
    UI::Tabs tabs;
    UI::StatusBar statusBar;
    UI::ProgressBar progressBar;

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
            int yearIndex = reader.findHeaderIndex("Year-Of-Publication");

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

        statusBar.SetText(1, L"Data dimuat dari CSV dalam " + timer.durationStr());
    }

    LRESULT OnDestroy(UI::CallbackParam param)
    {
        WorkerThread::Destroy();
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        AddWindow::window.parentHwnd = window.hwnd;

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

        WorkerThread::EnqueueWork(DoLoad);
        TabAllBooks::EnqueueRefreshList();
        TabFindBooksRange::EnqueueRefreshList();

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = true;
        window.title = L"MainWindow";
        window.registerMessageListener(WM_CREATE, OnCreate);
        window.registerMessageListener(WM_DESTROY, OnDestroy);
        UI::ShowWindowClass(window);
    }
}

void EnqueueRefreshAllList()
{
    TabAllBooks::EnqueueRefreshList();
    TabFindBooksRange::EnqueueRefreshList();
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    WorkerThread::Init();

    UI::Setup(hInst, cmdShow);
    MainWindow::Show();
    return UI::RunEventLoop();
}