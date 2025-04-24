#include <iostream>
#include <unordered_map>
using namespace std;

namespace lru_cache {

    // todo, not finished yet.

template <typename K, typename V>
class LRUcache {
private:
    struct Node {
        K key;
        V val;
        Node* next;
        Node* prev;
        Node(K k, V v) : key(k), val(v), next(nullptr), prev(nullptr) {}
    };
    // cycle doubly linked list, dummy is in the list, but not in the map
    // dummy->next is the head.
    Node* dummy;
    size_t capacity;
    unordered_map<int, Node*> map;

    void removeNode(Node*& node) {
        // no need to free node mem, reuse it later.
        node->next->prev = node->prev;
        node->prev->next = node->next;

        node->next = NULL;
        node->prev = NULL;
    }

    void insertNode(Node*& node) {
        node->next = dummy->next;
        node->prev = dummy;

        dummy->next = node;
        node->next->prev = node;
    }

public:
    LRUcache(int capa) : capacity(capa) {
        dummy = new Node(-1, -1);
        dummy->next = dummy;
        dummy->prev = dummy;
    }
    ~LRUcache() {
        for (auto& elem : map) {
            delete elem.second;
        }
        delete dummy;
        dummy = nullptr;
    }

    void printAll() {
        auto cur = dummy->next;
        while (cur != dummy) {
            printf("val = %d,\n", cur->val);
            cur = cur->next;
        }
    }

    V get(K key) {
        if (!map.count(key)) {
            return; // todo what to return
        }
        auto node = map[key];
        int res = node->val;

        // put the entry to the most recent used place
        // which is the next of the dummy.
        removeNode(node);
        insertNode(node);
        return res;
    }
    void put(K key, V val) {
        Node* node = NULL;
        if (map.count(key)) {
            // update the target entry in the LRU
            // 这里包括了2种情况
            // 1, value也是一样
            // 2, value不一样
            node = map[key];
            // don't need to update the map, since we reuse the node
            node->val = val;
            removeNode(node);
            insertNode(node);
            return;
        }
        if (map.size() == capacity) {
            // update the tail entry
            node = dummy->prev;
            int oldKey = node->key;
            map.erase(oldKey);
            removeNode(node);
            node->key = key;
            node->val = val;
        } else {
            node = new Node(key, val);
        }
        insertNode(node);
        map[key] = node;
    }
};
int cppMain() {
    LRUcache<int, int> lru(5);
    lru.printAll();
    lru.put(1, 1);
    lru.put(2, 2);
    lru.printAll();
    return 0;
}

}  // namespace lru_cache

/*===== Output =====

[RUN  ] lru_cache
val = 2,
val = 1,
[tid=1] [Memory Report] globalNewCnt = 6, globalDeleteCnt = 6, globalNewMemSize = 488, globalDeleteMemSize = 488
[   OK] lru_cache

*/
