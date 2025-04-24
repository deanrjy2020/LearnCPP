#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
using namespace std;

#include "g_tests.h"
#include "utils.h"  // MemoryTracker

void printTests() {
    cout << "Available tests:\n";
    for (const auto& [name, _] : testMap) {
        cout << "  " << name << "\n";
    }
}

int main(int argc, char** argv) {
    // 打印所有有效的测试名字
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <test_name>\n";
        printTests();
        return 1;
    }

    // 这里只能跑单个test, 用.sh跑全部.
    string testName = argv[1];
    auto it = testMap.find(testName);
    if (it == testMap.end()) {
        cout << "Test '" << testName << "' not found.\n";
        printTests();
        return 1;
    }

    unordered_set<string> memoryTrackerBlackList = {
        "design_pattern",
        "destructor_basic",
        "impl_shared_ptr",
        "lru_cache",
        "memory_pool",
        "new_delete",
        "object_pool",
        "thread_basic",
        "thread_prod_cons",
        "thread_rwlock",
        "virtual_basic",
    };

    cout << endl;
    cout << "[RUN  ] " << testName << endl;
    {
        assert(MemoryTracker::getCurrent() == nullptr);
        MemoryTracker tracker;
        if (memoryTrackerBlackList.count(testName)) {
            // disable mem tracker.
            MemoryTracker::getCurrent() = nullptr;
        }
        it->second();  // 运行对应测试
    }
    cout << "[   OK] " << testName << endl;

    return 0;
}
