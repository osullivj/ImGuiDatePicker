#include "ImGuiDatePicker.hpp"
#include <imgui_internal.h>
#include <cstdint>
#include <chrono>
#include <vector>
#include <unordered_map>

// No struct tm at Emscripten boundary; so we change the API
// to not use struct tm.
#define GET_DAY(ymd) ymd[2]
#define GET_MONTH_IDX(ymd) (ymd[1] - 1)
#define GET_MONTH(ymd) ymd[1]
#define GET_YEAR(ymd) ymd[0]

namespace ImGui {

static const std::vector<std::string> MONTHS = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

static const std::vector<std::string> DAYS = {
    "Mo",
    "Tu",
    "We",
    "Th",
    "Fr",
    "Sa",
    "Su"
};

// Implements Zeller's Congruence to determine the day of week [1, 7](Mon-Sun) from the given parameters
inline static int DayOfWeek(int dayOfMonth, int month, int year) noexcept
{
    if ((month == 1) || (month == 2)) {
        month += 12;
        year -= 1;
    }

    int h = (dayOfMonth
            + static_cast<int>(std::floor((13 * (month + 1)) / 5.0))
            + year
            + static_cast<int>(std::floor(year / 4.0))
            - static_cast<int>(std::floor(year / 100.0))
            + static_cast<int>(std::floor(year / 400.0))) % 7;

    return static_cast<int>(std::floor(((h + 5) % 7) + 1));
}

constexpr static bool IsLeapYear(int year) noexcept
{
    if ((year % 400) == 0)
        return true;
    if ((year % 4 == 0) && ((year % 100) != 0))
        return true;
    return false;
}

inline static int NumDaysInMonth(int month, int year)
{
    if (month == 2)
        return IsLeapYear(year) ? 29 : 28;

    // Month index paired to the number of days in that month excluding February
    static const std::unordered_map<int, int> monthDayMap = {
            { 1,  31 },
            { 3,  31 },
            { 4,  30 },
            { 5,  31 },
            { 6,  30 },
            { 7,  31 },
            { 8,  31 },
            { 9,  30 },
            { 10, 31 },
            { 11, 30 },
            { 12, 31 }
    };
    return monthDayMap.at(month);
}

// Returns the number of calendar weeks spanned by month in the specified year
inline static int NumWeeksInMonth(int month, int year)
{
    int days = NumDaysInMonth(month, year);
    int firstDay = DayOfWeek(1, month, year);
    return static_cast<int>(std::ceil((days + firstDay - 1) / 7.0));
}

// Returns a vector containing dates as they would appear on the calendar for a given week. Populates 0 if there is no day.
inline static std::vector<int> CalendarWeek(int week, int startDay, int daysInMonth)
{
    std::vector<int> res(7, 0);
    int startOfWeek = 7 * (week - 1) + 2 - startDay;

    if (startOfWeek >= 1)
        res[0] = startOfWeek;

    for (int i = 1; i < 7; ++i) {
        int day = startOfWeek + i;
        if ((day >= 1) && (day <= daysInMonth))
            res[i] = day;
    }
    return res;
}

// TODO: move sematics for ret val
inline static std::string TimePointToLongString(int ymd[3]) noexcept
{
    std::string day = std::to_string(GET_DAY(ymd));
    std::string month = MONTHS[GET_MONTH_IDX(ymd)];
    std::string year = std::to_string(GET_YEAR(ymd));
    return std::string(day + " " + month + " " + year);
}


inline void PreviousMonth(int ymd[3]) noexcept
{
    int month = GET_MONTH(ymd);
    int year = GET_YEAR(ymd);

    if (month == 1) {
        int newDay = std::min(GET_DAY(ymd), NumDaysInMonth(12, --year));
        ymd[0] = year;
        ymd[2] = newDay;
        return;
    }

    int newDay = std::min(GET_DAY(ymd), NumDaysInMonth(--month, year));
    ymd[1] = month;
    ymd[2] = newDay;
    return;
}

inline void NextMonth(int ymd[3]) noexcept
{
    int month = GET_MONTH(ymd);
    int year = GET_YEAR(ymd);

    if (month == 12) {
        int newDay = std::min(GET_DAY(ymd), NumDaysInMonth(1, ++year));
        ymd[0] = year;
        ymd[2] = newDay;
    }

    int newDay = std::min(GET_DAY(ymd), NumDaysInMonth(++month, year));
    ymd[1] = month;
    ymd[2] = newDay;
    return;
}

constexpr static bool IsMinDate(int ymd[3]) noexcept
{
    return (GET_MONTH(ymd) == 1) && (GET_YEAR(ymd) == IMGUI_DATEPICKER_YEAR_MIN);
}

constexpr static bool IsMaxDate(int ymd[3]) noexcept
{
    return (GET_MONTH(ymd) == 12) && (GET_YEAR(ymd) == IMGUI_DATEPICKER_YEAR_MAX);
}

static bool ComboBox(const std::string& label, const std::vector<std::string>& items, int& v)
{
    bool res = false;

    if (ImGui::BeginCombo(label.c_str(), items[v].c_str())) {
        for (int i = 0; i < items.size(); ++i) {
            bool selected = (items[v] == items[i]);
            if (ImGui::Selectable(items[i].c_str(), &selected)) {
                v = i;
                res = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return res;
}

bool DatePicker(const char* clabel, int ymd[3], float table_size[2], bool clampToBorder, ImGuiTableFlags table_flags)
{
    bool res = false;
    std::string label( clabel);
    ImGuiWindow* window = GetCurrentWindow();

    if (window->SkipItems)
        return false;

    bool isHiddenLabel = label.substr(0, 2) == "##";
    std::string myLabel = isHiddenLabel ? label.substr(2) : label;

    // Combo (drop down) for date
    std::string top_label("##");
    top_label += myLabel;
    if (BeginCombo(top_label.c_str(), TimePointToLongString(ymd).c_str())) {
        int monthIdx = GET_MONTH_IDX(ymd);
        int year = GET_YEAR(ymd);

        PushItemWidth((GetContentRegionAvail().x * 0.5f));
        std::string month_label("##CmbMonth_");
        month_label += myLabel;
        if (ComboBox(month_label.c_str(), MONTHS, monthIdx)) {
            ymd[1] = monthIdx + 1;
            res = true;
        }
        PopItemWidth();

        SameLine();

        PushItemWidth(GetContentRegionAvail().x);
        std::string year_label("##IntYear_");
        year_label += myLabel;
        if (InputInt(year_label.c_str(), &year)) {
            ymd[0] = std::min(std::max(IMGUI_DATEPICKER_YEAR_MIN, year), IMGUI_DATEPICKER_YEAR_MAX);
            res = true;
        }
        PopItemWidth();

        const float contentWidth = GetContentRegionAvail().x;
        const float arrowSize = GetFrameHeight();
        const float arrowButtonWidth = arrowSize * 2.0f + GetStyle().ItemSpacing.x;
        const float bulletSize = arrowSize - 5.0f;
        const float bulletButtonWidth = bulletSize + GetStyle().ItemSpacing.x;
        const float combinedWidth = arrowButtonWidth + bulletButtonWidth;
        const float offset = (contentWidth - combinedWidth) * 0.5f;

        SetCursorPosX(GetCursorPosX() + offset);
        PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
        PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        BeginDisabled(IsMinDate(ymd));

        // Left arrow to go to previous month
        if (ArrowButtonEx(std::string("##ArrowLeft_" + myLabel).c_str(), 
                            ImGuiDir_Left, ImVec2(arrowSize, arrowSize))) {
            PreviousMonth(ymd);
            res = true;
        }

        EndDisabled();
        PopStyleColor(2);
        SameLine();
        PushStyleColor(ImGuiCol_Button, GetStyleColorVec4(ImGuiCol_Text));
        SetCursorPosY(GetCursorPosY() + 2.0f);

        // Right arrow for next month
        if (ButtonEx(std::string("##ArrowMid_" + myLabel).c_str(), ImVec2(bulletSize, bulletSize))) {
            /* TODO: reimplement
            v = Today();
            res = true; */
            res = false;
            CloseCurrentPopup();
        }

        PopStyleColor();
        SameLine();
        PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        BeginDisabled(IsMaxDate(ymd));

        if (ArrowButtonEx(std::string("##ArrowRight_" + myLabel).c_str(), 
                                ImGuiDir_Right, ImVec2(arrowSize, arrowSize))) {
            NextMonth(ymd);
            res = true;
        }

        EndDisabled();
        PopStyleColor(2);
        PopStyleVar();

        // Table full of DD with 7 cols for Mo, Tu, We, Th, Fr, Sa, Su and several rows for the dates
        // eg  1  2  3  4  5  6  7
        //     8  9 10 11 12 13 14
        //    15 16 17 18 19 20 21
        if (BeginTable(std::string("##Table_" + myLabel).c_str(), 7, table_flags)) {
            for (const auto& day : DAYS)
                TableSetupColumn(day.c_str());

            PushStyleColor(ImGuiCol_HeaderHovered, GetStyleColorVec4(ImGuiCol_TableHeaderBg));
            PushStyleColor(ImGuiCol_HeaderActive, GetStyleColorVec4(ImGuiCol_TableHeaderBg));
            TableHeadersRow();
            PopStyleColor(2);

            TableNextRow();
            TableSetColumnIndex(0);

            int month = monthIdx + 1;
            int firstDayOfMonth = DayOfWeek(1, month, year);
            int numDaysInMonth = NumDaysInMonth(month, year);
            int numWeeksInMonth = NumWeeksInMonth(month, year);

            for (int i = 1; i <= numWeeksInMonth; ++i) {
                for (const auto& day : CalendarWeek(i, firstDayOfMonth, numDaysInMonth)) {
                    if (day != 0) {
                        const bool selected = day == GET_DAY(ymd);
                        if (!selected) {
                            PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                            PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                        }

                        if (Button(std::to_string(day).c_str(), ImVec2(GetContentRegionAvail().x, 
                                                        GetTextLineHeightWithSpacing() + 5.0f))) {
                            ymd[2] = day; // v = EncodeTimePoint(day, month, year);
                            res = true;
                            CloseCurrentPopup();
                        }
                        if (!selected)
                            PopStyleColor(2);
                    }

                    if (day != numDaysInMonth)
                        TableNextColumn();
                }
            }
            EndTable();
        }
        EndCombo();
    }
    return res;
}
    
bool BufferingBar(const char* label, float value,  const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
       
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = size_arg;
    size.x -= style.FramePadding.x * 2;
        
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;
       
    // Render
    const float circleStart = size.x * 0.7f;
    const float circleEnd = size.x;
    const float circleWidth = circleEnd - circleStart;
        
    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart*value, bb.Max.y), fg_col);
        
    const float t = (float)g.Time;
    const float r = size.y / 2;
    const float speed = 1.5f;
        
    const float a = speed*0;
    const float b = speed*0.333f;
    const float c = speed*0.666f;
        
    const float o1 = (circleWidth+r) * (t+a - speed * (int)((t+a) / speed)) / speed;
    const float o2 = (circleWidth+r) * (t+b - speed * (int)((t+b) / speed)) / speed;
    const float o3 = (circleWidth+r) * (t+c - speed * (int)((t+c) / speed)) / speed;
        
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
    return true;
}

bool Spinner(const char* label, float radius, int thickness, int color)
{
    const ImU32 col = ImGui::GetColorU32(color);
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) {
        printf("spinner: window->SkipItems true\n");
        return false;
    }
        
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
       
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius )*2, (radius + style.FramePadding.y)*2);
       
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id)) {            
        printf("spinner: ItemAdd failed\n");
        return false;
    }
        
    // Render
    window->DrawList->PathClear();
       
    int num_segments = 30;
    int start = (int)abs(ImSin((float)g.Time*1.8f)*(num_segments-5));
        
    const float a_min = IM_PI*2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI*2.0f * ((float)num_segments-3) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x+radius, pos.y+radius+style.FramePadding.y);
        
    for (int i = 0; i < num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + (float)g.Time * 8) * radius,
            centre.y + ImSin(a + (float)g.Time * 8) * radius));
    }

    window->DrawList->PathStroke(col, 0, thickness);
    return true;
}

}   // namespace ImGui
