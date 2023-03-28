#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <commctrl.h>
#include "GNSS.h"

#define ID_TIMER1 999 //ütemezés intervalluma
//GUI vezérlők azonosítói
#define OBJ_ID101 101
#define OBJ_ID104 104
#define OBJ_ID105 105
#define OBJ_ID106 106

/*globális változók a GUI-hoz*/
#define HIBA_00 TEXT("Error:Window couldn't be registered.")
#define IDC_STATIC -1
#define LVM_SELECTED 2
#define WINSTYLE_NORMAL (WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX)
#define WINSTYLE_DIALOG (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU)
#define WINSTYLE_NONESIZEABLE (WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX)
#define WINSTYLE_CAPTIONONLY (WS_DLGFRAME)
#define WINSTYLE_NOCAPTON (WS_POPUP)

HINSTANCE hInstGlob;
int SajatiCmdShow;

LRESULT CALLBACK WndProc_GNSS_Setup(HWND, UINT, WPARAM, LPARAM);

HWND GNSS_Setup_Window;
HWND GNSSConfig_Button;
HWND Sensor_upd_intvl;
HWND Apply_upd_intvl;
HWND Stop_update;
HWND ComboBox_Sensorlist;
HWND Sensor_live_data;

//Segédfüggvények
void ShowMessage(LPCTSTR uzenet, LPCTSTR cim, HWND kuldo);
void Main_Close(void);
void update_sensor_list(void);
void apply_sensor_update_interval(void);

//Főprogram
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
 static TCHAR szAppName[] = TEXT("StdWinClassName");
 HWND hwnd;
 MSG msg;
 WNDCLASS wndclass3;
 LoadLibraryA("COMCTL32.DLL");
 SajatiCmdShow = iCmdShow;
 hInstGlob = hInstance;

 init_GNSS();

 wndclass3.style = CS_HREDRAW | CS_VREDRAW;
 wndclass3.lpfnWndProc = WndProc_GNSS_Setup;
 wndclass3.cbClsExtra = 0;
 wndclass3.cbWndExtra = 0;
 wndclass3.hInstance = hInstance;
 wndclass3.hIcon = LoadIcon(NULL, IDI_APPLICATION);
 wndclass3.hCursor = LoadCursor(NULL, IDC_ARROW);
 wndclass3.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
 wndclass3.lpszMenuName = NULL;
 wndclass3.lpszClassName = TEXT("WIN3");

 if (!RegisterClass(&wndclass3))
 {
  MessageBox(NULL, HIBA_00, TEXT("Program Start"), MB_ICONERROR); return 0;
 }

 GNSS_Setup_Window = CreateWindow(TEXT("WIN3"),
  TEXT("GNSS tracker"),
  WINSTYLE_DIALOG,
  50,
  400,
  435,
  314,
  NULL,
  NULL,
  hInstance,
  NULL);

 ShowWindow(GNSS_Setup_Window, SajatiCmdShow);
 UpdateWindow(GNSS_Setup_Window);

 while (GetMessage(&msg, NULL, 0, 0))
 {
  TranslateMessage(&msg);
  DispatchMessage(&msg);
 }
 return msg.wParam;
}

//Ablak eseménykezelője
LRESULT CALLBACK WndProc_GNSS_Setup(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
 int s1;
 HDC hdc;
 PAINTSTRUCT ps;
 RECT rect;

 switch (message)
 {
 case WM_CREATE:
  /*Initializing*/;
  ComboBox_Sensorlist = CreateWindow(TEXT("combobox"), NULL
   , WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST, 50, 20, 320, 150
   , hwnd, (HMENU)(OBJ_ID104), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Sensor_live_data = CreateWindow(TEXT("edit"), TEXT("")
   , WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE, 50, 55, 320, 130
   , hwnd, (HMENU)(OBJ_ID101), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  SetWindowTextA(Sensor_live_data, "Sensor live data");
  Sensor_upd_intvl = CreateWindow(TEXT("edit"), TEXT("1000")
   , WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 50, 200, 100, 20
   , hwnd, (HMENU)(IDC_STATIC), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Apply_upd_intvl = CreateWindow(TEXT("button"), TEXT("Apply interval")
   , WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE, 160, 200, 120, 30
   , hwnd, (HMENU)(OBJ_ID105), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  Stop_update = CreateWindow(TEXT("button"), TEXT("STOP")
   , WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE, 290, 200, 80, 30
   , hwnd, (HMENU)(OBJ_ID106), ((LPCREATESTRUCT)lParam)->hInstance, NULL);
  log_init();
  update_sensor_list();
  return 0;

 case WM_TIMER:
  switch (wParam)
  {
  case ID_TIMER1:
   get_sensor_values(selected_sensor_index);
   display_sensor_values(Sensor_live_data);
   log_location_data();
   break;
  }
  return 0;

 case WM_COMMAND:
  if (LOWORD(wParam) == OBJ_ID104 && HIWORD(wParam) == CBN_SELCHANGE)
  {
   selected_sensor_index = SendMessageA(ComboBox_Sensorlist, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
   if (selected_sensor_index == 0) { selected_sensor_index = -1; KillTimer(GNSS_Setup_Window, ID_TIMER1); }
   else selected_sensor_index -= 1;
   if (selected_sensor_index > -1)
   {
    get_sensor_values(selected_sensor_index);
    display_sensor_values(Sensor_live_data);
   }   
  }
  switch (LOWORD(wParam))
  {/*Processing commands*/;
  case OBJ_ID105:apply_sensor_update_interval();
   break;
  case OBJ_ID106:KillTimer(GNSS_Setup_Window, ID_TIMER1);
   break;
  }
  return 0;

 case WM_SIZE:
  return 0;

 case WM_PAINT:
  hdc = BeginPaint(hwnd, &ps);
  EndPaint(hwnd, &ps);
  return 0;

 case WM_CLOSE:
  Main_Close();
  DestroyWindow(hwnd);
  return 0;

 case WM_DESTROY:
  PostQuitMessage(0);
  return 0;
 }
 return DefWindowProc(hwnd, message, wParam, lParam);
}

void ShowMessage(LPCTSTR uzenet, LPCTSTR cim, HWND kuldo)
{
 MessageBox(kuldo, uzenet, cim, MB_OK);
}

//Segédfüggvények

//Ütemező leállítása
void Main_Close(void)
{
 KillTimer(GNSS_Setup_Window, ID_TIMER1);
 release_GNSS();
}

//Szenzorok lekérdezése
void update_sensor_list(void)
{
 int i;
 SendMessageA(ComboBox_Sensorlist, CB_RESETCONTENT, 0, 0);
 SendMessageA(ComboBox_Sensorlist, CB_ADDSTRING, 0, (LPARAM)"Select a GNSS sensor");
 for (i = 0; i < sensor_current_count; ++i)
 {
  SendMessageA(ComboBox_Sensorlist, CB_ADDSTRING, 0, (LPARAM)szenzorinfok[i].sensor_name);
 }
 SendMessageA(ComboBox_Sensorlist, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
}

//Ütemező indítása és leállítása
void apply_sensor_update_interval(void)
{
 if (selected_sensor_index == -1) return;
 char szoveg[16];
 int upd_interval;
 GetWindowTextA(Sensor_upd_intvl, szoveg, GetWindowTextLengthA(Sensor_upd_intvl) + 1);
 upd_interval = atoi(szoveg);
 if (upd_interval < 100) upd_interval = 100;
 if (sensor_update_active == 1) KillTimer(GNSS_Setup_Window, ID_TIMER1);
 else sensor_update_active = 1;
 SetTimer(GNSS_Setup_Window, ID_TIMER1, upd_interval, (TIMERPROC)NULL);
}
