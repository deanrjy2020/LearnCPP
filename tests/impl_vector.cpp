#include <cassert>
#include <iostream>

#define USE_MY_VECTOR 1
#if !USE_MY_VECTOR
#include <vector>
#endif

using namespace std;

namespace impl_vector {

#if USE_MY_VECTOR
template <typename T>
class vector {
public:
    vector() : array(nullptr), sz(0), capa(0) {}

    ~vector() {
        delete[] array;
    }

    void push_back(const T& value) {
        if (sz >= capa) {
            resize();
        }
        array[sz++] = value;
    }

    void pop_back() {
        if (sz > 0) {
            --sz;
        } else {
            cout << "pop_back() called on empty vector\n";
        }
    }

    void clear() {
        sz = 0;
    }

    T& operator[](size_t index) {
        assert(index < sz);
        return array[index];
    }

    size_t size() const {
        return sz;
    }

    size_t capacity() const {
        return capa;
    }

private:
    void resize() {
        cout << "resize\n";
        size_t expandedCapa = (capa == 0) ? 1 : capa * 2;
        T* expandedArray = new T[expandedCapa];

        for (size_t i = 0; i < sz; ++i) {
            expandedArray[i] = array[i];  // copy construct
        }

        delete[] array;
        array = expandedArray;
        capa = expandedCapa;
    }

    T* array;
    size_t sz;
    size_t capa;
};
#endif

int cppMain() {
    vector<int> vec;

    for (int i = 0; i < 10; ++i) {
        vec.push_back(i * 10);
        cout << "Added: " << vec[i] << ", size = " << vec.size() << ", capacity = " << vec.capacity() << endl;
    }

    vec.pop_back();
    cout << "After pop_back: ";
    for (size_t i = 0; i < vec.size(); ++i)
        cout << vec[i] << " ";
    cout << endl;

    vec.clear();
    cout << "After clear: size = " << vec.size() << endl;

    return 0;
}

}  // namespace impl_vector

/*===== Output =====

[RUN  ] impl_vector
resize
Added: 0, size = 1, capacity = 1
resize
Added: 10, size = 2, capacity = 2
resize
Added: 20, size = 3, capacity = 4
Added: 30, size = 4, capacity = 4
resize
Added: 40, size = 5, capacity = 8
Added: 50, size = 6, capacity = 8
Added: 60, size = 7, capacity = 8
Added: 70, size = 8, capacity = 8
resize
Added: 80, size = 9, capacity = 16
Added: 90, size = 10, capacity = 16
After pop_back: 0 10 20 30 40 50 60 70 80 
After clear: size = 0
[tid=1] [Memory Report] globalNewCnt = 5, globalDeleteCnt = 5, globalNewMemSize = 344, globalDeleteMemSize = 344
[   OK] impl_vector

*/
