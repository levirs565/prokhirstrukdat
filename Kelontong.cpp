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
#include <stdlib.h>

struct Product
{
    std::wstring sku;
    std::wstring name;
    std::wstring category;
    std::wstring price;
};

struct ProductNameCompare
{
    int compare(const Product &a, const Product &b)
    {
        int compare = Utils::CompareWStringHalfInsensitive(a.name, b.name);

        if (compare != 0)
            return compare;

        return Utils::CompareWStringHalfInsensitive(a.name, b.name);
    }
};

int CompareNumericWString(const std::wstring &a, const std::wstring &b)
{
    long numA = std::stol(a);
    long numB = std::stol(b);
    if (numA < numB)
        return -1;
    if (numA > numB)
        return 1;
    return 0;
}

struct ProductPriceCompareReversed
{
    int compare(Product *a, Product *b)
    {
        int compare = CompareNumericWString(a->price, b->price);
        if (compare != 0)
            return -compare;

        return Utils::CompareWStringHalfInsensitive(a->name, b->name);
    }
};

struct ProductNameHasher
{
    uint64_t seed = 0xe17a1465;

    uint64_t hash(const std::wstring &wstr)
    {
        return HalfSipHash_64(wstr.data(), sizeof(wchar_t) * wstr.size(), &seed);
    }
};

RBTree<Product, ProductNameCompare> tree;
RobinHoodHashMap<std::wstring, Product, ProductNameHasher> hashTable;
RBTree<Product, ProductNameCompare> removeHistoryTree;

void ClearAllList();
// Jangan merefresh caller window
void EnqueueRefreshAll(UI::Window *callerWindow);

namespace AddWindow
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label sku;
    UI::Label nama;
    UI::Label kategori;
    UI::Label harga;
    UI::TextBox skuTextBox;
    UI::TextBox namaTextBox;
    UI::TextBox kategoriTextBox;
    UI::TextBox hargaTextBox;
    UI::Button btnAdd;
    Product product;

    void DoAdd()
    {
        Timer t;

        t.start();
        hashTable.put(product.sku, product);
        tree.insert(std::move(product));
        t.end();

        std::wstring message = L"Product Telah berhasil Ditambahkan dalam Waktu " + t.durationStr();
        MessageBoxW(window.hwnd, message.c_str(), L"Success", MB_OK);
        window.CloseModal();
    }

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        product = Product{
            skuTextBox.getText(),
            namaTextBox.getText(),
            kategoriTextBox.getText(),
            hargaTextBox.getText()};

        product.price.erase(product.price.find_last_not_of(' ') + 1);
        product.price.erase(0, product.price.find_first_not_of(' '));

        try
        {
            if (product.sku.size() == 0)
            {
                throw std::domain_error("SKU Tidak Boleh Kosong");
            }

            /*if (product.sku.size() != 9)
            {
                throw std::domain_error("SKU Harus 9 Angka");
            }*/
            int Xcount = 0;
            for (wchar_t ch : product.sku)
            {
                if (ch == L'X')
                {
                    Xcount++;
                    continue;
                }
                if (!iswdigit(ch))
                {
                    throw std::domain_error("SKU harus berupa angka!");
                }
            }

            if ((Xcount == 1 && product.sku[product.sku.size() - 1] != L'X') || (Xcount > 1))
            {
                throw std::domain_error("X hanya Bisa Di akhir SKU");
            }
            if (hashTable.get(product.sku) != nullptr)
            {
                throw std::domain_error("Product dengan SKU sama telah ada");
            }

            if (product.name.size() == 0)
            {
                throw std::domain_error("Nama Tidak Boleh Kosong");
            }
            if (product.category.size() == 0)
            {
                throw std::domain_error("Kategori Tidak Boleh Kosong");
            }
            if (product.price.size() == 0)
            {
                throw std::domain_error("Harga Terbit Tidak Boleh Kosong");
            }

            try
            {
                size_t pos;
                int price = std::stoi(product.price, &pos);
                if (pos != product.price.size())
                {
                    throw std::invalid_argument(" ");
                }
                if (price < 0)
                    throw std::domain_error("Harga Minimal 1000");
                if (price > 999999)
                    throw std::domain_error("Harga Maximal 999999");
            }
            catch (std::out_of_range const &)
            {
                throw std::domain_error("Angka Melampaui Batas");
            }
            catch (std::invalid_argument const &)
            {
                throw std::domain_error("Harga Harus Berupa Angka");
            }

            if (product.category.size() == 0)
            {
                throw std::domain_error("Kategori Tidak Boleh Kosong");
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

        label.SetText(L"Masukkan Data Produk!");
        sku.SetText(L"SKU :");
        nama.SetText(L"Nama Produk :");
        kategori.SetText(L"Kategori Produk :");
        harga.SetText(L"Harga Produk :");

        btnAdd.SetText(L"Tambahkan Produk");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &sku)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &skuTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nama)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &namaTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &kategori)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &kategoriTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &harga)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &hargaTextBox)},
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

struct ProductListView : UI::VListView
{
    std::vector<Product *> items;

    void Create(UI::Window *window, HWND hParent, POINT pos, SIZE size) override
    {
        _dwStyle |= LVS_REPORT | WS_BORDER | LVS_SHOWSELALWAYS;
        UI::VListView::Create(window, hParent, pos, size);

        SetExtendedStyle(LVS_EX_FULLROWSELECT);
        InsertColumn(L"SKU", 100);
        InsertColumn(L"Nama", 200);
        InsertColumn(L"Kategori", 100);
        InsertColumn(L"Harga", 100);
    }

    const std::wstring OnGetItem(int row, int column) override
    {
        Product *product = items[row];
        if (column == 0)
            return product->sku;
        else if (column == 1)
            return product->name;
        else if (column == 2)
            return product->category;
        else if (column == 3)
            return product->price;
        return L"";
    }
};

void MessageSetWait(UI::LabelWorkMessage *message, bool clear = true)
{
    if (clear)
        message->Clear();
    message->AddMessage(L"Menunggu antrian tugas");
}

namespace TabHistoryDelete
{
    UI::Window window;
    UI::LabelWorkMessage message;
    UI::ComboBox combobox;
    UI::Button btnRestore;
    UI::Button btnTampil;
    UI::ProgressBar progress;
    ProductListView listView;

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
            std::function<void(RBNode<Product> *)> visitor = [&](RBNode<Product> *node)
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
        MessageSetWait(&message);
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
        message.ReplaceLastMessage(L"Merestore Produk");
        progress.SetWaiting(true);

        std::vector<Product> products;
        for (int v : listView.GetSelectedIndex())
        {
            products.push_back(*listView.items[v]);
        }

        listView.SetRowCount(0);

        Timer t;

        t.start();
        for (Product &product : products)
        {
            removeHistoryTree.remove(product);

            hashTable.put(product.sku, product);
            tree.insert(std::move(product));
        }
        t.end();

        progress.SetWaiting(false);
        message.ReplaceLastMessage(L"Produk telah direstore dalam waktu " + t.durationStr());
        MessageSetWait(&message, false);
    }

    LRESULT OnRestoreClick(UI::CallbackParam param)
    {
        MessageSetWait(&message);
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

void DoRemove(Product &&product, HWND window)
{
    if (!tree.remove(product))
        MessageBoxA(window, "Penghapusan di RBTree gagal", "Gagal", MB_OK);

    if (!hashTable.remove(product.sku))
        MessageBoxA(window, "Penghapusan di RobinHoodHashTable gagal", "Gagal", MB_OK);

    removeHistoryTree.insert(std::move(product));
}

void DoRemoveByListViewSelection(ProductListView *listView, UI::ProgressBar *progress, UI::LabelWorkMessage *message)
{
    progress->SetWaiting(true);
    message->ReplaceLastMessage(L"Menghapus data");

    std::vector<Product> selectedProduct;
    for (int v : listView->GetSelectedIndex())
    {
        selectedProduct.push_back(*listView->items[v]);
    }

    ClearAllList();

    Timer t;

    t.start();
    for (Product &produk : selectedProduct)
    {
        DoRemove(std::move(produk), listView->_window->hwnd);
    }
    t.end();

    message->ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr() + L". Penghapusan mungkin terlihat lama karena proses mendapatkan pilihan dari UI");
    MessageSetWait(message, false);
    progress->SetWaiting(false);
}

void CopySKUByListViewSelection(ProductListView *listView)
{
    try
    {
        std::vector<int> focusedIndexs = listView->GetSelectedIndex();

        if (focusedIndexs.size() == 0)
            throw std::domain_error("Tidak ada item yang dipilih");
        if (focusedIndexs.size() > 1)
            throw std::domain_error("Item yang dipilih lebih dari 1");

        std::wstring sku = listView->items[focusedIndexs[0]]->sku;
        Utils::CopyToClipboard(sku);
    }
    catch (std::domain_error const &e)
    {
        MessageBoxA(listView->hwnd, e.what(), "Gagal", MB_OK);
    }
}

namespace TabCheapProducts
{
    UI::Label label;
    UI::Window window;
    UI::SpinBox spinBox;
    UI::Button findButton;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::Button btnAdd, btnDelete, btnCopySKU;
    ProductListView listView;

    void SetEnable(boolean enable)
    {
        spinBox.SetEnable(enable);
        findButton.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopySKU.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        message.ReplaceLastMessage(L"Memproses data");
        listView.SetRowCount(0);
        progress.SetWaiting(true);

        int count = spinBox.GetValue();

        Timer timer;
        timer.start();
        TopKLargest<Product *, ProductPriceCompareReversed> topK(count);
        tree.preorder(tree.root, [&](RBNode<Product> *node)
                      { topK.add(&node->value); });

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
        MessageSetWait(&message);
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
        MessageSetWait(&message);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
        return 0;
    }

    LRESULT OnCopyNameClick(UI::CallbackParam param)
    {
        CopySKUByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Jumlah: ");

        findButton.SetText(L"Temukan");
        findButton.commandListener = OnFindClick;

        btnCopySKU.SetText(L"Salin SKU Produk");
        btnCopySKU.commandListener = OnCopyNameClick;

        btnAdd.SetText(L"Tambahkan Produk");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Produk");
        btnDelete.commandListener = OnDeleteClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &label),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &spinBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &findButton)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnCopySKU),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

        UI::LayoutControls(&window, true);

        spinBox.SetRange(0, 1000);
        spinBox.SetValue(100);

        return 0;
    };

    void Init()
    {
        window.title = L"TabCheapProducts";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
};

namespace TabFindProductsRange
{
    UI::Window window;
    UI::Label fromLabel, toLabel;
    UI::TextBox fromTextBox, toTextBox;
    UI::Button btnFind;
    UI::Button btnAdd, btnDelete, btnCopySKU;
    UI::ProgressBar progress;
    UI::LabelWorkMessage label;
    ProductListView listView;

    void SetEnable(boolean enable)
    {
        fromTextBox.SetEnable(enable);
        toTextBox.SetEnable(enable);
        btnFind.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopySKU.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        listView.items.clear();
        listView.SetRowCount(0);

        label.ReplaceLastMessage(L"Menemukan data");
        Timer timer;

        progress.SetWaiting(true);

        {
            timer.start();
            tree.findBetween(Product{L".", fromTextBox.getText()}, {L":", toTextBox.getText()}, [&](RBNode<Product> *node)
                             { listView.items.push_back(&node->value); });
            timer.end();
        }

        progress.SetWaiting(false);
        listView.SetRowCount(listView.items.size());

        label.ReplaceLastMessage(L"Data ditemukan dalam " + timer.durationStr());
        progress.SetWaiting(false);

        SetEnable(true);
    }

    void EnqueueRefreshList()
    {
        MessageSetWait(&label);
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
        MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
        return 0;
    }

    LRESULT OnCopyNameClick(UI::CallbackParam param)
    {
        CopySKUByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        fromLabel.SetText(L"Dari");
        toLabel.SetText(L"Ke");

        btnFind.SetText(L"Cari");
        btnFind.commandListener = OnFindClick;

        btnCopySKU.SetText(L"Salin SKU Produk");
        btnCopySKU.commandListener = OnCopyNameClick;

        btnAdd.SetText(L"Tambahkan Produk");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Produk");
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
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnCopySKU),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

        UI::LayoutControls(&window, true);

        return 0;
    }

    void Init()
    {
        window.title = L"TabFindProductsRange";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabAllProducts
{
    UI::Window window;
    UI::ComboBox combobox;
    UI::Button button;
    UI::Button btnAdd;
    UI::Button btnDelete;
    UI::Button btnCopySKU;
    UI::ProgressBar progress;
    UI::LabelWorkMessage label;
    ProductListView listView;

    void SetEnable(boolean enable)
    {
        combobox.SetEnable(enable);
        button.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopySKU.SetEnable(enable);
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
            std::function<void(RBNode<Product> *)> visitor = [&](RBNode<Product> *node)
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
        MessageSetWait(&label);
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
        MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);
        return 0;
    }

    LRESULT OnCopyNameClick(UI::CallbackParam param)
    {
        CopySKUByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.SetText(L"Tampilkan");
        button.commandListener = OnShowClick;

        btnAdd.SetText(L"Tambahkan Produk");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Produk");
        btnDelete.commandListener = OnDeleteClick;

        btnCopySKU.SetText(L"Salin SKU Produk");
        btnCopySKU.commandListener = OnCopyNameClick;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &button)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnCopySKU),
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
        window.title = L"TabAllProducts";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabDetailsProducts
{
    UI::Window window;
    UI::LabelWorkMessage label;
    UI::Label namelabel;
    UI::Button btnSearch;
    UI::TextBox skuTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Button btnAdd;
    UI::Button btnDelete;
    Product currentProduct;

    void SetEnable(boolean enable)
    {
        btnSearch.SetEnable(enable);
        listView.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
    }

    void DoRefresh()
    {
        label.ReplaceLastMessage(L"Mencari produk sesuai dengan SKU");
        Timer timer;

        {
            std::wstring sku = skuTextBox.getText();
            timer.start();
            Product *produk = hashTable.get(sku);
            timer.end();
            if (produk == nullptr)
            {
                label.ReplaceLastMessage(L"Tidak ditemukan. Membutuhkan waktu " + timer.durationStr());
                currentProduct = {};
            }
            else
            {
                label.ReplaceLastMessage(L"Ditemukan dalam waktu " + timer.durationStr());
                currentProduct = *produk;
            }

            listView.SetText(0, 1, currentProduct.sku);
            listView.SetText(1, 1, currentProduct.name);
            listView.SetText(2, 1, currentProduct.category);
            listView.SetText(3, 1, currentProduct.price);
        }
        SetEnable(true);
    }

    void EnqueueRefresh()
    {
        MessageSetWait(&label);
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
        DoRemove(std::move(currentProduct), window.hwnd);
        t.end();

        label.ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr());
        label.AddMessage(L"Memuat data");
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        if (currentProduct.name.empty())
            return 0;

        MessageSetWait(&label);
        SetEnable(false);
        WorkerThread::EnqueueWork(DoDelete);
        WorkerThread::EnqueueWork(DoRefresh);
        EnqueueRefreshAll(&window);

        return 0;
    }

    LRESULT onCreate(UI::CallbackParam param)
    {
        namelabel.SetText(L"Cari Berdasarkan SKU :");

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        btnAdd.SetText(L"Tambahkan Produk");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Produk");
        btnDelete.commandListener = OnDeleteClick;

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &namelabel),
             UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &skuTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"SKU");
        listView.InsertRow(L"Nama Produk");
        listView.InsertRow(L"Kategori Produk");
        listView.InsertRow(L"Harga Produk");
        listView.InsertColumn(L"Value", 200);
        return 0;
    }

    void Init()
    {
        window.title = L"DetailsProducts";
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
            CSVReader<CSVReaderIOBuffSync> reader("data/products.csv", ';');
            reader.startRead();

            int skuIndex = reader.findHeaderIndex("sku");
            int nameIndex = reader.findHeaderIndex("name");
            int categoryIndex = reader.findHeaderIndex("category");
            int priceIndex = reader.findHeaderIndex("price");

            while (reader.readData())
            {
                Product product{
                    Utils::stringviewToWstring(reader.data[skuIndex]),
                    Utils::stringviewToWstring(reader.data[nameIndex]),
                    Utils::stringviewToWstring(reader.data[categoryIndex]),
                    Utils::stringviewToWstring(reader.data[priceIndex])};
                hashTable.put(product.sku, product);
                tree.insert(std::move(product));
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
        statusBar.SetParts({118, 300, 300, 300});

        progressBar.Create(&window, statusBar.hwnd, {9, 2}, {100, 19});

        window.fixedControls = {&statusBar};

        TabAllProducts::Init();
        tabs.AddPage(L"Semua Produk", &TabAllProducts::window);

        TabFindProductsRange::Init();
        tabs.AddPage(L"Temukan Produk dalam Rentang", &TabFindProductsRange::window);

        TabCheapProducts::Init();
        tabs.AddPage(L"Produk Termurah", &TabCheapProducts::window);

        TabDetailsProducts::Init();
        tabs.AddPage(L"Details Produk", &TabDetailsProducts::window);

        TabHistoryDelete::Init();
        tabs.AddPage(L"Delete History", &TabHistoryDelete::window);

        WorkerThread::EnqueueWork(DoLoad);
        EnqueueRefreshAll(&window);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = true;
        window.title = L"Toko Serba Ada";
        window.registerMessageListener(WM_CREATE, OnCreate);
        window.registerMessageListener(WM_CLOSE, OnClose);
        window.registerMessageListener(WM_DESTROY, OnDestroy);
        UI::ShowWindowClass(window);
    }
}

void ClearAllList()
{
    TabAllProducts::listView.SetRowCount(0);
    TabFindProductsRange::listView.SetRowCount(0);
    TabCheapProducts::listView.SetRowCount(0);
}

void EnqueueRefreshAll(UI::Window *callerWindow)
{
    if (callerWindow != &TabAllProducts::window)
        TabAllProducts::EnqueueRefreshList();
    if (callerWindow != &TabFindProductsRange::window)
        TabFindProductsRange::EnqueueRefreshList();
    if (callerWindow != &TabCheapProducts::window)
        TabCheapProducts::EnqueueRefreshList();
    if (callerWindow != &TabDetailsProducts::window)
        TabDetailsProducts::EnqueueRefresh();
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