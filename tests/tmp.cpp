#include <iostream>
using namespace std;

namespace tmp {

//=========================================================
// tmp

int cppMain() {
    cout << __FUNCTION__ << endl;

    return 0;
}

}  // namespace tmp

/*===== Output =====

[RUN  ] tmp
adding new test here.
[tid=1] [Memory Report] globalNewCnt = 0, globalDeleteCnt = 0, globalNewMemSize = 0, globalDeleteMemSize = 0
[   OK] tmp

*/
