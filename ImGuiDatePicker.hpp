#pragma once
#include <imgui.h>
#include <string>


#ifndef IMGUI_DATEPICKER_YEAR_MIN
    #define IMGUI_DATEPICKER_YEAR_MIN 1900
#endif // !IMGUI_DATEPICKER_YEAR_MIN

#ifndef IMGUI_DATEPICKER_YEAR_MAX
    #define IMGUI_DATEPICKER_YEAR_MAX 3000
#endif // !IMGUI_DATEPICKER_YEAR_MAX


// ImGui::Begin("Progress Indicators");
//      const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
//      const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);
//      ImGui::Spinner("##spinner", 15, 6, col);
//      ImGui::BufferingBar("##buffer_bar", 0.7f, ImVec2(400, 6), bg, col);
// ImGui::End();

namespace ImGui
{
    IMGUI_API bool DatePicker(const char* label, int ymd[3], float tsz[2], bool clamp, ImGuiTableFlags table_flags);
    IMGUI_API bool Spinner(const char* label, float radius, int thickness, int color);
    IMGUI_API bool BufferingBar(const char* label, float value,  const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);
}
