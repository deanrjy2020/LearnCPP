#include <iostream>
#include <queue>
#include <vector>
using namespace std;

namespace dijkstra {

//=========================================================
// dijkstra
/*
https://www.freecodecamp.org/chinese/news/dijkstras-shortest-path-algorithm-visual-introduction/
一个图, 边有权重且不是复数, 求某一点origin到其他所有点的最短路径, 自己到自己的路径为0.
思路:
    从origin开始, 当作一个新graph, 每次增加一个点, 这个点是新graph的相邻点, 且离origin距离最小. 直到所有点都加入新graph.
辅助数据:
    vector distance[i] 表示origin到i的最短路径
    vector visited[i] 表示i是否已经加入新graph
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>>> pq; 和新graph相连的点中(离origin)的最小距离
        pair.second为node
        pair.first为node到origin的距离

adjMatrix表示:
    时间:
        O(V^2), 下面算法中, while是每个点都要加到pq里面O(v), 里面的for是对每个点都过一边全部点, 看是不是邻居O(v)
    空间:
        O(3V), 存adjMatrix不算.

adjList表示:
    时间:
        每个N入队一次, 出队一次 O(VlogV)
        每条边最多松弛一次 → O(E log V)
            每次成功更新会 push 入堆一次（log V），总成功更新不超过 E 次，因此 O(E log V)
        共O((V+E)*logV)
        稀疏图情况下（E ≈ V），效率很高。
    👉 稠密图（E ≈ V²）时，退化到 O(V² log V)。
    空间:
        O(3V), 存adjList不算.

其他:
    distance 用 long long, 防溢出
*/

void dijkstra(vector<vector<int>>& adjMatrix) {
    vector<int> distance(adjMatrix.size(), INT_MAX);
    vector<int> visited(adjMatrix.size(), 0);
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;

    distance[0] = 0;  // idx 0 is origin
    pq.push({0, 0});  // <distance from origin to node, node>
    while (!pq.empty()) {
        auto [distToNode, node] = pq.top();
        pq.pop();

        if (visited[node]) {
            continue;
        }

        visited[node] = 1;

        // go thru the neib of node
        for (size_t neib = 0; neib < adjMatrix[node].size(); neib++) {
            if (!visited[neib] &&                                       // not visited
                adjMatrix[node][neib] < INT_MAX &&                      // can reach to neib
                distToNode + adjMatrix[node][neib] < distance[neib]) {  // found shorter distance
                distance[neib] = distToNode + adjMatrix[node][neib];
                pq.push({distance[neib], neib});
            }
        }
    }

    // print all the distances.
    for (auto& d : distance) {
        cout << d << " ";
    }
    cout << endl;
}

void dijkstra(vector<vector<pair<int, int>>>& adjList) {
    vector<int> distance(adjList.size(), INT_MAX);
    vector<int> visited(adjList.size(), 0);
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq;

    distance[0] = 0;  // idx 0 is origin
    pq.push({0, 0});  // <distance from origin to node, node>
    while (!pq.empty()) {
        auto [distToNode, node] = pq.top();
        pq.pop();

        if (visited[node]) {
            continue;
        }

        visited[node] = 1;

        // go thru the neib of node
        for (auto [neib, weight] : adjList[node]) {
            if (!visited[neib] &&                        // not visited
                distToNode + weight < distance[neib]) {  // found shorter distance
                distance[neib] = distToNode + weight;
                pq.push({distance[neib], neib});
            }
        }
    }

    // print all the distances.
    for (auto& d : distance) {
        cout << d << " ";
    }
    cout << endl;
}

int cppMain() {
    cout << "adding new test here." << endl;

    vector<vector<int>> adjMatrix = {
        {0, 50, 30, 100, 10},
        {50, 0, 5, 20, INT_MAX},
        {30, 5, 0, 50, INT_MAX},
        {100, 20, 50, 0, 10},
        {10, INT_MAX, INT_MAX, 10, 0},
    };
    dijkstra(adjMatrix);

    // pair<node, weight>
    vector<vector<pair<int, int>>> adjList = {
        /*0->*/ {{1, 50}, {2, 30}, {3, 100}, {4, 10}},
        /*1->*/ {{0, 50}, {2, 5}, {3, 20}},
        /*2->*/ {{0, 30}, {1, 5}, {3, 50}},
        /*3->*/ {{0, 100}, {1, 20}, {2, 50}, {4, 10}},
        /*4->*/ {{0, 10}, {3, 10}},
    };
    dijkstra(adjList);

    return 0;
}

}  // namespace dijkstra

/*===== Output =====

[RUN  ] dijkstra
adding new test here.
0 35 30 20 10 
0 35 30 20 10 
[tid=1] [Memory Report] globalNewCnt = 32, globalDeleteCnt = 32, globalNewMemSize = 2296, globalDeleteMemSize = 2296
[   OK] dijkstra

*/
