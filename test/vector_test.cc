#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <sstream>

template <typename T>
void PrintVector(const std::vector<T> &vec) {
  std::cout << "{";
  for (const T &ele: vec) {
    std::cout << ele << ", ";
  }
  std::cout << "\b\b}\n" << std::endl;
}


class A {
 public:
  A(): a_(0) { std::cout << "construct 0, a is " << 0 << std::endl; }

  A(int a): a_(a) { std::cout << "construct 1, a is " << a << std::endl; }

  A(const A & other) { 
    std::cout << "copy construct, a is " << other.a_ << std::endl; 
    a_ = other.a_;
  }

  A(const A && other) { 
    std::cout << "move construct, a is " << other.a_ << std::endl; 
    a_ = other.a_;
  }

  ~A() { std::cout << "de-construct, a is " << a_ << std::endl; }

  A & operator=(const A & other) { 
    std::cout << "operator=, a is " << other.a_ << std::endl; 
    a_ = other.a_;
    return *this;
  }

  friend std::ostream & operator<<(std::ostream & o, const A & a);

 private:
  int a_;
};


std::ostream & operator<<(std::ostream & o, const A & a) {
  o << "A(" << a.a_ << ")";
  return o;
}


TEST(VECTOR, Construct) {
  std::vector<int> first;  // size为0, capacity为0, sizeof vector is 24
  std::cout << "sizeof vector is " << sizeof(first) << ", size is " << first.size() << ", capacity is " << first.capacity() << std::endl;
  /* The 24 size you see can be explained as 3 pointers. These pointers can be:
   * ----------------------------------
   * ^           ^                     ^
   * |           |                     |
   * begin      end                 reserved
   * size = end - begin
   * capacity = reserved - begin
   * sizeof(vector) = 3 * 8 = 24, 就是begin, end, reserved三个指针
   * resize并不一定改变capacity, 但是可能导致元素的构造与析构
   * shrink_to_fit 的作用是使 reserved == end, 可能重新分配内存
   */

  std::vector<int> second (4);                           // size为4, 值为0, 注意其中是有值的
  std::vector<int> third (second.begin(),second.end());  // 使用迭代器构造[begin, end), 事实上end是指向最后一个元素的下一个位置
  std::vector<int> fourth (third);                       // copy/move 构造

  // 迭代器很多时候相当于指针
  int myints[] = {16, 2, 77, 29};   // sizeof(array), 包括了元素空间, 但sizeof(vector)没有, 单位是byte
  std::vector<int> fifth (myints, myints + sizeof(myints) / sizeof(int));
  std::vector<int> six = {1, 4, 7, 0};  // 用初始化器构造 等价于 six({1, 4, 7, 0})
}


TEST(VECTOR, ElementAccess) {
  // 所有的 ElementAccess 操作都不会使 size 改变
  // front()/back() 返回道/尾元素的引用, 可以直接修改(也有const版本)
  // operator[]/at() 前者不会进行out_of_range检查, 后者会. 所以使用operator[]要小心

  std::vector<int> myvector (10);   // 10 zero-initialized elements
  myvector.reserve(30);
  // myvector.at(25) = 5;           // 报错, 25 > 10
  myvector[25] = 5;                 // 不报错, 但是是未定义行为, size不会增加
  myvector[40] = 10;                // 不报错, 但是是未定义行为, 可能出core
  std::cout << myvector[25] << ", size is " << myvector.size() << std::endl;

  int* p = myvector.data();         // data() 可以返回指针(也有const版本)
  *p = 10;
  ++p;    // 移动指针
  *p = 20;
  p[2] = 100;
  PrintVector(myvector);

}


TEST(VECTOR, Modifiers){
  // 1) 1, 2通过隐式转换成对象A, 并生成初始化列表
  // 2) 然后copy构造到vector (因为是用初始化列表中的元素构造, 所以是copy)
  // 3) 析构初始化列表, 进而析构隐式转换成对象, 顺序为2, 1
  std::vector<A> myvector = {1, 2};
  std::cout << "sizeof vector is " << sizeof(myvector) << ", size is " << myvector.size() << ", capacity is " << myvector.capacity() << std::endl;
  
  // 1) 3通过隐式转换成对象A 
  // 2) 将myvector的capacity extend成4
  // 3) 并通过move构造, 将A(3) move到index 2上 
  // 4) 从后向前将元素copy构造新位置上
  // 5) 释方老内存, 析构老元素 2, 1
  // 6) 析构隐式转换成对象A(3)
  myvector.push_back(3);
  std::cout << "sizeof vector is " << sizeof(myvector) << ", size is " << myvector.size() << ", capacity is " << myvector.capacity() << std::endl;
  
  // 1) 构造临时对象A(4)
  // 2) 将A(4) move到index 3上 
  // 3) 析造临时对象A(4)
  myvector.push_back(A(4));
  std::cout << "sizeof vector is " << sizeof(myvector) << ", size is " << myvector.size() << ", capacity is " << myvector.capacity() << std::endl;
  
  // 1) 将myvector的capacity extend成8
  // 2) 在index 4上直接构造A(5)
  // 3) 从后向前将元素copy构造新位置上
  // 4) 释方老内存, 析构老元素 4, 3, 2, 1
  myvector.emplace_back(5);
  std::cout << "sizeof vector is " << sizeof(myvector) << ", size is " << myvector.size() << ", capacity is " << myvector.capacity() << std::endl;
  
  myvector.emplace_back(A(6));  // 等价于 myvector.push_back(A(4))

  myvector.emplace(myvector.end(), 7);  // 等价于 myvector.emplace_back(7)
  std::cout << "sizeof vector is " << sizeof(myvector) << ", size is " << myvector.size() << ", capacity is " << myvector.capacity() << std::endl;
  
  // 1) 构造临时对象A(0)
  // 2) 移动构造最后一个元素A(7)
  // 3) 通过赋值移动中间元素
  // 4) 析构临时对象A(0)
  myvector.emplace(myvector.begin(), 0);
  std::cout << "sizeof vector is " << sizeof(myvector) << ", size is " << myvector.size() << ", capacity is " << myvector.capacity() << std::endl;
  
  PrintVector<A>(myvector);
}