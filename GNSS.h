#pragma once
#include <Sensorsapi.h>
#include <Sensors.h>
#include <initguid.h>
#include <FunctionDiscoveryKeys.h>
#include <propkeydef.h>
#include <propvarutil.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

HRESULT hr = S_OK;
FILE* file1;

// Segádváltozók
ISensorManager* pSensorManager = NULL;
ISensorCollection* pSensorColl = NULL;
ISensor* pSensor = NULL;
ISensorDataReport* pReport = NULL;

int selected_sensor_index = -1, sensor_current_count = 0, sensor_update_active = -1;
double sensor_longitude = 0, sensor_latitude = 0, sensor_altitude = 0, sensor_accuracy = 0, sensor_speed = 0;

typedef struct sensor_details {
 char sensor_name[256];
 int real_index;
};
sensor_details szenzorinfok[10];

//Segédfüggvények
template <class T> void SafeRelease(T** ppT);
void init_GNSS(void);
void release_GNSS(void);
void get_sensor_values(int sensor_index);
void display_sensor_values(HWND display_objektum);
void log_init(void);
void log_location_data(void);

//Inicializálás
void init_GNSS(void)
{
 int i;
 int sensor_count = 0;
 for (i = 0; i < 10; ++i)
 {
  szenzorinfok[i].real_index = 0;
  strcpy_s(szenzorinfok[i].sensor_name, "");
 }
 LoadLibraryA("SensorsApi.dll");
 if (CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK) return;
 // Szenzormenedzser létrehozása
 hr = CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSensorManager));

 if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DISABLED_BY_POLICY)) return;

 // Szenzorok lekérése
 hr = pSensorManager->GetSensorsByCategory(SENSOR_CATEGORY_LOCATION, &pSensorColl);
 if (SUCCEEDED(hr))
 {
  ULONG ulCount = 0;

  hr = pSensorColl->GetCount(&ulCount);

  if (SUCCEEDED(hr))
  {
   if (ulCount < 1)
   {
    sensor_count = 0;
    return;
   }
   else
   {
    sensor_count = ulCount;
    if (sensor_count > 10) sensor_count = 10;
   }
  }
 }

 for (i = 0; i < sensor_count; ++i)
 {
  if (SUCCEEDED(hr))
  {
   hr = pSensorColl->GetAt(i, &pSensor);
  }

  if (SUCCEEDED(hr))
  {
   SensorState state = SENSOR_STATE_NOT_AVAILABLE;
   hr = pSensor->GetState(&state);

   if (SUCCEEDED(hr))
   {
    if (state == SENSOR_STATE_ACCESS_DENIED)
    {
     hr = pSensorManager->RequestPermissions(0, pSensorColl, FALSE);

     if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ||
      hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return;
    }
    else
    {
     if (SUCCEEDED(hr))
     {
      hr = pSensor->GetData(&pReport);
     }
     if (SUCCEEDED(hr))
     {
      PROPVARIANT var = {};

      hr = pSensor->GetProperty(SENSOR_PROPERTY_MODEL, &var);
      szenzorinfok[sensor_current_count].real_index = i;
      {
       int hossz = wcslen(var.bstrVal) + 1;
       WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, szenzorinfok[sensor_current_count].sensor_name, hossz, NULL, NULL);
      }
      ++sensor_current_count;
     }
    }
   }
  }
 }

 return;
}

//Memóriaterületek felszabadítása
void release_GNSS(void)
{
 SafeRelease(&pSensorManager);
 SafeRelease(&pSensorColl);
 SafeRelease(&pSensor);
 SafeRelease(&pReport);
}

template <class T> void SafeRelease(T** ppT)
{
 if (*ppT)
 {
  (*ppT)->Release();
  *ppT = NULL;
 }
}

//Szenzorértékek lekérdezése
void get_sensor_values(int sensor_index)
{
 SensorState state = SENSOR_STATE_NOT_AVAILABLE;
 hr = pSensor->GetState(&state);
 if (SUCCEEDED(hr)) hr = pSensorColl->GetAt(szenzorinfok[sensor_index].real_index, &pSensor);
 if (SUCCEEDED(hr)) hr = pSensor->GetData(&pReport);
 if (SUCCEEDED(hr))
 {
  PROPVARIANT var = {};

  hr = pSensor->GetProperty(SENSOR_PROPERTY_MODEL, &var);
  if (SUCCEEDED(hr)) hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_ERROR_RADIUS_METERS, &var);
  sensor_accuracy = var.dblVal;
  if (SUCCEEDED(hr)) hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_LONGITUDE_DEGREES, &var);
  sensor_longitude = var.dblVal;
  if (SUCCEEDED(hr)) hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_LATITUDE_DEGREES, &var);
  sensor_latitude = var.dblVal;
  if (SUCCEEDED(hr)) hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_ALTITUDE_ELLIPSOID_METERS, &var);
  sensor_altitude = var.dblVal;
  if (SUCCEEDED(hr)) hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_SPEED_METERS_PER_SECOND, &var);
  sensor_speed = var.dblVal;
 }
}

//Szenzorértékek megjelenításe
void display_sensor_values(HWND display_objektum)
{
 char szoveg[1024], szoveg2[64];
 strcpy_s(szoveg, "Sensor values:");
 sprintf_s(szoveg2, "%f", sensor_accuracy);
 strcat_s(szoveg, "\r\nAccuracy: "); strcat_s(szoveg, szoveg2);
 sprintf_s(szoveg2, "%f", sensor_longitude);
 strcat_s(szoveg, "\r\nLongitude: "); strcat_s(szoveg, szoveg2);
 sprintf_s(szoveg2, "%f", sensor_latitude);
 strcat_s(szoveg, "\r\nLatitude: "); strcat_s(szoveg, szoveg2);
 sprintf_s(szoveg2, "%f", sensor_altitude);
 strcat_s(szoveg, "\r\nAltitude: "); strcat_s(szoveg, szoveg2);
 sprintf_s(szoveg2, "%f", sensor_speed);
 strcat_s(szoveg, "\r\nSpeed: "); strcat_s(szoveg, szoveg2);
 SetWindowTextA(display_objektum, szoveg);
}

//Naplózás inicializálása
void log_init(void)
{
 fopen_s(&file1, "GPS_data.txt", "wt");
 if (file1 == NULL) return;
 fprintf_s(file1, "ACCURACY,LONGITUDE,LATITUDE,ALTITUDE,SPEED (M/S)\r\n");
 fclose(file1);
}

//Naplózás
void log_location_data(void)
{
 char szoveg[1024], szoveg2[64];
 fopen_s(&file1, "GPS_data.txt", "at");
 if (file1 == NULL) return;
 strcpy_s(szoveg, ""); strcpy_s(szoveg2, "");
 sprintf_s(szoveg2, "%f", sensor_accuracy);
 strcat_s(szoveg, szoveg2); strcat_s(szoveg, ",");
 sprintf_s(szoveg2, "%f", sensor_longitude);
 strcat_s(szoveg, szoveg2); strcat_s(szoveg, ",");
 sprintf_s(szoveg2, "%f", sensor_latitude);
 strcat_s(szoveg, szoveg2); strcat_s(szoveg, ",");
 sprintf_s(szoveg2, "%f", sensor_altitude);
 strcat_s(szoveg, szoveg2); strcat_s(szoveg, ",");
 sprintf_s(szoveg2, "%f", sensor_speed);
 strcat_s(szoveg, szoveg2); strcat_s(szoveg, "\r\n");
 fprintf_s(file1,"%s",szoveg);
 fclose(file1);
}
