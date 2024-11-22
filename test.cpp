#ifndef UNICODE
#define UNICODE
#endif

#include "winlamb/window_main.h"
#include "winlamb/label.h"
#include "winlamb/textbox.h"
#include "winlamb/button.h"
#include "winlamb/listview.h"
#include "winlamb/resizer.h"
#include "Layouter.hpp"

class My_Window : public wl::window_main
{
public:
    wl::listview mList;
    wl::label mLabelFrom;
    wl::textbox mTextBoxFrom;
    wl::label mLabelTo;
    wl::textbox mTextBoxTo;
    wl::button mButtonSearch;
    wl::resizer mResizer;

    My_Window()
    {
        setup.wndClassEx.lpszClassName = L"SOME_CLASS_NAME"; // class name to be registered
        setup.title = L"This is my window";
        setup.size = {200, 200};
        setup.style = WS_OVERLAPPEDWINDOW;

        on_message(WM_CREATE, [this](wl::wm::create p) -> LRESULT
                   {
            set_text(L"A new title for the window");

            Layouter layouter(hwnd());

            POINT pos = layouter.pos();
            SIZE sz = layouter.component(35, 24);
            mLabelFrom.create(this, 4005, L"From", pos, sz);

            layouter.nextColumn();
            pos = layouter.pos();
            sz = layouter.edit();
            mTextBoxFrom.create(this, 4001, wl::textbox::type::NORMAL, pos, sz.cx, sz.cy);

            layouter.nextColumn();
            pos = layouter.pos(10);
            sz = layouter.component(20, 24);
            mLabelTo.create(this, 4005, L"To", pos, sz);

            layouter.nextColumn();
            pos = layouter.pos();
            sz = layouter.edit();
            mTextBoxTo.create(this, 4001, wl::textbox::type::NORMAL, pos, sz.cx, sz.cy);


            layouter.nextColumn();
            pos = layouter.pos(10);
            mButtonSearch.create(this, 4003, L"Cari", pos, layouter.button());

            layouter.nextRow();
            pos = layouter.pos();

            mList.create(this, 4002, pos, layouter.fill(), WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_NOSORTHEADER | WS_TABSTOP, WS_EX_CLIENTEDGE);
            mList.columns.add(L"Test", 50).set_width_to_fill(0);
            mList.items.add(L"Test");
            mList.set_focus();

        mResizer.add(mList, wl::resizer::go::RESIZE, wl::resizer::go::RESIZE);

            return true; });

        on_message(WM_SIZE, [this](wl::wm::size sz) -> LRESULT
                   {
            mResizer.adjust(sz);
            return true; });

        on_command(4003, [&] (wl::params p) -> LRESULT {
            mList.items.remove_all();
            return true;
        });
    }
};

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int cmdShow)
{
    return wl::_wli::run_main<My_Window>(hInst, cmdShow);
}
