#ifndef UNTITLED1_ALLOC_H
#define UNTITLED1_ALLOC_H
#pragma once
#include "common.h"
#include "thread_cache.h"
#include "page_cache.h"
static inline void * _alloc(size_t size){ // 分配内存
    if (size > MAX_BYTES){ // 改用大页分配
        span * _span = page_cache::get_instance()->alloc_big_page(size);
        void * ptr = (void *)(_span->_page_id << PAGE_SHIFT);
        return ptr;
    } else { // 小分配
        if (thread_list == nullptr) // 注册
            thread_list = new thread_cache;
        return thread_list->allocate(size);
    }
}
static inline void _free(void * ptr){ // 释放内存
    span * _span = page_cache::get_instance()->map_to_span(ptr);
    size_t size = _span->_object_size;
    if (size > MAX_BYTES){ // 释放页
        page_cache::get_instance()->free_big_page(ptr,_span);
    } else {
        thread_list->deallocate(ptr,size);
    }
}

#endif //UNTITLED1_ALLOC_H
