#include <gtest/gtest.h>
#include "DynamicArray.hpp"
#include <string>
#include <utility>

TEST(ArrayBasic, Constructors) {
    DynamicArray::Array<int> a;
    EXPECT_EQ(a.size(), 0);
    DynamicArray::Array<int> b(5);
    EXPECT_EQ(b.capacity(), 5);
    for(int i = 0; i < 5; ++i)
		b.insert(i);
    DynamicArray::Array<int> c(std::move(b));
	EXPECT_EQ(c.size(), 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(c[i], i);

}

TEST(ArrayEdge, InsertAtBeginning) {
    DynamicArray::Array<int> a;
    a.insert(2);
    a.insert(3);
    a.insert(0, 1);

    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 2);
    EXPECT_EQ(a[2], 3);
}

TEST(ArrayEdge, InsertAtEnd) {
    DynamicArray::Array<int> a;
    a.insert(1);
    a.insert(2);
    a.insert(2, 3);

    EXPECT_EQ(a[2], 3);
}

TEST(ArrayBasic, InsertAtMiddle) {
    DynamicArray::Array<int> a;
    for (int i = 0; i < 5; ++i) 
        a.insert(i + 1);
    a.insert(2, 99);
    ASSERT_EQ(a.size(), 6);
    EXPECT_EQ(a[2], 99);
    EXPECT_EQ(a[3], 3);
}

TEST(ArrayEdge, RemoveBeginning) {
    DynamicArray::Array<int> a;
    a.insert(1);
    a.insert(2);
    a.insert(3);

    a.remove(0);

    EXPECT_EQ(a[0], 2);
    EXPECT_EQ(a[1], 3);
}

TEST(ArrayEdge, RemoveEnd) {
    DynamicArray::Array<int> a;
    a.insert(1);
    a.insert(2);
    a.insert(3);

    a.remove(2);
    EXPECT_EQ(a.size(), 2);
}

TEST(ArrayEdge, RemoveAtMiddle) {
    DynamicArray::Array<int> a;
    for (int i = 0; i < 5; ++i)
        a.insert(i + 1);
    a.remove(1);
    ASSERT_EQ(a.size(), 4);
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 3);
    EXPECT_EQ(a[3], 5);
}

TEST(ArrayBasic, Clear) {
    DynamicArray::Array<int> a;
    a.insert(1);
    a.insert(2);
    a.insert(3);

    a.clear();
    EXPECT_EQ(a.size(), 0);
    EXPECT_TRUE(a.empty());
}

TEST(ArrayBasic, CopyAndMove) {
    DynamicArray::Array<std::string> a;
    a.insert("one");
    a.insert("two");
    DynamicArray::Array<std::string> b = a; // copy constructor calling
    EXPECT_EQ(b.size(), 2);
    EXPECT_EQ(b[0], "one");
    DynamicArray::Array<std::string> c = std::move(a);
    EXPECT_EQ(c.size(), 2);
}

TEST(ArrayBasic, RangeFor) {
    DynamicArray::Array<int> a;
    for (int i = 0; i < 10; ++i) 
        a.insert(i + 1);
    int expected = 1;
    for (auto& x : a) {
        EXPECT_EQ(x, expected);
        expected++;
    }
}

TEST(ArrayBasic, Swap) {
    DynamicArray::Array<int> a;
    for (int i = 0; i < 10; ++i)
        a.insert(i + 1);

    DynamicArray::Array<int> b(a);

    for (auto it = a.iterator(); it.hasNext(); it.next())
        it.set(it.get() * 2);

    a.swap(b);
    int expected = 2;
    for (auto it = b.iterator(); it.hasNext(); it.next()) {
        EXPECT_EQ(it.get(), expected);
        expected += 2;
    }
}

TEST(ArrayIterator, IterateAndModify) {
    DynamicArray::Array<int> a;
    for (int i = 0; i < 10; ++i)
        a.insert(i + 1);

    for (auto it = a.iterator(); it.hasNext(); it.next())
        it.set(it.get() * 2);

    const DynamicArray::Array<int> b(a);
    int expected = 2;
    for (auto it = b.iterator(); it.hasNext(); it.next()) {
        EXPECT_EQ(it.get(), expected);
        expected += 2;
    }
}

TEST(ArrayIterator, ManualIteration) {
    DynamicArray::Array<int> a;
    a.insert(10);
    a.insert(20);
    a.insert(30);

    auto it = a.iterator();
    EXPECT_TRUE(it.hasNext());
    EXPECT_EQ(*(it++), 10);
    EXPECT_EQ(*it, 20);
    EXPECT_EQ(*(++it), 30);
}

struct Counter {
    int value;
    static int ConstructorCalling;
    static int CopyConstructorCalling;
    static int MoveConstructorCalling;
    static int Destructor;

    Counter(int v = 0) : value(v) { ConstructorCalling++; }
    Counter(const Counter& other) : value(other.value) { CopyConstructorCalling++; }
    Counter(Counter&& other) noexcept : value(other.value) { MoveConstructorCalling++; }
    ~Counter() { Destructor++; }

    static void Reset() {
        ConstructorCalling = CopyConstructorCalling = MoveConstructorCalling = Destructor = 0;
    }
};

int Counter::ConstructorCalling = 0;
int Counter::CopyConstructorCalling = 0;
int Counter::MoveConstructorCalling = 0;
int Counter::Destructor = 0;

TEST(ArrayComplex, CheckComplexTypeConstructionDestruction) {
    Counter::Reset();
    {
        // Force reallocations so we can verify that move-semantics are used during growth.
        DynamicArray::Array<Counter> arr(1);
        arr.insert(Counter(5));
        arr.insert(Counter(10));
        arr.insert(Counter(20));
    }
    EXPECT_EQ(Counter::ConstructorCalling, 3);
    // With initial capacity=1 the array must reallocate at least once, so moves must happen.
    EXPECT_GT(Counter::MoveConstructorCalling, 0);
    // Every constructed Counter must be destructed exactly once.
    EXPECT_EQ(Counter::Destructor,
              Counter::ConstructorCalling + Counter::CopyConstructorCalling + Counter::MoveConstructorCalling);
}