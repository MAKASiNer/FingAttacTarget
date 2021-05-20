#include <Windows.h>
#include <commctrl.h>
#include <windowsx.h>
#pragma comment(lib, "Comctl32.lib")    // �������� ���� ��� spine control (CreateUpDownControl)

#include <string>
#include <vector>
#include <array>


// ���������� ������� ���-��� �������
// techProperty[m] - ��������������� ������� m
// techProperty[m][0] - ������ ���������
// techProperty[m][1] - �������� 
// techProperty[m][2] - ������������ ����������
#define TECHTYPECOUNT 3
std::array<int, TECHTYPECOUNT> techProperty[] = {
    {40, 100, 20},
    {40, 200, 16},
    {40, 300, 13},
};

// ���� �������
enum TechType {
    tank,
    launcher,
    contactpoint,
};

// ��������� ����������
struct Tech {
    int x, y;       // ���������
    TechType type;  // ���

    Tech() {}
    Tech(TechType t) {
        type = t;
    }

    // \return ������ ���������
    inline int r() const{
        return techProperty[type][0];
    }

    // \return ��������
    inline int v() const{
        return techProperty[type][1];
    }

    // \return ������������ ����������
    inline int maxCount() const{
        return techProperty[type][2];
    }
}
// ���������
// �� ������ ���� ������� ����� ����� ����������� �����
techBillet;

// ������ �������
std::array<std::vector<Tech>, TECHTYPECOUNT> techlist;

// ����������� ��������������
#define RADIOBUT_MENU(num) (100 + num)          // n-��� ������
#define RADIOBUT_INT(but) (but - 100)           // ����� ������
#define FIRST_RADIOBUT_MENU RADIOBUT_MENU(0)    // ������ ������
#define LAST_RADIOBUT_MENU RADIOBUT_MENU(TECHTYPECOUNT - 1) // ��������� ������
std::array<HWND, TECHTYPECOUNT> hRadioBut;

// ����������� ������
#define FINDBUT_MENU    200
#define DESTROYBUT_MENU 201
#define REFRESHBUT_MENU 202
#define APPLYBUT_MENU   203
HWND hFindBut;      // ����� ����
HWND hDestroyBut;   // ������ ����
HWND hRefreshBut;   // �����
HWND hApplyBut;     // ������� ���������

// ����������� ����� ��������������
std::array<HWND, TECHTYPECOUNT> hPropRadius;
std::array<HWND, TECHTYPECOUNT> hPropValue;
std::array<HWND, TECHTYPECOUNT> hPropCount;


// ���������� ����������
HINSTANCE hInst;

// �������� �����������(� �������� ��������) � ����(� ���������)
#define MOVE_SPEED 18
#define ZOOM_SPEED 7

// ���� ��� �����
#define BG_SIZE_W 600
#define BG_SIZE_H 600
const RECT render = { 300, 20, 300 + BG_SIZE_W, 20 + BG_SIZE_H };


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    static POINT target = { -1, -1 };       // ����, ������������� ���������� - ���� �� ������
    static long long result_value = 0;      // ������������ ��������, 0 - ���� ������ �������
    static std::vector<Tech> selectedTech;  // �������, ������������ "��� ����"

    static float zoom = 1.0;    // �����������
    static int w_shift = 0;     // ����� ����� �� �
    static int h_shift = 0;     // ����� ����� �� �

    switch (uMsg) {
    // �������� ���� ������
    case WM_CREATE:
    {
        // ������������� �������������
        hRadioBut[0] = CreateWindow(L"button", L"�����",
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTORADIOBUTTON,
            30, 30, 200, 50,
            hWnd, (HMENU)RADIOBUT_MENU(TechType::tank), hInst, NULL);

        hRadioBut[1] = CreateWindow(L"button", L"�������� ���������",
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTORADIOBUTTON,
            30, 79, 200, 50,
            hWnd, (HMENU)RADIOBUT_MENU(TechType::launcher), hInst, NULL);

        hRadioBut[2] = CreateWindow(L"button", L"����� �����",
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTORADIOBUTTON ,
            30, 128, 200, 50,
            hWnd, (HMENU)RADIOBUT_MENU(TechType::contactpoint), hInst, NULL);

        // �������� ������������� "�����" �� ���������
        CheckRadioButton(hWnd, FIRST_RADIOBUT_MENU, LAST_RADIOBUT_MENU, RADIOBUT_MENU(TechType::tank));
        techBillet.type = TechType::tank;


        // ������������� ������
        hFindBut = CreateWindow(L"button", L"����� ����",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            30, 207, 200, 50,
            hWnd, (HMENU)FINDBUT_MENU, hInst, NULL);

        hDestroyBut = CreateWindow(L"button", L"���������� ����",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            30, 256, 200, 50,
            hWnd, (HMENU)DESTROYBUT_MENU, hInst, NULL);

        hRefreshBut = CreateWindow(L"button", L"�������� ����",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            30, 334, 200, 50,
            hWnd, (HMENU)REFRESHBUT_MENU, hInst, NULL);

        hApplyBut = CreateWindow(L"button", L"������� ���������",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            930, 480, 200, 50,
            hWnd, (HMENU)APPLYBUT_MENU, hInst, NULL);

        // ������������� ���� ������� 
        for (TechType tech = TechType::tank; tech < TECHTYPECOUNT; tech = TechType((int)tech + 1)) {
            int x = render.left + BG_SIZE_W + 130;
            int y = 30 + (int)tech * 150;
            // ��������
            std::wstring name;
            if (tech == tank) name = L"�����";
            else if (tech == launcher) name = L"�������� ���������";
            else if (tech == contactpoint) name = L"����� �����";

            CreateWindow(L"edit", name.c_str(),
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 200, 20,
                hWnd, NULL, hInst, NULL);

            x += 30;

            // ���� ����� �������
            y += 30;
            hPropRadius[tech] = CreateWindow(L"edit", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                x, y, 100, 20,
                hWnd, NULL, hInst, NULL);
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
                0, 0, 0, 0,
                hWnd, NULL, hInst,
                hPropRadius[tech], BG_SIZE_W / 2, 0, Tech(tech).r());

            CreateWindow(L"edit", L"������",
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 100, 20,
                hWnd, NULL, hInst, NULL);

            // ���� ����� ��������
            y += 30;
            hPropValue[tech] = CreateWindow(L"edit", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                x, y, 100, 20,
                hWnd, NULL, hInst, NULL);
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
                0, 0, 0, 0,
                hWnd, NULL, hInst,
                hPropValue[tech], INT16_MAX, 0, Tech(tech).v());
            CreateWindow(L"edit", L"��������",
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 100, 20,
                hWnd, NULL, hInst, NULL);

            // ���� ����� ����������
            y += 30;
            hPropCount[tech] = CreateWindow(L"edit", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                x, y, 100, 20,
                hWnd, NULL, hInst, NULL);
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
                0, 0, 0, 0,
                hWnd, NULL, hInst,
                hPropCount[tech], INT16_MAX, 0, Tech(tech).maxCount());
            CreateWindow(L"edit", L"����������",
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 100, 20,
                hWnd, NULL, hInst, NULL);
        }

        break;
    }

    // ���������
    case WM_PAINT:
    {
        // ��������� ����
        static HBITMAP hBgImage = (HBITMAP)LoadImage(NULL, L"bg.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        // ����� ������ � ������
        static HBRUSH bg_brush = CreateHatchBrush(HS_BDIAGONAL, RGB(0, 0, 0));
        static HBRUSH tank_brush = CreateSolidBrush(RGB(88, 128, 236));
        static HBRUSH launcher_brush = CreateSolidBrush(RGB(236, 88, 128));
        static HBRUSH contactpoint_brush = CreateSolidBrush(RGB(236, 226, 88));

        static HPEN red_pen = CreatePen(PS_INSIDEFRAME, 2, RGB(200, 0, 0));
        static HPEN blue_pen = CreatePen(PS_INSIDEFRAME, 1, RGB(112, 165, 255));
        static HPEN orange_pen = CreatePen(PS_INSIDEFRAME, 1, RGB(255, 202, 112));

        // ��������� ��� �������� ���� � ��� ����
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);       
        HDC bmpDC = CreateCompatibleDC(NULL);

        SelectObject(hDC, GetStockObject(BLACK_PEN));

        // ��������� �����������
        // ����������� ������������� �������������� ��� ������� ����������
        // � �� ���������� ��������������� ���, ���� �� �������� �� �������

        if (w_shift < 0) w_shift = 0;
        else if (BG_SIZE_W / zoom + w_shift > BG_SIZE_W) w_shift = BG_SIZE_W * (1.0 - 1 / zoom);
        if (h_shift < 0) h_shift = 0;
        else if (BG_SIZE_H / zoom + h_shift > BG_SIZE_H) h_shift = BG_SIZE_H * (1.0 - 1 / zoom);

        int x0 = w_shift;
        int y0 = h_shift;
        int x1 = BG_SIZE_W / zoom;
        int y1 = BG_SIZE_H / zoom;

        // ��������� �����
        SelectObject(bmpDC, hBgImage);
        StretchBlt(hDC,                             // ����������
            render.left, render.top, BG_SIZE_W, BG_SIZE_H, // ���������� ������� ����������
            bmpDC,                          // ��������
            x0, y0, x1, y1,                 // ���������� ������� ���������
            SRCCOPY);                       // ����� ���������
        DeleteDC(bmpDC);

        // ��������� ������
        SelectObject(hDC, tank_brush);
        static const int tank_r = 4;
        for (auto& tank : techlist[TechType::tank]) {
            Ellipse(hDC,
                (tank.x - w_shift) * zoom - tank_r + render.left,
                (tank.y - h_shift) * zoom - tank_r + render.top,
                (tank.x - w_shift) * zoom + tank_r + render.left,
                (tank.y - h_shift) * zoom + tank_r + render.top
            );
        }

        // ��������� �������� ���������
        SelectObject(hDC, launcher_brush);
        static const int launcher_r = 4;
        for (auto& launcher : techlist[TechType::launcher]) {
            Ellipse(hDC,
                (launcher.x - w_shift)* zoom - launcher_r + render.left,
                (launcher.y - h_shift)* zoom - launcher_r + render.top,
                (launcher.x - w_shift)* zoom + launcher_r + render.left,
                (launcher.y - h_shift)* zoom + launcher_r + render.top
            );
        }

        // ��������� ������ �����
        SelectObject(hDC, contactpoint_brush);
        static const int contactpoint_r = 4;
        for (auto& contactpoint : techlist[TechType::contactpoint]) {
            Ellipse(hDC,
                (contactpoint.x - w_shift)* zoom - contactpoint_r + render.left,
                (contactpoint.y - h_shift)* zoom - contactpoint_r + render.top,
                (contactpoint.x - w_shift)* zoom + contactpoint_r + render.left,
                (contactpoint.y - h_shift)* zoom + contactpoint_r + render.top
            );
        }

        // ���� �������� �� ����� 0, �� ���� ����
        // ������������ ������� ���������
        if (result_value) {
            // ������� - �� ����������
            SelectObject(hDC, blue_pen);
            for (auto& list : techlist) {
                for (auto& tech : list) {
                    Arc(hDC,
                        (tech.x - w_shift - tech.r()) * zoom + render.left,
                        (tech.y - h_shift - tech.r()) * zoom + render.top,
                        (tech.x - w_shift + tech.r()) * zoom + render.left,
                        (tech.y - h_shift + tech.r()) * zoom + render.top,
                        0, 0, 0, 0);
                }
            }

            // ��������� - ����������
            SelectObject(hDC, orange_pen);
            for (auto& tech : selectedTech) {
                Arc(hDC,
                    (tech.x - w_shift - tech.r()) * zoom + render.left,
                    (tech.y - h_shift - tech.r()) * zoom + render.top,
                    (tech.x - w_shift + tech.r()) * zoom + render.left,
                    (tech.y - h_shift + tech.r()) * zoom + render.top,
                    0, 0, 0, 0);
            }
        }

        // ����
        if (target.x >= 0 and target.y >= 0) {
            SelectObject(hDC, red_pen);
            MoveToEx(hDC,
                (target.x - w_shift) * zoom - 10 + render.left,
                (target.y - h_shift) * zoom + 10 + render.top,
                NULL);
            LineTo(hDC,
                (target.x - w_shift) * zoom + 10 + render.left,
                (target.y - h_shift) * zoom - 10 + render.top);

            MoveToEx(hDC,
                (target.x - w_shift) * zoom - 10 + render.left,
                (target.y - h_shift) * zoom - 10 + render.top,
                NULL);
            LineTo(hDC,
                (target.x - w_shift) * zoom + 10 + render.left,
                (target.y - h_shift) * zoom + 10 + render.top);

        }
        EndPaint(hWnd, &ps);
        break;
    }

    // ������ ����������
    case WM_KEYDOWN:
    {
        if (wParam == VK_UP) h_shift -= MOVE_SPEED;
        if (wParam == VK_RIGHT) w_shift += MOVE_SPEED;
        if (wParam == VK_DOWN) h_shift += MOVE_SPEED;
        if (wParam == VK_LEFT) w_shift -= MOVE_SPEED;
        InvalidateRect(hWnd, &render, false);
        break;
    }

    // �������� �����
    case WM_MOUSEWHEEL:
    {
        static POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);

        // ���� ���� � ��������������
        if (render.left < mousePos.x && mousePos.x < render.right and
            render.top < mousePos.y && mousePos.y < render.bottom) {
            static int zoom_p = 0;
            zoom_p += GET_WHEEL_DELTA_WPARAM(wParam) / abs(GET_WHEEL_DELTA_WPARAM(wParam));
            zoom = max(1.0, zoom = pow(1.0 + ZOOM_SPEED / 100.0, zoom_p));
            InvalidateRect(hWnd, &render, false);
        }
        break;
    }

    // ����� ���� �����
    case WM_LBUTTONDOWN:
    {
        // ������� ����� ������������ ����
        static POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);
        

        // ���� ���� ��� �� �����
        if (render.left < mousePos.x && mousePos.x < render.right and
            render.top < mousePos.y && mousePos.y < render.bottom) {

            // ������� ��������� ���� � ���������� ����
            techBillet.x = (mousePos.x - render.left) / zoom + w_shift;
            techBillet.y = (mousePos.y - render.top) / zoom + h_shift;

            // ��������� ����� ������� �� ����, ���� �� �������� �� �����
            if (techlist[techBillet.type].size() < techBillet.maxCount()) {
                techlist[techBillet.type].push_back(techBillet);
                // ����� ����
                target = { -1, -1 };
                // ����� ��������
                result_value = 0;
                
                RECT rect = { 
                    techBillet.x + render.left - w_shift - 4, techBillet.y + render.top - h_shift - 4,
                    techBillet.x + render.left - w_shift + 4, techBillet.y + render.top - h_shift + 4
                };

                SetFocus(hWnd);
                InvalidateRect(hWnd, &render, false);
            }
        }
        break;
    }

    // ������ ����
    case WM_COMMAND:
    {
        // ������ �� ��������������, ������ ��� �������
        if (FIRST_RADIOBUT_MENU <= wParam && wParam <= LAST_RADIOBUT_MENU) {
            techBillet.type = (TechType)RADIOBUT_INT(wParam);
            SetFocus(hWnd);
        };

        // ����� �� ������ ������
        if (REFRESHBUT_MENU == wParam) {
            // ������� ������ �������
            for (auto& list : techlist) list.clear();
            // ����� ����
            target = { -1, -1 };
            // ����� ��������
            result_value = 0;
            
            SetFocus(hWnd);
            InvalidateRect(hWnd, &render, false);
        }

        // ����� �����������
        if (DESTROYBUT_MENU == wParam) {
            // ���� ������� ������� � �������� ���������, �� ������� ��
            decltype(techlist) buftechlist;
            for (int i = 0 ; i < TECHTYPECOUNT; i++){
                for (int j = 0; j < techlist[i].size(); j++) {
                    auto& tech = techlist[i][j];
                    // ���� ������ �� ���������, �������� ��� � ������
                    if (pow(target.x - tech.x, 2) + pow(target.y - tech.y, 2) > pow(tech.r(), 2))
                        buftechlist[i].push_back(tech);
                }
            }
            // �������������� ������ �������
            techlist = buftechlist;
            // ����� ����
            target = { -1, -1 };
            // ����� ��������
            result_value = 0;

            SetFocus(hWnd);
            InvalidateRect(hWnd, &render, false);
        }

        // ����� �� ������ �������� ��������
        if (APPLYBUT_MENU == wParam) {
            wchar_t buf[256];

            // �������� ���������� �������, ���� ���������� �� ���� ������ ����������
            // ������ ��������������
            for (TechType tech = TechType::tank; tech < TECHTYPECOUNT; tech = TechType((int)tech + 1)) {
                GetWindowText(hPropCount[tech], buf, 256);
                if (_wtoi(buf) < techlist[tech].size()) {
                    int answ = MessageBox(hWnd,
                        L"���������� �������� �� ���� ������ ���������� ����������.���� ����������, ���� ����� ��������",
                        L"��������������",
                        MB_OKCANCEL | MB_ICONEXCLAMATION);

                    if (answ == IDOK) {
                        for (auto& list : techlist) list.clear();
                        target = { -1, -1 };
                        result_value = 0;
                        break;
                    }
                    else if (answ == IDCANCEL) {
                        for (int i = 0; i < TECHTYPECOUNT; i++) {
                            SetWindowText(hPropRadius[i], std::to_wstring(Tech(TechType(i)).r()).c_str());
                            SetWindowText(hPropValue[i], std::to_wstring(Tech(TechType(i)).v()).c_str());
                            SetWindowText(hPropCount[i], std::to_wstring(Tech(TechType(i)).maxCount()).c_str());
                        }
                        return NULL;
                    }
                }
            }
            // ���� ������ ��������, �� ����������� ���������
            for (TechType tech = TechType::tank; tech < TECHTYPECOUNT; tech = TechType((int)tech + 1)) {
                GetWindowText(hPropRadius[tech], buf, 256);
                techProperty[tech][0] = _wtoi(buf);
                GetWindowText(hPropValue[tech], buf, 256);
                techProperty[tech][1] = _wtoi(buf);
                GetWindowText(hPropCount[tech], buf, 256);
                techProperty[tech][2] = _wtoi(buf);
            }
                

            SetFocus(hWnd);
            InvalidateRect(hWnd, &render, false);
        }

        // �������� ����� ���� �� ������� ������ ������
        if (FINDBUT_MENU == wParam) {
            // �������� �� ������� �������
            bool have_tech = false;
            for (auto& list : techlist)
                if (list.size()) have_tech = true;
            if (not have_tech) {
                MessageBox(hWnd,
                    L"�� ���� ��� �������",
                    L"��������������",
                    MB_OK | MB_ICONEXCLAMATION);
                return NULL;
            }

            // �������� ������ ����
            for (auto& list : techlist) {
                for (auto& unit : list) {
                    // ���������� ��������� ������ ����� ������� �� ���������� ������,
                    // ��� ���� n ������ ������� ������������, �� � ��� �� ����������
                    // ���� ������� ������������.
                    //
                    // ��������� ��������� �������� ����� ����� k * n^2
                    // ��� k - ������� �������������� �� ����� �����������(�������� ���������)
                    //
                    // ����� ������� ������� (unit), � ����� ����� �� ���������� ������� 
                    // ��������� ���� ������� �������� (utin). ������ ��������� ����������� 
                    // ����� � ������ ��������� ��������� ������. ���� ���������� �� �����
                    // �� ��������� ������ ������� ������� (other) �� ������ ������� ���������
                    // ���� ����� ������� (other), �� ����������� ����, � ������ ����������� 
                    // �������� ����� �� ���������� ���� ������� (other)
                    for (int y = unit.y - unit.r(); y < unit.y + unit.r(); y++) {
                        for (int x = unit.x - unit.r(); x < unit.x + unit.r(); x++) {

                            // ���� ����� �� ����� �� ����������, ���������� 
                            if (pow(x - unit.x, 2) + pow(y - unit.y, 2) != unit.r() * unit.r()) continue;

                            // ���� �����
                            long long value = 0;
                            std::vector<Tech> bufselectedTech;
                            for (auto& list : techlist) {
                                for (auto& other : list) {
                                    // ���������� ����� ������ �� ���������� � ��������
                                    float dist = sqrt(powf(x - other.x, 2) + powf(y - other.y, 2));

                                    // ���� ���������� ������ ������� "������" ������� �������, 
                                    // �� ����������� ����, � ������ ����������� �������� �����
                                    if (dist <= other.r()) {
                                        value += other.v();
                                        bufselectedTech.push_back(other);
                                    }
                                }
                            }
                            // ���� ����� ���������� "����� ������", �� ���������� �� ��� ����
                            // � ���������� ��� �������, ������� ��� ����� �������� (��� 
                            // ����������� ���� �����, �� �� ��� ����������� �������)
                            if (result_value < value) {
                                result_value = value;
                                target.x = x;
                                target.y = y;
                                selectedTech = bufselectedTech;
                            }
                        }
                    }                   
                }
            }
            // ����� ��� ���������� (� ������� ����� ������)

            // ��� �������� ������, ����� ������� ���� ������ �� ����� ���� ������
            // ������� ����������� �����. ���������� �������� �� ���� ��������*
            // ������ � ���� ������� � ����������, ����� ����� ��������.
            // ����� ������� ����� ����� ���������� �������� �������������� ��
            // ���������� �� ������� ���� ������.
            // ��� ��� ���������� �� ������������� ����� ������ ����� �������.
            //
            // *� ������ ������ �������� ����� ����� 1 ������� ���� ��� �������� 
            // 1.0, ���� �������� ����� ����� ����� 2, �� ����������� ���������
            // ���������� � 2^2 ����, �� ����������� �������� +-(2 - 1) ��������.


            // ����������� ������
            int min_r = INT32_MAX;
            for (auto& tech : selectedTech)
                if (tech.r() < min_r) min_r = tech.r();

            // ������ �� ������� � �������� min_r � ������� � target
            float min_armc_r = INT32_MAX; // ������� �������������� ��������
            POINT result_target = { -1, -1 };
            for (int y = target.y - min_r; y < target.y + min_r; y++) {
                for (int x = target.x - min_r; x < target.x + min_r; x++) {
                    // �������� ����������
                    long long value = 0;
                    float armc_r = 0;
                    for (auto& tech : selectedTech) {
                        float dist = sqrt(powf(x - tech.x, 2) + powf(y - tech.y, 2));
                        if (dist < tech.r()) {
                            value += tech.v();
                            armc_r += dist;
                        }
                    }

                    // ���� �������� ��� ����� �� � ������� �������������� ���� 
                    // ���������� �� �������� ������, �� ��������� ��������
                    armc_r /= selectedTech.size();
                    if (value == result_value and armc_r < min_armc_r) {
                        result_target.x = x;
                        result_target.y = y;
                        min_armc_r = armc_r;
                    }
                }
            }
            if (result_target.x != -1 and result_target.y != -1)
                target = result_target;

            SetFocus(hWnd);
            InvalidateRect(hWnd, &render, false);

            MessageBox(hWnd,
                (L"�������� �������� �����: " + std::to_wstring(result_value)).c_str(),
                L"���������",
                MB_OK);
            return NULL;
        }
        break;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(NULL);
        break;
    }
    default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return NULL;
}


INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    ::hInst = hInst;
    //InitCommonControls();

    WNDCLASSEX wc;
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = L"myClass";
    wc.cbWndExtra    = NULL;
    wc.cbClsExtra    = NULL;
    wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm       = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(240, 240, 240));
    wc.hInstance     = hInst;
    if(!RegisterClassEx(&wc)) return NULL;

    // ����������� �������� ����
    HWND hMainWnd = CreateWindow( L"myClass", L"����������",
        WS_OVERLAPPEDWINDOW &~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 680,
        NULL, NULL, hInst, NULL);
    if(!hMainWnd) return NULL;

    // ����������� �������
    SetTimer(hMainWnd, NULL, 10, NULL);

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    MSG msg;
    while(GetMessage(&msg, NULL, NULL, NULL)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}