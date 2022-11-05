#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <string.h>
#include <malloc.h>
#include <vector>
struct v3 {
    float x, y, z;
};

struct Action{
    unsigned char action;
    unsigned int player;
    unsigned int entityCount;
    unsigned int *entities;
    unsigned int targetEntity;
    v3 float_data;
    unsigned int data1, data2, data3, data4;
};

//a simple and limited implementation of vector
template <typename T>
class vector{
    #define VECTOR_BLOCK_SIZE 2000

private:
    unsigned int _size;
    unsigned int _allocatedSize;
    T *memBlocks[1000];
    void alloc(void){
        int block = _size / VECTOR_BLOCK_SIZE;
        memBlocks[block] = (T *)malloc(sizeof(T) * VECTOR_BLOCK_SIZE);
        _allocatedSize += VECTOR_BLOCK_SIZE;
    }

public:
    vector(){
        _size = 0;
        _allocatedSize = 0;
        memset(memBlocks, 0, 1000*sizeof(T*));
    }
    ~vector(){
        int blocks = _allocatedSize / VECTOR_BLOCK_SIZE;
        for(int i = 0; i < blocks; i++){
            free(memBlocks[i]);
        }
    }
    T& operator[](unsigned int index){
        if (index >= _size) {
            std::cout << "Assert: Vector index out of bound in operator[].";
            exit(0);
        }
        int block = index / VECTOR_BLOCK_SIZE;
        int b_index = index % VECTOR_BLOCK_SIZE;
        return memBlocks[block][b_index];
    }
    void clear(void){
        _size = 0;
    }
    void push_back(void){
        if(_size >= _allocatedSize){
            alloc();
        }

        int block = _size / VECTOR_BLOCK_SIZE;
        int b_index = _size % VECTOR_BLOCK_SIZE;
        memset(&memBlocks[block][b_index], 0, sizeof(T));
        ++_size;
    }
    T &back(void){
        if(_size == 0) {
            std::cout << "Assert: Vector::back() failed. No element available.";
            exit(0);
        }
        return (*this)[_size-1];
    }
    unsigned int size(void){
        return _size;
    }
};


#endif
