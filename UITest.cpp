#include <iostream>
#include "UI.hpp"
#include <wingdi.h>

struct
{
    UI::Window window;
    UI::Button button;
    UI::ComboBox combo;
} secondWindowTab2;

LRESULT SedondWindowTab2ButtonClick(UI::CallbackParam param)
{
    std::cout << secondWindowTab2.combo.GetSelectedIndex() << std::endl;
    if (secondWindowTab2.combo.GetSelectedIndex() != -1)
    {
        std::wcout << secondWindowTab2.combo.GetSelectedText() << std::endl;
    }
    return 0;
}

LRESULT SecondWindowTab2OnCreate(UI::CallbackParam param)
{
    secondWindowTab2.window.controlsLayout = {
        {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &secondWindowTab2.button)},
        {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &secondWindowTab2.combo)}};
    secondWindowTab2.button.commandListener = SedondWindowTab2ButtonClick;

    UI::LayoutControls(&secondWindowTab2.window, true);

    secondWindowTab2.combo.AddItem(L"Satu");
    secondWindowTab2.combo.AddItem(L"Dua");
    secondWindowTab2.combo.SetSelectedIndex(0);

    return 0;
}

struct
{
    UI::Window window;
    UI::Button button;
} secondWindowTab1;

LRESULT SecondWindowTab1OnCreate(UI::CallbackParam param)
{
    secondWindowTab1.window.controlsLayout = {
        {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &secondWindowTab1.button)}};
    UI::LayoutControls(&secondWindowTab1.window, true);
    return 0;
}

struct
{
    UI::Window window;
    UI::Tabs tabs;
} secondWindow;

LRESULT SecondWindowOnCreate(UI::CallbackParam param)
{
    secondWindow.window.controlsLayout = {
        {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &secondWindow.tabs)}};
    UI::LayoutControls(&secondWindow.window, true);

    secondWindowTab1.window.title = L"SecondWindowPage1";
    secondWindowTab1.window.registerMessageListener(WM_CREATE, SecondWindowTab1OnCreate);
    secondWindow.tabs.AddPage(L"Satu", &secondWindowTab1.window);

    secondWindowTab2.window.title = L"SecondWindowPage2";
    secondWindowTab2.window.registerMessageListener(WM_CREATE, SecondWindowTab2OnCreate);
    secondWindow.tabs.AddPage(L"Dua", &secondWindowTab2.window);

    return 0;
}

void ShowSecondWindow()
{
    secondWindow.window.title = L"Second Window";
    secondWindow.window.registerMessageListener(WM_CREATE, SecondWindowOnCreate);
    UI::ShowWindowClass(secondWindow.window);
}

struct
{
    UI::Window window;
    UI::Button button1;
    UI::Button button2;
    UI::Button button3;
    UI::Button button4;
    UI::TextBox fromTextBox, toTextBox;
    UI::ListView listView;
    UI::Label fromLabel, toLabel;
    UI::StatusBar statusBar;
    UI::ProgressBar progressBar;
} mainWindow;

LRESULT OnMainWindowButton1Command(UI::CallbackParam param)
{
    for (wchar_t ch = L'a'; ch <= L'z'; ch++)
    {
        int row = mainWindow.listView.InsertRow(std::to_wstring((int)ch));
        mainWindow.listView.SetText(row, 1, std::wstring{ch});
    }
    return 0;
}

LRESULT OnMainWindowButton2Command(UI::CallbackParam param)
{
    mainWindow.progressBar.SetWaiting(true);
    return 0;
}

LRESULT OnMainWindowButton3Command(UI::CallbackParam param)
{
    mainWindow.progressBar.SetWaiting(false);
    return 0;
}

LRESULT OnMainWindowButton4Command(UI::CallbackParam param)
{
    secondWindow.window.parentHwnd = mainWindow.window.hwnd;
    ShowSecondWindow();
    return 0;
}

LRESULT OnMainWindowCreate(UI::CallbackParam param)
{
    mainWindow.button1.SetText(L"Button 1");
    mainWindow.button1.commandListener = OnMainWindowButton1Command;

    mainWindow.button2.SetText(L"Button 2");
    mainWindow.button2.commandListener = OnMainWindowButton2Command;

    mainWindow.button3.SetText(L"Button 3");
    mainWindow.button3.commandListener = OnMainWindowButton3Command;

    mainWindow.button4.SetText(L"Button 4");
    mainWindow.button4.commandListener = OnMainWindowButton4Command;

    mainWindow.fromLabel.SetText(L"Dari: ");
    mainWindow.fromTextBox._dwStyle |= WS_BORDER;
    mainWindow.toLabel.SetText(L"Sampai: ");
    mainWindow.toTextBox._dwStyle |= WS_BORDER;

    mainWindow.listView._dwStyle |= LVS_REPORT | WS_BORDER;

    mainWindow.window.controlsLayout = {
        {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.button1),
         UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.button2)},
        {UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.fromLabel),
         UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.fromTextBox),
         UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.toLabel),
         UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.toTextBox),
         UI::ControlCell(UI::SIZE_DEFAULT, UI::SIZE_DEFAULT, &mainWindow.button3)},
        {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_DEFAULT, &mainWindow.button4)},
        {UI::ControlCell(UI::SIZE_FILL, UI::SIZE_FILL, &mainWindow.listView)}};
    mainWindow.window.layouter.paddingBottom = 25;
    UI::LayoutControls(&mainWindow.window, true);

    mainWindow.statusBar.Create(&mainWindow.window);
    mainWindow.statusBar.SetParts({118, 100, 100});

    mainWindow.progressBar.Create(&mainWindow.window, mainWindow.statusBar.hwnd, {9, 2}, {100, 19});
    mainWindow.progressBar.SetProgress(50);

    mainWindow.statusBar.SetText(1, L"Yes");
    mainWindow.statusBar.SetText(2, L"No");
    mainWindow.window.fixedControls.push_back(&mainWindow.statusBar);

    mainWindow.listView.InsertColumn(L"Kode", 100);
    mainWindow.listView.InsertColumn(L"Karakter", 100);

    for (wchar_t ch = L'A'; ch <= L'Z'; ch++)
    {
        int row = mainWindow.listView.InsertRow(std::to_wstring((int)ch));
        mainWindow.listView.SetText(row, 1, std::wstring{ch});
    }

    return 0;
}

void ShowMainWindow()
{
    mainWindow.window.quitWhenClose = true;
    mainWindow.window.title = L"Main";
    mainWindow.window.registerMessageListener(WM_CREATE, OnMainWindowCreate);

    UI::ShowWindowClass(mainWindow.window);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    UI::Setup(hInst, cmdShow);
    ShowMainWindow();
    return UI::RunEventLoop();
}