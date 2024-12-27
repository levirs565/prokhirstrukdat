#include "UI.hpp"

namespace UIUtils
{

    void MessageSetWait(UI::LabelWorkMessage *message, bool clear = true)
    {
        if (clear)
            message->Clear();
        message->AddMessage(L"Menunggu antrian tugas");
    }

}