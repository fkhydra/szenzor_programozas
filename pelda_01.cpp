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
#include <iostream>

HRESULT hr = S_OK;

// Segédváltozók
ISensorManager* pSensorManager = NULL;
ISensorCollection* pSensorColl = NULL;
ISensor* pSensor = NULL;
ISensorDataReport* pReport = NULL;

typedef struct sensor_details {
 char sensor_name[256];
 int real_index;
};
sensor_details szenzorinfok[10];

ULONG sensor_count = 0;
double sensor_longitude = 0;
double sensor_latitude = 0;
double sensor_altitude = 0;
double sensor_accuracy = 0;
double sensor_speed = 0;

//Függvények
template <class T> void SafeRelease(T** ppT);
void init_GNSS(void);
void release_GNSS(void);
void get_sensor_values(int sensor_index);
void display_sensor_values(void);

int main()
{
 init_GNSS();
 //folyamatos frissítéshez
 //while (1)
 //{
 // get_sensor_values(0);
 // display_sensor_values();
 // Sleep(1000);
 // system("cls");
 //}
}

//Inicializálás
void init_GNSS(void)
{
 int i;
 int sensor_current_count = 0;

 for (i = 0; i < 10; ++i)
 {
  szenzorinfok[i].real_index = 0;
  strcpy_s(szenzorinfok[i].sensor_name, "");
 }
 LoadLibraryA("SensorsApi.dll");
 
 //COM könyvtár inicializalasa
 if (CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK) return;
 
 // A szenzor menedzser beállítása
 hr = CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSensorManager));

 if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DISABLED_BY_POLICY)) return;
 if (hr != S_OK) return;

 // Szenzoradatok lelkérése
 hr = pSensorManager->GetSensorsByCategory(SENSOR_CATEGORY_LOCATION, &pSensorColl);
 if (SUCCEEDED(hr))
 {
  hr = pSensorColl->GetCount(&sensor_count);

  if (SUCCEEDED(hr))
  {
   if (sensor_count < 1) return;
  }
 }

 for (i = 0; i < sensor_count; ++i)
 {
  //A listában soron következő szenzor lekérése
  if (SUCCEEDED(hr)) hr = pSensorColl->GetAt(i, &pSensor);
  printf_s("%i. szenzor adatai:\n",i+1);

  if (SUCCEEDED(hr))
  {
   // Szenzor állapotűnak ellenőrzése
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
      // Szenzor értékeinek kinyerése
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
       printf_s("%s\n", szenzorinfok[sensor_current_count].sensor_name);
      }
      get_sensor_values(i);
      display_sensor_values();
      ++sensor_current_count;
     }
    }
   }
  }
 }
}

//Szenzoradatok lekérdezése
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

//Szenzoradatok kiírása
void display_sensor_values(void)
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
 printf_s("%s\n\n", szoveg);
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
