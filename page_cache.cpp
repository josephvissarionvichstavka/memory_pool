#include "page_cache.h"
page_cache page_cache::_inst;
span *page_cache::alloc_big_page(size_t size) {
    assert(size > MAX_BYTES);
    size = size_class::_round_up(size,PAGE_SHIFT); // 内存对齐
    size_t n_page = size >> PAGE_SHIFT;
    if (n_page < N_PAGES){ // 页数在数组里
        span * _span = new_span(n_page);
        _span->_object_size = size;
        return _span;
    } else {
        void * ptr = VirtualAlloc(0,n_page << PAGE_SHIFT,
                MEM_COMMIT | MEM_RESERVE ,PAGE_READWRITE); // windows系统调用 动态分配内存
        if (ptr == nullptr){
            throw std::bad_alloc();
        }
        span * _span = new span;
        _span->_n_page = n_page;
        _span->_page_id = (page_id)ptr >> PAGE_SHIFT; // 唯一标识
        _span->_object_size = n_page << PAGE_SHIFT; // 原size
        _id_span_map[_span->_page_id] = _span; // 映射表
        return _span;
    }
}
void page_cache::free_big_page(void *ptr, span *span) {
    size_t n_page = span->_object_size >> PAGE_SHIFT; // 页数
    if (n_page < N_PAGES){ // 小于128页
        span->_object_size = 0;
        release_span_to_page_cache(span);
    } else {
        _id_span_map.erase(n_page); // 删除映射表
        delete span;
        VirtualFree(ptr,0,MEM_RELEASE);
    }
}
span *page_cache::_new_span(size_t n) { // 新建一个页
    assert(n < N_PAGES); // 页号小于最大值
    if (!_span_list[n].empty()){ // 在页链表里面，该页不是空的
        return _span_list[n].pop_fount(); // 返回第一个页
    }
    for (size_t i = n + 1; i < N_PAGES; ++i) { // 循环遍历
        if (!_span_list[i].empty()){ // 这个页不是空的
            span * _span = _span_list[i].pop_fount(); // 返回
            span * s = new span; // 分裂页
            s->_page_id = _span->_page_id; // 页的编号
            s->_n_page = n; // 页的大小
            _span->_page_id = _span->_page_id + n; // 页号
            _span->_n_page = _span->_n_page - n; // 页数
            for (i = 0;i < n; ++i){
                _id_span_map[s->_page_id + i] = s;
            }
            _span_list[_span->_n_page].push_fount(_span);
            return s;
        }
    }
    span * _span = new span; // 链表中没有合适的
#ifdef _WIN64
    void * ptr = VirtualAlloc(0,(N_PAGES - 1)* (1 << PAGE_SHIFT),
            MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE); // 系统调用
#else
    //
#endif
    _span->_page_id = (page_id) ptr >> PAGE_SHIFT;
    _span->_n_page = N_PAGES - 1;
    for (size_t i = 0; i < _span->_n_page ; ++ i){
        _id_span_map[_span->_page_id + i] = _span;
    }
    _span_list[_span->_n_page].push_fount(_span); // 将新的页添加进去
    return _new_span(n);
}
span *page_cache::new_span(size_t n) {
    std::unique_lock<std::mutex> lock(mutex); // 加全局锁
    return _new_span(n); // 新建页
}
span *page_cache::map_to_span(void *obj) { // 获取页的映射
    page_id id = (page_id)obj >> PAGE_SHIFT;
    auto it = _id_span_map.find(id);
    if (it != _id_span_map.end()){
        return it->second;
    } else {
        assert(false);
        return nullptr;
    }
}
void page_cache::release_span_to_page_cache(span *_span) { // 合并删除
    std::unique_lock<std::mutex> lock(mutex); // 加全局锁
    if (_span->_n_page >= N_PAGES){ // 大于128页 系统调用
        void * ptr = (void *)(_span->_page_id << PAGE_SHIFT);
        _id_span_map.erase(_span->_page_id);
        VirtualFree(ptr,0,MEM_RELEASE);
        delete _span;
        return;
    }
    while (1){ // 向前合并
        page_id current_id = _span->_page_id;
        page_id current_prev_id = current_id - 1;
        auto it =  _id_span_map.find(current_prev_id);

        if (it == _id_span_map.end()){ // 没有前部
            break;
        }
        if (it->second->_use_count != 0){ // 工作中
            // 不是空闲
            break;
        }
        span * prev = it->second;

        if (_span->_n_page + prev->_n_page > N_PAGES -1){ // 大于128页
            break;
        }
        _span_list[prev->_n_page].erase(prev); // 删除

        prev->_n_page += _span->_n_page; // 合并
        for (page_id i = 0; i < _span->_n_page ; ++i) {
            _id_span_map[_span->_page_id + 1] = prev;
        }
        delete _span;

        _span = prev;
    }

    while (1){
        page_id current_id = _span->_page_id;
        page_id current_next_id = current_id + _span->_n_page;
        auto it =  _id_span_map.find(current_next_id);

        if (it == _id_span_map.end()){
            break;
        }
        if (it->second->_use_count != 0){
            // 不是空闲
            break;
        }
        span * next = it->second;

        if (_span->_n_page + next->_n_page > N_PAGES -1){
            break;
        }
        _span_list[next->_n_page].erase(next);

        _span->_n_page += next->_n_page;
        for (page_id i = 0; i < next->_n_page ; ++i) {
            _id_span_map[next->_page_id + 1] = _span;
        }
        delete next;

    }
    _span_list[_span->_n_page].push_fount(_span); // 归还
}
