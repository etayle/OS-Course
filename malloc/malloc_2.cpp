#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;

void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _size_meta_data();
size_t _num_meta_data_bytes();

struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* free_head = NULL;
size_t allocated_blocks = 0;
size_t allocated_bytes = 0;
void* first_heap_address = sbrk(0); //TODO: I DO THIS TO TEST, we need to what in out method dont work
void* smalloc(size_t size){
    if(size > 100000000 || size == 0){
        return NULL;
    }
    size_t metadata_size = sizeof(MallocMetadata);
    MallocMetadata* current = free_head;
    while(current != NULL){
        if(current->size >= size){
            if(current == free_head){
                free_head = current->next;
            }
            else if(current->next == NULL){
                (current->prev)->next = current->next;
            }
            else{
                (current->prev)->next = current->next;
                (current->next)->prev = current->prev;
            }
            current->prev = NULL;
            current->next = NULL;
            current->is_free = false;
            allocated_blocks++;
            allocated_bytes += size + metadata_size;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    size_t to_allocate = metadata_size + size;
    void* return_address = sbrk(to_allocate);
    if(return_address==(void*)-1){
        return NULL;
    }
    MallocMetadata* metadata = (MallocMetadata*)return_address;
    metadata->size = size;
    metadata->is_free = false;
    metadata->next = NULL;
    metadata->prev = NULL;
    allocated_blocks++;
    allocated_bytes += size + metadata_size;
    return (void*)((MallocMetadata*)return_address + 1);
}

void* scalloc(size_t num, size_t size){
    MallocMetadata* address = (MallocMetadata*)smalloc(num*size);
    if(!address){
        return NULL;
    }
    memset(address, 0, num*size);
    return address;
}

void sfree(void* p){
    if(p == NULL){
        return;
    }
    MallocMetadata* free_metadata = (MallocMetadata*)p-1;
    if(free_metadata->is_free){
        return;
    }
    if(free_head == NULL){
        free_head = free_metadata;
        free_metadata->next = NULL;
        free_metadata->prev = NULL;
    }
    else{
        MallocMetadata* current = free_head;
        while(current->next && current < free_metadata){
            current = current->next;
        }
        if(current < free_metadata){
            current->next = free_metadata;
            free_metadata->prev = current;
            free_metadata->next = NULL;
        }
        else{
            if(current == free_head){
                free_head = free_metadata;
            }
            else{
                (current->prev)->next = free_metadata;
                free_metadata->prev = current->prev;
            }
            free_metadata->next = current;
            current->prev = free_metadata;
        }
    }
    free_metadata->is_free = true;
    allocated_blocks--;
    allocated_bytes -= free_metadata->size + sizeof(MallocMetadata);
}

void* srealloc(void* oldp, size_t size){
    if(oldp == NULL){
        return smalloc(size);
    }
    if(size > 100000000 || size == 0){
        return NULL;
    }
    MallocMetadata* metadata = (MallocMetadata*)oldp-1;
    if(metadata->size > size){
        return oldp;
    }
    void* newp = smalloc(size);
    if(!newp){
        return NULL;
    }
    memcpy(newp, oldp, metadata->size);
    sfree(oldp);
    return newp;
}

size_t _num_free_blocks(){
    MallocMetadata* current = free_head;
    size_t size = 0;
    while(current){
        size++;
        current = current->next;
    }
    return size;
}

size_t _num_free_bytes(){
    size_t size = 0;
    size_t metadata_size = sizeof(MallocMetadata);
    MallocMetadata* current = free_head;
    while(current){
        size += current->size;    //TODO: I change this from   size += metadata_size + current->size
                                 // to size += current->size
        current = current->next;
    }
    return size;
}

size_t _num_allocated_blocks(){
    return allocated_blocks + _num_free_blocks();
}

size_t _num_allocated_bytes(){
    // TODO : was only this row at older code : return allocated_bytes + _num_free_bytes() change method to debug
    void* current_sbr_adress = sbrk(0);
    size_t overall_allocated_bytes = (char*)current_sbr_adress - (char*)first_heap_address;
    return  overall_allocated_bytes - _size_meta_data()*_num_allocated_blocks();

}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}
size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * _size_meta_data();
}


