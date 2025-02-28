#include "UI.hpp"
#include "CSVReader.hpp"
#include "RBTree.hpp"
#include "Timer.hpp"
#include "Utils.hpp"
#include "RobinHoodHashMap.hpp"
#include "HalfSipHash.h"
#include "unordered_map"
#include "WorkerThread.hpp"
#include "TopKLargest.hpp"
#include "UIUtils.hpp"
#include <stdlib.h>

struct Book
{
    std::wstring isbn;
    std::wstring title;
    std::wstring author;
    std::wstring publisher;
    int year;
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

struct BookYearCompareReversed
{
    int compare(Book *a, Book *b)
    {
        int compare = a->year - b->year;
        if (compare != 0)
            return -compare;

        return -Utils::CompareWStringHalfInsensitive(a->isbn, b->isbn);
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

void ClearAllList();
// Jangan merefresh caller window
void EnqueueRefreshAll(UI::Window *callerWindow);

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
    UI::SpinBox tahunSpinBox;
    UI::TextBox penerbitTextBox;
    UI::Button btnAdd;
    Book book;

    void DoAdd()
    {
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
        std::pair<int, bool> tahun = tahunSpinBox.GetValuePair();
        book = Book{
            isbnTextBox.getText(),
            judulTextBox.getText(),
            penulisTextBox.getText(),
            penerbitTextBox.getText(),
            tahun.first};

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
            if (tahun.second)
            {
                throw std::domain_error("Tahun tidak valid. Tahun harus berupa angka dari 1000 sampai 2500");
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
        EnqueueRefreshAll(&window);

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
        tahunSpinBox._upDown._dwStyle |= UDS_NOTHOUSANDS;
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
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &tahunSpinBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &penerbit)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &penerbitTextBox)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &btnAdd)}};

        UI::LayoutControls(&window, true);

        tahunSpinBox.SetRange(1000, 2500);

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

    const std::wstring OnGetItem(int row, int column) override
    {
        Book *book = items[row];
        if (column == 0)
            return book->isbn;
        else if (column == 1)
            return book->title;
        else if (column == 2)
            return book->author;
        else if (column == 3)
            return book->publisher;
        else if (column == 4)
            return std::to_wstring(book->year);
        return L"";
    }
};

namespace TabHistoryDelete
{
    UI::Window window;
    UI::LabelWorkMessage message;
    UI::ComboBox combobox;
    UI::Button btnRestore;
    UI::Button btnTampil;
    UI::ProgressBar progress;
    BookListView listView;

    void SetEnable(bool enable)
    {
        combobox.SetEnable(enable);
        btnRestore.SetEnable(enable);
        btnTampil.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        progress.SetWaiting(true);
        message.ReplaceLastMessage(L"Memuat data");

        std::wstring type = combobox.GetSelectedText();
        listView.items.resize(removeHistoryTree.count);
        listView.SetRowCount(0);

        Timer timer;

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

        message.ReplaceLastMessage(L"Data dimuat dalam " + timer.durationStr());
        SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        UIUtils::MessageSetWait(&message);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoRestore()
    {
        message.ReplaceLastMessage(L"Merestore buku");
        progress.SetWaiting(true);

        std::vector<Book> books;
        for (int v : listView.GetSelectedIndex())
        {
            books.push_back(*listView.items[v]);
        }

        listView.SetRowCount(0);

        Timer t;

        t.start();
        for (Book &book : books)
        {
            removeHistoryTree.remove(book);

            hashTable.put(book.isbn, book);
            tree.insert(std::move(book));
        }
        t.end();

        progress.SetWaiting(false);
        message.ReplaceLastMessage(L"Buku telah direstore dalam waktu " + t.durationStr());
        UIUtils::MessageSetWait(&message, false);
    }

    LRESULT OnRestoreClick(UI::CallbackParam param)
    {
        UIUtils::MessageSetWait(&message);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoRestore);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        btnTampil.SetText(L"Tampilkan");
        btnTampil.commandListener = OnShowClick;

        btnRestore.SetText(L"Restore");
        btnRestore.commandListener = OnRestoreClick;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnTampil)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
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

void DoRemove(Book &&book, HWND window)
{
    if (!tree.remove(book))
        MessageBoxA(window, "Penghapusan di RBTree gagal", "Gagal", MB_OK);

    if (!hashTable.remove(book.isbn))
        MessageBoxA(window, "Penghapusan di RobinHoodHashTable gagal", "Gagal", MB_OK);

    removeHistoryTree.insert(std::move(book));
}

void DoRemoveByListViewSelection(BookListView *listView, UI::ProgressBar *progress, UI::LabelWorkMessage *message)
{
    progress->SetWaiting(true);
    message->ReplaceLastMessage(L"Menghapus data");

    std::vector<Book> selectedBook;
    for (int v : listView->GetSelectedIndex())
    {
        selectedBook.push_back(*listView->items[v]);
    }

    ClearAllList();

    Timer t;

    t.start();
    for (Book &buku : selectedBook)
    {
        DoRemove(std::move(buku), listView->_window->hwnd);
    }
    t.end();

    message->ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr() + L". Penghapusan mungkin terlihat lama karena proses mendapatkan pilihan dari UI");
    UIUtils::MessageSetWait(message, false);
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

namespace TabOldBooks
{
    UI::Label label;
    UI::Window window;
    UI::SpinBox spinBox;
    UI::Button findButton;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::CheckBox ignoreInvalidYearCheck;
    UI::Button btnAdd, btnDelete, btnCopyISBN;
    BookListView listView;

    void SetEnable(boolean enable)
    {
        spinBox.SetEnable(enable);
        findButton.SetEnable(enable);
        ignoreInvalidYearCheck.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyISBN.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        message.ReplaceLastMessage(L"Memproses data");
        listView.SetRowCount(0);
        progress.SetWaiting(true);

        int count = spinBox.GetValue();
        bool ignoreInvalid = ignoreInvalidYearCheck.GetCheck() == BST_CHECKED;

        Timer timer;
        timer.start();
        TopKLargest<Book *, BookYearCompareReversed> topK(count);
        tree.preorder(tree.root, [&](RBNode<Book> *node)
                      {
            if (node->value.year == 0 && ignoreInvalid) return;
            topK.add(&node->value); });

        int listSize = topK.getCount();
        listView.items.resize(listSize);

        for (int i = listSize - 1; i >= 0; i--)
        {
            listView.items[i] = topK.removeTop();
        }
        timer.end();

        listView.SetRowCount(listSize);
        progress.SetWaiting(false);
        message.ReplaceLastMessage(L"Data ditemukan dalam " + timer.durationStr());
        SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        UIUtils::MessageSetWait(&message);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnFindClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoDelete()
    {
        DoRemoveByListViewSelection(&listView, &progress, &message);
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        UIUtils::MessageSetWait(&message);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
        return 0;
    }

    LRESULT OnCopyISBNClick(UI::CallbackParam param)
    {
        CopyISBNByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Jumlah: ");

        findButton.SetText(L"Temukan");
        findButton.commandListener = OnFindClick;

        ignoreInvalidYearCheck.SetText(L"Abaikan Buku dengan Tahun Tidak Valid");

        btnCopyISBN.SetText(L"Salin ISBN");
        btnCopyISBN.commandListener = OnCopyISBNClick;

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Buku");
        btnDelete.commandListener = OnDeleteClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &label),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &spinBox),
             UI::ControlCell(250, UI::SIZE_DEFAULT, &ignoreInvalidYearCheck),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &findButton)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyISBN),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

        UI::LayoutControls(&window, true);

        ignoreInvalidYearCheck.SetCheck(BST_CHECKED);
        spinBox.SetRange(0, 1000);
        spinBox.SetValue(100);

        return 0;
    };

    void Init()
    {
        window.title = L"TabOldBooks";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
};

namespace TabFindBooksRange
{
    UI::Window window;
    UI::Label fromLabel, toLabel;
    UI::TextBox fromTextBox, toTextBox;
    UI::Button btnFind;
    UI::Button btnAdd, btnDelete, btnCopyISBN;
    UI::ProgressBar progress;
    UI::LabelWorkMessage label;
    BookListView listView;

    void SetEnable(boolean enable)
    {
        fromTextBox.SetEnable(enable);
        toTextBox.SetEnable(enable);
        btnFind.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyISBN.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        listView.items.clear();
        listView.SetRowCount(0);

        label.ReplaceLastMessage(L"Menemukan data");
        std::wstring from = fromTextBox.getText(), to = toTextBox.getText();
        Timer timer;

        progress.SetWaiting(true);

        {
            timer.start();
            tree.findBetween(Book{L".", from}, {L":", to}, [&](RBNode<Book> *node)
                             { listView.items.push_back(&node->value); });
            timer.end();
        }

        progress.SetWaiting(false);
        listView.SetRowCount(listView.items.size());

        label.ReplaceLastMessage(L"Data ditemukan dalam dalam " + timer.durationStr());
        progress.SetWaiting(false);

        SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        UIUtils::MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnFindClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoDelete()
    {
        DoRemoveByListViewSelection(&listView, &progress, &label);
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        UIUtils::MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
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

        btnFind.SetText(L"Cari");
        btnFind.commandListener = OnFindClick;

        btnCopyISBN.SetText(L"Salin ISBN");
        btnCopyISBN.commandListener = OnCopyISBNClick;

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
    UI::LabelWorkMessage label;
    BookListView listView;

    void SetEnable(boolean enable)
    {
        combobox.SetEnable(enable);
        button.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyISBN.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        std::wstring type = combobox.GetSelectedText();

        listView.items.resize(tree.count);
        listView.SetRowCount(0);

        Timer timer;
        label.ReplaceLastMessage(L"Memuat data");

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

        label.ReplaceLastMessage(L"Data dimuat dalam " + timer.durationStr());

        SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        UIUtils::MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        EnqueueRefreshList();
        return 0;
    }

    void DoDelete()
    {
        DoRemoveByListViewSelection(&listView, &progress, &label);
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        UIUtils::MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
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
    UI::LabelWorkMessage label;
    UI::Label ISBNlabel;
    UI::Button btnSearch;
    UI::TextBox ISBNTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Button btnAdd;
    UI::Button btnDelete;
    Book currentBook;

    void SetEnable(boolean enable)
    {
        btnSearch.SetEnable(enable);
        listView.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
    }

    void DoRefresh()
    {
        label.ReplaceLastMessage(L"Mencari buku sesuai dengan ISBN");
        Timer timer;

        {
            std::wstring isbn = ISBNTextBox.getText();
            timer.start();
            Book *buku = hashTable.get(isbn);
            timer.end();
            if (buku == nullptr)
            {
                label.ReplaceLastMessage(L"Tidak ditemukan. Membutuhkan waktu " + timer.durationStr());
                currentBook = {};
            }
            else
            {
                label.ReplaceLastMessage(L"Ditemukan dalam waktu " + timer.durationStr());
                currentBook = *buku;
            }

            listView.SetText(0, 1, currentBook.isbn);
            listView.SetText(1, 1, currentBook.title);
            listView.SetText(2, 1, currentBook.author);
            listView.SetText(3, 1, currentBook.publisher);
            listView.SetText(4, 1, std::to_wstring(currentBook.year));
        }
        SetEnable(true);
    }

    void EnqueueRefresh()
    {
        UIUtils::MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoRefresh);
    }

    LRESULT OnFindClick(UI::CallbackParam)
    {
        EnqueueRefresh();
        return 0;
    }

    void DoDelete()
    {
        ClearAllList();
        label.ReplaceLastMessage(L"Menghapus data");
        Timer t;

        t.start();
        DoRemove(std::move(currentBook), window.hwnd);
        t.end();

        label.ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr());
        label.AddMessage(L"Memuat data");
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        if (currentBook.isbn.empty())
            return 0;

        UIUtils::MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);

        return 0;
    }

    LRESULT onCreate(UI::CallbackParam param)
    {
        ISBNlabel.SetText(L"Cari Berdasarkan ISBN :");

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        btnAdd.SetText(L"Tambahkan Buku");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Buku");
        btnDelete.commandListener = OnDeleteClick;

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
                    std::stoi(Utils::stringviewToWstring(reader.data[yearIndex]))};
                hashTable.put(book.isbn, book);
                tree.insert(std::move(book));
            }
            timer.end();
        }
        progressBar.SetWaiting(false);

        statusBar.SetText(1, L"Data dimuat dari CSV dalam " + timer.durationStr());
    }

    LRESULT OnClose(UI::CallbackParam param)
    {
        if (!WorkerThread::IsWorking())
        {
            window.Destroy();
            return 0;
        }

        MessageBoxW(window.hwnd, L"Menunggu tugas selesai", L"Tunggu Dulu", MB_OK);

        return 0;
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
        statusBar.SetParts({118, 300});

        progressBar.Create(&window, statusBar.hwnd, {9, 2}, {100, 19});

        window.fixedControls = {&statusBar};

        TabAllBooks::Init();
        tabs.AddPage(L"Semua Buku", &TabAllBooks::window);

        TabFindBooksRange::Init();
        tabs.AddPage(L"Temukan Buku dalam Rentang", &TabFindBooksRange::window);

        TabOldBooks::Init();
        tabs.AddPage(L"Buku Tertua", &TabOldBooks::window);

        TabDetailsBooks::Init();
        tabs.AddPage(L"Details Buku", &TabDetailsBooks::window);

        TabHistoryDelete::Init();
        tabs.AddPage(L"Delete History", &TabHistoryDelete::window);

        WorkerThread::EnqueueWork(DoLoad);
        EnqueueRefreshAll(&window);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = true;
        window.title = L"MainWindow";
        window.registerMessageListener(WM_CREATE, OnCreate);
        window.registerMessageListener(WM_CLOSE, OnClose);
        window.registerMessageListener(WM_DESTROY, OnDestroy);
        UI::ShowWindowClass(window);
    }
}

void ClearAllList()
{
    TabAllBooks::listView.SetRowCount(0);
    TabFindBooksRange::listView.SetRowCount(0);
    TabOldBooks::listView.SetRowCount(0);
}

void EnqueueRefreshAll(UI::Window *callerWindow)
{
    if (callerWindow != &TabAllBooks::window)
        TabAllBooks::EnqueueRefreshList();
    if (callerWindow != &TabFindBooksRange::window)
        TabFindBooksRange::EnqueueRefreshList();
    if (callerWindow != &TabOldBooks::window)
        TabOldBooks::EnqueueRefreshList();
    if (callerWindow != &TabDetailsBooks::window)
        TabDetailsBooks::EnqueueRefresh();
    if (callerWindow != &TabHistoryDelete::window)
        TabHistoryDelete::EnqueueRefreshList();
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    WorkerThread::Init();

    UI::Setup(hInst, cmdShow);
    MainWindow::Show();
    return UI::RunEventLoop();
}