#include "linuxUI.h"
#include <iostream>

std::vector<HEAPRECORD> HeapRecords;

HANDLE HeapCreate(DWORD  flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
{
    HANDLE hHeap = NULL;
    HEAPRECORD hHeapRecord;
    hHeapRecord.dwMaximumSize = dwMaximumSize;
    // hHeap = malloc(dwInitialSize);

    if (hHeap == NULL)
        return NULL;
    
    hHeapRecord.dwSize = dwInitialSize;
    hHeapRecord.hHeap = hHeap;
    hHeapRecord.dwAllocatedSizeOffset = 0;
    hHeapRecord.HeapID = HeapRecords.size()+1;
    HeapRecords.push_back(hHeapRecord);

    return hHeap;
}

size_t max(size_t A, size_t B)
{
    if(A>B)
        return A;
    else
        return B;
}

LPVOID HeapAlloc(HANDLE hHeap, DWORD  dwFlags, SIZE_T dwBytes)
{
        LPVOID p = malloc(dwBytes);
        return p;
}

BOOL HeapFree(HANDLE hHeap, DWORD  dwFlags, LPVOID lpMem)
{
        free(lpMem);
        return TRUE;
}

void OutputDebugString(char* str)
{

}

double GetTickCount(void) 
{
  timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now))
    return 0;
  return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;

}