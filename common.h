#ifndef UNTITLED1_COMMON_H
#define UNTITLED1_COMMON_H
#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <windows.h>
#define MAX_BYTES 64*1024  // 一个线程申请的最大内存
#define N_LISTS  184  // 线程数组大小
#define PAGE_SHIFT 12
#define N_PAGES 129 // 页数量

inline static void* & next_obj(void * obj){ // 获取obj指向的对象的引用  反射
    return *((void **)obj);
}

class free_list {
    void * _list = nullptr; // 空闲对象首地址
    size_t _size = 0;  // 当前空闲数量
    size_t _max_size = 1; // 最大空闲数量
public:
    void push(void * obj){ // 压栈
        next_obj(obj) = _list;
        _list = obj;
        ++_size;
    }
    void push_range(void * start,void * end,size_t n){
        next_obj(end) = _list;
        _list = start;
        _size += n;
    }
    void * pop(){
        void * obj = _list;
        _list = next_obj(obj);
        --_size;
        return obj;
    }
    void * pop_range(){
        _size = 0;
        void * list = _list;
        _list = nullptr;
        return list;
    }
    bool empty(){
        return _list == nullptr;
    }
    size_t size() const {
        return _size;
    }
    size_t max_size() const  {
        return _max_size;
    }
    void set_max_size(size_t size){
        this->_max_size = size;
    }
};
class size_class {
public:
    inline static size_t _index(size_t size,size_t align){ // 对齐后的位数
        return ((size + (1 << align) - 1) >> align) - 1;
    }
    inline static size_t _round_up(size_t size,size_t align){ // 向上取整,到align的倍数
        return ((size + (1 << align) - 1)&~((1 << align) - 1));
    }
    inline static size_t index(size_t size){ // 返回索引数
        assert(size <= MAX_BYTES);
        static int group_array[] = {16,56,56,56}; // 每个区间有多少链
        if (size <= 128){
            return _index(size,3);
        } else if (size <= 1024){
            return _index(size - 128,4) + group_array[0];
        } else if (size <= 1024 * 8){
            return _index(size - 1024,7) + group_array[0] + group_array[1];
        } else if (size <= 1024 * 64){
            return _index(size - 8192,10) + group_array[0] + group_array[1] + group_array[2];
        }
    }
    inline static size_t round_up(size_t bytes){ // 返回字节数
        assert(bytes <= MAX_BYTES);
        if (bytes <= 128){
            return _round_up(bytes,3);
        } else if (bytes <= 1024){
            return _round_up(bytes,4);
        } else if (bytes <= 1024 * 8){
            return _round_up(bytes,7);
        } else if (bytes <= 1024 * 64){
            return _round_up(bytes,10);
        }
    }
    static size_t num_move_size(size_t size){ // 计算移动大小
        if (size == 0) return 0;
        int num = (MAX_BYTES) / size;
        if (num < 2) num = 2;
        if (num > 512) num = 512;
        return num;
    }
    static size_t num_move_page(size_t size){ // 计算移动页数
        size_t num = num_move_size(size);
        size_t n_page = num * size;
        n_page >>= PAGE_SHIFT;
        if (n_page == 0)
            n_page = 1;
        return n_page;
    }
};
#ifdef _WIN32
typedef size_t page_id ;
#elif _WIN64
typedef long long page_id;
#endif
struct span{
    page_id _page_id = 0;// 页号
    size_t _n_page = 0; // 页数
    span * _prev = nullptr;
    span * _next = nullptr;
    void * list = nullptr; // 自由链表
    size_t _object_size = 0; // 对象大小
    size_t _use_count = 0; // 对象使用计数
};
class span_list{
public:
    span * _head;
    std::mutex _mutex;
    span_list(){ // 双头链表
        _head = new span;
        _head->_next = _head;
        _head->_prev = _head;
    }
    span * begin(){
        return _head->_next;
    }
    span * end(){
        return _head;
    }
    span_list(const span_list&) = delete ;
    span_list & operator=(const span_list&) = delete ;
    ~span_list(){
        span * current = _head->_next;
        while (current != _head){
            span * next = current->_next;
            delete current;
            current = next;
        }
        delete _head;
        _head = nullptr;
    }
    void insert(span * current,span * new_span){
        span * _prev = current->_prev;
        _prev->_next = new_span;
        new_span->_next = current;
        new_span->_prev = _prev;
        current->_prev = new_span;
    }
    void erase(span * current){
        span * _prev = current->_prev;
        span * _next = current->_next;
        _prev->_next = _next;
        _next->_prev = _prev;
    }
    void push_back(span * new_span){
        insert(end(),new_span);
    }
    void push_fount(span * new_span){
        insert(begin(),new_span);
    }
    span * pop_back(){
        span * _span = _head->_prev;
        erase(_span);
        return _span;
    }
    span * pop_fount(){
        span * _span = _head->_next;
        erase(_span);
        return _span;
    }
    bool empty(){
        return _head->_next == _head;
    }
    void lock(){
        _mutex.lock();
    }
    void unlock(){
        _mutex.unlock();
    }
};


#endif //UNTITLED1_COMMON_H
