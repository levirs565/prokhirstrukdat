
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
#include "TopKLargest.hpp"
#include "UIUtils.hpp"
#include <stdlib.h>

struct Student
{
    std::wstring nisn;
    std::wstring name;
    std::wstring origin;
    std::wstring entry;
    std::wstring password;
};

struct StudentNameComparer
{
    int compare(const Student &a, const Student &b)
    {
        int compare = Utils::CompareWStringHalfInsensitive(a.name, b.name);

        if (compare != 0)
            return compare;

        return Utils::CompareWStringHalfInsensitive(a.nisn, b.nisn);
    }
};

// struct StudentEntryCompareReversed
// {
//     int compare(Student *a, Student *b)
//     {
//         int compare = a->entry - b->entry;
//         if (compare != 0)
//             return -compare;

//         return -Utils::CompareWStringHalfInsensitive(a->nisn, b->nisn);
//     }
// };

struct StudentNameHasher
{
    uint64_t seed = 0xe17a1465;
    uint64_t hash(const std::wstring &wstr)
    {
        return HalfSipHash_64(wstr.data(), sizeof(wchar_t) * wstr.size(), &seed);
    }
};

RBTree<Student, StudentNameComparer> tree;
RobinHoodHashMap<std::wstring, Student, StudentNameHasher> hashTable;
RBTree<Student, StudentNameComparer> removeHistoryTree;

void ClearAllList();
// Jangan merefresh caller window
void EnqueueRefreshAll(UI::Window *callerWindow);

namespace Register
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label NISN;
    UI::Label name;
    UI::Label origin;
    UI::Label entry;
    UI::Label password;
    UI::TextBox NISNTextBox;
    UI::TextBox nameTextBox;
    UI::TextBox originTextBox;
    UI::TextBox entryTextBox;
    UI::TextBox passwordTextBox;
    UI::Button btnAdd;
    Student student;

    void DoAdd()
    {
        Timer t;

        t.start();
        hashTable.put(student.nisn, student);
        tree.insert(std::move(student));
        t.end();

        std::wstring message = L"Akun PPDB Telah Berhasil Dibuat dalam Waktu " + t.durationStr();
        MessageBoxW(window.hwnd, message.c_str(), L"Success", MB_OK);
        window.CloseModal();
    }

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        student = Student{
            NISNTextBox.getText(),
            nameTextBox.getText(),
            originTextBox.getText(),
            entryTextBox.getText(),
            passwordTextBox.getText()};

        try
        {
            if (student.nisn.size() == 0)
            {
                throw std::domain_error("NISN Tidak Boleh Kosong");
            }

            if (student.nisn.size() != 10)
            {
                throw std::domain_error("NISN Harus 10 Angka");
            }
            int Xcount = 0;
            for (wchar_t ch : student.nisn)
            {
                if (ch == L'X')
                {
                    Xcount++;
                    continue;
                }
                if (!iswdigit(ch))
                {
                    throw std::domain_error("NISN harus berupa angka!");
                }
            }

            if ((Xcount == 1 && student.nisn[student.nisn.size() - 1] != L'X') || (Xcount > 1))
            {
                throw std::domain_error("X hanya Bisa Di akhir NISN");
            }
            if (hashTable.get(student.nisn) != nullptr)
            {
                throw std::domain_error("Siswa dengan NISN sama telah ada");
            }

            if (student.name.size() == 0)
            {
                throw std::domain_error("Nama Siswa Tidak Boleh Kosong");
            }
            if (student.origin.size() == 0)
            {
                throw std::domain_error("Asal Sekolah Siswa Tidak Boleh Kosong");
            }
            if (student.entry.size() == 0)
            {
                throw std::domain_error("Jalur Masuk Siswa Tidak Boleh Kosong");
            }
            if (student.password.size() == 0)
            {
                throw std::domain_error("Password Tidak Boleh Kosong");
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

        label.SetText(L"Masukkan Data Siswa!");
        NISN.SetText(L"NISN :");
        name.SetText(L"Nama Siswa :");
        origin.SetText(L"Asal Sekolah Siswa :");
        entry.SetText(L"Jalur Masuk Siswa :");
        password.SetText(L"Password :");
        btnAdd.SetText(L"Buat Akun");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &NISN)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &NISNTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &name)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nameTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &origin)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &originTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &entry)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &entryTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &password)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &passwordTextBox)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &btnAdd)}};

        UI::LayoutControls(&window, true);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = false;
        window.title = L"Buat Akun PPDB";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

struct StudentListView : UI::VListView
{
    std::vector<Student *> items;

    void Create(UI::Window *window, HWND hParent, POINT pos, SIZE size) override
    {
        _dwStyle |= LVS_REPORT | WS_BORDER | LVS_SHOWSELALWAYS;
        UI::VListView::Create(window, hParent, pos, size);

        SetExtendedStyle(LVS_EX_FULLROWSELECT);
        InsertColumn(L"NISN", 100);
        InsertColumn(L"Nama", 200);
        InsertColumn(L"Asal Sekolah", 100);
        InsertColumn(L"Jalur Masuk", 100);
        InsertColumn(L"Password", 100);
    }

    const std::wstring *OnGetItem(int row, int column) override
    {
        Student *student = items[row];
        if (column == 0)
            return &student->nisn;
        else if (column == 1)
            return &student->name;
        else if (column == 2)
            return &student->origin;
        else if (column == 3)
            return &student->entry;
        else if (column == 4)
            return &student->password;
        return nullptr;
    }
};

namespace Login
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label NISN;
    UI::Label password;
    UI::TextBox NISNTextBox;
    UI::TextBox passwordTextBox;
    UI::Button btnAdd;
    Student student;

    void DoAdd()
    {
        Timer t;

        t.start();
        hashTable.put(student.nisn, student);
        tree.insert(std::move(student));
        t.end();

        std::wstring message = L"Akun PPDB Telah Berhasil Dibuat dalam Waktu " + t.durationStr();
        MessageBoxW(window.hwnd, message.c_str(), L"Success", MB_OK);
        window.CloseModal();
    }

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        student = Student{
            NISNTextBox.getText(),
            passwordTextBox.getText()};

        try
        {
            if (student.nisn.size() == 0)
            {
                throw std::domain_error("NISN Tidak Boleh Kosong");
            }

            if (student.nisn.size() != 10)
            {
                throw std::domain_error("NISN Harus 10 Angka");
            }
            int Xcount = 0;
            for (wchar_t ch : student.nisn)
            {
                if (ch == L'X')
                {
                    Xcount++;
                    continue;
                }
                if (!iswdigit(ch))
                {
                    throw std::domain_error("NISN harus berupa angka!");
                }
            }

            if ((Xcount == 1 && student.nisn[student.nisn.size() - 1] != L'X') || (Xcount > 1))
            {
                throw std::domain_error("X hanya Bisa Di akhir NISN");
            }
            if (hashTable.get(student.nisn) != nullptr)
            {
                throw std::domain_error("Siswa dengan NISN sama telah ada");
            }
            if (student.password.size() == 0)
            {
                throw std::domain_error("Password Tidak Boleh Kosong");
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

        label.SetText(L"Masukkan Data Siswa!");
        NISN.SetText(L"NISN :");
        password.SetText(L"Password :");
        btnAdd.SetText(L"Login");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &NISN)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &NISNTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &password)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &passwordTextBox)},
            {UI::EmptyCell(UI::SIZE_FILL, 23)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &btnAdd)}};

        UI::LayoutControls(&window, true);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = false;
        window.title = L"Login";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

namespace TabHistoryDelete
{
    UI::Window window;
    UI::LabelWorkMessage message;
    UI::ComboBox combobox;
    UI::Button btnRestore;
    UI::Button btnTampil;
    UI::ProgressBar progress;
    StudentListView listView;

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
            std::function<void(RBNode<Student> *)> visitor = [&](RBNode<Student> *node)
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
        message.ReplaceLastMessage(L"Merestore Data Siswa");
        progress.SetWaiting(true);

        std::vector<Student> students;
        for (int v : listView.GetSelectedIndex())
        {
            students.push_back(*listView.items[v]);
        }

        listView.SetRowCount(0);

        Timer t;

        t.start();
        for (Student &student : students)
        {
            removeHistoryTree.remove(student);

            hashTable.put(student.nisn, student);
            tree.insert(std::move(student));
        }
        t.end();

        progress.SetWaiting(false);
        message.ReplaceLastMessage(L"Data Siswa telah direstore dalam waktu " + t.durationStr());
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
    Register::Show();
    return 0;
}

void DoRemove(Student &&Student, HWND window)
{
    if (!tree.remove(Student))
        MessageBoxA(window, "Penghapusan di RBTree gagal", "Gagal", MB_OK);

    if (!hashTable.remove(Student.nisn))
        MessageBoxA(window, "Penghapusan di RobinHoodHashTable gagal", "Gagal", MB_OK);

    removeHistoryTree.insert(std::move(Student));
}

void DoRemoveByListViewSelection(StudentListView *listView, UI::ProgressBar *progress, UI::LabelWorkMessage *message)
{
    progress->SetWaiting(true);
    message->ReplaceLastMessage(L"Menghapus data");

    std::vector<Student> selectedStudent;
    for (int v : listView->GetSelectedIndex())
    {
        selectedStudent.push_back(*listView->items[v]);
    }

    ClearAllList();

    Timer t;

    t.start();
    for (Student &student : selectedStudent)
    {
        DoRemove(std::move(student), listView->_window->hwnd);
    }
    t.end();

    message->ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr() + L". Penghapusan mungkin terlihat lama karena proses mendapatkan pilihan dari UI");
    UIUtils::MessageSetWait(message, false);
    progress->SetWaiting(false);
}

void CopynisnByListViewSelection(UI::ListView *listView)
{
    try
    {
        std::vector<int> focusedIndexs = listView->GetSelectedIndex();

        if (focusedIndexs.size() == 0)
            throw std::domain_error("Tidak ada item yang dipilih");
        if (focusedIndexs.size() > 1)
            throw std::domain_error("Item yang dipilih lebih dari 1");

        std::wstring nisn = listView->GetText(focusedIndexs[0], 0);
        Utils::CopyToClipboard(nisn);
    }
    catch (std::domain_error const &e)
    {
        MessageBoxA(listView->hwnd, e.what(), "Gagal", MB_OK);
    }
}

namespace TabOldStudents
{
    UI::Label label;
    UI::Window window;
    UI::SpinBox spinBox;
    UI::Button findButton;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::CheckBox ignoreInvalidYearCheck;
    UI::Button btnAdd, btnDelete, btnCopyNISN;
    StudentListView listView;

    void SetEnable(boolean enable)
    {
        spinBox.SetEnable(enable);
        findButton.SetEnable(enable);
        ignoreInvalidYearCheck.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyNISN.SetEnable(enable);
        listView.SetEnable(enable);
    }

    void DoRefresh()
    {
        message.ReplaceLastMessage(L"Memproses data");
        listView.SetRowCount(0);
        progress.SetWaiting(true);

        // int count = spinBox.GetValue();
        // bool ignoreInvalid = ignoreInvalidYearCheck.GetCheck() == BST_CHECKED;

        Timer timer;
        timer.start();
        // TopKLargest<Student *, StudentYearCompareReversed> topK(count);
        // tree.preorder(tree.root, [&](RBNode<Student> *node)
        //               {
        //     if (node->value.year == 0 && ignoreInvalid) return;
        //     topK.add(&node->value); });

        // int listSize = topK.getCount();
        // listView.items.resize(listSize);

        // for (int i = listSize - 1; i >= 0; i--)
        // {
        //     listView.items[i] = topK.removeTop();
        // }
        timer.end();

        // listView.SetRowCount(listSize);
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

    LRESULT OnCopynisnClick(UI::CallbackParam param)
    {
        CopynisnByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Jumlah: ");

        findButton.SetText(L"Temukan");
        findButton.commandListener = OnFindClick;

        // ignoreInvalidYearCheck.SetText(L"Abaikan Buku dengan Tahun Tidak Valid");

        btnCopyNISN.SetText(L"Salin NISN");
        btnCopyNISN.commandListener = OnCopynisnClick;

        btnAdd.SetText(L"Tambahkan Data");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");
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
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyNISN),
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
        window.title = L"TabOldStudents";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
};

// namespace TabFindStudentsRange
// {
//     UI::Window window;
//     UI::Label fromLabel, toLabel;
//     UI::TextBox fromTextBox, toTextBox;
//     UI::Button btnFind;
//     UI::Button btnAdd, btnDelete, btnCopynisn;
//     UI::ProgressBar progress;
//     UI::LabelWorkMessage label;
//     StudentListView listView;

//     void SetEnable(boolean enable)
//     {
//         fromTextBox.SetEnable(enable);
//         toTextBox.SetEnable(enable);
//         btnFind.SetEnable(enable);
//         btnAdd.SetEnable(enable);
//         btnDelete.SetEnable(enable);
//         btnCopynisn.SetEnable(enable);
//         listView.SetEnable(enable);
//     }

//     void DoRefresh()
//     {
//         listView.items.clear();
//         listView.SetRowCount(0);

//         label.ReplaceLastMessage(L"Menemukan data");
//         std::wstring from = fromTextBox.getText(), to = toTextBox.getText();
//         Timer timer;

//         progress.SetWaiting(true);

//         {
//             timer.start();
//             tree.findBetween(Student{L".", from}, {L":", to}, [&](RBNode<Student> *node)
//                              { listView.items.push_back(&node->value); });
//             timer.end();
//         }

//         progress.SetWaiting(false);
//         listView.SetRowCount(listView.items.size());

//         label.ReplaceLastMessage(L"Data ditemukan dalam dalam " + timer.durationStr());
//         progress.SetWaiting(false);

//         SetEnable(true);
//     }

//     void EnqueueRefreshList()
//     {
//         UIUtils::MessageSetWait(&label);
//         SetEnable(false);
//         WorkerThread::EnqueueWork(DoRefresh);
//     }

//     LRESULT OnFindClick(UI::CallbackParam param)
//     {
//         EnqueueRefreshList();
//         return 0;
//     }

//     void DoDelete()
//     {
//         DoRemoveByListViewSelection(&listView, &progress, &label);
//     }

//     LRESULT OnDeleteClick(UI::CallbackParam param)
//     {
//         UIUtils::MessageSetWait(&label);
//         SetEnable(false);
//         WorkerThread::EnqueueWork(DoDelete);
//         WorkerThread::EnqueueWork(DoRefresh);
//         EnqueueRefreshAll(&window);
//         return 0;
//     }

//     LRESULT OnCopynisnClick(UI::CallbackParam param)
//     {
//         CopynisnByListViewSelection(&listView);
//         return 0;
//     }

//     LRESULT OnCreate(UI::CallbackParam param)
//     {
//         fromLabel.SetText(L"Dari");
//         toLabel.SetText(L"Ke");

//         btnFind.SetText(L"Cari");
//         btnFind.commandListener = OnFindClick;

//         btnCopynisn.SetText(L"Salin nisn");
//         btnCopynisn.commandListener = OnCopynisnClick;

//         btnAdd.SetText(L"Tambahkan Buku");
//         btnAdd.commandListener = OnAddClick;

//         btnDelete.SetText(L"Hapus Buku");
//         btnDelete.commandListener = OnDeleteClick;

//         window.controlsLayout = {
//             {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &fromLabel),
//              UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &fromTextBox),
//              UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &toLabel),
//              UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &toTextBox),
//              UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnFind)},
//             {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
//             {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
//             {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
//             {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
//              UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopynisn),
//              UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
//              UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};

//         UI::LayoutControls(&window, true);

//         return 0;
//     }

//     void Init()
//     {
//         window.title = L"TabFindStudentsRange";
//         window.registerMessageListener(WM_CREATE, OnCreate);
//     }
// }

namespace TabAllStudents
{
    UI::Window window;
    UI::ComboBox combobox;
    UI::Button button;
    UI::Button btnAdd;
    UI::Button btnDelete;
    UI::Button btnCopyNISN;
    UI::ProgressBar progress;
    UI::LabelWorkMessage label;
    StudentListView listView;

    void SetEnable(boolean enable)
    {
        combobox.SetEnable(enable);
        button.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        btnCopyNISN.SetEnable(enable);
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
            std::function<void(RBNode<Student> *)> visitor = [&](RBNode<Student> *node)
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

    LRESULT OnCopynisnClick(UI::CallbackParam param)
    {
        CopynisnByListViewSelection(&listView);
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        button.SetText(L"Tampilkan");
        button.commandListener = OnShowClick;

        btnAdd.SetText(L"Tambahkan Data");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");
        btnDelete.commandListener = OnDeleteClick;

        btnCopyNISN.SetText(L"Salin NISN");
        btnCopyNISN.commandListener = OnCopynisnClick;

        window.controlsLayout = {
            {UI::ControlCell(90, UI::SIZE_DEFAULT, &combobox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &button)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &btnCopyNISN),
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
        window.title = L"TabAllStudents";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabFindNISN
{
    UI::Window window;
    UI::LabelWorkMessage label;
    UI::Label NISNlabel;
    UI::Button btnSearch;
    UI::TextBox NISNTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Button btnAdd;
    UI::Button btnDelete;
    Student currentStudent;

    void SetEnable(boolean enable)
    {
        btnSearch.SetEnable(enable);
        listView.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
    }

    void DoRefresh()
    {
        label.ReplaceLastMessage(L"Mencari Siswa Berdasarkan NISN");
        Timer timer;

        {
            std::wstring nisn = NISNTextBox.getText();
            timer.start();
            Student *student = hashTable.get(nisn);
            timer.end();
            if (student == nullptr)
            {
                label.ReplaceLastMessage(L"Tidak ditemukan. Membutuhkan waktu " + timer.durationStr());
                currentStudent = {};
            }
            else
            {
                label.ReplaceLastMessage(L"Ditemukan dalam waktu " + timer.durationStr());
                currentStudent = *student;
            }

            listView.SetText(0, 1, currentStudent.nisn);
            listView.SetText(1, 1, currentStudent.name);
            listView.SetText(2, 1, currentStudent.origin);
            listView.SetText(3, 1, currentStudent.entry);
            listView.SetText(4, 1, currentStudent.password);
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
        DoRemove(std::move(currentStudent), window.hwnd);
        t.end();

        label.ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr());
        label.AddMessage(L"Memuat data");
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        if (currentStudent.nisn.empty())
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
        NISNlabel.SetText(L"Cari Berdasarkan NISN :");

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        btnAdd.SetText(L"Tambahkan Data");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");
        btnDelete.commandListener = OnDeleteClick;

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &NISNlabel),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &NISNTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"NISN");
        listView.InsertRow(L"Nama Siswa");
        listView.InsertRow(L"Asal Sekolah Siswa");
        listView.InsertRow(L"Jalur Masuk Siswa");
        listView.InsertRow(L"Password");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"FindNISN";
        window.registerMessageListener(WM_CREATE, onCreate);
    }
}

namespace TabFindEntry
{
    UI::Window window;
    UI::LabelWorkMessage label;
    UI::Label entrylabel;
    UI::Button btnSearch;
    UI::TextBox entryTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Button btnAdd;
    UI::Button btnDelete;
    Student currentStudent;

    void SetEnable(boolean enable)
    {
        btnSearch.SetEnable(enable);
        listView.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
    }

    void DoRefresh()
    {
        label.ReplaceLastMessage(L"Mencari Siswa Berdasarkan Jalur Masuk");
        Timer timer;

        {
            std::wstring entry = entryTextBox.getText();
            timer.start();
            Student *student = hashTable.get(entry);
            timer.end();
            if (student == nullptr)
            {
                label.ReplaceLastMessage(L"Tidak ditemukan. Membutuhkan waktu " + timer.durationStr());
                currentStudent = {};
            }
            else
            {
                label.ReplaceLastMessage(L"Ditemukan dalam waktu " + timer.durationStr());
                currentStudent = *student;
            }

            listView.SetText(0, 1, currentStudent.nisn);
            listView.SetText(1, 1, currentStudent.name);
            listView.SetText(2, 1, currentStudent.origin);
            listView.SetText(3, 1, currentStudent.entry);
            listView.SetText(4, 1, currentStudent.password);
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
        DoRemove(std::move(currentStudent), window.hwnd);
        t.end();

        label.ReplaceLastMessage(L"Penghapusan selesai dalam " + t.durationStr());
        label.AddMessage(L"Memuat data");
    }

    LRESULT OnDeleteClick(UI::CallbackParam param)
    {
        if (currentStudent.entry.empty())
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
        entrylabel.SetText(L"Cari Berdasarkan Jalur Masuk :");

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        btnAdd.SetText(L"Tambahkan Data");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");
        btnDelete.commandListener = OnDeleteClick;

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &entrylabel),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &entryTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"NISN");
        listView.InsertRow(L"Nama Siswa");
        listView.InsertRow(L"Asal Sekolah Siswa");
        listView.InsertRow(L"Jalur Masuk Siswa");
        listView.InsertRow(L"Password");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"FindEntry";
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
            // CSVReader<CSVReaderIOBuffSync> reader("data/students.csv", ';');
            // reader.startRead();

            // int nisnIndex = reader.findHeaderIndex("NISN");
            // int nameIndex = reader.findHeaderIndex("Name");
            // int originIndex = reader.findHeaderIndex("Origin");
            // int entryIndex = reader.findHeaderIndex("Entry");
            // int passwordIndex = reader.findHeaderIndex("Password");

            // while (reader.readData())
            // {
            //     Student Student{
            //         Utils::stringviewToWstring(reader.data[nisnIndex]),
            //         Utils::stringviewToWstring(reader.data[nameIndex]),
            //         Utils::stringviewToWstring(reader.data[originIndex]),
            //         Utils::stringviewToWstring(reader.data[entryIndex]),
            //         Utils::stringviewToWstring(reader.data[passwordIndex])};
            //     hashTable.put(Student.nisn, Student);
            //     tree.insert(std::move(Student));
            // }
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
        Register::window.parentHwnd = window.hwnd;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &tabs)}};
        window.layouter.paddingBottom = 20;
        UI::LayoutControls(&window, true);

        statusBar.Create(&window);
        statusBar.SetParts({118, 300});

        progressBar.Create(&window, statusBar.hwnd, {9, 2}, {100, 19});

        window.fixedControls = {&statusBar};

        TabAllStudents::Init();
        tabs.AddPage(L"Semua Buku", &TabAllStudents::window);

        // TabFindStudentsRange::Init();
        // tabs.AddPage(L"Temukan Buku dalam Rentang", &TabFindStudentsRange::window);

        TabOldStudents::Init();
        tabs.AddPage(L"Buku Tertua", &TabOldStudents::window);

        TabFindNISN::Init();
        tabs.AddPage(L"Details Buku", &TabFindNISN::window);

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
    TabAllStudents::listView.SetRowCount(0);
    // TabFindStudentsRange::listView.SetRowCount(0);
    TabOldStudents::listView.SetRowCount(0);
}

void EnqueueRefreshAll(UI::Window *callerWindow)
{
    if (callerWindow != &TabAllStudents::window)
        TabAllStudents::EnqueueRefreshList();
    // if (callerWindow != &TabFindStudentsRange::window)
    //     TabFindStudentsRange::EnqueueRefreshList();
    if (callerWindow != &TabOldStudents::window)
        TabOldStudents::EnqueueRefreshList();
    if (callerWindow != &TabFindNISN::window)
        TabFindNISN::EnqueueRefresh();
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
