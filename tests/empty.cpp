#include <iostream>
using namespace std;

namespace empty {

//=========================================================
// empty

int cppMain() {
    cout << "adding new test here." << endl;

    return 0;
}

}  // namespace empty

/*===== Output =====

[RUN  ] empty
adding new test here.
[tid=1] [Memory Report] globalNewCnt = 0, globalDeleteCnt = 0, globalNewMemSize = 0, globalDeleteMemSize = 0
[   OK] empty

*/
