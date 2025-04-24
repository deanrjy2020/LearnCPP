#include <iostream>
using namespace std;

#include <../../LearnOpenGL/dean/includes/glm/glm.hpp>
using namespace glm;

#include <algorithm>
#include <cmath>
#include <cstdlib>  // rand
// #include <ctime>    // time
#include <memory>
#include <vector>

namespace gfx_quad_tree {

//=========================================================
// gfx_quad_tree
/*
常见用途
    空间查询：范围查询（矩形/圆）、点查询、邻域搜索（粗略近邻）。
        点查询思路:
            从根开始，如果 bounds 不包含该点，直接返回不存在。
            若是叶子：线性扫 pts，用公差 eps 比较（浮点相等别用 ==）。
            若已分裂：按点所在象限优先递归（最多会落在一个子象限；边界点按你定义的半开/闭区间规则，只走一个子树，避免抖动）。
                bool exists(const glm::vec2& p, float eps=1e-6f) const {
                    if (!bounds.contains(p)) return false;
                    if (!divided) {
                        for (auto& q : pts) {
                            if (glm::length2(q - p) <= eps*eps) return true;
                        }
                        return false;
                    }
                    // 按象限走唯一子树（边界策略要一致）
                    return (nw->exists(p, eps) || ne->exists(p, eps) ||
                            sw->exists(p, eps) || se->exists(p, eps));
                }
            强调“半开区间”或“含边一致规则”，避免边界点被分到多个象限。
            浮点比较使用 eps。
    碰撞检测/可见性剔除：快速过滤不相交对象。
    地图/瓦片管理：只加载/处理视野范围内的瓦片或要素。
        1. 为什么要用“瓦片”管理地图数据
            假设你有一个全球地图（比如 Google Maps、Apple Maps、OpenStreetMap）：
            全部数据一次性加载到内存是不可能的（数据量太大）
            用户在屏幕上只能看到一个很小的区域
            用户可能缩放（Zoom）和拖动（Pan）
            解决方法：把地图切成小块（瓦片，Tile），比如 256×256 像素一块，或者按经纬度分割固定范围。
            这样可以：
                只加载当前屏幕范围内的瓦片（节省内存）
                按需加载/释放瓦片（节省网络带宽）
                支持多级缩放（LOD）
        2. QuadTree 如何管理瓦片
            以 2D QuadTree 为例：
                根节点表示整个世界（Zoom 0），覆盖经纬度范围 [-180°,180°] × [-90°,90°]
                每分裂一次，就把当前区域均匀切成 4 个象限（NW、NE、SW、SE），相当于 Zoom Level +1
                叶节点对应一个具体瓦片（zoom, x, y）
                    Zoom 0: [1 tile] 世界
                    Zoom 1: [4 tiles] NW, NE, SW, SE
                    Zoom 2: [16 tiles] 每块再分 4
                每个节点（瓦片）存储：
                    范围（AABB，经纬度 min/max）
                    瓦片纹理/矢量数据
                    是否已加载到内存
                QuadTree 可以很快判断某个视野范围内有哪些瓦片节点（范围查询）
        3. 流程（典型地图渲染循环里会做）：
            用户摄像机有一个**视锥（Frustum）**或 2D 视口范围（经纬度 min/max）
            在 QuadTree 中做 范围查询（intersects 检查）
            返回所有和视口相交的瓦片节点
            对这些节点：
                如果数据未加载 → 异步请求（网络/磁盘）
                如果已加载 → 提交给渲染管线
            其它不在视野里的瓦片 → 卸载/释放内存
            好处：
                内存省：同时只保留少量瓦片纹理（比如 50～200 张）
                性能好：渲染批次少，GPU/CPU 不浪费在不可见区域
                网络省流量：只请求必要的数据
    图像/地理数据分块存储与多级细节（LOD）。
        1. 为什么和 QuadTree / OctTree 天然契合 LOD
            QuadTree (2D)：每个分裂层级就是一个 LOD 层级
                Zoom 0：全局最低分辨率
                Zoom N：局部高分辨率
            OctTree (3D)：每个分裂层级就是 3D 模型 / 体素数据的不同分辨率
                LOD 0：全局粗糙模型
                LOD N：局部精细模型
            这种树结构允许在不同区域使用不同分辨率，不必一次性加载全部高精度数据
        2. 数据分块存储的意义
            图像数据
                大规模影像（卫星影像、医学影像）无法一次读入 → 切成小块（tile/chunk）存储
                每个块对应树节点，磁盘或网络按需读取
                访问顺序由 QuadTree 范围查询结果决定（保证只读视野内的块）
            地理数据（GIS 矢量/栅格）
                同理按经纬度切块
                分级存储不同精度的数据（比如道路在 zoom 低时只保留主干道，高 zoom 才加载小街道）
        3. LOD 选择策略（面试可提）
            常见策略：
            基于距离：离摄像机近 → 用高精度块；远处用低精度块
            基于屏幕投影大小：某块在屏幕上的像素大小超过阈值才切换到更高 LOD
            混合策略：距离 + 重要性权重（重要对象优先加载高精度）
*/
struct AABB {
    vec2 min, max;
    bool contains(const vec2& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y;
    }
    bool intersects(const AABB& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y);
    }
};

struct Circle {
    vec2 center;
    float r;
    bool contains(const vec2& p) const {
        vec2 d = p - center;
        return dot(d, d) <= r * r;
    }
    bool intersects(const AABB& box) const {
        // std 做法
        // float cx = std::clamp(center.x, box.min.x, box.max.x);
        // float cy = std::clamp(center.y, box.min.y, box.max.y);
        // vec2 d = vec2(cx, cy) - center;
        // glm 做法
        // Tool: collision detection page里面是在AABB的local space做
        // p_local = clamp(BC向量, AABB_local space);
        // p_world = p_local + B世界坐标, 即上面左右都加上B世界坐标
        // p_world = clamp(, AABB_world space);
        // 这里的circle和AABB已经在world space里面了. todo, 以后在看.
        vec2 p = glm::clamp(center, box.min, box.max);
        vec2 d = p - center;
        return dot(d, d) <= r * r;
    }
};

// 每个叶子node最多3个点, 超过了就分裂
#define CAPACITY 3
// #define MAX_DEPTH 10

class QuadTreeNode {
    // 当前node对应的矩形
    AABB bounds;
    // 当前AABB范围内的点个数, 如果超过CAPACITY=3个了, 就分裂, 最多3个点.
    vector<vec2> pts;
    // true代表当前node已经分裂, 不是叶子node了.
    bool divided = false;

    // 孩子
    unique_ptr<QuadTreeNode> nw, ne, sw, se;

    // 当前node的depth, root node的depth=0, 往下+1
    // 到达了MAX_DEPTH就不分裂了.
    // int depth;

    // 按AABB的中心, 分裂成4个.
    void subdivide() {
        vec2 mid = (bounds.min + bounds.max) * 0.5f;
        nw = make_unique<QuadTreeNode>(AABB{{bounds.min.x, mid.y}, {mid.x, bounds.max.y}});
        ne = make_unique<QuadTreeNode>(AABB{{mid.x, mid.y}, {bounds.max.x, bounds.max.y}});
        sw = make_unique<QuadTreeNode>(AABB{{bounds.min.x, bounds.min.y}, {mid.x, mid.y}});
        se = make_unique<QuadTreeNode>(AABB{{mid.x, bounds.min.y}, {bounds.max.x, mid.y}});
        divided = true;

        // 分裂后, 将当前AABB范围内的点分到4个子node, 当前变成internal node, 就不再拥有点了.
        for (auto& p : pts)
            (nw->insert(p) || ne->insert(p) || sw->insert(p) || se->insert(p));
        pts.clear();
    }

public:
    explicit QuadTreeNode(const AABB& b) : bounds(b) {}

    // true - 插入成功
    bool insert(const vec2& p) {
        if (!bounds.contains(p)) return false;
        if (!divided && (int)pts.size() <= CAPACITY) {
            pts.push_back(p);
            return true;
        }
        if (!divided) subdivide();
        return nw->insert(p) || ne->insert(p) || sw->insert(p) || se->insert(p);
    }

    // 输入一个AABB/举行 range, 输出该范围内的点
    void query(const AABB& range, vector<vec2>& out) const {
        // 如果 range 不与 bounds 相交，肯定不会和孩子相交, 直接返回
        if (!bounds.intersects(range)) return;

        if (!divided) {
            // 叶子node, 看看里面的点是否在 range 内
            for (auto& p : pts)
                if (range.contains(p)) out.push_back(p);
        } else {
            // 如果当前 node 已经分裂了，internal node没有保留点, 继续递归
            nw->query(range, out);
            ne->query(range, out);
            sw->query(range, out);
            se->query(range, out);
        }
    }

    // 和AABB一样的代码, 用多肽.
    void query(const Circle& range, vector<vec2>& out) const {
        if (!range.intersects(bounds)) return;
        if (!divided) {
            for (auto& p : pts)
                if (range.contains(p)) out.push_back(p);
        } else {
            nw->query(range, out);
            ne->query(range, out);
            sw->query(range, out);
            se->query(range, out);
        }
    }
};

int cppMain() {
    cout << "adding new test here." << endl;

    // srand((unsigned)time(nullptr));
    srand(41);

    // 创建一个范围 [-100,-100] 到 [100,100] 的 QuadTreeNode
    QuadTreeNode qt(AABB{{-100, -100}, {100, 100}});

    // 插入 20 个随机点
    for (int i = 0; i < 20; ++i) {
        vec2 p{(float)(rand() % 200 - 100), (float)(rand() % 200 - 100)};
        qt.insert(p);
        cout << "Insert: (" << p.x << ", " << p.y << ")\n";
    }

    // 矩形范围查询, 返回在queryRect里的所有点
    AABB queryRect{{-20, -20}, {20, 20}};
    vector<vec2> hits;
    qt.query(queryRect, hits);
    cout << "\nRect hits: " << hits.size() << "\n";
    for (auto& p : hits)
        cout << "(" << p.x << ", " << p.y << ")\n";

    // 圆形范围查询, 返回在queryCircle里的所有点
    hits.clear();
    Circle queryCircle{{0, 0}, 30.0f};
    qt.query(queryCircle, hits);
    cout << "\nCircle hits: " << hits.size() << "\n";
    for (auto& p : hits)
        cout << "(" << p.x << ", " << p.y << ")\n";

    return 0;
}

}  // namespace gfx_quad_tree

/*===== Output =====

[RUN  ] gfx_quad_tree
adding new test here.
Insert: (72, -80)
Insert: (-95, 92)
Insert: (68, 53)
Insert: (4, 71)
Insert: (-35, 17)
Insert: (57, 6)
Insert: (0, 20)
Insert: (12, -14)
Insert: (38, -45)
Insert: (20, 19)
Insert: (-40, -51)
Insert: (-29, 93)
Insert: (55, 49)
Insert: (95, 83)
Insert: (-80, -2)
Insert: (81, -72)
Insert: (57, -6)
Insert: (44, -28)
Insert: (93, -9)
Insert: (88, 27)

Rect hits: 3
(0, 20)
(20, 19)
(12, -14)

Circle hits: 3
(0, 20)
(20, 19)
(12, -14)
[tid=1] [Memory Report] globalNewCnt = 43, globalDeleteCnt = 43, globalNewMemSize = 3356, globalDeleteMemSize = 3356
[   OK] gfx_quad_tree

*/
