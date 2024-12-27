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

struct Student
{
    std::wstring nisn;
    std::wstring name;
    std::wstring origin;
    std::wstring entry;
    std::wstring password;
};

bool StudentNISNCompare(const Student &a, const Student &b)
{
    int compare = Utils::CompareWStringHalfInsensitive(a.nisn, b.nisn);

    if (compare == 0)
        return a.nisn < b.nisn;

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

uint64_t StudentHash(const std::wstring &wstr)
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

RBTree<Student, decltype(&StudentNISNCompare)> tree(StudentNISNCompare);
RobinHoodHashMap<std::wstring, Student, decltype(&StudentHash)> hashTable(StudentHash);
// std::unordered_map<std::wstring, Book> hashTable;

void RefreshAllList();

namespace Register
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label labelk;
    UI::Label nisn;
    UI::Label name;
    UI::Label origin;
    UI::Label entry;
    UI::Label password;
    UI::TextBox nisnTextBox;
    UI::TextBox nameTextBox;
    UI::TextBox originTextBox;
    UI::TextBox entryTextBox;
    UI::TextBox passwordTextBox;
    UI::Button btnAdd;

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        Student student{
            nisnTextBox.getText(),
            nameTextBox.getText(),
            originTextBox.getText(),
            entryTextBox.getText(),
            passwordTextBox.getText(),};

        // book.year.erase(book.year.find_last_not_of(' ') + 1);
        // book.year.erase(0, book.year.find_first_not_of(' '));

        try
        {
        if (student.nisn.size() == 0)
            {
                throw std::domain_error("NISN Tidak Boleh Kosong");
            }


            if (student.nisn.size() != 9)
            {
                throw std::domain_error("NISN Harus 9 Angka");
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
            
            // if ((Xcount == 1 && student.nisn[student.nisn.size() - 1] != L'X') || (Xcount > 1))
            // {
            //     throw std::domain_error("X hanya Bisa Di akhir NISN");
            // }
            if (hashTable.get(student.nisn) != nullptr)
            {
                throw std::domain_error("Siswa dengan NISN sama telah ada");
            }

            if (student.name.size() == 0)
            {
                throw std::domain_error("Nama Tidak Boleh Kosong");
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

        hashTable.put(student.nisn, student);
        tree.insert(std::move(student));

        MessageBoxW(window.hwnd, L"Akun PPDB Telah berhasil Dibuat", L"Success", MB_OK);
        window.Destroy();

        RefreshAllList();

        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Masukkan Data Siswa!");
        nisn.SetText(L"NISN :");
        name.SetText(L"Nama Siswa :");
        origin.SetText(L"Asal Sekolah :");
        entry.SetText(L"Jalur Masuk :");
        password.SetText(L"Password :");
        labelk.SetText(L" ");

        nisnTextBox._dwStyle |= WS_BORDER;
        nameTextBox._dwStyle |= WS_BORDER;
        originTextBox._dwStyle |= WS_BORDER;
        entryTextBox._dwStyle |= WS_BORDER;
        passwordTextBox._dwStyle |= WS_BORDER;

        btnAdd.SetText(L"Buat Akun PPDB");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nisn)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nisnTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &name)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nameTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &origin)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &originTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &entry)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &entryTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &password)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &passwordTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &btnAdd)}};

        UI::LayoutControls(&window, true);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = false;
        window.title = L"Register";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

LRESULT OnAddClick(UI::CallbackParam param)
{
    Register::Show();
    return 0;
}

void RemoveByListViewSelection(UI::ListView &listView)
{
    for (int v : listView.GetSelectedIndex())
    {
        std::wstring nisn = listView.GetText(v, 0);
        Student *student = hashTable.get(nisn);
        if (!tree.remove(*student))
            std::cout << "Penghapusan di RBTree gagal" << std::endl;

        if (!hashTable.remove(nisn))
            std::cout << "Penghapusan di RobinHoodHashTable gagal" << std::endl;
    }

    RefreshAllList();
}

namespace Login
{
    UI::Window window;
    UI::Tabs tabs;
    UI::Label label;
    UI::Label labelk;
    UI::Label nisn;
    UI::Label password;
    UI::TextBox nisnTextBox;
    UI::TextBox passwordTextBox;
    UI::Button btnAdd;

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        Student student{
            nisnTextBox.getText(),
            passwordTextBox.getText(),};

        // book.year.erase(book.year.find_last_not_of(' ') + 1);
        // book.year.erase(0, book.year.find_first_not_of(' '));

        try
        {
        if (student.nisn.size() == 0)
            {
                throw std::domain_error("NISN Tidak Boleh Kosong");
            }


            if (student.nisn.size() != 9)
            {
                throw std::domain_error("NISN Harus 9 Angka");
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
            
            // if ((Xcount == 1 && student.nisn[student.nisn.size() - 1] != L'X') || (Xcount > 1))
            // {
            //     throw std::domain_error("X hanya Bisa Di akhir NISN");
            // }
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

        hashTable.put(student.nisn, student);
        tree.insert(std::move(student));

        MessageBoxW(window.hwnd, L"Berhasil Login", L"Success", MB_OK);
        window.Destroy();

        RefreshAllList();

        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        label.SetText(L"Masukkan Username dan Password!");
        nisn.SetText(L"NISN :");
        password.SetText(L"Password :");
        labelk.SetText(L" ");

        nisnTextBox._dwStyle |= WS_BORDER;
        passwordTextBox._dwStyle |= WS_BORDER;

        btnAdd.SetText(L"Login");
        btnAdd.commandListener = OnAddClick;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nisn)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nisnTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &password)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &passwordTextBox)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk)},
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

// LRESULT OnAddClick(UI::CallbackParam param)
// {
//     Login::Show();
//     return 0;
// }

// void RemoveByListViewSelection(UI::ListView &listView)
// {
//     for (int v : listView.GetSelectedIndex())
//     {
//         std::wstring nisn = listView.GetText(v, 0);
//         Student *student = hashTable.get(nisn);
//         if (!tree.remove(*student))
//             std::cout << "Penghapusan di RBTree gagal" << std::endl;

//         if (!hashTable.remove(nisn))
//             std::cout << "Penghapusan di RobinHoodHashTable gagal" << std::endl;
//     }

//     RefreshAllList();
// }

namespace TabAllStudents
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
    std::vector<Student *> studentsList;

    wchar_t *OnGetItem(int row, int column)
    {
        Student *student = studentsList[row];
        if (column == 0)
            return const_cast<wchar_t *>(student->nisn.c_str());
        else if (column == 1)
            return const_cast<wchar_t *>(student->name.c_str());
        else if (column == 2)
            return const_cast<wchar_t *>(student->origin.c_str());
        else if (column == 3)
            return const_cast<wchar_t *>(student->entry.c_str());
        return nullptr;
    }

    void DoRefresh()
    {
        std::wstring type = combobox.GetSelectedText();

        studentsList.resize(tree.count);
        listView.SetRowCount(0);

        label.SetText(L"Memuat data");
        Timer timer;

        progress.SetWaiting(true);

        {
            timer.start();
            size_t current = 0;
            std::function<void(RBNode<Student> *)> visitor = [&](RBNode<Student> *node)
            {
                studentsList[current] = &node->value;
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
        listView.SetRowCount(studentsList.size());

        std::wstringstream stream;
        stream << "Data dimuat dalam " << timer.durationStr();
        label.SetText(stream.str().c_str());

        button.SetEnable(true);
    }

    void RefreshList()
    {
        button.SetEnable(false);
        if (showThread.joinable())
        {
            showThread.join();
        }
        showThread = std::thread(DoRefresh);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        RefreshList();
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

        btnAdd.SetText(L"Buat Akun PPDB");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");
        btnDelete.commandListener = OnDeleteClick;

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

        listView.InsertColumn(L"NISN", 100);
        listView.InsertColumn(L"Nama Siswa", 200);
        listView.InsertColumn(L"Asal Sekolah", 200);
        listView.InsertColumn(L"Jalur Masuk", 200);

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
    UI::Label label;
    UI::Label NISNlabel;
    UI::Button btnSearch;
    UI::TextBox NISNTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Label labelk;
    UI::Button btnAdd;
    UI::Button btnDelete;

    void DoFind()
    {
        label.SetText(L"Mencari data siswa berdasarkan NISN");
        Timer timer;

        {
            std::wstring nisn = NISNTextBox.getText();
            timer.start();
            Student *student = hashTable.get(nisn);
            timer.end();
            if (student == nullptr)
            {
                label.SetText(L"TIdak Di Temukan");
            }
            else
            {
                listView.SetText(0, 1, student->nisn);
                listView.SetText(1, 1, student->name);
                listView.SetText(2, 1, student->origin);
                listView.SetText(3, 1, student->entry);
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
        NISNlabel.SetText(L"Cari Berdasarkan NISN :");
        NISNTextBox._dwStyle |= WS_BORDER;

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        label.SetText(L"Data Belum Dimuat");

        labelk.SetText(L" ");

        btnAdd.SetText(L"Buat Akun PPDB");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &NISNlabel),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &NISNTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label),
             UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"NISN               :");
        listView.InsertRow(L"Nama Siswa        :");
        listView.InsertRow(L"Asal Sekolah      :");
        listView.InsertRow(L"Jalur Masuk     :");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"TabFindNISN";
        window.registerMessageListener(WM_CREATE, onCreate);
    }
}

// menggunakan ptb
namespace TabFindEntry
{
    UI::Window window;
    UI::Label label;
    UI::Label entrylabel;
    UI::Button btnSearch;
    UI::TextBox entryTextBox;
    UI::StatusBar statusBar;
    UI::ListView listView;
    UI::Label labelk;
    UI::Button btnAdd;
    UI::Button btnDelete;

    // void DoFind()
    // {
    //     label.SetText(L"Mencari data siswa berdasarkan Jalur Masuk");
    //     Timer timer;

    //     {
    //         std::wstring nisn = NISNTextBox.getText();
    //         timer.start();
    //         Student *student = hashTable.get(nisn);
    //         timer.end();
    //         if (student == nullptr)
    //         {
    //             label.SetText(L"TIdak Di Temukan");
    //         }
    //         else
    //         {
    //             listView.SetText(0, 1, student->nisn);
    //             listView.SetText(1, 1, student->name);
    //             listView.SetText(2, 1, student->origin);
    //             listView.SetText(3, 1, student->entry);
    //         }
    //     }
    //     btnSearch.SetEnable(true);
    // }

    LRESULT OnFindClick(UI::CallbackParam)
    {
        btnSearch.SetEnable(false);
        // DoFind();
        return 0;
    }

    LRESULT onCreate(UI::CallbackParam param)
    {
        entrylabel.SetText(L"Cari Berdasarkan Jalur Masuk :");
        entryTextBox._dwStyle |= WS_BORDER;

        btnSearch.SetText(L"Cari");
        btnSearch.commandListener = OnFindClick;

        label.SetText(L"Data Belum Dimuat");

        labelk.SetText(L" ");

        btnAdd.SetText(L"Buat Akun PPDB");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus Data");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {
            {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &entrylabel),
             UI::ControlCell(100, UI::SIZE_DEFAULT, &entryTextBox),
             UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnSearch)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
            {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &label),
             UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &labelk),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnAdd),
             UI::ControlCell(180, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Prop", 100);
        listView.InsertRow(L"NISN               :");
        listView.InsertRow(L"Nama Siswa        :");
        listView.InsertRow(L"Asal Sekolah      :");
        listView.InsertRow(L"Jalur Masuk     :");
        listView.InsertColumn(L"Value", 200);

        return 0;
    }

    void Init()
    {
        window.title = L"TabFindEntry";
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

        listView.InsertColumn(L"NISN", 100);
        listView.InsertColumn(L"Nama Siswa", 200);

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

            int nisnIndex = reader.findHeaderIndex("NISN");
            int nameIndex = reader.findHeaderIndex("Name");
            int originIndex = reader.findHeaderIndex("Origin");
            int entryIndex = reader.findHeaderIndex("Entry");
            int passwordIndex = reader.findHeaderIndex("Password");

            while (reader.readData())
            {
                Student student{
                    Utils::stringviewToWstring(reader.data[nisnIndex]),
                    Utils::stringviewToWstring(reader.data[nameIndex]),
                    Utils::stringviewToWstring(reader.data[originIndex]),
                    Utils::stringviewToWstring(reader.data[entryIndex]),
                    Utils::stringviewToWstring(reader.data[passwordIndex])};
                hashTable.put(student.nisn, student);
                // hashTable.insert(std::make_pair(student.nisn, student));
                tree.insert(std::move(student));
            }
            timer.end();
        }
        progressBar.SetWaiting(false);

        std::wstringstream stream;
        stream << L"Data dimuat dari CSV dalam " << timer.durationStr();
        statusBar.SetText(1, stream.str());

        size_t maxPsl = 0;
        std::wstring nisn = L"";
        for (size_t i = 0; i < hashTable.bucketSize; i++)
        {
            if (hashTable.buckets[i].filled)
            {
                if (hashTable.buckets[i].psl >= maxPsl)
                {
                    maxPsl = hashTable.buckets[i].psl;
                    nisn = hashTable.buckets[i].key;
                }
            }
        }

        std::cout << "Max PSL: " << maxPsl << std::endl;

        timer.start();
        std::wcout << hashTable.get(nisn)->nisn << std::endl;
        std::wcout << hashTable.get(L"0374157065")->name << std::endl;
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

        TabAllStudents::Init();
        tabs.AddPage(L"Semua Buku", &TabAllStudents::window);

        TabFindNISN::Init();
        tabs.AddPage(L"Cari Berdasarkan NISN", &TabFindNISN::window);

        TabFindEntry::Init();
        tabs.AddPage(L"Cari Berdasarkan Jalur Masuk", &TabFindNISN::window);

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

void RefreshAllList()
{
    TabAllStudents::RefreshList();
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    UI::Setup(hInst, cmdShow);
    Login::Show();
    return UI::RunEventLoop();
}