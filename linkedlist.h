#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <utility>
#include "types.h"
#include "traits.h"
using namespace std;

template <typename T>
class LLNode{
private:
    using    Type = T;
    using    Node = typename LLNode<T>;
    Type     m_data;
    Ref      m_ref; 
    Node    *m_pNext = nullptr;

public:
    LLNode(Type &elem, Ref ref, LLNode<T> *pNext = nullptr)
        : m_data(elem), m_pNext(pNext){
    }
    Type   GetData()    const  { return m_data;     }
    Type  &GetDataRef() { return m_data;     }
    Ref    GetRef()     { return m_ref;      }
    Node * GetNext()    const  { return m_pNext;    }
    Node *&GetNextRef() { return m_pNext;    }
};

template <typename T>
class CLinkedList{
private:
    using Type = T; 
    using Node = LLNode<Type>  ; 
    Node*m_pHead = nullptr;
    size_t m_size = 0;
    std::mutex m_mutex;
    void InternalInsert(LLNode<T>*& rParent, T elem, Ref ref) {
        if (!rParent || elem < rParent->GetData()) {
            rParent = new LLNode<T>(std::move(elem), ref, rParent);
            m_size++;
            return;
        }
        InternalInsert(rParent->GetNextRef(), std::move(elem), ref);
    }
    void InternalClear() {
        while (m_pHead) {
            auto temp = m_pHead;
            m_pHead = m_pHead->GetNext();
            delete temp;
        }
        m_size = 0;
    }
public:
    // Constructor
    CLinkedList();
    // TODO: Constructor Copia
    CLinkedList(CLinkedList &other) {
        std::scoped_lock lock(other.m_mutex);
        for (auto& item : other) {
            Insert(item, Ref{});
        }
    }

    // TODO: Move contructor
    CLinkedList(CLinkedList &&other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_pHead = std::exchange(other.m_pHead, nullptr);
        m_size = std::exchange(other.m_size, 0);
    }

    // Destructor seguro
    virtual ~CLinkedList(){
    	std::scoped_lock lock(m_mutex);
        InternalClear();
	}

    void Insert(Type &elem, Ref ref);
private:
    // TODO: Implementar
    void InternalInsert(Node *&rParent, Type &elem, Ref ref);
};

template <typename T>
void CLinkedList<T>::Insert(Type &elem, Ref ref){
    InternalInsert(m_pHead, elem, ref);
}

void InternalInsert(Type &elem, Ref ref) {
    std::scoped_lock lock(m_mutex);
    InternalInsert(m_pHead, std::move(elem), ref);
}

void Clear() {
    std::scoped_lock lock(m_mutex);
    InternalClear();
}
    
template <typename T>
void CLinkedList<T>::InternalInsert(Node *&rParent, Type &elem, Ref ref){
    if( !rParent || elem < rParent->GetDataRef() ){
        rParent = new Node(elem, ref, rParent);
        return;
    }
    // Tail recursion
    InternalInsert(rParent->GetNextRef(), elem, ref);
}

template <typename T>
CLinkedList<T>::CLinkedList()
{
    bool Contains(const T& elem) const {
        std::scoped_lock lock(m_mutex);
        auto current = m_pHead;
        while (current) {
            if (current->GetData() == elem) return true;
            current = current->GetNext();
        }
        return false;
    }

    // Eliminación práctica
    bool Remove(const T& elem) {
        std::scoped_lock lock(m_mutex);
        auto current = &m_pHead;

        while (*current) {
            if ((*current)->GetData() == elem) {
                auto temp = *current;
                *current = (*current)->GetNext();
                delete temp;
                m_size--;
                return true;
            }
            current = &(*current)->GetNextRef();
        }
        return false;
    }
    size_t Size() const { 
        std::scoped_lock lock(m_mutex);  // Thread-safe
        return m_size; 
    }
    bool Empty() const {
        std::scoped_lock lock(m_mutex);
        return m_size == 0;
    }
   class Iterator {
    private:
        LLNode<T>* m_pCurrent;
    public:
        Iterator(LLNode<T>* pNode = nullptr) : m_pCurrent(pNode) {}

        T& operator*() { return m_pCurrent->GetDataRef(); }
        T* operator->() { return &m_pCurrent->GetDataRef(); }

        Iterator& operator++() {
            m_pCurrent = m_pCurrent->GetNext();
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const Iterator& other) { 
            return m_pCurrent == other.m_pCurrent; 
        }

        bool operator!=(const Iterator& other) { 
            return m_pCurrent != other.m_pCurrent; 
        }
    };

    Iterator Begin() { 
        std::scoped_lock lock(m_mutex);
        return Iterator(m_pHead); 
    }
    Iterator End() { return Iterator(nullptr); }
    Iterator begin() { return Begin(); }
    Iterator end() { return End(); }
    // Serializacion
    void Write(std::ostream &os) {
        std::scoped_lock lock(m_mutex);
        os << m_size << "\n";
        auto current = m_pHead;
        while (current) {
            os << current->GetData() << " ";
            os.write(reinterpret_cast<char*>(&current->GetRef()), sizeof(Ref));
            current = current->GetNext();
        }
    }
    
    void Read(std::istream &is) {
        std::scoped_lock lock(m_mutex);
        InternalClear();
        size_t elementCount;
        is >> elementCount;
        for (size_t i = 0; i < elementCount; ++i) {
            T data;
            Ref ref;
            is >> data;
            is.read(reinterpret_cast<char*>(&ref), sizeof(Ref));
            Insert(std::move(data), ref);
        }
    }


    CLinkedList& operator=(CLinkedList other) {
        swap(other);
        return *this;
    }
}   
    void swap(CLinkedList& other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        std::swap(m_pHead, other.m_pHead);
        std::swap(m_size, other.m_size);
    }
    T* Front() {
        std::scoped_lock lock(m_mutex);
        return m_pHead ? &m_pHead->GetData() : nullptr;
    }

// Función swap 
template<typename T>
void swap(CLinkedList<T>& lhs, CLinkedList<T>& rhs) {
    lhs.swap(rhs);
}

#endif // __LINKEDLIST_H__
