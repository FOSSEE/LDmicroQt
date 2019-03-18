#include "linuxUI.h"
#include <iostream>

using namespace std;

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

LPVOID HeapAlloc(HANDLE hHeap, DWORD  dwFlags, SIZE_T dwBytes)
{
    // if (hHeap == NULL)
    // {
        // printf("Alloc**********NULL HEAP***************\n");
        LPVOID p = malloc(dwBytes);
        return p;
    // }

    // auto it = std::find_if(HeapRecords.begin(), HeapRecords.end(),  [&hHeap](HEAPRECORD &Record) { return Record.hHeap == hHeap; });
    
    // if (it == HeapRecords.end())
        // return NULL;

    // if ((*it).dwAllocatedSizeOffset + dwBytes > (*it).dwSize)
    // {
    //     if ((*it).dwMaximumSize != 0)
    //         if((*it).dwAllocatedSizeOffset + dwBytes > (*it).dwMaximumSize)
    //             return NULL;
        
    //     (*it).hHeap = realloc((*it).hHeap, (*it).dwAllocatedSizeOffset + dwBytes);
    //     hHeap = (*it).hHeap;
    // }

    // /// HEAP_ZERO_MEMORY is set by default
    // DWORD flags = MAP_ANONYMOUS;

    // // if ( !((dwFlags & HEAP_ZERO_MEMORY) == HEAP_ZERO_MEMORY) )
    // //     flags = MAP_ANONYMOUS | MAP_UNINITIALIZED;

    // /* Use for setting a meamory chunck with some value
    //  * void * memset ( void * ptr, int value, size_t num );
    // */
    // LPVOID p = mmap(hHeap + (*it).dwAllocatedSizeOffset, dwBytes, PROT_EXEC, flags, -1, 0);

    // if (p == NULL)
    //     return NULL;
    
    // (*it).dwAllocatedSizeOffset += dwBytes;
    // HEAPCHUNCK chunck;
    // chunck.Chunck = p;
    // chunck.dwSize = dwBytes;
    // (*it).Element.push_back(chunck);
    
    // return p;
}

BOOL HeapFree(HANDLE hHeap, DWORD  dwFlags, LPVOID lpMem)
{
    /// if NULL free()
    // if (hHeap == NULL)
    // {
        // printf("free*********NULL HEAP***************\n");
        free(lpMem);
        return TRUE;
    // }
    // auto heap_it = std::find_if(HeapRecords.begin(), HeapRecords.end(),  [&hHeap](HEAPRECORD &Record) { return Record.hHeap == hHeap; });

    // if (heap_it == HeapRecords.end())
    //     return FALSE;
    
    // auto chunck_it = std::find_if((*heap_it).Element.begin(), (*heap_it).Element.end(),  [&lpMem](HEAPCHUNCK &Chunck) { return Chunck.Chunck == lpMem; });
    
    // if (chunck_it == (*heap_it).Element.end())
    //     return FALSE;
    
    // int result = munmap((*chunck_it).Chunck, (*chunck_it).dwSize);

    // if (result == 0)
    // {
    //     (*heap_it).Element.erase(chunck_it);
    //     return TRUE;
    // }
    // else 
    //     return FALSE;

}

HICON LoadImage(HINSTANCE hinst, LPCTSTR lpszName, UINT uType, int cxDesired,
    int cyDesired, UINT fuLoad)
{
    HICON pixbuf;
    GError *error = NULL;
    pixbuf = gdk_pixbuf_new_from_file(lpszName, &error);

    if(!pixbuf) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
    }

    return pixbuf;
}

void RECT_to_GDRECT(const RECT *rc, GDRECT *gdrc)
{
    // gdrc->x = rc->left;
    // gdrc->y = rc->top;
    // gdrc->width = rc->right - rc->left;
    // gdrc->height = rc->bottom - rc->top;
}

void OutputDebugString(char* str)
{

}

double GetTickCount(void) 
{
//   timespec now;
//   clock_gettime()
//   if (clock_gettime(CLOCK_MONOTONIC, &now))
//     return 0;
  return 10.2;//now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}
