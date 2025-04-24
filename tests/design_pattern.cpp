#include <iostream>
#include <memory>
#include <vector>
using namespace std;

namespace design_pattern {

//=========================================================
// 非模板实现Singleton Logger
// 全局只有一个Logger类型的单例, 如果要一个Config类型的单例, 要在写一份
// 关键部分:
// 1, static成员函数返回static对象, 返回reference不是copy (用的时候对应用reference接收)
// 2, 删除拷贝构造和赋值运算符, 构造析构私有

class Logger {
public:
    // 获取唯一实例的静态方法
    static Logger& getInstance() {
        static Logger instance;  // C++11 后是线程安全的
        return instance;
    }

    // 删除拷贝构造和赋值运算符，防止复制
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 提供一个成员函数用于演示
    void doSomething() {
        cout << "Logger is doing something!" << endl;
    }

private:
    // 构造函数设为私有，防止外部创建
    Logger() {
        cout << "Logger Constructor called." << endl;
    }

    // 析构函数
    ~Logger() {
        cout << "Logger Destructor called." << endl;
    }
};

void subtest1() {
    cout << __FUNCTION__ << endl;

    Logger& log1 = Logger::getInstance();
    log1.doSomething();

    Logger& log2 = Logger::getInstance();
    if (&log1 == &log2) {
        cout << "log1 and log2 are same." << endl;
    }
}

//=========================================================
// 模板实现的 Singleton
// 高复用，任何类型都能作为单例, 每个类型一个单例
// 和上面一样, 就多了模板

template <typename T>
class Singleton {
public:
    // 获取单例实例
    static T& getInstance() {
        static T instance;  // 这个静态局部变量会在第一次调用时创建
        return instance;
    }

    // 禁止拷贝构造和赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    // 私有化构造函数，外部不能直接创建实例
    Singleton() {
        cout << "Singleton instance created!" << endl;
    }

    // 私有化析构函数
    ~Singleton() {
        cout << "Singleton instance destroyed!" << endl;
    }
};

// 类型例子.
class Config {
public:
    void showMessage() {
        cout << "Hello from Config class!" << endl;
    }
};

void subtest2() {
    cout << __FUNCTION__ << endl;

    // 获取并使用 Singleton 实例
    Config& config1 = Singleton<Config>::getInstance();
    config1.showMessage();

    Config& config2 = Singleton<Config>::getInstance();
    config2.showMessage();

    if (&config1 == &config2) {
        cout << "config1 and config2 are same" << endl;
    }
}

//=========================================================
// Observer pattern 101

class Observer {
public:
    virtual void update() = 0;
};

class Subject {
private:
    vector<shared_ptr<Observer>> observers;

    void notify() {
        for (auto& observer : observers) {
            observer->update();
        }
    }

public:
    void attach(shared_ptr<Observer> observer) {
        observers.push_back(observer);
    }

    void changeState() {
        cout << "State changed!" << endl;
        notify();
    }
};

class ConcreteObserver : public Observer {
public:
    void update() override {
        cout << "Observer updated!" << endl;
    }
};

void subtest3() {
    cout << __FUNCTION__ << endl;

    shared_ptr<Subject> subject = make_shared<Subject>();
    shared_ptr<Observer> observer1 = make_shared<ConcreteObserver>();
    shared_ptr<Observer> observer2 = make_shared<ConcreteObserver>();

    subject->attach(observer1);
    subject->attach(observer2);

    subject->changeState();  // 触发状态变化并通知所有观察者
}

//=========================================================
// Factory pattern 101

class Shape {
public:
    virtual void draw() = 0;
    virtual ~Shape() {}
};

class Circle : public Shape {
public:
    void draw() override {
        cout << "Drawing Circle" << endl;
    }
};

class Rectangle : public Shape {
public:
    void draw() override {
        cout << "Drawing Rectangle" << endl;
    }
};

class ShapeFactory {
public:
    shared_ptr<Shape> createShape(const string& shapeType) {
        if (shapeType == "Circle") {
            return make_shared<Circle>();
        } else if (shapeType == "Rectangle") {
            return make_shared<Rectangle>();
        }
        return nullptr;
    }
};

void subtest4() {
    cout << __FUNCTION__ << endl;

    ShapeFactory factory;
    auto shape1 = factory.createShape("Circle");
    shape1->draw();
    auto shape2 = factory.createShape("Rectangle");
    shape2->draw();
}

int cppMain() {
    subtest1();
    subtest2();
    subtest3();
    subtest4();
    return 0;
}

}  // namespace design_pattern

/*===== Output =====

[RUN  ] design_pattern
subtest1
Logger Constructor called.
Logger is doing something!
log1 and log2 are same.
subtest2
Hello from Config class!
Hello from Config class!
config1 and config2 are same
subtest3
State changed!
Observer updated!
Observer updated!
subtest4
Drawing Circle
Drawing Rectangle
[tid=1] [Memory Report] globalNewCnt = 7, globalDeleteCnt = 7, globalNewMemSize = 492, globalDeleteMemSize = 492
[   OK] design_pattern
Logger Destructor called.

*/
