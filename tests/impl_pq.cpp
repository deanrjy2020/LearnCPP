#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

#define USE_MY_PQ 1
#if !USE_MY_PQ
#include <queue>  // pq
#endif

using namespace std;

namespace impl_pq {

//=========================================================
// priority_queue
/*
和std的兼容, 默认max-heap

min-heap的修改点:
    1, template里面加上: typename Container = vector<T>, typename Compare = less<T>
    2, Container用到data上, Compare用到cmp上
    3, 函数里面所有的data元素的比较都用cmp(a,b)
*/

#if USE_MY_PQ

// template <typename T>
template <typename T, typename Container = vector<T>, typename Compare = less<T>>
class priority_queue {
private:
    // vector<T> data;
    Container data;
    Compare cmp;

    void topDownHeapify() {
        size_t end = data.size();
        size_t cur = 0;
        while (cur < end) {
            // target son to swap
            size_t son = cur * 2 + 1;
            if (son >= end) {  // no son
                break;
                //} else if (son + 1 < sz && data[son] < data[son + 1]) {
            } else if (son + 1 < end && cmp(data[son], data[son + 1])) {
                // has right son, and right son is larger
                // right son is the tartget to swap
                son++;
            }

            // if (data[cur] < data[son]) {
            if (cmp(data[cur], data[son])) {
                swap(data[cur], data[son]);
                cur = son;
            } else {
                break;
            }
        }
    }

    void bottomUpHeapify() {
        size_t end = data.size();
        size_t cur = end - 1;
        while (cur > 0) {
            size_t par = (cur - 1) / 2;
            // 都用<, 否则若 T 要定义 < 和 >
            // if (data[par] < data[cur]) {
            if (cmp(data[par], data[cur])) {
                swap(data[cur], data[par]);
                cur = par;
            } else {
                break;
            }
        }
    }

public:
    void push(const T& n) {
        data.push_back(n);
        bottomUpHeapify();
    }

    void push(T&& v) {
        data.push_back(std::move(v));
        bottomUpHeapify();
    }

    const T& top() const {
        assert(!data.empty());
        return data[0];
    }

    void pop() {
        assert(!data.empty());

        swap(data[0], data[data.size() - 1]);
        data.pop_back();

        topDownHeapify();
    }
    bool empty() const { return data.empty(); }
};

#endif  // USE_MY_PQ

vector<int> vec = {8, 3, 5, 2, 7, 9, 1, 6, 0, 4};

void subtest1() {
    cout << __FUNCTION__ << endl;

    // default max-heap
    {
        priority_queue<int> pq;
        for (auto n : vec) {
            pq.push(n);
        }

        while (!pq.empty()) {
            cout << pq.top() << " ";
            pq.pop();
        }
        cout << endl;
    }

    // min-heap
    {
        priority_queue<int, vector<int>, greater<int>> pq;
        for (auto n : vec) {
            pq.push(n);
        }

        while (!pq.empty()) {
            cout << pq.top() << " ";
            pq.pop();
        }
        cout << endl;
    }
}

//=========================================================
// 在array上 inplace heap sort
/*
 */

// 从start ~ end-1位置是做heapify, half-open
void topDownHeapify(vector<int>& data, size_t start, size_t end) {
    size_t cur = start;
    while (cur < end) {
        // target son to swap
        size_t son = cur * 2 + 1;
        if (son >= end) {  // no son
            break;
        } else if (son + 1 < end && data[son] < data[son + 1]) {
            // has right son, and right son is larger
            // right son is the tartget to swap
            son++;
        }

        if (data[cur] < data[son]) {
            swap(data[cur], data[son]);
            cur = son;
        } else {
            break;
        }
    }
}

void buildHeap(vector<int>& vec) {
    size_t end = vec.size();
    for (int i = end / 2; i >= 0; --i) {  // 这里注意要用int, i到-1停止.
        topDownHeapify(vec, i, end);
    }
}

// 从小到达排序
void heapSort(vector<int>& vec) {
    // step 1, 构建max-heap
    buildHeap(vec);

    // step 2, 每次pop最大的, 放到pos上.
    size_t end = vec.size();
    size_t pos = end - 1;
    while (pos > 0) {
        swap(vec[0], vec[pos]);

        topDownHeapify(vec, 0, pos);

        pos--;
    }
}

void subtest2() {
    cout << __FUNCTION__ << endl;
    // 在线排序.
    {
        heapSort(vec);

        for (auto n : vec) {
            cout << n << " ";
        }
        cout << endl;
    }
}

int cppMain() {
    subtest1();
    subtest2();

    return 0;
}

}  // namespace impl_pq

/*===== Output =====

[RUN  ] impl_pq
subtest1
9 8 7 6 5 4 3 2 1 0 
0 1 2 3 4 5 6 7 8 9 
subtest2
0 1 2 3 4 5 6 7 8 9 
[tid=1] [Memory Report] globalNewCnt = 10, globalDeleteCnt = 10, globalNewMemSize = 688, globalDeleteMemSize = 688
[   OK] impl_pq

*/
