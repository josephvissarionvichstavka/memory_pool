//
// Created by father on 2024/4/14.
//

#ifndef UNTITLED1_PAGE_CACHE_H
#define UNTITLED1_PAGE_CACHE_H

#include "common.h"

class page_cache {
    span_list _span_list[N_PAGES]; // 页数组
    std::unordered_map<page_id ,span *> _id_span_map; // 映射表
    std::mutex mutex; // 锁
    static page_cache _inst; // 单例模式
    page_cache(){}
public:
    static page_cache * get_instance(){
        return &_inst;
    }
    span * alloc_big_page(size_t size); // 申请大页内存
    void free_big_page(void * ptr,span * span); // 释放大页内存
    span * _new_span(size_t n); // 新建页
    span * new_span(size_t n); // 新建页
    span * map_to_span(void * obj);// 查找映射表
    void release_span_to_page_cache(span * _span); // 合并整理空间
};
#endif //UNTITLED1_PAGE_CACHE_H
