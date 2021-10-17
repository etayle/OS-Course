#include <unistd.h>

void* smalloc(size_t size){
    if(size > 100000000 or size == 0){
        return NULL;
    }
    void* return_address = sbrk(size);
    return return_address == (void*)-1 ? NULL : return_address;
}