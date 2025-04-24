// This file is auto-generated. DO NOT EDIT.

// This file is part of the main.cpp

// 声明外部的测试入口
namespace constructor_basic   { void cppMain(); }
namespace design_pattern      { void cppMain(); }
namespace destructor_basic    { void cppMain(); }
namespace impl_semaphore      { void cppMain(); }
namespace impl_shared_ptr     { void cppMain(); }
namespace impl_unique_ptr     { void cppMain(); }
namespace impl_vector         { void cppMain(); }
namespace inheritance_basic   { void cppMain(); }
namespace lru_cache           { void cppMain(); }
namespace mem_addr_align      { void cppMain(); }
namespace memory_pool         { void cppMain(); }
namespace memory_tracker      { void cppMain(); }
namespace new_delete          { void cppMain(); }
namespace object_pool         { void cppMain(); }
namespace template_basic      { void cppMain(); }
namespace thread_basic        { void cppMain(); }
namespace thread_example      { void cppMain(); }
namespace thread_pool         { void cppMain(); }
namespace thread_prod_cons    { void cppMain(); }
namespace thread_rwlock       { void cppMain(); }
namespace virtual_basic       { void cppMain(); }
namespace empty               { void cppMain(); }

// 注册所有测试函数
// use map for alphabetical order when printing all.
map<string, function<void()>> testMap = {
    // namespace string     namespace             entry
    { "constructor_basic",  constructor_basic   ::cppMain },
    { "design_pattern",     design_pattern      ::cppMain },
    { "destructor_basic",   destructor_basic    ::cppMain },
    { "impl_semaphore",     impl_semaphore      ::cppMain },
    { "impl_shared_ptr",    impl_shared_ptr     ::cppMain },
    { "impl_unique_ptr",    impl_unique_ptr     ::cppMain },
    { "impl_vector",        impl_vector         ::cppMain },
    { "inheritance_basic",  inheritance_basic   ::cppMain },
    { "lru_cache",          lru_cache           ::cppMain },
    { "mem_addr_align",     mem_addr_align      ::cppMain },
    { "memory_pool",        memory_pool         ::cppMain },
    { "memory_tracker",     memory_tracker      ::cppMain },
    { "new_delete",         new_delete          ::cppMain },
    { "object_pool",        object_pool         ::cppMain },
    { "template_basic",     template_basic      ::cppMain },
    { "thread_basic",       thread_basic        ::cppMain },
    { "thread_example",     thread_example      ::cppMain },
    { "thread_pool",        thread_pool         ::cppMain },
    { "thread_prod_cons",   thread_prod_cons    ::cppMain },
    { "thread_rwlock",      thread_rwlock       ::cppMain },
    { "virtual_basic",      virtual_basic       ::cppMain },
    { "empty",              empty               ::cppMain },
};
