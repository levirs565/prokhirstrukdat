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

struct Event
{
    std::wstring id;
    std::wstring name;
    std::wstring location;
    std::wstring country;
    std::wstring genre;
    int visitor;
    SYSTEMTIME date;
    std::wstring description;
};

struct EventNameComparer
{
    int compare(const Event &a, const Event &b)
    {
        int compare = Utils::CompareWStringHalfInsensitive(a.name, b.name);

        if (compare != 0)
            return compare;

        return Utils::CompareWStringHalfInsensitive(a.id, b.id);
    }
};

struct EventDateCompareReversed
{
    int compare(Event *a, Event *b)
    {
        LONGLONG compare = Utils::SystemTimeTo100Nanos(a->date) - Utils::SystemTimeTo100Nanos(b->date);
        if (compare > 0)
            return 1;
        if (compare < 0)
            return -1;

        return Utils::CompareWStringHalfInsensitive(a->id, b->id);
    }
};

struct EventNameHasher
{
    uint64_t seed = 0xe17a1465;

    uint64_t hash(const std::wstring &wstr)
    {
        return HalfSipHash_64(wstr.data(), sizeof(wchar_t) * wstr.size(), &seed);
    }
};

RBTree<Event, EventNameComparer> tree;
RobinHoodHashMap<std::wstring, Event, EventNameHasher> hashTable;
RBTree<Event, EventNameComparer> removeHistoryTree;

void ClearAllList();
// Jangan merefresh caller window
void EnqueueRefreshAll(UI::Window *callerWindow);

namespace AddWindow
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label id;
    UI::Label nama;
    UI::Label lokasi;
    UI::Label negara;
    UI::Label aliran;
    UI::Label pengunjung;
    UI::Label tanggal;
    UI::Label deskripsi;
    UI::TextBox idTextBox;
    UI::TextBox namaTextBox;
    UI::TextBox lokasiTextBox;
    UI::TextBox negaraTextBox;
    UI::TextBox aliranTextBox;
    UI::SpinBox kapasitasSpinBox;
    UI::TextBox deskripsiTextBox;
    UI::DateTimePicker dtpTanggal;
    UI::Button btnAdd;
    Event event;

    void DoAdd()
    {
        Timer t;

        t.start();
        hashTable.put(event.id, event);
        tree.insert(std::move(event));
        t.end();

        std::wstring message = L"Event Telah berhasil Ditambahkan dalam Waktu " + t.durationStr();
        MessageBoxW(window.hwnd, message.c_str(), L"Success", MB_OK);
        window.CloseModal();
    }

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        std::pair<int, bool> kapasitas = kapasitasSpinBox.GetValuePair();
        event = Event{
            idTextBox.getText(),
            namaTextBox.getText(),
            lokasiTextBox.getText(),
            negaraTextBox.getText(),
            aliranTextBox.getText(),
            kapasitas.first,
            dtpTanggal.GetValue(),
            deskripsiTextBox.getText()};

        try
        {
            if (event.id.size() == 0)
            {
                throw std::domain_error("ID Event Tidak Boleh Kosong");
            }

            if (event.id.size() != 5)
            {
                throw std::domain_error("ID Event Harus 5 Angka");
            }
            
            if (!isalpha(event.id[0]) !=0)
            {
                throw std::domain_error("ID Event Diawali Alphabet");
            }

            if (hashTable.get(event.id) != nullptr)
            {
                throw std::domain_error("Event dengan ID sama telah ada");
            }

            if (event.name.size() == 0)
            {
                throw std::domain_error("Nama Event Tidak Boleh Kosong");
            }
            if (event.location.size() == 0)
            {
                throw std::domain_error("Lokasi Event Tidak Boleh Kosong");
            }
            if (event.country.size() == 0)
            {
                throw std::domain_error("Negara Event Tidak Boleh Kosong");
            }
            if (event.genre.size() == 0)
            {
                throw std::domain_error("Genre Musik Event Tidak Boleh Kosong");
            }

            if (kapasitas.second) 
            {
                throw std::domain_error("Kapasitas harus angka di dari 1000 sampai 500000");
            }
                                 
             if (event.description.size() == 0)
            {
                throw std::domain_error("Deskripsi Event Tidak Boleh Kosong");
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

        label.SetText(L"Masukkan Data Event!");
        id.SetText(L"ID Event :");
        nama.SetText(L"Nama Event :");
        lokasi.SetText(L"Lokasi Event :");
        negara.SetText(L"Negara Event Berlangsung :");
        aliran.SetText(L"Genre Musik Event :");
        kapasitasSpinBox._upDown._dwStyle |= UDS_NOTHOUSANDS;
        tanggal.SetText(L"Tanggal Event Berlangsung :");
        deskripsi.SetText(L"Deskripsi Mengenai Event :");

        btnAdd.SetText(L"Tambahkan Event");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &id)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &idTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nama)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &namaTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &lokasi)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &lokasiTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &negara)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &negaraTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &aliran)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &aliranTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &pengunjung)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &kapasitasSpinBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &tanggal)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &dtpTanggal)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &deskripsi)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &deskripsiTextBox)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &btnAdd)}};

        UI::LayoutControls(&window, true);

        kapasitasSpinBox.SetRange(1000, 500000);

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

struct EventListView : UI::VListView
{
    std::vector<Event *> items;

    void Create(UI::Window *window, HWND hParent, POINT pos, SIZE size) override
    {
        _dwStyle |= LVS_REPORT | WS_BORDER | LVS_SHOWSELALWAYS;
        UI::VListView::Create(window, hParent, pos, size);

        SetExtendedStyle(LVS_EX_FULLROWSELECT);
        InsertColumn(L"ID Event", 100);
        InsertColumn(L"Nama Event", 200);
        InsertColumn(L"Lokasi Event", 100);
        InsertColumn(L"Negara Event", 100);
        InsertColumn(L"Genre Musik", 100);
        InsertColumn(L"Kapasitas Pengunjung", 100);
        InsertColumn(L"Tanggal Event", 100);
        InsertColumn(L"Deskripsi Event", 100);
    }

    const std::wstring OnGetItem(int row, int column) override
    {
        Event *event = items[row];
        if (column == 0)
            return event->id;
        else if (column == 1)
            return event->name;
        else if (column == 2)
            return event->location;
        else if (column == 3)
            return event->country;
        else if (column == 4)
            return event->genre;
        else if (column == 5)
            return std::to_wstring(event->visitor);
        else if (column == 6)
            return Utils::SystemTimeToDateStr(event->date);
        else if (column == 7)
            return event->description;
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
    EventListView listView;

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
            std::function<void(RBNode<Event> *)> visitor = [&](RBNode<Event> *node)
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
        message.ReplaceLastMessage(L"Merestore Event");
        progress.SetWaiting(true);

        std::vector<Event> events;
        for (int v : listView.GetSelectedIndex())
        {
            events.push_back(*listView.items[v]);
        }

        listView.SetRowCount(0);

        Timer t;

        t.start();
        for (Event &event : events)
        {
            removeHistoryTree.remove(event);

            hashTable.put(event.id, event);
            tree.insert(std::move(event));
        }
        t.end();

        progress.SetWaiting(false);
        message.ReplaceLastMessage(L"Event telah direstore dalam waktu " + t.durationStr());
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

void DoRemove(Event &&event, HWND window)
{
    if (!tree.remove(event))
        MessageBoxA(window, "Penghapusan di RBTree gagal", "Gagal", MB_OK);

    if (!hashTable.remove(event.id))
        MessageBoxA(window, "Penghapusan di RobinHoodHashTable gagal", "Gagal", MB_OK);

    removeHistoryTree.insert(std::move(event));
}

void DoRemoveByListViewSelection(EventListView *listView, UI::ProgressBar *progress, UI::LabelWorkMessage *message)
{
    progress->SetWaiting(true);
    message->ReplaceLastMessage(L"Menghapus data");

    std::vector<Event> selectedEvent;
    for (int v : listView->GetSelectedIndex())
    {
        selectedEvent.push_back(*listView->items[v]);
    }

    ClearAllList();

    Timer t;

    t.start();
    for (Event &event : selectedEvent)
    {
        DoRemove(std::move(event), listView->_window->hwnd);
    }
    t.end();

    message->ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr() + L". Penghapusan mungkin terlihat lama karena proses mendapatkan pilihan dari UI");
    MessageSetWait(message, false);
    progress->SetWaiting(false);
}

void CopyIdByListViewSelection(UI::ListView *listView)
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

namespace TabOldEvent
{
    UI::Label label;
    UI::Window window;
    UI::SpinBox spinBox;
    UI::Button findButton;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::Button btnAdd, btnDelete, btnCopyId;
    EventListView listView;

    void SetEnable(boolean enable)
    {
        spinBox.SetEnable(enable);
        findButton.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyId.SetEnable(enable);
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
        TopKLargest<Event *, EventDateCompareReversed> topK(count);
        tree.preorder(tree.root, [&](RBNode<Event> *node)
                      {
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

    LRESULT OnCopyIdClick(UI::CallbackParam param)
    {
        CopyIdByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Jumlah: ");

        findButton.SetText(L"Temukan");
        findButton.commandListener = OnFindClick;

        btnCopyId.SetText(L"Salin Id Event");
        btnCopyId.commandListener = OnCopyIdClick;

        btnAdd.SetText(L"Tambahkan Event");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Event");
        btnDelete.commandListener = OnDeleteClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &label),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &spinBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &findButton)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyId),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

        UI::LayoutControls(&window, true);

        spinBox.SetRange(0, 5000000);
        spinBox.SetValue(100);

        return 0;
    };

    void Init()
    {
        window.title = L"TabOldEvent";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
};

namespace TabFindEventsRange
{
    UI::Window window;
    UI::Label fromLabel, toLabel;
    UI::TextBox fromTextBox, toTextBox;
    UI::Button btnFind;
    UI::Button btnAdd, btnDelete, btnCopyId;
    UI::ProgressBar progress;
    UI::LabelWorkMessage label;
    EventListView listView;

    void SetEnable(boolean enable)
    {
        fromTextBox.SetEnable(enable);
        toTextBox.SetEnable(enable);
        btnFind.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyId.SetEnable(enable);
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
            tree.findBetween(Event{L".", fromTextBox.getText()}, {L":", toTextBox.getText()}, [&](RBNode<Event> *node)
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

    LRESULT OnCopyIdClick(UI::CallbackParam param)
    {
        CopyIdByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        fromLabel.SetText(L"Dari");
        toLabel.SetText(L"Ke");

        btnFind.SetText(L"Cari");
        btnFind.commandListener = OnFindClick;

        btnCopyId.SetText(L"Salin Id Event");
        btnCopyId.commandListener = OnCopyIdClick;

        btnAdd.SetText(L"Tambahkan Event");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Event");
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
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyId),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

        UI::LayoutControls(&window, true);

        return 0;
    }

    void Init()
    {
        window.title = L"TabFindEventsRange";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabAllEvents
{
    UI::Window window;
    UI::ComboBox combobox;
    UI::Button button;
    UI::Button btnAdd;
    UI::Button btnDelete;
    UI::Button btnCopyId;
    UI::ProgressBar progress;
    UI::LabelWorkMessage label;
    EventListView listView;

    void SetEnable(boolean enable)
    {
        combobox.SetEnable(enable);
        button.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyId.SetEnable(enable);
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
            std::function<void(RBNode<Event> *)> visitor = [&](RBNode<Event> *node)
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

    LRESULT OnCopyIdClick(UI::CallbackParam param)
    {
        CopyIdByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.SetText(L"Tampilkan");
        button.commandListener = OnShowClick;

        btnAdd.SetText(L"Tambahkan Event");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Event");
        btnDelete.commandListener = OnDeleteClick;

        btnCopyId.SetText(L"Salin Id Event");
        btnCopyId.commandListener = OnCopyIdClick;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &button)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyId),
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
        window.title = L"TabAllEvent";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabDetailsEvents
{
    UI::Window window;
    UI::LabelWorkMessage label;
    UI::Label Idlabel;
    UI::Button btnSearch;
    UI::TextBox IdTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Button btnAdd;
    UI::Button btnDelete;
    Event currentEvent;

    void SetEnable(boolean enable)
    {
        btnSearch.SetEnable(enable);
        listView.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
    }

    void DoRefresh()
    {
        label.ReplaceLastMessage(L"Mencari Event sesuai dengan ID Event");
        Timer timer;

        {
            std::wstring id = IdTextBox.getText();
            timer.start();
            Event *event = hashTable.get(id);
            timer.end();
            if (event == nullptr)
            {
                label.ReplaceLastMessage(L"Tidak ditemukan. Membutuhkan waktu " + timer.durationStr());
                currentEvent = {};
            }
            else
            {
                label.ReplaceLastMessage(L"Ditemukan dalam waktu " + timer.durationStr());
                currentEvent = *event;
            }

            listView.SetText(0, 1, currentEvent.id);
            listView.SetText(1, 1, currentEvent.name);
            listView.SetText(2, 1, currentEvent.location);
            listView.SetText(3, 1, currentEvent.country);
            listView.SetText(4, 1, currentEvent.genre);
            listView.SetText(5, 1, std::to_wstring(currentEvent.visitor));
            listView.SetText(6, 1, Utils::SystemTimeToDateStr(currentEvent.date));
            listView.SetText(7, 1, currentEvent.description);
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
        DoRemove(std::move(currentEvent), window.hwnd);
        t.end();

        label.ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr());
        label.AddMessage(L"Memuat data");
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        if (currentEvent.id.empty())
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
        Idlabel.SetText(L"Cari Berdasarkan Id Event :");

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        btnAdd.SetText(L"Tambahkan Event");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Event");
        btnDelete.commandListener = OnDeleteClick;

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &Idlabel),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &IdTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"ID Event");
        listView.InsertRow(L"Nama Event");
        listView.InsertRow(L"Lokasi Event");
        listView.InsertRow(L"Negara Event");
        listView.InsertRow(L"Genre Musik");
        listView.InsertRow(L"Kapasitas Pengunjung");
        listView.InsertRow(L"Tanggal Event");
        listView.InsertRow(L"Deskripsi Event");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"DetailsEvents";
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
            CSVReader<CSVReaderIOBuffSync> reader("data/FestDataFiksBgt.csv", ';');
            reader.startRead();

            int idIndex = reader.findHeaderIndex("Festival_ID");
            int nameIndex = reader.findHeaderIndex("Festival_Name");
            int locationIndex = reader.findHeaderIndex("Location");
            int countryIndex = reader.findHeaderIndex("Country");
            int genreIndex = reader.findHeaderIndex("Genre");
            int visitorIndex = reader.findHeaderIndex("Visitor");
            int dateIndex = reader.findHeaderIndex("Date");
            int descriptionIndex = reader.findHeaderIndex("Description");

            while (reader.readData())
            {
                Event event{
                    Utils::stringviewToWstring(reader.data[idIndex]),
                    Utils::stringviewToWstring(reader.data[nameIndex]),
                    Utils::stringviewToWstring(reader.data[locationIndex]),
                    Utils::stringviewToWstring(reader.data[countryIndex]),
                    Utils::stringviewToWstring(reader.data[genreIndex]),
                    std::stoi(Utils::stringviewToWstring(reader.data[visitorIndex])),
                    Utils::DateStrToSystemTime(Utils::stringviewToWstring(reader.data[dateIndex])),
                    Utils::stringviewToWstring(reader.data[descriptionIndex])};
                hashTable.put(event.id, event);
                tree.insert(std::move(event));
            }
            timer.end();
        }
        progressBar.SetWaiting(false);

        statusBar.SetText(1, L"Data dimuat dari CSV dalam " + timer.durationStr());
    }

    LRESULT OnClose(UI::CallbackParam param) {
        if (!WorkerThread::IsWorking()) {
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

        TabAllEvents::Init();
        tabs.AddPage(L"Semua Event", &TabAllEvents::window);

        TabFindEventsRange::Init();
        tabs.AddPage(L"Temukan Nama Event dalam Rentang", &TabFindEventsRange::window);

        TabOldEvent::Init();
        tabs.AddPage(L"Event Awal", &TabOldEvent::window);

        TabDetailsEvents::Init();
        tabs.AddPage(L"Details Event", &TabDetailsEvents::window);

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
    TabAllEvents::listView.SetRowCount(0);
    TabFindEventsRange::listView.SetRowCount(0);
    TabOldEvent::listView.SetRowCount(0);
}

void EnqueueRefreshAll(UI::Window *callerWindow)
{
    if (callerWindow != &TabAllEvents::window)
        TabAllEvents::EnqueueRefreshList();
    if (callerWindow != &TabFindEventsRange::window)
        TabFindEventsRange::EnqueueRefreshList();
    if (callerWindow != &TabOldEvent::window)
        TabOldEvent::EnqueueRefreshList();
    if (callerWindow != &TabDetailsEvents::window)
        TabDetailsEvents::EnqueueRefresh();
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