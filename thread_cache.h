#ifndef UNTITLED1_THREAD_CACHE_H
#define UNTITLED1_THREAD_CACHE_H
#include "common.h"
class thread_cache {
    free_list _free_list[N_LISTS];
public:
    void * allocate(size_t size); // 开辟内存空间
    void deallocate(void * ptr,size_t size); // 释放内存空间
    void * fetch_from_central_cache(size_t index,size_t size); // 索要内存
    void list_tool_long(free_list * list,size_t size); // 大额内存归还
};
__declspec (thread) static thread_cache* thread_list = nullptr;// 局部静态变量
#endif //UNTITLED1_THREAD_CACHE_H
