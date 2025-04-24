#include <assert.h>

#include <iostream>
using namespace std;

// 和glm兼容.
#define USE_GLM 1
#if USE_GLM
#include <../../LearnOpenGL/dean/includes/glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <../../LearnOpenGL/dean/includes/glm/gtx/norm.hpp>  // for length2()
using namespace glm;
#else
#include "vec3.h"
#endif

#include "utils.h"

namespace gfx_vec3 {

//=========================================================
// gfx_vec3

void subtest1() {
    cout << __FUNCTION__ << endl;

    vec3 v1 = {1, 2, 3};
    vec3 v2 = {4, 5, 6};

    // 测试+
    {
        // 简单+
        vec3 v3 = v1 + v2;
        assert(v3 == vec3(5, 7, 9));
        // 累加
        vec3 v4 = v1;
        v4 += v2;
        assert(v4 == vec3(5, 7, 9));
    }

    // 测试-
    {
        // 简单-
        vec3 v3 = v1 - v2;
        assert(v3 == vec3(-3, -3, -3));
        // 取负
        vec3 v4 = -v1;
        assert(v4 == vec3(-1, -2, -3));
        // 累减
        v4 -= v3;
        assert(v4 == vec3(2, 1, 0));
    }

    // 测试*
    {
        vec3 v3 = v1 * 2.0f;
        assert(v3 == vec3(2, 4, 6));
        vec3 v4 = 2.0f * v1;
        assert(v4 == vec3(2, 4, 6));
        // 累乘
        v4 *= 2.0f;
        assert(v4 == vec3(4, 8, 12));
    }

    // 测试length
    {
        assert(nearlyEqual(length(v1), 3.741657f));
        assert(nearlyEqual(length2(v1), 14.0f));
    }

    // 测试distance
    {
        assert(length(v1 - v2) == distance(v1, v2));
    }

    // 测试normalize
    {
        vec3 v3 = normalize(v1);
        assert(nearlyEqual(length(v3), 1.0f));

#if !USE_GLM
        vec3 v4 = {0, 0, 0.000001f};
        auto v5 = normalize(v4);
        assert(v5 == v4);
#endif
    }

    // 测试dot
    {
        float v3 = dot(v1, v2);
        assert(nearlyEqual(v3, 32.0f));
    }

    // 测试cross
    {
        vec3 v3 = cross(v1, v2);
        assert(v3 == vec3(-3, 6, -3));
    }
}

int cppMain() {
    subtest1();

    cout << "cppMain done." << endl;
    return 0;
}

}  // namespace gfx_vec3

/*===== Output =====

[RUN  ] gfx_vec3
subtest1
cppMain done.
[tid=1] [Memory Report] globalNewCnt = 0, globalDeleteCnt = 0, globalNewMemSize = 0, globalDeleteMemSize = 0
[   OK] gfx_vec3

*/
