#include "WorkerThread.hpp"
#include "UI.hpp"
#include "Utils.hpp"
#include "HalfSipHash.h"
#include "RBTree.hpp"
#include "RobinHoodHashMap.hpp"
#include "CSVReader.hpp"
#include <sstream>
#include <iomanip>

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
    std::wstring id, name;
    PatientGroup group;
    SYSTEMTIME start, end;

    int GetDayPrice()
    {
        return GetPatientGroupPricePerDay(group);
    }

    int GetDurationDay()
    {
        return Utils::GetSystemDateDifferenceDays(end, start);
    }

    int GetTotalPrice()
    {
        return GetDurationDay() * GetDayPrice();
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

std::wstring lastId = L"000000000";
RBTree<HospitalPatient, HospitalPatientNameComparer> tree;
RobinHoodHashMap<std::wstring, HospitalPatient, HospitalPatientIDHasher> hashTable;

std::wstring GenNextId()
{
    int lastIdInt = std::stoi(lastId);
    lastIdInt++;
    std::wstringstream stream;
    stream << std::setw(9) << std::setfill(L'0') << std::right << lastIdInt;
    return stream.str();
}

namespace AddWindow
{
    UI::Window window;
    UI::Label idLabel, nameLabel, groupLabel, durationLabel, dateLabel, startLabel, endLabel, priceDayLabel, priceTotal;
    UI::TextBox nameTextBox;
    UI::ComboBox groupComboBox;
    UI::DateTimePicker dtpStart, dtpEnd;
    UI::Button addButton;
    HospitalPatient patient;

    LRESULT OnAddClick(UI::CallbackParam param)
    {
        patient.name = nameTextBox.getText();

        try
        {
            if (patient.name.empty())
                throw std::domain_error("Nama tidak boleh kosong");
            if (patient.GetDurationDay() <= 0)
                throw std::domain_error("Tanggal sampai harus lebih dari tanggal dari");
        }
        catch (std::domain_error const &e)
        {
            MessageBoxA(window.hwnd, e.what(), "Gagal", MB_OK);
            return 0;
        }
        lastId = patient.id;
        hashTable.put(patient.id, patient);
        tree.insert(std::move(patient));

        window.CloseModal();
        return 0;
    }

    void UpdateLabel()
    {
        patient.start = dtpStart.GetValue();
        patient.end = dtpEnd.GetValue();
        patient.group = (PatientGroup)groupComboBox.GetSelectedIndex();

        durationLabel.SetText(L"Durasi: " + std::to_wstring(patient.GetDurationDay()) + L" hari");
        priceDayLabel.SetText(L"Biaya Per Hari: " + std::to_wstring(patient.GetDayPrice()));
        priceTotal.SetText(L"Biaya Total: " + std::to_wstring(patient.GetTotalPrice()));
    }

    LRESULT OnStartEndChanged(UI::CallbackParam param)
    {
        UpdateLabel();
        return 0;
    }

    LRESULT OnComboChanged(UI::CallbackParam param)
    {
        UpdateLabel();
        return 0;
    }

    LRESULT OnCreate(UI::CallbackParam param)
    {
        patient.id = GenNextId();

        window.InitModal();
        idLabel.SetText(L"ID: " + patient.id);
        nameLabel.SetText(L"Nama");
        groupLabel.SetText(L"Golongan");
        durationLabel.SetText(L"Durasi: 0 hari");
        addButton.SetText(L"Tambah");
        dateLabel.SetText(L"Tanggal Rawat Inap");
        startLabel.SetText(L"Dari");
        endLabel.SetText(L"Sampai");
        priceDayLabel.SetText(L"Biaya Per Hari: ");
        priceTotal.SetText(L"Biaya Total: ");

        addButton.commandListener = OnAddClick;

        window.controlsLayout = {{UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &idLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nameLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &nameTextBox)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &groupLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &groupComboBox)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &priceDayLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &dateLabel)},
                                 {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &startLabel),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &dtpStart),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &endLabel),
                                  UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &dtpEnd),
                                  UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &durationLabel)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &priceTotal)},
                                 {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &addButton)}};
        UI::LayoutControls(&window, true);

        window.registerNotifyListener(dtpStart._controlId, DTN_DATETIMECHANGE, OnStartEndChanged);
        window.registerNotifyListener(dtpEnd._controlId, DTN_DATETIMECHANGE, OnStartEndChanged);

        groupComboBox.AddItem(L"A");
        groupComboBox.AddItem(L"B");
        groupComboBox.AddItem(L"C");
        groupComboBox.AddItem(L"D");
        groupComboBox.AddItem(L"E");
        window.registerCommandListener(groupComboBox._controlId, OnComboChanged);
        groupComboBox.SetSelectedIndex(0);

        SYSTEMTIME time;
        GetSystemTime(&time);
        time.wHour = 0;
        time.wMinute = 0;
        time.wSecond = 0;
        time.wMilliseconds = 0;

        dtpStart.SetValue(time);
        dtpEnd.SetValue(time);

        UpdateLabel();

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
        InsertColumn(L"Dari", 100);
        InsertColumn(L"Sampai", 100);
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
            return std::to_wstring(patient->GetDayPrice());
        if (column == 4)
            return Utils::SystemTimeToDateStr(patient->start);
        if (column == 5)
            return Utils::SystemTimeToDateStr(patient->end);
        if (column == 6)
            return std::to_wstring(patient->GetDurationDay()) + L" hari";
        if (column == 7)
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

    void DoShow() {
        listView.SetRowCount(0);
        listView.items.resize(tree.count);

        size_t current = 0;
        tree.inorder(tree.root, [&](RBNode<HospitalPatient> *node)
                     {
            listView.items[current] = &node->value;
            current++; });

        listView.SetRowCount(tree.count);
    }

    LRESULT OnShowClick(UI::CallbackParam param)
    {
        DoShow();
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

    void DoLoad()
    {
        CSVReader<CSVReaderIOBuffSync> reader("data/Rumah_Sakit.csv", ',');

        reader.startRead();

        int idIndex = reader.findHeaderIndex("ID");
        int nameIndex = reader.findHeaderIndex("Name");
        int groupIndex = reader.findHeaderIndex("Group");
        int startIndex = reader.findHeaderIndex("StartDate");
        int endIndex = reader.findHeaderIndex("EndDate");

        while (reader.readData())
        {
            HospitalPatient patient{
                Utils::stringviewToWstring(reader.data[idIndex]),
                Utils::stringviewToWstring(reader.data[nameIndex]),
                PatientGroup(reader.data[groupIndex].begin[0] - 'A'),
                Utils::DateStrToSystemTime(Utils::stringviewToWstring(reader.data[startIndex])),
                Utils::DateStrToSystemTime(Utils::stringviewToWstring(reader.data[endIndex])),
            };
            if (patient.id > lastId) lastId = patient.id;
            hashTable.put(patient.id, patient);
            tree.insert(std::move(patient));
        }
    }

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

        DoLoad();
        TabAllPatient::DoShow();

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