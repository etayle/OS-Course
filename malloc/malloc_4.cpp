#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/mman.h>

using namespace std;

#define LARGE_SIZE 128*1024

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

MallocMetadata* allocated_head = NULL;
MallocMetadata* allocated_tail = NULL;
MallocMetadata* mmap_head = NULL;
size_t allocated_blocks = 0;
size_t allocated_bytes = 0;
void* first_heap_address = sbrk(0); //TODO: I DO THIS TO TEST, we need to what in out method dont work

void _insert_node(MallocMetadata* to_insert){
    if(allocated_head == NULL){
        allocated_head = to_insert;
        allocated_tail = to_insert;
        to_insert->next = NULL;
        to_insert->prev = NULL;
    }
    else{
        MallocMetadata* current;
        current = allocated_head;
        while(current->next && current < to_insert){
            current = current->next;
        }
        if(current < to_insert){
            current->next = to_insert;
            to_insert->prev = current;
            to_insert->next = NULL;
            allocated_tail = to_insert;
        }
        else{
            if(current == allocated_head){
                allocated_head = to_insert;
            }
            else{
                (current->prev)->next = to_insert;
                to_insert->prev = current->prev;
            }
            to_insert->next = current;
            current->prev = to_insert;
        }
    }
}

void _remove_node(MallocMetadata* to_remove){
    if(!allocated_head){
        return;
    }
    if(to_remove == allocated_head){
        allocated_head = to_remove->next;
    }
    else if(to_remove->next == NULL){
        (to_remove->prev)->next = to_remove->next;
        allocated_tail = to_remove->prev;
    }
    else{
        (to_remove->prev)->next = to_remove->next;
        (to_remove->next)->prev = to_remove->prev;
    }
}

void _remove_node_mmap(MallocMetadata* to_remove){
    if(!mmap_head){
        return;
    }
    if(to_remove == mmap_head){
        mmap_head = to_remove->next;
    }
    else if(to_remove->next == NULL){
        (to_remove->prev)->next = to_remove->next;
    }
    else{
        (to_remove->prev)->next = to_remove->next;
        (to_remove->next)->prev = to_remove->prev;
    }
}

void _add_node_mmap(MallocMetadata* to_add){
    to_add->next = mmap_head;
    if(mmap_head){
        mmap_head->prev = to_add;
    }
    mmap_head = to_add;
    to_add->prev = NULL;
}

void* _down_merge(MallocMetadata* to_merge){
    (to_merge->prev)->size += to_merge->size + _size_meta_data();
    _remove_node(to_merge);
    return to_merge->prev;
}

void* _up_merge(MallocMetadata* to_merge){
    to_merge->size += (to_merge->next)->size + _size_meta_data();
    _remove_node(to_merge->next);
    return to_merge;
}

void* _free_merge(MallocMetadata* to_merge){
    if(to_merge->next && (to_merge->next)->is_free){
        to_merge = (MallocMetadata*)_up_merge(to_merge);
    }
    if(to_merge->prev && (to_merge->prev)->is_free){
        to_merge = (MallocMetadata*)_down_merge(to_merge);
    }
    return to_merge;
}

void _split_memory(MallocMetadata* metadata, size_t size){
    size_t unused_space = metadata->size - size;
    if(unused_space >= 128 + _size_meta_data()){
        // splits the block to the allocated one and splited one
        metadata->size = size;
        metadata->is_free = false;
        MallocMetadata* splited = (MallocMetadata*)((char*)metadata + metadata->size + _size_meta_data());
        splited->size = unused_space - _size_meta_data();
        splited->is_free = false;
        _insert_node(splited);
        sfree(splited + 1);
    }
}

void* smalloc(size_t size){
    int left = size % 8;
    if(left!=0){
        size += 8 - left;
    }
    if(size > 100000000 || size == 0){
        return NULL;
    }
    if(size>=LARGE_SIZE){
        MallocMetadata* return_address = (MallocMetadata*)mmap(NULL, size+_size_meta_data(),
                PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(return_address==(void*)-1){
            return NULL;
        }
        return_address->size = size;
        return_address->is_free = false;
        _add_node_mmap(return_address);
        return (void*)((MallocMetadata*)return_address + 1);
    }
    size_t metadata_size = sizeof(MallocMetadata);
    MallocMetadata* current = allocated_head;
    while(current != NULL){
        if(current->size >= size && current->is_free){
            // allocate on previous freed block and maybe splits it
            _split_memory(current, size);
            current->is_free = false;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    void* return_address;
    bool expanded = false;
    if(allocated_tail && allocated_tail->is_free){
        // if we expand the last one
        size_t to_add = size - allocated_tail->size;
        return_address = sbrk(to_add);
        if(return_address==(void*)-1){
            return NULL;
        }
        return_address = allocated_tail;
        expanded = true;
    }
    else{
        // if we allocate a new block at the end
        size_t to_allocate = metadata_size + size;
        return_address = sbrk(to_allocate);
    }
    if(return_address==(void*)-1){
        return NULL;
    }
    MallocMetadata* metadata = (MallocMetadata*)return_address;
    metadata->size = size;
    metadata->is_free = false;
    if(!expanded){
        _insert_node(metadata);
    }
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
    if(free_metadata->size >= LARGE_SIZE){
        _remove_node_mmap(free_metadata);
        munmap(free_metadata, _size_meta_data() + free_metadata->size);
        return;
    }
    free_metadata->is_free = true;
    _free_merge(free_metadata);
}

void* srealloc(void* oldp, size_t size){
    if(oldp == NULL){
        return smalloc(size);
    }
    int left = size % 8;
    if(left!=0){
        size += 8 - left;
    }
    if(size > 100000000 || size == 0){
        return NULL;
    }
    bool merged = true;
    MallocMetadata* metadata = (MallocMetadata*)oldp-1;
    void* newp;
    if(size >= LARGE_SIZE){
        newp = smalloc(size);
        if(size < metadata->size){
            memcpy(newp, oldp, size);
        }
        else{
            memcpy(newp, oldp, metadata->size);
        }
        sfree(oldp);
        return newp;
    }
    else if(metadata->size >= size){
        newp = (MallocMetadata*)oldp-1;
    }
    else if(allocated_tail == metadata && sbrk(size - allocated_tail->size) != (void*)-1){
        metadata->size = size;
        return oldp;
    }
    else if(metadata->prev && metadata->prev->is_free && metadata->size + (metadata->prev)->size +
    _size_meta_data() >= size){
        newp = _down_merge(metadata);
    }
    else if(metadata->next && metadata->next->is_free && metadata->size + (metadata->next)->size +
    _size_meta_data() >= size){
        newp = _up_merge(metadata);
    }
    else if(metadata->prev && metadata->next && metadata->next->is_free && metadata->prev->is_free &&
    metadata->size + (metadata->next)->size + (metadata->prev)->size +
    _size_meta_data()*2 >= size){
        newp = _up_merge(metadata);
        newp = _down_merge((MallocMetadata*)newp);
    }
    else{
        merged = false;
        newp = smalloc(size);
    }
    if(!newp){
        return NULL;
    }
    if(!merged){
        memcpy(newp, oldp, metadata->size);
        sfree(oldp);
        return newp;
    }
    memcpy((MallocMetadata*)newp+1, oldp, metadata->size);
    _split_memory((MallocMetadata*)newp, size);
    ((MallocMetadata*)newp)->is_free=false;
    return (MallocMetadata*)newp+1;
}

size_t _num_free_blocks(){
    MallocMetadata* current = allocated_head;
    size_t size = 0;
    while(current){
        if(current->is_free) {
            size++;
        }
        current = current->next;
    }
    return size;
}

size_t _num_free_bytes(){
    size_t size = 0;
    MallocMetadata* current = allocated_head;
    while(current){
        if(current->is_free){
            size += current->size;
        }
        current = current->next;
    }
    return size;
}

size_t _num_allocated_blocks(){
    size_t size = 0;
    MallocMetadata* current = allocated_head;
    while(current){
        size++;
        current = current->next;
    }
    current = mmap_head;
    while(current){
        size++;
        current = current->next;
    }
    return size;
}

size_t _num_allocated_bytes(){
    size_t size = 0;
    MallocMetadata* current = allocated_head;
    while(current){
        size += current->size;
        current = current->next;
    }
    current = mmap_head;
    while(current){
        size += current->size;
        current = current->next;
    }
    return size;
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}
size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * _size_meta_data();
}


//int main() {
//    std::cout <<sizeof(MallocMetadata)<<std::endl;
//    std::cout <<smalloc(10)<<std::endl;
//    int* ptr = (int*)smalloc(32);
//    for (int i = 0; i < (32 / sizeof(int)); i++) {
//        ptr[i] = i;
//    }
//
//    for (int i = 0; i < (32 / sizeof(int)); i++) {
//        cout << ptr[i] << "  "; // needs to print 0 1 2 3 4 5 6 7
//    }
//    cout << endl;
//
//    int* ptr2 = (int*) srealloc(ptr, 32);
//    if (ptr2 == ptr) cout << "not suppose to happen";
//
//    for (int i = 0; i < (32 / sizeof(int)); i++) {
//        if (ptr2[i] != i) {
//            cout << "You have a mistake you stupid bronze 1 ";
//            cout << "expectd: " << i << " got: " << ptr2[i] << endl;
//        }
//    }
//    cout << endl;
//
//    int* ptrnew = (int*)smalloc(32);
//    if (ptrnew != ptr) cout << "your programming sucks. you free'd ptr so now ptrnew needs to get the same adrress..." << endl;
//
//    std::cout <<ptr<<std::endl;
//    // sfree (ptr);
//    ptr = (int*)smalloc(15);
//    std::cout <<ptr<<std::endl;
//    memset(ptr, 5 ,15);
//    //std::cout <<"nside ptr  :"<<((int)ptr)<<std::endl;
//    //std::cout <<"size ptr"<<alocationList->getSize(ptr)<<std::endl;
//    std::cout << srealloc(ptr,7) << std::endl;
//    std::cout << srealloc(ptr,25) << std::endl;
//    std::cout <<smalloc(18)<<std::endl;
//    ptr = (int*) scalloc(4,6);
//    std::cout <<ptr<<std::endl;
//
//    char* ptr3 = (char*)scalloc(1, 10);
//    for (int i = 0; i < 10; i++) {
//        if (ptr3[i] != 0) cout << "WTF you have a problem. A sheep programming for ISIS does a better job than you";
//    }
////    std::cout <<"size ptr"<<alocationList->getSize(ptr)<<std::endl;
//    return 0;
//}




