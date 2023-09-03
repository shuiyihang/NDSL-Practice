

#### 定义抽象数据类型：类



a.fun().test()

class和struct区别：
默认访问权限不同，class不指定pp时候默认private，struct默认public

**1.const成员函数**
参数列表之后的const关键字，修改this指针类型

```
this默认为：
type* const this;// 指针变量不可修改

成员函数
void func() const{} // 指针指向的值也不可修改
等价为：
const type* const this;
```
成员函数体可以随意使用类中的其他成员而无需在意成员出现的次序

const成员函数以引用的形式返回*this,返回类型将是常量的引用

**可变数据成员**
属性声明加mutable，即使是const成员函数(常函数)中也可以改变mutable变量的值

**2.类作用域和成员函数**
类外定义成员函数
```
1. 定义与声明一致
2. 格式： 返回类型 类名::成员函数(){}
```

#### 构造函数

类的对象被创建，就会执行构造函数

构造函数的名字和类名相同，<font color=#ff000>没有返回类型</font>
1. 没有显示定义构造函数，编译器会隐式定义一个默认构造函数
2. 一旦定义其他的构造函数，类将没有默认构造函数


```
1. 默认构造函数
类名() = default;
2. 构造函数初始值列表
sales_data(unsigned n = 10):units_sold(n){};
```

3. 初始化列表中，初始化顺序与类定义中的出现顺序一致
4. 默认实参: 构造函数为所有参数提供默认实参，则实际上也定义了默认构造函数。

**委托构造函数**

受委托的构造函数体不为空的话，先执行，控制权才会交还给委托者的函数体。
```
class window_mgr{
    public:
        window_mgr(std::string s,int n):text(s),num(n){cout<<"被委托者"<<endl;}
        window_mgr():window_mgr("text",15){cout<<"委托者"<<endl;}
        void show(){cout<<text<<" "<<num<<endl;}
    private:
        std::vector<screen> screens{screen(24,80,' ')};
        std::string text;
        int num;
        
};

 window_mgr w1;
 w1.show();

// 输出：
被委托者
委托者
text 15

```

**拷贝构造函数**
<font color=#ff000>有了拷贝函数，不会再有其他构造函数</font>

```
struct sales_data{
    // sales_data() = default;
    // sales_data(unsigned n):units_sold(n){};
    // sales_data(std::istream &is);
    sales_data(const sales_data& p){};

    unsigned units_sold = 0;
    double revenue = 0.0;
};

sales_data t1,t2; // no default constructor exists for class "sales_data"
```
调用时机：

1. 赋值
2. 传参
3. 返回值

**深浅拷贝**
浅拷贝：赋值拷贝操作
深拷贝：有属性在堆区开辟，提供拷贝构造函数。


#### 析构函数
~类名(){}
1. 无返回值
2. 析构函数无参数，不可发生重载
3. 对象销毁前自动调用析构


#### 访问控制与封装

1. public
成员整个程序内可被访问
2. private
使用该类的代码不可访问，仅成员函数可以访问

一个类可以包含0或多个访问说明符。有效范围直到出现下一个访问说明符或者达到类的结尾为止。



**友元**
1. 类的非成员接口在类中声明
```
class  类名{
    friend 函数声明;
}
```
2. 类之间的友元关系
```
class screen{
    friend class window_mgr;// window_mgr成员函数可以访问screen的私有部分
}
```
<font color=#ff000>类的友元关系不存在传递性</font>

3. 成员函数做友元
```
class screen{
    friend void window_mgr::clear();
}
```
友元声明的作用是影响访问权限，本身并非声明
成员函数做内联函数

1. 定义在类内部的成员函数自动inline
2. 类内部inline声明成员函数或者类外部inline函数定义

**类数据成员的初始值**
类内初始值使用等号的初始化形式或者花括号括起来的初始化形式。

```
1. 变量 = 初始值
2. 变量{初始值}

class window_mgr{
        std::string text = "yes";
        int num{20};
};
```
int a[fun()]



**类的作用域**
名字的查找

```
1. 成员函数内查找名字的声明
2. 在类内继续查找，类的所有成员都可以被考虑
3. 在成员函数定义之前的作用域内继续查找
```
遇到同名，可以通过加上类的名字或显示使用this访问成员。


**默认初始化和值初始化**
定义变量时没有指定初值，则变量被默认初始化

**隐式转换**
```
item.combine(string("xxx")); // combine接受一个scale_data的类，构造函数创建了scale_data对象。

```
在类内声明构造函数时使用explicit阻止隐式转换。


#### 类的静态成员
静态数据成员作用：对象共享数据。
静态成员函数：只能访问静态成员。

使用作用域运算符::或者使用类的对象、引用、指针来访问静态成员。

1. 类外定义静态成员函数时候，不能重复static关键字，该关键字只能出现类内部的声明语句。
2. 在类的外部定义和初始化每个静态成员。<font color=#ff000>即类内声明类外初始化。</font>

#### 小结
类：
1. 抽象：定义数据成员和函数成员
2. 封装：控制类的成员的访问

构造函数：控制初始化对象的方式。
可变成员mutable
静态成员static