#include "central_cache.h"
#include "page_cache.h"
central_cache central_cache::_inst;
span *central_cache::get_one_to_page_cache(span_list &list, size_t size) {
    span * _span = list.begin(); // 遍历页链表
    while (_span != list.end()){
        if (_span->list != nullptr){
            return _span;
        } else {
            _span = _span->_next;
        }
    }
    // 没找到 申请一个
    span * new_span = page_cache::get_instance()->new_span(size_class::num_move_page(size));// 计算所需
    char * current = (char *)(new_span->_page_id << PAGE_SHIFT);
    char * end = current + (new_span->_n_page << PAGE_SHIFT);
    new_span->list = current;
    new_span->_object_size = size;
    while (current + size < end){
        char * next = current + size;
        next_obj(current) = next;
        current = next;
    }
    next_obj(current) = nullptr;
    list.push_fount(new_span);
    return new_span;
}
size_t central_cache::fetch_range(void *&start,void * &end,size_t n,size_t size){
    size_t index = size_class::index(size);
    span_list& list = _span_list[index]; // 返回索引值的引用
    _span_list->lock();
    std::unique_lock<std::mutex> lock(_span_list->_mutex); // 加锁
    span* _span = get_one_to_page_cache(list,size); // 新的页
    void * current = _span->list;
    void * prev = nullptr;
    int batch_size = 0;
    for (int i = 0; i < n; ++i) {
        prev = current;
        current = next_obj(current);
        ++batch_size;
        if (current == nullptr)
            break;
    }
    start = _span->list;
    end = prev;
    _span->list = current;
    _span->_use_count += batch_size;
    if (_span->list == nullptr){ // 链表是空的放后面
        _span_list->erase(_span);
        _span_list->push_back(_span);
    }
    _span_list->unlock();
    return batch_size;
}
void  central_cache::release_list_to_spans(void * start,size_t size){
    size_t index = size_class::index(size);
    span_list & spanList = _span_list[index];
    _span_list->lock();
    std::unique_lock<std::mutex> lock(_span_list->_mutex); // 加锁
    while (start){ // start不为空
        void * next = next_obj(start); // start下一个地址
        _span_list->lock();
        span * _span = page_cache::get_instance()->map_to_span(start); // 找到start的映射
        next_obj(start) = _span->list;
        _span->list = start;
        if (--(_span->_use_count) == 0){
            _span_list->erase(_span);
            page_cache::get_instance()->release_span_to_page_cache(_span);
        }
        _span_list->unlock();
        start = next;
    }
    _span_list->unlock();
}