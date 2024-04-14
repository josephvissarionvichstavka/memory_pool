#ifndef UNTITLED1_CENTRAL_CACHE_H
#define UNTITLED1_CENTRAL_CACHE_H
#pragma once
#include "common.h"
class central_cache {
    span_list _span_list[N_LISTS];
    static central_cache _inst;
    central_cache(){}
    central_cache(central_cache&) = delete ;
public:
    static central_cache * get_instance(){
        return &_inst;
    }
    span * get_one_to_page_cache(span_list & list,size_t size);
    size_t fetch_range(void *&start,void * &end,size_t n,size_t size);
    void  release_list_to_spans(void * start,size_t size);
};
#endif //UNTITLED1_CENTRAL_CACHE_H
