#include <cstring>
#include <iostream>
using namespace std;

namespace constructor_basic {

class Buffer {
private:
    size_t size;
    char* data;

    void freeData() {
        cout << "Freed " << size << " bytes\n";
        delete[] data;
        data = nullptr;
        size = 0;
    }

public:
    /*
     1. Default Constructor
     就是不用任何参数, 如Buffer(size_t sz = 128) { ... }也是default constructor
     如果创建一个类没有写任何构造函数,则系统会自动生成implicit inline default构造函数
     只要你写了一个下面的某一种构造函数，系统就不会再自动生成这样一个默认的构造函数，
     如果希望有一个这样的无参构造函数，则需要自己显示地写出来, 或者用delete
     Buffer() = delete;
    */
    Buffer() : Buffer(128) {
        cout << "Default Constructor\n";
    }

    /*
     2. Overloaded Constructor
     一般构造函数可以有各种参数形式, 一个类可以有多个一般构造函数，
     前提是参数的个数或者类型不同（基于c++的函数overload原理）
     创建对象时根据传入的参数不同调用不同的构造函数.
    */
    explicit Buffer(size_t sz) : size(sz), data(new char[sz]) {
        cout << "Constructor: Allocated " << size << " bytes\n";
    }

    /*
     3. Copy Constructor (Deep Copy)
     和上面一个类似, 但参数为类对象本身的引用，用于根据一个已存在的对象复制出一个新的该类的对象
     一般在函数中会将已存在对象的数据成员的值复制一份到新创建的对象中.
     若没有显示的写复制构造函数，则系统会默认创建一个复制构造函数，但当类中有指针成员时,
     由系统默认创建该复制构造函数会存在风险，具体原因请查询有关“浅拷贝” 、“深拷贝”的文章论述.
    */
    // 直接用private变量other.size, 将对象other中的数据成员值复制过来
    Buffer(const Buffer& other) : size(other.size), data(new char[other.size]) {
        memcpy(data, other.data, size);
        cout << "Copy Constructor: Deep copied " << size << " bytes\n";
    }

    void foo(const Buffer& buf) {
        // 这样也是可以调用private的. 不单单是在构造函数里面.
        // 当然用的很少.
        size_t size = buf.size;
        (void)size;  // 不用, 去掉编译warning
    }

    /*
     4. Copy Assignment
     等号运算符重载. 注意，这个类似复制构造函数，将=右边的本类对象的值复制给等号左边的对象，
     但是它不属于构造函数，等号左右两边的对象必须已经被创建
     若没有显示的写=运算符重载，则系统也会创建一个默认的=运算符重载，只做一些基本的拷贝工作
    */
    Buffer& operator=(const Buffer& other) {
        // 首先检测等号右边的是否就是左边的对象本身，若是本对象本身,则直接返回
        if (this != &other) {
            freeData();  // delete current data.
            size = other.size;
            // 注意这里new可能fail, 但是当前的data已经free了.
            // 更好的做法是backup一下, 一切都ok了最后free.
            data = new char[size];
            // 如果是自定义类的话, 如A* data; 也可以用下面的involve A类的 = op 函数
            // *data = *other.data;
            memcpy(data, other.data, size);
            cout << "Copy Assignment: Deep copied " << size << " bytes\n";
        }
        // 把等号左边的对象再次传出
        // 目的是为了支持连等 eg: a=b=c; 系统首先运行 b=c
        // 然后运行a=(b=c的返回值,这里应该是复制c值后的b对象)
        return *this;
    }

    // 5. Move Constructor
    Buffer(Buffer&& other) noexcept : size(other.size), data(other.data) {
        other.data = nullptr;
        other.size = 0;
        cout << "Move Constructor: Moved ownership\n";
    }

    // 6. Move Assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            freeData();  // delete current data.
            size = other.size;
            data = other.data;
            // clear other Buffer
            other.data = nullptr;
            other.size = 0;
            cout << "Move Assignment: Moved ownership\n";
        }
        return *this;
    }

    // 7. Destructor
    ~Buffer() {
        freeData();
    }

    // 8. Utility function
    void fill(char ch) const {
        memset(data, ch, size);
    }

    void print(size_t count = 10) const {
        for (size_t i = 0; i < min(count, size); ++i) {
            cout << data[i];
        }
        cout << '\n';
    }
};

int cppMain() {
    // 有空把这个看完, update上去
    // https://blog.csdn.net/TABE_/article/details/116714304
    // conversion con 不要用.

    int* aa = new int(33);
    delete aa;

    // function-style notation
    Buffer a(10);
    a.fill('A');
    a.print();

    // 也可以使用下面的形式, 创建一个临时的对象, 然后赋值给a2
    // 创建临时对象用的是overloaded con, 赋值用的是copy con.
    // 应该是compiler做了优化, 直接用一次overloaded con就好了.
    Buffer a2 = Buffer(12);

    // using uniform initialization, preferred
    Buffer a3{13};

    // copy con (有下面两种调用方式)
    cout << "--- Copy Constructor ---\n";
    // b还没有创建, 这里会call copy con去创建
    // 注意和 = op重载区分, 这里等号左边的对象b还没创建,
    // 故需要调用copy con，参数为a, 只会call copy con, 不会call = operator
    Buffer b = a;
    b.print();

    Buffer b2(a);
    b2.print();

    cout << "--- Copy Assignment ---\n";
    Buffer c;
    // 把b的数据成员的值赋值给c
    // 由于c已经事先被创建，故此处不会调用任何构造函数
    // 只会调用 = copy assignment operator
    c = b;

    cout << "--- Move Constructor ---\n";
    Buffer d = move(a);

    cout << "--- Move Assignment ---\n";
    Buffer e;
    e = move(b);

    return 0;
}

}  // namespace constructor_basic

/*===== Output =====

[RUN  ] constructor_basic
Constructor: Allocated 10 bytes
AAAAAAAAAA
Constructor: Allocated 12 bytes
Constructor: Allocated 13 bytes
--- Copy Constructor ---
Copy Constructor: Deep copied 10 bytes
AAAAAAAAAA
Copy Constructor: Deep copied 10 bytes
AAAAAAAAAA
--- Copy Assignment ---
Constructor: Allocated 128 bytes
Default Constructor
Freed 128 bytes
Copy Assignment: Deep copied 10 bytes
--- Move Constructor ---
Move Constructor: Moved ownership
--- Move Assignment ---
Constructor: Allocated 128 bytes
Default Constructor
Freed 128 bytes
Move Assignment: Moved ownership
Freed 10 bytes
Freed 10 bytes
Freed 10 bytes
Freed 10 bytes
Freed 0 bytes
Freed 13 bytes
Freed 12 bytes
Freed 0 bytes
[tid=1] [Memory Report] globalNewCnt = 9, globalDeleteCnt = 9, globalNewMemSize = 721, globalDeleteMemSize = 721
[   OK] constructor_basic

*/
