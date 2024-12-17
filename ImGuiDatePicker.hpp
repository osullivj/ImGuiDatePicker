#pragma once
#include <imgui.h>
#include <string>


#ifndef IMGUI_DATEPICKER_YEAR_MIN
    #define IMGUI_DATEPICKER_YEAR_MIN 1900
#endif // !IMGUI_DATEPICKER_YEAR_MIN

#ifndef IMGUI_DATEPICKER_YEAR_MAX
    #define IMGUI_DATEPICKER_YEAR_MAX 3000
#endif // !IMGUI_DATEPICKER_YEAR_MAX

namespace ImGui
{
    IMGUI_API bool DatePicker(const char* label, int ymd[3], float tsz[2], bool clamp, ImGuiTableFlags table_flags);
}
