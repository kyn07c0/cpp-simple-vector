#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <iterator>
#include "array_ptr.h"

using namespace std;

class ReserveProxyObj
{
    public:
        explicit ReserveProxyObj(size_t capacity) : capacity_(capacity) {}
        [[nodiscard]] size_t get_capacity() const { return capacity_; }

    private:
        size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity)
{
    return ReserveProxyObj(capacity);
}


template <typename Type>
class SimpleVector
{
    public:
        using Iterator = Type*;
        using ConstIterator = const Type*;

        SimpleVector() noexcept = default;

        // Создаёт вектор из size элементов, инициализированных значением по умолчанию
        explicit SimpleVector(size_t size) : array_(size), size_(size), capacity_(size)
        {
            fill(this->begin(), this->end(), Type());
        }

        // Создаёт вектор из size элементов, инициализированных значением value
        SimpleVector(size_t size, const Type& value) : array_(size), size_(size), capacity_(size)
        {
            fill(this->begin(), this->end(), value);
        }

        // Создаёт вектор из std::initializer_list
        SimpleVector(initializer_list<Type> init) : array_(init.size()), size_(init.size()), capacity_(init.size())
        {
            copy(init.begin(), init.end(), array_.Get());
        }

        SimpleVector(const SimpleVector& other) : size_(other.size_), capacity_(other.capacity_)
        {
            if(other.begin() != nullptr)
            {
                ArrayPtr<Type> new_array(other.size_);
                move(other.begin(), other.end(), new_array.Get());
                array_.swap(new_array);
            }
        }

        SimpleVector(SimpleVector&& other) noexcept
        {
            array_ = move(other.array_);
            size_ = exchange(other.size_, 0);
            capacity_ = exchange(other.capacity_, 0);
        }

        explicit SimpleVector(ReserveProxyObj proxy_obj)
        {
            capacity_ = proxy_obj.get_capacity();
        }

        SimpleVector& operator=(const SimpleVector& rhs)
        {
            if(this != &rhs)
            {
                if (rhs.IsEmpty())
                {
                    Clear();
                    return *this;
                }

                SimpleVector temp(rhs.size_);
                copy(rhs.begin(), rhs.end(), temp.begin());
                temp.capacity_ = rhs.capacity_;
                swap(temp);
            }

            return *this;
        }

        SimpleVector& operator=(SimpleVector&& rhs) noexcept
        {
            if(this != &rhs)
            {
                if (rhs.IsEmpty())
                {
                    Clear();
                    return *this;
                }

                SimpleVector temp(rhs.size_);
                copy(rhs.begin(), rhs.end(), temp.begin());
                temp.capacity_ = rhs.capacity_;
                swap(temp);
            }

            return *this;
        }

        void Reserve(size_t new_capacity)
        {
            if(new_capacity > capacity_)
            {
                ArrayPtr<Type> new_array(new_capacity);
                copy(begin(), end(), new_array.Get());
                array_.swap(new_array);

                capacity_ = new_capacity;
            }
        }

        // Добавляет элемент в конец вектора
        // При нехватке места увеличивает вдвое вместимость вектора
        void PushBack(const Type& item)
        {
            if(size_ < capacity_)
            {
                *(array_.Get() + size_) = item;
                ++size_;
            }
            else
            {
                size_t new_capacity = max(size_+ 1, capacity_ * 2);

                ArrayPtr<Type> new_array(new_capacity);
                copy(begin(), end(), new_array.Get());

                Type* new_item_ptr = new_array.Get() + size_;
                *new_item_ptr = item;

                array_.swap(new_array);

                ++size_;
                capacity_ = new_capacity;
            }
        }

        void PushBack(Type&& item)
        {
            if(size_ < capacity_)
            {
                *(array_.Get() + size_) = move(item);
                ++size_;
            }
            else
            {
                size_t new_capacity = max(size_+ 1, capacity_ * 2);

                ArrayPtr<Type> new_array(new_capacity);
                move(begin(), end(), new_array.Get());
                new_array[size_] = move(item);

                array_.swap(new_array);

                ++size_;
                capacity_ = new_capacity;
            }
        }

        // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
        void PopBack() noexcept
        {
            if(size_ > 0)
            {
                --size_;
            }
        }

        // Обменивает значение с другим вектором
        void swap(SimpleVector& other) noexcept
        {
            array_.swap(other.array_);

            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        }

        // Возвращает количество элементов в массиве
        [[nodiscard]] size_t GetSize() const noexcept
        {
            return size_;
        }

        // Возвращает вместимость массива
        [[nodiscard]] size_t GetCapacity() const noexcept
        {
            return capacity_;
        }

        // Сообщает, пустой ли массив
        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return size_ == 0;
        }

        // Возвращает ссылку на элемент с индексом index
        Type& operator[](size_t index) noexcept
        {
            return array_[index];
        }

        // Возвращает константную ссылку на элемент с индексом index
        const Type& operator[](size_t index) const noexcept
        {
            return array_[index];
        }

        // Возвращает константную ссылку на элемент с индексом index
        // Выбрасывает исключение std::out_of_range, если index >= size
        Type& At(size_t index)
        {
            if(index >= size_)
                throw out_of_range("index >= size");

            return array_[index];
        }

        // Возвращает константную ссылку на элемент с индексом index
        // Выбрасывает исключение std::out_of_range, если index >= size
        const Type& At(size_t index) const
        {
            if(index >= size_)
                throw out_of_range("index >= size");

            return array_[index];
        }

        // Обнуляет размер массива, не изменяя его вместимость
        void Clear() noexcept
        {
            size_ = 0;
        }

        // Изменяет размер массива.
        // При увеличении размера новые элементы получают значение по умолчанию для типа Type
        void Resize(size_t new_size)
        {
            if(new_size <= size_)
            {
                size_ = new_size;
            }
            else if(new_size <= capacity_)
            {
                for(size_t i = size_; i < new_size; i++)
                {
                    array_[i] = Type();
                }
                size_ = new_size;
            }
            else if(new_size > capacity_)
            {
                size_t new_capacity = max(new_size, capacity_ * 2);

                ArrayPtr<Type> new_array(new_capacity);
                copy(make_move_iterator(begin()), make_move_iterator(end()), new_array.Get());
                array_.swap(new_array);

                size_ = new_size;
                capacity_ = new_capacity;
            }
        }

        // Возвращает итератор на начало массива
        // Для пустого массива может быть равен (или не равен) nullptr
        Iterator begin() noexcept
        {
            return array_.Get();
        }

        // Возвращает итератор на элемент, следующий за последним
        // Для пустого массива может быть равен (или не равен) nullptr
        Iterator end() noexcept
        {
            return array_.Get() + size_;
        }

        // Возвращает константный итератор на начало массива
        // Для пустого массива может быть равен (или не равен) nullptr
        ConstIterator begin() const noexcept
        {
            return array_.Get();
        }

        // Возвращает итератор на элемент, следующий за последним
        // Для пустого массива может быть равен (или не равен) nullptr
        ConstIterator end() const noexcept
        {
            return array_.Get() + size_;
        }

        // Возвращает константный итератор на начало массива
        // Для пустого массива может быть равен (или не равен) nullptr
        ConstIterator cbegin() const noexcept
        {
            return array_.Get();
        }

        // Возвращает итератор на элемент, следующий за последним
        // Для пустого массива может быть равен (или не равен) nullptr
        ConstIterator cend() const noexcept
        {
            return array_.Get() + size_;
        }

        // Вставляет значение value в позицию pos.
        // Возвращает итератор на вставленное значение
        // Если перед вставкой значения вектор был заполнен полностью,
        // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
        Iterator Insert(ConstIterator pos, const Type& value)
        {
            assert(pos >= begin() && pos <= end());

            Type* insert_pos = const_cast<Type*>(pos);
            size_t index_insert = pos - begin();

            if(size_ == capacity_)
            {
                size_t new_capacity = max(size_ + 1, capacity_ * 2);

                ArrayPtr<Type> new_array(new_capacity);
                copy(begin(), insert_pos, new_array.Get());
                new_array[index_insert] = copy(value);
                copy(insert_pos, end(), new_array.Get() + index_insert + 1);

                array_.swap(new_array);
                capacity_ = new_capacity;
            }
            else
            {
                copy_backward(insert_pos, end(), end() + 1);
                array_[index_insert] = value;
            }

            ++size_;

            return insert_pos;
        }

        Iterator Insert(ConstIterator pos, Type&& value)
        {
            assert(pos >= begin() && pos <= end());

            Type* insert_pos = const_cast<Type*>(pos);
            size_t index_insert = pos - begin();

            if(size_ == capacity_)
            {
                size_t new_capacity = max(size_ + 1, capacity_ * 2);

                ArrayPtr<Type> new_array(new_capacity);
                move(begin(), insert_pos, new_array.Get());
                move_backward(insert_pos, end(), new_array.Get() + size_ + 1);
                new_array[index_insert] = move(value);

                array_.swap(new_array);
                capacity_ = new_capacity;
            }
            else
            {
                move_backward(insert_pos, end(), end() + 1);
                array_[index_insert] = move(value);
            }

            ++size_;
            return Iterator{&array_[index_insert]};
        }

        // Удаляет элемент вектора в указанной позиции
        Iterator Erase(ConstIterator pos)
        {
            assert(pos >= begin() && pos <= end());

            if(size_ == 0)
                return nullptr;

            Type* del_pos = const_cast<Type*>(pos);

            if(del_pos == end())
            {
                --size_;
            }
            else
            {
                move(del_pos + 1, end(), del_pos);
                --size_;
            }

            return del_pos;
        }

    private:
        ArrayPtr<Type> array_;

        size_t size_ = 0;
        size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return (lhs < rhs) || (lhs == rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs < rhs) && (lhs != rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
    return !(lhs < rhs);
}
