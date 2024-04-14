#include "thread_cache.h"
#include "central_cache.h"
void *thread_cache::allocate(size_t size) {
    size_t index = size_class::index(size); // 计算索引号
    free_list * freeList = &_free_list[index]; // 指向索引号
    if (!freeList->empty()) // 里面还有空间
        return freeList->pop(); // 分配
     else
        return fetch_from_central_cache(index,size_class::round_up(size)); // 向中心申请
}
void thread_cache::deallocate(void *ptr, size_t size) {
    size_t index = size_class::index(size);
    free_list *freeList = &_free_list[index];
    freeList->push(ptr); // 回收空间
    if (freeList->size() >= freeList->max_size())  // 如果回收的太多了
        list_tool_long(freeList, size); // 释放空间
}
void *thread_cache::fetch_from_central_cache(size_t index, size_t size) {
    free_list * freeList = &_free_list[index]; // 自由块
    size_t max_size = freeList->max_size();
    size_t num_to_move = std::min(size_class::num_move_size(size),max_size);
    void * start = nullptr,* end = nullptr;
    size_t batch_size = central_cache::get_instance()->fetch_range(start,end,num_to_move,size);// 申请空间
    if (batch_size > 1) // 申请了多余空间
        freeList->push_range(next_obj(start),end,batch_size - 1); // 放入自由链表
    if (batch_size >= freeList->max_size())
        freeList->set_max_size(max_size + 1);
    return start;
}
void thread_cache::list_tool_long(free_list *list, size_t size) {
    void * start = list->pop_range(); // 归还全部空间
    central_cache::get_instance()->release_list_to_spans(start,size); // 合并空间
}