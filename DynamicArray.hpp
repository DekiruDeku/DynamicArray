#pragma once

#include <cassert>
#include <cstdlib>      
#include <new>          
#include <type_traits>
#include <utility>      

template <typename T>
class Array final {
public:
    class Iterator;
    class ConstIterator;

    Array() : data_(allocate_raw(kDefaultCapacity)), size_(0), capacity_(kDefaultCapacity) {}

    explicit Array(int capacity)
        : data_(allocate_raw(normalize_capacity(capacity))),
          size_(0),
          capacity_(normalize_capacity(capacity)) {}

    Array(const Array& other)
        : data_(allocate_raw(other.capacity_ > 0 ? other.capacity_ : kDefaultCapacity)),
          size_(0),
          capacity_(other.capacity_ > 0 ? other.capacity_ : kDefaultCapacity) {
        try {
            for (int i = 0; i < other.size_; ++i) {
                construct_at(data_ + i, other.data_[i]);
                ++size_;
            }
        } catch (...) {
            destroy_range(data_, size_);
            std::free(data_);
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
            throw;
        }
    }

    Array(Array&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    Array& operator=(const Array& other) {
        if (this == &other) return *this;
        Array tmp(other);
        swap(tmp);
        return *this;
    }

    Array& operator=(Array&& other) noexcept {
        if (this == &other) return *this;
        clear_and_free();
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        return *this;
    }

    ~Array() { clear_and_free(); }

    int insert(const T& value) {
        ensure_capacity(size_ + 1);
        construct_at(data_ + size_, value);
        return size_++;
    }

    int insert(T&& value) {
        ensure_capacity(size_ + 1);
        construct_at(data_ + size_, std::move(value));
        return size_++;
    }

    int insert(int index, const T& value) {
        assert(index >= 0 && index <= size_);
        ensure_capacity(size_ + 1);
        shift_right(index);
        construct_at(data_ + index, value);
        ++size_;
        return index;
    }

    int insert(int index, T&& value) {
        assert(index >= 0 && index <= size_);
        ensure_capacity(size_ + 1);
        shift_right(index);
        construct_at(data_ + index, std::move(value));
        ++size_;
        return index;
    }

    void remove(int index) {
        assert(index >= 0 && index < size_);
        destroy_one(data_ + index);
        shift_left(index);
        --size_;
    }

    T& operator[](int index) {
        assert(index >= 0 && index < size_);
        return data_[index];
    }

    const T& operator[](int index) const {
        assert(index >= 0 && index < size_);
        return data_[index];
    }

    int size() const noexcept { return size_; }

    int capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }
    void clear() noexcept {
        destroy_range(data_, size_);
        size_ = 0;
    }

    class Iterator {
    public:
        Iterator(Array* owner, int pos, bool reverse) noexcept
            : owner_(owner), pos_(pos), reverse_(reverse) {}

        const T& get() const { return (*owner_)[pos_]; }
        void set(const T& value) { (*owner_)[pos_] = value; }

        void next() { reverse_ ? --pos_ : ++pos_; }

        bool hasNext() const {
            if (!owner_) return false;
            return reverse_ ? (pos_ >= 0) : (pos_ < owner_->size());
        }

        const T& operator*() const { return get(); }
        Iterator& operator++() {
            next();
            return *this;
        }
        Iterator operator++(int) {
            Iterator tmp = *this;
            next();
            return tmp;
        }

    private:
        Array* owner_;
        int pos_;
        bool reverse_;
    };

    class ConstIterator {
    public:
        ConstIterator(const Array* owner, int pos, bool reverse) noexcept
            : owner_(owner), pos_(pos), reverse_(reverse) {}

        const T& get() const { return (*owner_)[pos_]; }

        void next() { reverse_ ? --pos_ : ++pos_; }

        bool hasNext() const {
            if (!owner_) return false;
            return reverse_ ? (pos_ >= 0) : (pos_ < owner_->size());
        }

        const T& operator*() const { return get(); }
        ConstIterator& operator++() {
            next();
            return *this;
        }
        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            next();
            return tmp;
        }

    private:
        const Array* owner_;
        int pos_;
        bool reverse_;
    };

    Iterator iterator() { return Iterator(this, 0, false); }
    ConstIterator iterator() const { return ConstIterator(this, 0, false); }

    Iterator reverseIterator() {
        return Iterator(this, size_ > 0 ? (size_ - 1) : -1, true);
    }
    ConstIterator reverseIterator() const {
        return ConstIterator(this, size_ > 0 ? (size_ - 1) : -1, true);
    }

    T* begin() noexcept { return data_; }
    T* end() noexcept { return data_ + size_; }
    const T* begin() const noexcept { return data_; }
    const T* end() const noexcept { return data_ + size_; }
    const T* cbegin() const noexcept { return data_; }
    const T* cend() const noexcept { return data_ + size_; }

    void swap(Array& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

private:
    static constexpr int kDefaultCapacity = 8;
    static constexpr double kGrowthFactor = 2.0;

    T* data_ = nullptr;
    int size_ = 0;
    int capacity_ = 0;

    static int normalize_capacity(int cap) noexcept {
        return (cap > 0) ? cap : kDefaultCapacity;
    }

    static T* allocate_raw(int n) {
        if (n <= 0) return nullptr;
        void* mem = std::malloc(sizeof(T) * static_cast<std::size_t>(n));
        if (!mem) throw std::bad_alloc();
        return static_cast<T*>(mem);
    }

    static void destroy_one(T* ptr) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ptr->~T();
        }
    }

    static void destroy_range(T* ptr, int n) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (int i = 0; i < n; ++i) {
                (ptr + i)->~T();
            }
        }
    }

    template <typename U>
    static void construct_at(T* dst, U&& src) {
        ::new (static_cast<void*>(dst)) T(std::forward<U>(src));
    }

    void ensure_capacity(int min_capacity) {
        if (min_capacity <= capacity_) return;

        int new_capacity = capacity_;
        if (new_capacity <= 0) new_capacity = 1;

        while (new_capacity < min_capacity) {
            const int grown = static_cast<int>(new_capacity * kGrowthFactor);
            new_capacity = (grown > new_capacity) ? grown : (new_capacity + 1);
        }

        T* new_block = allocate_raw(new_capacity);
        int constructed = 0;
        try {
            for (int i = 0; i < size_; ++i) {
                if constexpr (std::is_move_constructible_v<T>) {
                    construct_at(new_block + i, std::move(data_[i]));
                } else {
                    static_assert(std::is_copy_constructible_v<T>,
                                  "T must be move-constructible or copy-constructible");
                    construct_at(new_block + i, data_[i]);
                }
                ++constructed;
            }
        } catch (...) {
            destroy_range(new_block, constructed);
            std::free(new_block);
            throw;
        }

        destroy_range(data_, size_);
        std::free(data_);

        data_ = new_block;
        capacity_ = new_capacity;
    }

    void shift_right(int from_index) {
        for (int i = size_; i > from_index; --i) {
            if constexpr (std::is_move_constructible_v<T>) {
                construct_at(data_ + i, std::move(data_[i - 1]));
            } else {
                construct_at(data_ + i, data_[i - 1]);
            }
            destroy_one(data_ + (i - 1));
        }
    }

    void shift_left(int from_index) {
        for (int i = from_index; i < size_ - 1; ++i) {
            if constexpr (std::is_move_constructible_v<T>) {
                construct_at(data_ + i, std::move(data_[i + 1]));
            } else {
                construct_at(data_ + i, data_[i + 1]);
            }
            destroy_one(data_ + (i + 1));
        }
    }

    void clear_and_free() noexcept {
        if (data_) {
            destroy_range(data_, size_);
            std::free(data_);
        }
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
};

namespace DynamicArray {
    template <typename T>
    using Array = ::Array<T>;
}
