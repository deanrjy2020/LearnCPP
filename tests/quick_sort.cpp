#include <iostream>
#include <vector>
using namespace std;

namespace quick_sort {

//=========================================================
// quick sort
/*
这里的left/right是闭区间. 一般用start/end或者first/last表示half-open区间

quickSort函数做递归, 很好理解.
重点是partition函数
    1, 随机选一个pivot值, 就是array里的第一个, 在一次partition里面, 值永远不变
    2, partition函数返回的是pivot值对应的idx
    3, 双层while循环, pivot先和right比较(因为初始化的是nums[left]的值)
    4, 后面根据记忆写吧.
*/

int partition(vector<int>& nums, int left, int right) {
    // 随机选一个pivot值, 就是array里的第一个, 在一次partition里面, 值永远不变
    int pivot = nums[left];
    while (left < right) {
        // pivot先和right比较(因为初始化的是nums[left]的值)
        while (left < right && pivot <= nums[right]) {
            right--;
        }
        swap(nums[left], nums[right]);
        while (left < right && nums[left] <= pivot) {
            left++;
        }
        swap(nums[left], nums[right]);
    }
    // 返回的是pivot值对应的idx
    return left;
}

void quickSort(vector<int>& nums, int left, int right) {
    if (left < right) {
        int pivotIdx = partition(nums, left, right);
        quickSort(nums, left, pivotIdx - 1);
        quickSort(nums, pivotIdx + 1, right);
    }
}

int cppMain() {
    cout << __FUNCTION__ << endl;

    cout << "test partition" << endl;
    {
        vector<int> vec = {4, 1, 3, 5, 2};
        cout << "Before partition: ";
        for (auto n : vec) {
            cout << n << " ";
        }
        cout << endl;

        cout << "pivotIdx= " << partition(vec, 0, vec.size() - 1) << endl;
        cout << "After partition: ";
        for (auto n : vec) {
            cout << n << " ";
        }
        cout << endl;
    }

    cout << "test quickSort" << endl;
    {
        vector<int> vec = {8, 3, 5, 2, 7, 9, 1, 6, 0, 4};
        quickSort(vec, 0, vec.size() - 1);

        for (auto n : vec) {
            cout << n << " ";
        }
        cout << endl;
    }
    return 0;
}

}  // namespace quick_sort

/*===== Output =====

[RUN  ] quick_sort
cppMain
test partition
Before partition: 4 1 3 5 2 
pivotIdx= 3
After partition: 2 1 3 4 5 
test quickSort
0 1 2 3 4 5 6 7 8 9 
[tid=1] [Memory Report] globalNewCnt = 2, globalDeleteCnt = 2, globalNewMemSize = 148, globalDeleteMemSize = 148
[   OK] quick_sort

*/
