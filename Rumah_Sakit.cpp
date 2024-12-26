#include "WorkerThread.hpp"
#include "UI.hpp"
#include "Utils.hpp"
#include "HalfSipHash.h"
#include "RBTree.hpp"
#include "RobinHoodHashMap.hpp"

enum class PatientGroup
{
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4
};

int PatientGroupPricePerDay[5] = {150000, 200000, 250000, 300000, 350000};
std::wstring PatientGroupName[5] = {L"A", L"B", L"C", L"D", L"E"};

int GetPatientGroupPricePerDay(PatientGroup group)
{
    return PatientGroupPricePerDay[int(group)];
}

const std::wstring &GetPatientGroupName(PatientGroup group)
{
    return PatientGroupName[int(group)];
}

struct HospitalPatient
{
    std::wstring name, id;
    PatientGroup group;
    int durationDay;

    int GetTotalPrice()
    {
        return durationDay * GetPatientGroupPricePerDay(group);
    }
};

struct HospitalPatientNameComparer
{
    int compare(const HospitalPatient &a, const HospitalPatient &b)
    {
        int cmp = Utils::CompareWStringHalfInsensitive(a.name, b.name);

        if (cmp != 0)
        {
            return cmp;
        }

        return Utils::CompareWStringHalfInsensitive(a.id, b.id);
    }
};

struct HospitalPatientIDHasher
{
    uint64_t seed = 0xe17a1465;

    uint64_t hash(const std::wstring &wstr)
    {
        return HalfSipHash_64(wstr.data(), sizeof(wchar_t) * wstr.size(), &seed);
    }
};

RBTree<HospitalPatient, HospitalPatientNameComparer> tree;
RobinHoodHashMap<std::wstring, HospitalPatient, HospitalPatientIDHasher> hashTable;

namespace AddWindow
{
    UI::Window window;
    UI::Label idLabel, nameLabel, groupLabel, durationLabel, durationSuffixLabel;
    UI::TextBox idTextBox, nameTextBox;
    UI::ComboBox groupComboBox;
    UI::SpinBox durationSpinBox;
    UI::DateTimePicker dtpStart, dtpEnd;
    UI::Button addButton;

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        // HospitalPatient patient = {
        //     nameTextBox.getText(),
        //     idTextBox.getText(),
        //     (PatientGroup)groupComboBox.GetSelectedIndex(),
        //     durationSpinBox.GetValue()};

        // hashTable.put(patient.id, patient);
        // tree.insert(std::move(patient));

        // window.CloseModal();

        std::wcout << Utils::SystemTimeToDateStr(dtpStart.GetValue()) << L"  " << Utils::SystemTimeToDateStr(dtpEnd.GetValue()) << std::endl;
        std::wcout << Utils::GetSystemDateDifferenceDays(dtpStart.GetValue(), dtpEnd.GetValue()) << std::endl;

        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        window.InitModal();
        idLabel.SetText(L"ID");
        nameLabel.SetText(L"Nama");
        groupLabel.SetText(L"Golongan");
        durationLabel.SetText(L"Lama Rawat Inap");
        durationSuffixLabel.SetText(L"hari");
        addButton.SetText(L"Tambah");

        addButton.commandListener = OnAddClick;

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &idLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &idTextBox)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nameLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nameTextBox)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &groupLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &groupComboBox)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &durationLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &durationSpinBox),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &durationSuffixLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &dtpStart)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &dtpEnd)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &addButton)}};
        UI::LayoutControls(&window, true);

        // idTextBox.SetEnable(false);

        groupComboBox.AddItem(L"A");
        groupComboBox.AddItem(L"B");
        groupComboBox.AddItem(L"C");
        groupComboBox.AddItem(L"D");
        groupComboBox.AddItem(L"E");

        durationSpinBox.SetRange(0, 1000);

        dtpStart.SetValue(Utils::DateStrToSystemTime(L"24/12/2024"));

        return 0;
    }

    void Show()
    {
        window.title = L"Tambah Pasien";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

LRESULT OnAddClick(UI::CallbackParam param)
{
    AddWindow::Show();
    return 0;
}

struct PatientListView : UI::VListView
{
    std::vector<HospitalPatient *> items;

    void Create(UI::Window *window, HWND hParent, POINT pos, SIZE size) override
    {
        _dwStyle |= LVS_REPORT | WS_BORDER | LVS_SHOWSELALWAYS;
        UI::VListView::Create(window, hParent, pos, size);

        InsertColumn(L"ID", 100);
        InsertColumn(L"Nama", 100);
        InsertColumn(L"Golongan", 100);
        InsertColumn(L"Biaya Per Hari", 100);
        InsertColumn(L"Lama Rawat Inap", 150);
        InsertColumn(L"Biaya Total", 200);
    }

    const std::wstring OnGetItem(int row, int column) override
    {
        HospitalPatient *patient = items[row];
        if (column == 0)
            return patient->id;
        if (column == 1)
            return patient->name;
        if (column == 2)
            return GetPatientGroupName(patient->group);
        if (column == 3)
            return std::to_wstring(GetPatientGroupPricePerDay(patient->group));
        if (column == 4)
            return std::to_wstring(patient->durationDay) + L" hari";
        if (column == 5)
            return std::to_wstring(patient->GetTotalPrice());
        return L"";
    }
};

namespace TabHistoryDelete
{
    UI::Window window;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::ComboBox comboType;
    UI::Button btnShow, btnRestore;
    PatientListView listView;

    void SetEnable(bool enable)
    {
        comboType.SetEnable(enable);
        btnShow.SetEnable(enable);
        btnRestore.SetEnable(enable);
        listView.SetEnable(enable);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        btnShow.SetText(L"Tampilkan");
        btnShow.commandListener = OnShowClick;

        btnRestore.SetText(L"Restore");

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &comboType),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnShow)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
                                 {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnRestore)}};
        UI::LayoutControls(&window, true);

        comboType.AddItem(L"Pre-order");
        comboType.AddItem(L"In-order");
        comboType.AddItem(L"Post-order");
        comboType.SetSelectedIndex(1);

        return 0;
    }

    void Init()
    {
        window.title = L"TabHistoryDelete";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabDetail
{
    UI::Window window;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::Label labelId;
    UI::TextBox textBoxId;
    UI::Button btnFind, btnAdd, btnDelete;
    UI::ListView listView;
    void SetEnable(bool enable)
    {
        textBoxId.SetEnable(enable);
        btnFind.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        listView.SetEnable(enable);
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        labelId.SetText(L"ID");
        btnFind.SetText(L"Cari");
        btnAdd.SetText(L"Tambah Pasien");
        btnAdd.commandListener = OnAddClick;
        btnDelete.SetText(L"Hapus");

        listView._dwStyle |= LVS_REPORT | WS_BORDER;

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &labelId),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &textBoxId),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnFind)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
                                 {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnAdd),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        listView.InsertColumn(L"Properti", 100);
        listView.InsertColumn(L"Nilai", 200);

        listView.InsertRow(L"ID");
        listView.InsertRow(L"Nama");
        listView.InsertRow(L"Golongan");
        listView.InsertRow(L"Biaya Per Hari");
        listView.InsertRow(L"Lama Rawat Inap");
        listView.InsertRow(L"Biaya Total");

        return 0;
    }

    void Init()
    {
        window.title = L"TabDetail";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabLongestDuration
{
    UI::Window window;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::Label labelCount;
    UI::SpinBox spinCount;
    UI::Button btnShow, btnCopyID, btnAdd, btnDelete;
    PatientListView listView;

    void SetEnable(bool enable)
    {
        spinCount.SetEnable(enable);
        btnShow.SetEnable(enable);
        btnCopyID.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        listView.SetEnable(enable);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        btnShow.SetText(L"Tampilkan");
        btnShow.commandListener = OnShowClick;

        btnCopyID.SetText(L"Salin ID");

        btnAdd.SetText(L"Tambah Pasien");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus");

        labelCount.SetText(L"Jumlah");

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &labelCount),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &spinCount),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnShow)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
                                 {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnCopyID),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnAdd),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        spinCount.SetRange(0, 1000);
        spinCount.SetValue(100);

        return 0;
    }

    void Init()
    {
        window.title = L"TabLongestDuration";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabFindRange
{
    UI::Window window;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::Label labelFrom, labelTo;
    UI::Button btnFind, btnCopyID, btnAdd, btnDelete;
    UI::TextBox textBoxFrom, textBoxTo;
    PatientListView listView;

    void SetEnable(bool enable)
    {
        textBoxFrom.SetEnable(enable);
        textBoxTo.SetEnable(enable);
        btnFind.SetEnable(enable);
        btnCopyID.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        listView.SetEnable(enable);
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        labelFrom.SetText(L"Dari");
        labelTo.SetText(L"Ke");
        btnFind.SetText(L"Cari");

        btnCopyID.SetText(L"Salin ID");

        btnAdd.SetText(L"Tambah Pasien");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus");

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &labelFrom),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &textBoxFrom),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &labelTo),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &textBoxTo),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnFind)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
                                 {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnCopyID),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnAdd),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        return 0;
    }

    void Init()
    {
        window.title = L"TabFindRange";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace TabAllPatient
{
    UI::Window window;
    UI::ProgressBar progress;
    UI::LabelWorkMessage message;
    UI::ComboBox comboType;
    UI::Button btnShow, btnCopyID, btnAdd, btnDelete;
    PatientListView listView;

    void SetEnable(bool enable)
    {
        comboType.SetEnable(enable);
        btnShow.SetEnable(enable);
        btnCopyID.SetEnable(enable);
        btnAdd.SetEnable(enable);
        btnDelete.SetEnable(enable);
        listView.SetEnable(enable);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        listView.SetRowCount(0);
        listView.items.resize(tree.count);

        size_t current = 0;
        tree.inorder(tree.root, [&](RBNode<HospitalPatient> *node)
                     {
            listView.items[current] = &node->value;
            current++; });

        listView.SetRowCount(tree.count);

        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        btnShow.SetText(L"Tampilkan");
        btnShow.commandListener = OnShowClick;

        btnCopyID.SetText(L"Salin ID");

        btnAdd.SetText(L"Tambah Pasien");
        btnAdd.commandListener = OnAddClick;

        btnDelete.SetText(L"Hapus");

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &comboType),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnShow)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &progress)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &message)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &listView)},
                                 {UI::EmptyCell(UI::SIZE_FILL, UI::SIZE_DEFAULT),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnCopyID),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnAdd),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &btnDelete)}};
        UI::LayoutControls(&window, true);

        comboType.AddItem(L"Pre-order");
        comboType.AddItem(L"In-order");
        comboType.AddItem(L"Post-order");
        comboType.SetSelectedIndex(1);

        return 0;
    }

    void Init()
    {
        window.title = L"TabAlPatient";
        window.registerMessageListener(WM_CREATE, OnCreate);
    }
}

namespace MainWindow
{
    UI::Window window;
    UI::Tabs tabs;

    LRESULT OnCreate(UI::CallbackParam)
    {
        AddWindow::window.parentHwnd = window.hwnd;

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &tabs)}};
        UI::LayoutControls(&window, true);

        TabAllPatient::Init();
        tabs.AddPage(L"Semua Pasien", &TabAllPatient::window);

        TabFindRange::Init();
        tabs.AddPage(L"Cari Berdasarkan Rentang Nama", &TabFindRange::window);

        TabDetail::Init();
        tabs.AddPage(L"Detail Pasien", &TabDetail::window);

        TabLongestDuration::Init();
        tabs.AddPage(L"Durasi Terlama", &TabLongestDuration::window);

        TabHistoryDelete::Init();
        tabs.AddPage(L"Riwayat Hapus", &TabHistoryDelete::window);

        return 0;
    }

    void Show()
    {
        window.quitWhenClose = true;
        window.title = L"Rumah Sakit";
        window.registerMessageListener(WM_CREATE, OnCreate);
        UI::ShowWindowClass(window);
    }
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    WorkerThread::Init();

    UI::Setup(hInst, cmdShow);
    MainWindow::Show();
    return UI::RunEventLoop();
}