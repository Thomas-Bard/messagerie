#include "../includes/data_types.h"

int ARRAY_Create(PVarArray array_ptr, size_t unit_size) {
    array_ptr->size = 1;
    array_ptr->unit_size = unit_size;
    if ((array_ptr->data = (char*)calloc(1, unit_size)) == NULL) return -1;
    return 0;
}

int ARRAY_Append(PVarArray array_ptr, void* element) {
    void* temp = realloc(array_ptr->data, (array_ptr->size + 1)*array_ptr->unit_size);
    if (temp == NULL) return -1;
    array_ptr->data = (char*)temp;
    memcpy(array_ptr->data + array_ptr->size*array_ptr->unit_size, element, array_ptr->unit_size);
    array_ptr->size++;
    return 0;
}

void* ARRAY_Index(PVarArray array_ptr, uint64_t index) {
    if (index >= array_ptr->size) return NULL;
    return &array_ptr->data[index*array_ptr->unit_size];
}

int ARRAY_Remove(PVarArray array_ptr, uint64_t index) {
    if (index >= array_ptr->size) return -1;
    memset(&array_ptr->data[index*array_ptr->unit_size], 0x00, array_ptr->unit_size);
    if (index != array_ptr->size - 1) {
        memcpy(&array_ptr->data[index*array_ptr->unit_size], &array_ptr->data[(index+1)*array_ptr->unit_size], (array_ptr->size - index)*array_ptr->unit_size);
        memset(array_ptr->data + (array_ptr->size - 1)*array_ptr->unit_size, 0x00, array_ptr->unit_size);
    }
    void* temp = realloc(array_ptr->data, (array_ptr->size-1)*array_ptr->unit_size);
    if (temp == NULL) return -2;
    array_ptr->size--;
    return 0;
}

void ARRAY_Delete(PVarArray array_ptr) {
    free(array_ptr->data);
    memset(array_ptr, 0x00, sizeof(VarArray));
}