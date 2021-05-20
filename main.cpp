#include <Windows.h>
#include <commctrl.h>
#include <windowsx.h>
#pragma comment(lib, "Comctl32.lib")    // линковка либы для spine control (CreateUpDownControl)

#include <string>
#include <vector>
#include <array>


// глобальная таблица хар-ков техники
// techProperty[m] - харрактеристики техники m
// techProperty[m][0] - радиус поражения
// techProperty[m][1] - ценность 
// techProperty[m][2] - максимальное количество
#define TECHTYPECOUNT 3
std::array<int, TECHTYPECOUNT> techProperty[] = {
    {40, 100, 20},
    {40, 200, 16},
    {40, 300, 13},
};

// типы техники
enum TechType {
    tank,
    launcher,
    contactpoint,
};

// структура транспорта
struct Tech {
    int x, y;       // положение
    TechType type;  // тип

    Tech() {}
    Tech(TechType t) {
        type = t;
    }

    // \return радиус поражения
    inline int r() const{
        return techProperty[type][0];
    }

    // \return ценность
    inline int v() const{
        return techProperty[type][1];
    }

    // \return максимальное количество
    inline int maxCount() const{
        return techProperty[type][2];
    }
}
// заготовка
// по выбору типа техники здесь будет содержаться макет
techBillet;

// список техники
std::array<std::vector<Tech>, TECHTYPECOUNT> techlist;

// дескрипторы переключателей
#define RADIOBUT_MENU(num) (100 + num)          // n-ная кнопка
#define RADIOBUT_INT(but) (but - 100)           // номер кнопки
#define FIRST_RADIOBUT_MENU RADIOBUT_MENU(0)    // первая кнопка
#define LAST_RADIOBUT_MENU RADIOBUT_MENU(TECHTYPECOUNT - 1) // последняя кнопка
std::array<HWND, TECHTYPECOUNT> hRadioBut;

// дескрипторы кнопки
#define FINDBUT_MENU    200
#define DESTROYBUT_MENU 201
#define REFRESHBUT_MENU 202
#define APPLYBUT_MENU   203
HWND hFindBut;      // поиск цели
HWND hDestroyBut;   // аттака цели
HWND hRefreshBut;   // сброс
HWND hApplyBut;     // принять настройки

// дескрипторы полей харрактеристик
std::array<HWND, TECHTYPECOUNT> hPropRadius;
std::array<HWND, TECHTYPECOUNT> hPropValue;
std::array<HWND, TECHTYPECOUNT> hPropCount;


// дескриптор приложения
HINSTANCE hInst;

// скорость перемещения(в пикселях монитора) и зума(в процентах)
#define MOVE_SPEED 18
#define ZOOM_SPEED 7

// окно под карту
#define BG_SIZE_W 600
#define BG_SIZE_H 600
const RECT render = { 300, 20, 300 + BG_SIZE_W, 20 + BG_SIZE_H };


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    static POINT target = { -1, -1 };       // цель, отрицательные координаты - цель не задана
    static long long result_value = 0;      // максимальная ценность, 0 - цель нельзя выбрать
    static std::vector<Tech> selectedTech;  // техника, находящааяся "под боем"

    static float zoom = 1.0;    // приближение
    static int w_shift = 0;     // сдвиг карты по х
    static int h_shift = 0;     // сдвиг карты по у

    switch (uMsg) {
    // создание всех кнопок
    case WM_CREATE:
    {
        // раскидываются переключатели
        hRadioBut[0] = CreateWindow(L"button", L"Танки",
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTORADIOBUTTON,
            30, 30, 200, 50,
            hWnd, (HMENU)RADIOBUT_MENU(TechType::tank), hInst, NULL);

        hRadioBut[1] = CreateWindow(L"button", L"Пусковые установки",
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTORADIOBUTTON,
            30, 79, 200, 50,
            hWnd, (HMENU)RADIOBUT_MENU(TechType::launcher), hInst, NULL);

        hRadioBut[2] = CreateWindow(L"button", L"Пункт связи",
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTORADIOBUTTON ,
            30, 128, 200, 50,
            hWnd, (HMENU)RADIOBUT_MENU(TechType::contactpoint), hInst, NULL);

        // отмечает переключатель "танки" по умолчанию
        CheckRadioButton(hWnd, FIRST_RADIOBUT_MENU, LAST_RADIOBUT_MENU, RADIOBUT_MENU(TechType::tank));
        techBillet.type = TechType::tank;


        // раскидываются кнопки
        hFindBut = CreateWindow(L"button", L"Найти цель",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            30, 207, 200, 50,
            hWnd, (HMENU)FINDBUT_MENU, hInst, NULL);

        hDestroyBut = CreateWindow(L"button", L"Уничтожить цель",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            30, 256, 200, 50,
            hWnd, (HMENU)DESTROYBUT_MENU, hInst, NULL);

        hRefreshBut = CreateWindow(L"button", L"Очистить поле",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            30, 334, 200, 50,
            hWnd, (HMENU)REFRESHBUT_MENU, hInst, NULL);

        hApplyBut = CreateWindow(L"button", L"Принять настройки",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            930, 480, 200, 50,
            hWnd, (HMENU)APPLYBUT_MENU, hInst, NULL);

        // раскидываются поля натроек 
        for (TechType tech = TechType::tank; tech < TECHTYPECOUNT; tech = TechType((int)tech + 1)) {
            int x = render.left + BG_SIZE_W + 130;
            int y = 30 + (int)tech * 150;
            // описание
            std::wstring name;
            if (tech == tank) name = L"Танки";
            else if (tech == launcher) name = L"Пусковые установки";
            else if (tech == contactpoint) name = L"Пункт связи";

            CreateWindow(L"edit", name.c_str(),
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 200, 20,
                hWnd, NULL, hInst, NULL);

            x += 30;

            // поле ввода радиуса
            y += 30;
            hPropRadius[tech] = CreateWindow(L"edit", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                x, y, 100, 20,
                hWnd, NULL, hInst, NULL);
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
                0, 0, 0, 0,
                hWnd, NULL, hInst,
                hPropRadius[tech], BG_SIZE_W / 2, 0, Tech(tech).r());

            CreateWindow(L"edit", L"Радиус",
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 100, 20,
                hWnd, NULL, hInst, NULL);

            // поле ввода ценности
            y += 30;
            hPropValue[tech] = CreateWindow(L"edit", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                x, y, 100, 20,
                hWnd, NULL, hInst, NULL);
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
                0, 0, 0, 0,
                hWnd, NULL, hInst,
                hPropValue[tech], INT16_MAX, 0, Tech(tech).v());
            CreateWindow(L"edit", L"Ценность",
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 100, 20,
                hWnd, NULL, hInst, NULL);

            // поле ввода количества
            y += 30;
            hPropCount[tech] = CreateWindow(L"edit", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_NUMBER,
                x, y, 100, 20,
                hWnd, NULL, hInst, NULL);
            CreateUpDownControl(WS_CHILD | WS_VISIBLE | WS_BORDER | UDS_ALIGNRIGHT | UDS_SETBUDDYINT,
                0, 0, 0, 0,
                hWnd, NULL, hInst,
                hPropCount[tech], INT16_MAX, 0, Tech(tech).maxCount());
            CreateWindow(L"edit", L"Количество",
                WS_CHILD | ES_LEFT | WS_VISIBLE | ES_READONLY,
                x - 100, y, 100, 20,
                hWnd, NULL, hInst, NULL);
        }

        break;
    }

    // отрисовка
    case WM_PAINT:
    {
        // подгрузка фона
        static HBITMAP hBgImage = (HBITMAP)LoadImage(NULL, L"bg.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        // набор кистей и перьев
        static HBRUSH bg_brush = CreateHatchBrush(HS_BDIAGONAL, RGB(0, 0, 0));
        static HBRUSH tank_brush = CreateSolidBrush(RGB(88, 128, 236));
        static HBRUSH launcher_brush = CreateSolidBrush(RGB(236, 88, 128));
        static HBRUSH contactpoint_brush = CreateSolidBrush(RGB(236, 226, 88));

        static HPEN red_pen = CreatePen(PS_INSIDEFRAME, 2, RGB(200, 0, 0));
        static HPEN blue_pen = CreatePen(PS_INSIDEFRAME, 1, RGB(112, 165, 255));
        static HPEN orange_pen = CreatePen(PS_INSIDEFRAME, 1, RGB(255, 202, 112));

        // контексты для главного окна и для фона
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);       
        HDC bmpDC = CreateCompatibleDC(NULL);

        SelectObject(hDC, GetStockObject(BLACK_PEN));

        // отрисовка изображения
        // изображение автоматически подстраивается под область получателя
        // и ее координаты устанавливаются так, чтоб не выходить за границы

        if (w_shift < 0) w_shift = 0;
        else if (BG_SIZE_W / zoom + w_shift > BG_SIZE_W) w_shift = BG_SIZE_W * (1.0 - 1 / zoom);
        if (h_shift < 0) h_shift = 0;
        else if (BG_SIZE_H / zoom + h_shift > BG_SIZE_H) h_shift = BG_SIZE_H * (1.0 - 1 / zoom);

        int x0 = w_shift;
        int y0 = h_shift;
        int x1 = BG_SIZE_W / zoom;
        int y1 = BG_SIZE_H / zoom;

        // отрисовка карты
        SelectObject(bmpDC, hBgImage);
        StretchBlt(hDC,                             // получатель
            render.left, render.top, BG_SIZE_W, BG_SIZE_H, // координаты области получателя
            bmpDC,                          // источник
            x0, y0, x1, y1,                 // координаты области источника
            SRCCOPY);                       // режим наложения
        DeleteDC(bmpDC);

        // отрисовка танков
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

        // отрисовка ракетной установки
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

        // отрисовка пункта связи
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

        // если ценность не равна 0, то есть цель
        // отрисовываем радиусы поражения
        if (result_value) {
            // голубые - не поражаемые
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

            // оранжевые - поражаемые
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

        // цель
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

    // ивенты клавиатуры
    case WM_KEYDOWN:
    {
        if (wParam == VK_UP) h_shift -= MOVE_SPEED;
        if (wParam == VK_RIGHT) w_shift += MOVE_SPEED;
        if (wParam == VK_DOWN) h_shift += MOVE_SPEED;
        if (wParam == VK_LEFT) w_shift -= MOVE_SPEED;
        InvalidateRect(hWnd, &render, false);
        break;
    }

    // колесико мышки
    case WM_MOUSEWHEEL:
    {
        static POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);

        // если мышь в прямоугольнике
        if (render.left < mousePos.x && mousePos.x < render.right and
            render.top < mousePos.y && mousePos.y < render.bottom) {
            static int zoom_p = 0;
            zoom_p += GET_WHEEL_DELTA_WPARAM(wParam) / abs(GET_WHEEL_DELTA_WPARAM(wParam));
            zoom = max(1.0, zoom = pow(1.0 + ZOOM_SPEED / 100.0, zoom_p));
            InvalidateRect(hWnd, &render, false);
        }
        break;
    }

    // левый клик мышки
    case WM_LBUTTONDOWN:
    {
        // позиция мышки относительно окна
        static POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);
        

        // если клик был по карте
        if (render.left < mousePos.x && mousePos.x < render.right and
            render.top < mousePos.y && mousePos.y < render.bottom) {

            // перевод координат окна в координаты поля
            techBillet.x = (mousePos.x - render.left) / zoom + w_shift;
            techBillet.y = (mousePos.y - render.top) / zoom + h_shift;

            // установка новой техники на поле, если не превышен ее лимит
            if (techlist[techBillet.type].size() < techBillet.maxCount()) {
                techlist[techBillet.type].push_back(techBillet);
                // сброс цели
                target = { -1, -1 };
                // сброс ценности
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

    // ивенты окон
    case WM_COMMAND:
    {
        // ивенты от переключателей, меняет тип болваки
        if (FIRST_RADIOBUT_MENU <= wParam && wParam <= LAST_RADIOBUT_MENU) {
            techBillet.type = (TechType)RADIOBUT_INT(wParam);
            SetFocus(hWnd);
        };

        // ивент от кнопки сброса
        if (REFRESHBUT_MENU == wParam) {
            // очищает список техники
            for (auto& list : techlist) list.clear();
            // сброс цели
            target = { -1, -1 };
            // сброс ценности
            result_value = 0;
            
            SetFocus(hWnd);
            InvalidateRect(hWnd, &render, false);
        }

        // ивент уничтожения
        if (DESTROYBUT_MENU == wParam) {
            // если единица техники в пределах поражения, то удаляем ее
            decltype(techlist) buftechlist;
            for (int i = 0 ; i < TECHTYPECOUNT; i++){
                for (int j = 0; j < techlist[i].size(); j++) {
                    auto& tech = techlist[i][j];
                    // если обьект НЕ уничтожен, помещвем его в буффер
                    if (pow(target.x - tech.x, 2) + pow(target.y - tech.y, 2) > pow(tech.r(), 2))
                        buftechlist[i].push_back(tech);
                }
            }
            // перезаписываем список техники
            techlist = buftechlist;
            // сброс цели
            target = { -1, -1 };
            // сброс ценности
            result_value = 0;

            SetFocus(hWnd);
            InvalidateRect(hWnd, &render, false);
        }

        // ивент от кнопки принятия настроек
        if (APPLYBUT_MENU == wParam) {
            wchar_t buf[256];

            // проверка количества техники, если количество на поле больше указанного
            // кидает предупреждение
            for (TechType tech = TechType::tank; tech < TECHTYPECOUNT; tech = TechType((int)tech + 1)) {
                GetWindowText(hPropCount[tech], buf, 256);
                if (_wtoi(buf) < techlist[tech].size()) {
                    int answ = MessageBox(hWnd,
                        L"Количество объектов на поле больше указанного количества.Если продолжить, поле будет сброшено",
                        L"Предупреждение",
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
            // если прошло проверку, то применяются настройки
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

        // начинает поиск цели по нажатие кнопки поиска
        if (FINDBUT_MENU == wParam) {
            // проверка на наличие техники
            bool have_tech = false;
            for (auto& list : techlist)
                if (list.size()) have_tech = true;
            if (not have_tech) {
                MessageBox(hWnd,
                    L"На поле нет техники",
                    L"Предупреждение",
                    MB_OK | MB_ICONEXCLAMATION);
                return NULL;
            }

            // алгоритм поиска цели
            for (auto& list : techlist) {
                for (auto& unit : list) {
                    // Достаточно проверять только точки лежащие на окружности потому,
                    // что если n кругов взаимно пересекаются, то и все их окружности
                    // тоже взаимно пересекаются.
                    //
                    // Сложность алгоритма примерно равна равна k * n^2
                    // где k - среднее арифметическое от длинн окружностей(радиусов поражения)
                    //
                    // Берем единицу техники (unit), а далее точку на окружности радиуса 
                    // поражения этой единицы техникии (utin). Теперь проверяем пересечение 
                    // точки с зонами поражения остальных единиц. Если расстояние от точки
                    // до координат другой единицы техники (other) не больше радиуса поражения
                    // этой самое единицы (other), то пересечение есть, а значит увеличиваем 
                    // ценность точки на показатель этой единицы (other)
                    for (int y = unit.y - unit.r(); y < unit.y + unit.r(); y++) {
                        for (int x = unit.x - unit.r(); x < unit.x + unit.r(); x++) {

                            // если точка не лежит на окружности, пропускаем 
                            if (pow(x - unit.x, 2) + pow(y - unit.y, 2) != unit.r() * unit.r()) continue;

                            // если лежит
                            long long value = 0;
                            std::vector<Tech> bufselectedTech;
                            for (auto& list : techlist) {
                                for (auto& other : list) {
                                    // расстояние между точкой на окружности и техникой
                                    float dist = sqrt(powf(x - other.x, 2) + powf(y - other.y, 2));

                                    // если расстояние меньше радиуса "другой" единицы техники, 
                                    // то пересечения есть, а значит увеличиваем ценность точки
                                    if (dist <= other.r()) {
                                        value += other.v();
                                        bufselectedTech.push_back(other);
                                    }
                                }
                            }
                            // если точка получилась "самой ценной", то определяем ее как цель
                            // и запоминаем всю технику, которую эта точка поражает (это 
                            // понадобится чуть позже, но НЕ для уничтожения техники)
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
            // ДАЛЕЕ МОЯ ОТСЕБЯТИНА (в шаблоне этого небыло)

            // Все довольно просто, общая область всех кругов не может быть больше
            // радиуса наименьшего круга. Необходимо пройтись по всем условным*
            // точкам в этом радиусе и определить, какая самая выгодная.
            // Самая выгодна точка имеет наименьшее среденее арифметическое от
            // расстояний до центров всех кругов.
            // Все это направлено на центрирование точки внутри общей области.
            //
            // *В данном случае условная точка равна 1 пикселю фона при масштабе 
            // 1.0, если условная точка будет равна 2, то колличество итерраций
            // уменьшится в 2^2 раза, но погрешность составит +-(2 - 1) пикселей.


            // минимальный радиус
            int min_r = INT32_MAX;
            for (auto& tech : selectedTech)
                if (tech.r() < min_r) min_r = tech.r();

            // проход по области с радиусом min_r и центром в target
            float min_armc_r = INT32_MAX; // среднее арифметическое радиусов
            POINT result_target = { -1, -1 };
            for (int y = target.y - min_r; y < target.y + min_r; y++) {
                for (int x = target.x - min_r; x < target.x + min_r; x++) {
                    // проверка координаты
                    long long value = 0;
                    float armc_r = 0;
                    for (auto& tech : selectedTech) {
                        float dist = sqrt(powf(x - tech.x, 2) + powf(y - tech.y, 2));
                        if (dist < tech.r()) {
                            value += tech.v();
                            armc_r += dist;
                        }
                    }

                    // Если ценность все такая же и среднее арифметическое всех 
                    // расстояний до радиусов меньше, то кордината выгоднее
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
                (L"Итоговая ценность точки: " + std::to_wstring(result_value)).c_str(),
                L"сообщение",
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

    // регистрация главного окна
    HWND hMainWnd = CreateWindow( L"myClass", L"Приложение",
        WS_OVERLAPPEDWINDOW &~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 680,
        NULL, NULL, hInst, NULL);
    if(!hMainWnd) return NULL;

    // регистрация таймера
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