#pragma once
#include "commons.h"

// Shortcut to dereference a pointer from an array as an integer
#define _DFINT *(int*)

typedef struct {
    char* data;
    size_t size;
    size_t unit_size;
} VarArray, *PVarArray, **LPVarArray;

// Initialize an array with no elements inside of size 1*unit_size bytes
// Returns 0 if it succeeds, -1 otherwise
int ARRAY_Create(PVarArray array_ptr, size_t unit_size);

// Appends a value to the array
int ARRAY_Append(PVarArray array_ptr, void* data);

// Returns the element at a certain index in the array
// Returns NULL if the index is out of bounds
void* ARRAY_Index(PVarArray array_ptr, uint64_t index);

// Removes element at index from array.
// WARNING Prevents fragmentation by therefore moving all objects after the index by 1 index position
// Avoid usage with complex structures using self-references and refresh all external pointers that could reference an element of the array
// Returns 0 if it succeeds, -1 otherwise
int ARRAY_Remove(PVarArray array_ptr, uint64_t index);

// Deletes the array and frees the memory
void ARRAY_Delete(PVarArray array_ptr);
