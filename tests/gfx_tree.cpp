#include <assert.h>

#include <iostream>
#include <vector>
using namespace std;

#include "vec3.h"

namespace gfx_tree {

//=========================================================
// gfx_tree
/*
图形/渲染引擎里的场景例子:
    Node 表示一个场景实体（Entity / Scene Node）
    存的典型数据：
        变换信息（Transform: position / rotation / scale）
        渲染组件（Mesh、材质、纹理等）
        行为组件（动画控制器、脚本逻辑等）
        子节点列表（组成层次结构）
    root
    ├─ CameraNode (存摄像机参数)
    ├─ CarNode (存车模型 mesh + 材质)
    │    ├─ WheelNode_FL (左前轮)
    │    ├─ WheelNode_FR (右前轮)
    │    ├─ WheelNode_RL
    │    └─ WheelNode_RR
    └─ BuildingNode (存建筑 mesh)
    子节点会继承父节点的 transform（旋转平移缩放）
    一次更新父节点，就能影响所有子节点（比如整辆车一起移动）

地图 / GIS 系统（Apple Maps、Google Maps）例子
    Node 表示地图上的一个“逻辑对象”
    存的典型数据：
        经纬度位置（lat, lon）
        地图 tile 的索引
        贴图数据（纹理 ID、缓存路径）
        样式信息（颜色、线型、字体）
        可能有矢量图形（polygon、polyline）
        子节点可以表示细分的结构，例如：
            一个城市节点 → 包含多个街道节点 → 街道节点包含多个 POI（point of interest）
    World
    ├─ Country(USA)
    │    ├─ State(California)
    │    │    ├─ City(San Francisco)
    │    │    │    ├─ Street(Market Street)
    │    │    │    │    ├─ POI(Starbucks)
    │    │    │    │    └─ POI(Apple Store)
    空间层次化存储，方便裁剪（frustum culling）和按需加载
    地图渲染时，直接遍历可见节点
 */

/*
follow-up:
1. 你为什么选择用 vector 存子节点？可以用 map 吗？
    vector<Node*> 是最自然的数据结构，因为：
        * 子节点是*有序的*（场景中 often 有渲染顺序、遍历顺序）
        * 不关心名字查找，也没有 O(1) 查找需求
        * 插入/遍历性能更高，cache-friendly
    可选方案：
        * map<string, Node*> 或 unordered_map 可用于：
        * 需要**按名字快速查找子节点**（如 UI 树、骨骼树）
        * 但会牺牲遍历性能，也会带来更复杂的内存释放问题
    使用 vector 是为了保持子节点的遍历顺序，并简化构造和遍历逻辑。如果场景需要按名字访问子节点，或者子节点是稀疏结构，我可以用 `unordered_map` 替代。”
2. 如果要支持删除节点怎么办?
    * 从 `vector<Node*>` 中删除某个节点，必须：
        * 在 `children` 中找到该指针
        * `delete` 它（因为我们是裸指针）
        * 从 `vector` 中擦除
    时间上, 如果是少量删除的话, 可以只是删掉, 不移动array里面的位置, 空着.
3. getGlobalPosition() 会不会重复计算？可以优化吗?
    * 当前实现是 **纯递归**，每次查询都递归到根节点。
    * 对于多次查询相同节点的位置，会重复计算
    1. Memoization（带缓存）**
        * 每次计算后将结果缓存
        * 只在 position 或 parent 变动后失效. 这里也不好处理, 如果祖父变动了，需要递归更新.
    2. 懒更新：只有 transform 改变时才更新全局位置（Push dirty down）
4. 如何检测是否成环？Scene Tree 本质上不应该是 DAG 吗？
    * 场景树应该是 **一棵树**（有向无环图），不能成环
    * 但一旦通过 `addChild()` 加入了祖先节点，就可能构成环
    * 需要在 `addChild()` 中进行**祖先检查**
        bool isAncestor(SceneNode* target) const {
            const SceneNode* cur = this;
            while (cur) {
                if (cur == target) return true;
                cur = cur->parent;
            }
            return false;
        }

        void addChild(SceneNode* child) {
            if (child->isAncestor(this)) {
                throw std::runtime_error("Cycle detected in scene graph!");
            }
            child->parent = this;
            children.push_back(child);
        }
5. 如果要支持多线程遍历 / 修改，怎么办？
    | 操作       | 需求                       |
    | -------- | ------------------------ |
    | 多线程遍历    | 要求节点结构是只读                |
    | 多线程修改    | 需要加锁（读写锁或全局锁）            |
    | 一边遍历一边修改 | 极其复杂，需用并发容器或 actor model |
    A. 多线程遍历
        * 前提：**节点结构是 immutable 或只读访问**
        * 直接用 `const SceneNode*` 并行 `traverse()`
        * e.g. root的每个孩子都用一个线程
            void traverseParallel_thread(Node* root) {
                std::vector<std::thread> threads;

                for (auto* child : root->children) {
                    threads.emplace_back([child]() {
                        traverse(child);  // 假设是只读的
                    });
                }

                // 等所有线程跑完
                for (auto& t : threads) {
                    t.join();
                }
            }
*/

/*

增加AABB, 代码量太多, 可能没有必要, 放着先, 看看思路, 以后整理.
    和BVH的区别是, BVH里面root就是最大的AABB, 孩子是root的一部分
    这里

可以加，而且很“面试加分”。思路是：**节点缓存 local AABB，按变换求 world AABB，并把自己与所有子树的 world AABB 做并集，作为可见性裁剪用的“聚合包围盒”**。下面给你一套能直接塞进你当前 `Node` 结构的实现。

---

# 核心结构
AABB可以作为一个新的util文件, 增加测试. 有必要?
struct AABB {
    vec3 min, max;

    static AABB empty() {
        return { vec3( +INFINITY), vec3( -INFINITY) }; // 便于做并集初始化
    }
    static AABB fromCenterExtent(const vec3& c, const vec3& e) {
        return { c - e, c + e };
    }
    void expand(const vec3& p) {
        min = { std::min(min.x,p.x), std::min(min.y,p.y), std::min(min.z,p.z) };
        max = { std::max(max.x,p.x), std::max(max.y,p.y), std::max(max.z,p.z) };
    }
    vec3 center() const { return (min + max) * 0.5f; }
    vec3 extent() const { return (max - min) * 0.5f; }

    // 并集
    static AABB merge(const AABB& a, const AABB& b) {
        AABB r;
        r.min = { std::min(a.min.x,b.min.x), std::min(a.min.y,b.min.y), std::min(a.min.z,b.min.z) };
        r.max = { std::max(a.max.x,b.max.x), std::max(a.max.y,b.max.y), std::max(a.max.z,b.max.z) };
        return r;
    }

    // 变换：通用仿射 (使用 center/extent + |R| * e 优化，避免 8 角逐点)
    // 假设 mat4 的前三行前三列是线性部分，最后一列是平移
    AABB transformed(const mat4& M) const {
        vec3 c = center();
        vec3 e = extent();

        // 新中心
        vec3 c2 = (M * vec4(c, 1.0f)).xyz();

        // 线性部分的绝对值乘以 extents
        //  e2 = |R| * e
        vec3 col0 = { M.m00, M.m10, M.m20 };
        vec3 col1 = { M.m01, M.m11, M.m21 };
        vec3 col2 = { M.m02, M.m12, M.m22 };
        vec3 e2 = { std::abs(col0.x)*e.x + std::abs(col1.x)*e.y + std::abs(col2.x)*e.z,
                    std::abs(col0.y)*e.x + std::abs(col1.y)*e.y + std::abs(col2.y)*e.z,
                    std::abs(col0.z)*e.x + std::abs(col1.z)*e.y + std::abs(col2.z)*e.z };

        return fromCenterExtent(c2, e2);
    }

    // 仅平移（你当前 Node 只有 position 时可用）
    AABB translated(const vec3& t) const {
        return { min + t, max + t };
    }
};
```

---

# Node 增加字段与脏标记

```cpp
struct Node {
    std::string name;
    vec3 position;            // 仅平移；若有旋转缩放，用 localMatrix()
    Node* parent = nullptr;
    std::vector<Node*> children;

    // 几何本地包围盒（只包含“自己”的几何，比如模型网格；没有几何就设 empty()）
    AABB localAABB = AABB::empty();

    // 缓存：自身 worldAABB（本节点几何变换后的 AABB）
    mutable AABB worldAABB_cache = AABB::empty();
    // 缓存：聚合 worldAABB（自己 + 全部子树 的并集）
    mutable AABB worldAABB_aggregate_cache = AABB::empty();

    mutable bool boundsDirty = true; // 变换或几何变化后置脏

    Node(const std::string& n, const vec3& pos) : name(n), position(pos) {}

    void addChild(Node* child) {
        child->parent = this;
        children.push_back(child);
        markBoundsDirtyUpwards(); // 结构变动，父系也要脏
    }

    // 仅平移版本：world 变换就是父的 world 平移累加
    vec3 worldTranslation() const {
        vec3 t(0,0,0);
        for (auto p = this; p; p = p->parent) t += p->position;
        return t;
    }

    // 若有旋转/缩放，改为：
    // mat4 localMatrix() const; mat4 worldMatrix() const;（递归相乘或缓存）
    // 并在 AABB 里用 transformed(worldMatrix)

    void setPosition(const vec3& p) {
        position = p;
        markBoundsDirtyUpwards();
    }
    void setLocalAABB(const AABB& aabb) {
        localAABB = aabb;
        markBoundsDirtyUpwards();
    }
    void markBoundsDirtyUpwards() {
        for (Node* n = this; n && !n->boundsDirty; n = n->parent)
            n->boundsDirty = true;
    }

    // 获取自身几何的 world AABB（不含子树）
    const AABB& worldAABB_self() const {
        if (!boundsDirty) return worldAABB_cache; // 已经由 aggregate 计算时一并写入
        // 仅平移：直接平移 localAABB；若有完整矩阵，用 transformed(worldMatrix)
        worldAABB_cache = localAABB.translated(worldTranslation());
        return worldAABB_cache;
    }

    // 获取聚合 AABB（自己 + 所有子孙）
    const AABB& worldAABB_aggregate() const {
        if (!boundsDirty) return worldAABB_aggregate_cache;

        // 先算自身
        AABB agg = worldAABB_self();
        // 并上所有子树
        for (auto* c : children) {
            agg = AABB::merge(agg, c->worldAABB_aggregate());
        }
        worldAABB_aggregate_cache = agg;
        boundsDirty = false;
        return worldAABB_aggregate_cache;
    }
};
```

> 解释：
>
> * **localAABB**：模型在本地坐标系的包围盒。
> * **worldAABB\_self**：把 localAABB 施加到自身 world 变换后的盒子。
> * **worldAABB\_aggregate**：把自己与所有子节点的 worldAABB 做并集（用于裁剪整棵子树）。
> * **boundsDirty**：任意几何或变换变化后置脏；**查询时**懒更新并缓存。

---

# 视锥裁剪（Frustum Culling）

用“中心-半径”快速测试：对每个平面，计算 AABB 投影半径 `r = dot(|n|, extent)`，距离 `s = dot(n, center) + d`，若 `s + r < 0` 则完全在平面外 → 剔除。

```cpp
struct Plane { vec3 n; float d; }; // n 需为单位向量
struct Frustum { Plane planes[6]; }; // 约定顺序: L,R,B,T,N,F

inline bool aabbInsideFrustum(const AABB& b, const Frustum& f) {
    const vec3 c = b.center();
    const vec3 e = b.extent();
    for (int i = 0; i < 6; ++i) {
        // 计算平面法线的 绝对值。这样做的目的是确保无论法线朝哪个方向（内或外），计算结果保持一致。
        const vec3 an = { std::abs(f.planes[i].n.x),
                          std::abs(f.planes[i].n.y),
                          std::abs(f.planes[i].n.z) };
        // 这个公式计算了 AABB 在该平面法线方向上的 投影半径，即 AABB 对应于平面法线的“投影长度”。
        float r = an.x * e.x + an.y * e.y + an.z * e.z;          // 半径
        // 计算 AABB 中心到平面 的距离（平面方程中的法线方向）。其中 f.planes[i].n 是平面法线，c 是 AABB 中心，f.planes[i].d 是平面的偏移量（常数项）。
        float s = dot(f.planes[i].n, c) + f.planes[i].d;         // 中心到平面距离
        // 通过 AABB的中心到平面距离 和 AABB的投影半径 判断是否完全在平面外。如果 s + r 小于 0，表示 AABB 完全在平面外，因此返回 false，说明 AABB 不在视锥内。
        if (s + r < 0) return false;                             // 完全在外
    }
    return true;
}
```

遍历时先测 **聚合 AABB**，不在视锥就整棵子树跳过：

```cpp
void cullAndDraw(Node* n, const Frustum& fr) {
    if (!n) return;
    if (!aabbInsideFrustum(n->worldAABB_aggregate(), fr)) return; // 剪掉整棵

    // 到这里说明子树“可能可见”，可进一步用 self AABB 精细判断再渲染
    if (aabbInsideFrustum(n->worldAABB_self(), fr)) {
        // drawSelf(n);  // 渲染本节点几何
    }
    for (auto* c : n->children) cullAndDraw(c, fr);
}
```

6个平面的法线都是朝向frustum**外面**的.
一个点到一个平面的距离, d > 0, 在法线那边(点在frustum外面), d<0, 在法线反方向(点可能在frustum内, 还要看其他平面), d=0,在平面上.

1, 简单思路:
    构造视锥 6 个平面（从 VP 矩阵取行/列），每个平面用单位法线 n 和平面上的点P表示。
        6个面的法线都是朝向frustum外面的.
        对每个面, 8个点到平面的距离都d0到d7
            如果全部正数, 在当前平面的外面, 即在整个frustum外面, 结束
            如果有一个负数, 在当前平面的内面, 当前平面就不用继续了, continue
2, production oode思路: 代码见上面aabbInsideFrustum
    1, 以AABB中心为坐标系原点, halfExtent为三维向量e, 方便理解
    2, e在n上的投影长度为r = dot(e,n), 这个长度在任何space下都是不变的
        这里n的所有分量都要用绝对值, 的到r是正数
    3, AABB中心点C到平面的距离d.
        用法线n和平面上一点P表示一个平面, 那么d = dot(n,PC); d有符号
    4, d + r < 0, AABB完全在平面外, 剔除
        d是负的, 不理解. 以后再看吧.
        理解了你的困惑。让我们重新整理一下 d + r < 0 的含义，并明确为什么它表示需要剔除 AABB。
        总结一下重要的概念：
        AABB的中心到平面的距离 d：这是 AABB 中心到该平面（视锥的一个面）的距离。如果 d < 0，那么 AABB 的中心点位于平面法线的反方向（即平面内侧）。
        半尺寸投影长度 r：这是 AABB 半尺寸在平面法线方向上的投影长度，始终为正数。它表示 AABB 在该方向上的最大“扩展范围”。
        为什么 d + r < 0 表示剔除 AABB：
        d 是负的，表示 AABB 中心在平面内侧（法线的反方向）。
        这意味着 AABB 的中心点位于平面法线的反方向（即视锥内侧）。
        r 是正的，表示 AABB 在平面法线方向上的“扩展”。
        AABB 是一个立方体，它的大小在每个方向上都有半尺寸。 r 就是 AABB 在平面法线方向上的半尺寸投影。
        d + r < 0 的含义：
        d + r < 0 表示，尽管 AABB 的中心在平面内侧，但 AABB 在该平面法线方向上的“扩展”还不够大，以至于其最远的角点仍然位于平面法线指向的外侧。
        换句话说，AABB 在这个平面的一侧的所有点都位于平面法线指向的外侧，因此 AABB 完全被剔除掉。
        更直观的解释：
        假设AABB完全在平面外：
        如果 AABB 的中心在平面内侧（d < 0），而且它的半尺寸投影长度 r 大到使得 AABB 的最远点也位于平面外侧（即 d + r < 0），那么整个 AABB 不可能在视锥内。
        这意味着无论 AABB 中心在哪里，它的最远点都已经超出了视锥的边界，所以可以完全剔除。
        举个例子：
        假设平面是 y = 0，AABB 的中心是 C(0, -2, 0)，它的半尺寸为 r = 1。
        AABB 中心到平面的距离是：d = -2（AABB 的中心在平面内侧）。
        AABB 在法线方向上的扩展是：r = 1。
        现在，计算 d + r：
        𝑑+𝑟=−2+1=−1
        由于 d + r < 0，说明 AABB 的最远点（法线方向上的扩展）已经超出了平面之外，即 AABB 完全位于视锥外，所以可以被剔除。
        总结：
        d < 0 表示 AABB 的中心在平面内侧（视锥内）。
        r 表示 AABB 在法线方向上的扩展，如果 AABB 的最远点也在平面外，则需要剔除这个 AABB。
        d + r < 0 表示即使 AABB 的中心在平面内侧，它的最远点已经完全超出了视锥，因此不需要再继续检查这个 AABB，可以直接剔除。
        希望这个解释能让你清楚为什么 d + r < 0 需要剔除 AABB！

遍历时先测“聚合 AABB”，能整棵子树早剔，大幅减少 draw 调用。

如果 以后有旋转/缩放，只需把 worldAABB_self() 改成用世界矩阵把 localAABB 变换为 AABB（“中心+半径法”：e' = |R|*e），接口不变。

这 4 点讲清楚就够了。他们要的是你的思路和抽象边界，而不是你把 6 个平面和矩阵细节都码完。

---

# 使用小结

* **只有平移**：`worldAABB_self = localAABB.translated(worldTranslation())`，够用也高效。
* **有旋转/缩放**：用 `AABB::transformed(worldMatrix)` 的中心-半径法，稳定不跑偏。
* **性能**：`boundsDirty` 懒更新，只有改动时才重算；裁剪用聚合 AABB 早剔除整棵子树。
* **扩展**：真实项目里会把 “自几何 AABB” 与 “聚合 AABB” 分开缓存，这里已经体现。

需要我把这套代码整成一个最小可编译 demo（含 `vec3/mat4` 简易实现 + 随机树 + 裁剪统计）给你本地跑吗？
*/

struct AABB {
    vec3 min, max;
    static AABB empty() {
        return {vec3(+INFINITY), vec3(-INFINITY)};
    }
    static AABB fromMinMax(const vec3& mn, const vec3& mx) { return {mn, mx}; }
    static AABB merge(const AABB& a, const AABB& b) {
        return {
            vec3(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z)),
            vec3(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z))};
    }

    AABB translated(const vec3& t) const { return {min + t, max + t}; }
};

struct Transform {
    vec3 translation;
    // vec3 rotation;
    // vec3 scale = {1, 1, 1};

    Transform(const vec3& t) : translation(t) {}

    // 如果引入了 translation（T）、rotation（R）和 scale（S），那么每个节点的
    // 局部变换（local transform） 应当表示为一个 4x4 的 齐次变换矩阵（homogeneous transformation matrix）
    // 那node里面就不是getGlobalPosition, 而是getGlobalMatrix
    // mat4 getLocalMatrix() const {
    //     mat4 T = translate(translation);
    //     mat4 R = toMat4(rotation);  // 或者 mat4(R)
    //     mat4 S = scaleMatrix(scale);
    //     return T * R * S;
    // }
};

struct Node {
    string name;
    // 在父节点坐标系下的坐标(即父节点坐标的一个位移), 那么当前真正的坐标就是从root到当前节点所有的pos的累加.
    Transform transform;
    Node* parent = nullptr;  // 父节点（根节点为 nullptr）
    vector<Node*> children;

    AABB localAABB = AABB::empty();  // 自几何, car在local space下的包围盒, 不包括子树

    // 构造函数
    Node(const std::string& name, const vec3& t) : name(name), transform(t) {}

    // ownership 不可共享，不能复制。
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // 只要 Node 类建立了对 children 的“拥有”关系（ownership），那么由 Node 来递归删除它们是合理且推荐的设计
    // 所谓“ownership”并不要求一定“自己 new 的”，而是, 谁负责“最终释放资源”，谁就是 owner。
    // 你在 addChild() 的时候，已经建立了这种所有权
    // 什么时候这样设计不合理？
    // 只有以下两种情况，不适合在析构里 delete children：
    // Node 并不拥有子节点，只是“观察者”或“共享者”（例如多个 parent 指向同一个 child）
    // 使用 shared_ptr / weak_ptr 管理资源，释放应交给引用计数管理
    ~Node() {
        cout << "Deleting node: " << name << endl;
        for (auto* child : children) {
            delete child;
        }
    }

    // 添加子节点
    void addChild(Node* child) {
        // 在加入孩子的时候就建立了父子关系, 把parent填上.
        child->parent = this;
        children.push_back(child);
    }

    // Part 3: 返回全局坐标
    // 迭代实现
    vec3 getGlobalPosition_iterative() const {
        vec3 globalPos = {0, 0, 0};  // glm要显式初始化, 默认undefined
        // 因为是const函数, this是const gfx_vec3::Node*
        // curNode可以变, 但是指向的内容不能改.
        const Node* curNode = this;
        while (curNode != nullptr) {
            globalPos += curNode->transform.translation;
            curNode = curNode->parent;
        }
        return globalPos;
    }

    // 递归实现
    vec3 getGlobalPosition_recursive() const {
        if (parent == nullptr) {
            return transform.translation;
        }
        return parent->getGlobalPosition_recursive() + transform.translation;
    }

    // mat4 getGlobalMatrix(const Node* node) {
    //     if (!node->parent) return node->localTransform.getLocalMatrix();
    //     return getGlobalMatrix(node->parent) * node->localTransform.getLocalMatrix();
    // }

    // 打印当前node信息或者所有node信息（pre-order递归方式）
    void print(int depth = 0) const {
        cout << string(depth * 2, ' ')
             << "name=" << name
             << ", translation=(" << transform.translation.x << ", " << transform.translation.y << ", " << transform.translation.z << ")" << endl;

        // for (auto& child : children) {
        //     child->print(depth + 1);
        // }
    }

    vec3 worldTranslation() const {  // 迭代累加到root
        vec3 t(0, 0, 0);
        for (auto* n = this; n; n = n->parent) t += n->transform.translation;
        return t;
    }

    // 自几何, car在local space下的包围盒, 不包括子树
    AABB worldAABB() const {
        return localAABB.translated(worldTranslation());
    }

    // 自己并上子树, 递归一直到叶子, 算是pre-order access
    AABB worldAABB_aggregate() const {
        AABB agg = worldAABB();
        for (auto* c : children) {
            agg = AABB::merge(agg, c->worldAABB_aggregate());
        }
        return agg;
    }
};

Node* createNode(const string& name, const vec3& pos, vector<Node*> children = {}) {
    auto node = new Node(name, pos);
    for (auto& child : children) {
        node->addChild(child);
    }
    return node;
}

void traverse(const Node* node, int level = 0) {
    if (node == nullptr) {
        return;
    }

    // pre-order access
    node->print(level);

    // debug code, 随便检查2的node, 确认getGlobalPosition_*正常工作.
    if (node->name == "7") {
        vec3 globalPos1 = node->getGlobalPosition_iterative();
        vec3 globalPos2 = node->getGlobalPosition_recursive();
        assert(globalPos1 == globalPos2 && globalPos1 == vec3(11, 13, 15));
    } else if (node->name == "16") {
        vec3 globalPos1 = node->getGlobalPosition_iterative();
        vec3 globalPos2 = node->getGlobalPosition_recursive();
        assert(globalPos1 == globalPos2 && globalPos1 == vec3(20, 22, 24));
    }

    for (auto& child : node->children) {
        traverse(child, level + 1);
    }
}

void subtest1() {
    cout << __FUNCTION__ << endl;

    cout << "build tree." << endl;
    auto root =
        // l0
        createNode("root", vec3(0, 0, 0), {
                                              // l1
                                              createNode("1", vec3(1, 2, 3), {
                                                                                 // l2
                                                                                 createNode("2", vec3(4, 5, 6), {}),
                                                                                 createNode("3", vec3(7, 8, 9), {}),
                                                                                 createNode("4", vec3(10, 11, 12), {}),
                                                                             }),
                                              createNode("5", vec3(4, 5, 6), {
                                                                                 createNode("6", vec3(4, 5, 6), {}),
                                                                                 createNode("7", vec3(7, 8, 9), {}),
                                                                                 createNode("8", vec3(10, 11, 12), {}),
                                                                             }),
                                              createNode("9", vec3(7, 8, 9), {
                                                                                 createNode("10", vec3(4, 5, 6), {}),
                                                                                 createNode("11", vec3(7, 8, 9), {}),
                                                                                 createNode("12", vec3(10, 11, 12), {}),
                                                                             }),
                                              createNode("13", vec3(10, 11, 12), {
                                                                                     createNode("14", vec3(4, 5, 6), {}),
                                                                                     createNode("15", vec3(7, 8, 9), {}),
                                                                                     createNode("16", vec3(10, 11, 12), {}),
                                                                                 }),
                                          });

    // root->print();

    cout << "traverse tree." << endl;
    traverse(root);

    delete root;
}

void subtest2() {
    cout << __FUNCTION__ << endl;

    // 假设场景是world里面有一个code car, 只有一个node wheel (单轮车), car和wheel的AABB有overlap, 但是不是包围关系.
    // World (root)
    //   └─ Car (Node)
    //     └─ Wheel (Node)

    auto root = new Node("root", {0, 0, 0});

    auto car = new Node("car", {5, 0, 0});                       // 位置（相对 World）：(5, 0, 0)
    car->localAABB = AABB::fromMinMax({-1, -1, -2}, {1, 1, 2});  // 假设车的模型长 2m、宽 2m、高 4m

    auto wheel = new Node("wheel", {1, -1, 0});                                      // 位置（相对 Car）：(1, -1, 0)
    wheel->localAABB = AABB::fromMinMax({-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f});  // 轮子直径 1m、宽 1m、厚 1m

    root->addChild(car);
    car->addChild(wheel);

    AABB sceneBox = root->worldAABB_aggregate();  // 面试展示：聚合包围盒
    assert(sceneBox.min == vec3(4, -1.5, -2) && sceneBox.max == vec3(6.5, 1, 2));

    delete root;
}

int cppMain() {
    subtest1();
    subtest2();

    return 0;
}

}  // namespace gfx_tree

/*===== Output =====

[RUN  ] gfx_tree
subtest1
build tree.
traverse tree.
name=root, translation=(0, 0, 0)
  name=1, translation=(1, 2, 3)
    name=2, translation=(4, 5, 6)
    name=3, translation=(7, 8, 9)
    name=4, translation=(10, 11, 12)
  name=5, translation=(4, 5, 6)
    name=6, translation=(4, 5, 6)
    name=7, translation=(7, 8, 9)
    name=8, translation=(10, 11, 12)
  name=9, translation=(7, 8, 9)
    name=10, translation=(4, 5, 6)
    name=11, translation=(7, 8, 9)
    name=12, translation=(10, 11, 12)
  name=13, translation=(10, 11, 12)
    name=14, translation=(4, 5, 6)
    name=15, translation=(7, 8, 9)
    name=16, translation=(10, 11, 12)
Deleting node: root
Deleting node: 1
Deleting node: 2
Deleting node: 3
Deleting node: 4
Deleting node: 5
Deleting node: 6
Deleting node: 7
Deleting node: 8
Deleting node: 9
Deleting node: 10
Deleting node: 11
Deleting node: 12
Deleting node: 13
Deleting node: 14
Deleting node: 15
Deleting node: 16
subtest2
Deleting node: root
Deleting node: car
Deleting node: wheel
[tid=1] [Memory Report] globalNewCnt = 42, globalDeleteCnt = 42, globalNewMemSize = 4352, globalDeleteMemSize = 4352
[   OK] gfx_tree

*/
