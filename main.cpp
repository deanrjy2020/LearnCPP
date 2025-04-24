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

    // mem tracker不支持多线程, 多线程的test可能有intermittent failure.
    unordered_set<string> memoryTrackerBlackList = {
        "impl_shared_ptr",  // todo, fixme
        "new_delete",       // todo, fixme
        "impl_semaphore",
        "thread_basic",
        "thread_example",
        "thread_pool",
        "thread_prod_cons",
        "thread_rwlock",
        "virtual_basic",
    };

    cout << endl;
    cout << "[RUN  ] " << testName << endl;
    {
        // assert(MemoryTracker::getCurrent() == nullptr);
        // MemoryTracker tracker;
        // if (memoryTrackerBlackList.count(testName)) {
        //     // disable mem tracker.
        //     MemoryTracker::getCurrent() = nullptr;
        // }
        // it->second();  // 运行对应测试
        // 如果测试不在黑名单中则启用追踪
        bool enableTracker = memoryTrackerBlackList.count(testName) == 0;
        MemoryTracker::Scope scope(enableTracker);
        it->second();  // 执行测试
    }
    cout << "[   OK] " << testName << endl;

    return 0;
}
