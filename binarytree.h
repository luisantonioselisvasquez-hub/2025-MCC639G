#ifndef __BINARY_TREE_H__  
#define __BINARY_TREE_H__ 
#include <utility>
#include <algorithm>
#include <cassert>
#include "types.h"
#include <mutex>
#include "util.h"
#include <iostream>
#include <vector>
using namespace std;

template <typename Traits, typename... Args>
class CBinaryTreeNode{
public:
  // TODO: Change T by KeyNode
  // TODO: Segura Alex (typedef -> using)
    using value_type = typename Traits::T;
private:
    using Node = CBinaryTreeNode<Traits, Args...>;
public:
    value_type m_data;
    Node *  m_pParent = nullptr;
    Ref     m_ref;
    vector<Node *> m_pChild = {nullptr, nullptr};
public:
    // TODO: Fuentes Patrick (revisar que el Ref llegue bien)
    CBinaryTreeNode(Node *pParent, T data, Ref ref = nullptr, Node *p1 = nullptr) 
        : m_pParent(pParent), m_data(data)
    {   m_pChild[0] = p0;   m_pChild[1] = p1;   }
    
    // Modificacion, Constructor copia
    CBinaryTreeNode(const Node &other)
        : m_data(other.m_data), m_ref(other.m_ref), m_pParent(nullptr)
    {
        m_pChild[0] = other.m_pChild[0] ? new Node(*other.m_pChild[0]) : nullptr;
        m_pChild[1] = other.m_pChild[1] ? new Node(*other.m_pChild[1]) : nullptr;} // Fin de modificacion

// TODO: Keynode 
    value_type getData() { return m_data; }
    value_type& getDataRef() { return m_data; }
 
 // TODO: review if these functions must remain public/private
    void      setpChild(const Node *pChild, size_t pos)  {   m_pChild[pos] = pChild;  }
    Node    * getChild(size_t branch){ return m_pChild[branch];  }
    Node    *&getChildRef(size_t branch){ return m_pChild[branch];  }
    Node    * getParent() { return m_pParent;   }};

template <typename Container, typename... Args>
class binary_tree_iterator : public general_iterator<Container, binary_tree_iterator<Container, Args...>> {
    using Parent = general_iterator<Container, binary_tree_iterator<Container, Args...>>;
    using Node = typename Container::Node;
    using myself = binary_tree_iterator<Container, Args...>;
public:
    binary_tree_iterator(Container* pContainer = nullptr, Node* pNode = nullptr)
        : Parent(pContainer, pNode) {}
    myself& operator++() {
        if (this->m_pNode) {
            Node* node = this->m_pNode;
            if (node->getChild(1)) {
                node = node->getChild(1);
                while (node->getChild(0)) node = node->getChild(0);
            } else {
                Node* parent = node->getParent();
                while (parent && node == parent->getChild(1)) {
                    node = parent;
                    parent = parent->getParent();
                }
                node = parent;
            }
            this->m_pNode = node;
        }
        return *this;}};

// Modificacion, iterador hacia atrás
template <typename Container, typename... Args>
class binary_tree_reverse_iterator : public general_iterator<Container, binary_tree_reverse_iterator<Container, Args...>> {
    using Parent = general_iterator<Container, binary_tree_reverse_iterator<Container, Args...>>;
    using Node = typename Container::Node;
    using myself = binary_tree_reverse_iterator<Container, Args...>;
public:
    binary_tree_reverse_iterator(Container* pContainer = nullptr, Node* pNode = nullptr)
        : Parent(pContainer, pNode) {}
    myself& operator++() {
        if (this->m_pNode) {
            Node* node = this->m_pNode;
            if (node->getChild(0)) {
                node = node->getChild(0);
                while (node->getChild(1)) node = node->getChild(1);
            } else {
                Node* parent = node->getParent();
                while (parent && node == parent->getChild(0)) {
                    node = parent;
                    parent = parent->getParent();
                }
                node = parent;
            }
            this->m_pNode = node;
        }
        return *this;}}; // Fin de modificacion


template <typename _T, typename... Args>
struct BinaryTreeAscTraits{
    using T = _T;
    using Node = CBinaryTreeNode<BinaryTreeAscTraits<_T, Args...>, Args...>;
    using CompareFn = less<T>;};

template <typename _T, typename... Args>
struct BinaryTreeDescTraits{
    using T = _T;
    using Node = CBinaryTreeNode<BinaryTreeDescTraits<_T, Args...>, Args...>;
    using CompareFn = greater<T>;};

// Árbol binario
template <typename Traits, typename... Args>
class CBinaryTree{
public:
    using value_type = typename Traits::T;
    using Node = typename Traits::Node;
    using CompareFn = typename Traits::CompareFn;
    using myself = CBinaryTree<Traits, Args...>;
    using iterator = binary_tree_iterator<myself, Args...>;
    using reverse_iterator = binary_tree_reverse_iterator<myself, Args...>;w
protected:
    Node    *m_pRoot = nullptr;
    size_t   m_size  = 0;
    CompareFn Compfn;
    mutable std::mutex m_mutex; // mutex para concurrente
public:
    CBinaryTree() = default;
    CBinaryTree(CBinaryTree&& other) noexcept
        : m_pRoot(other.m_pRoot), m_size(other.m_size), Compfn(std::move(other.Compfn))
    {
        other.m_pRoot = nullptr;
        other.m_size = 0;}
    // Destructor
    virtual ~CBinaryTree() {
        clear(m_pRoot);
        m_pRoot = nullptr;
        m_size = 0;
    }
    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    // Inserción concurrente
    virtual void insert(value_type& elem, LinkedValueType value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        internal_insert(elem, value, nullptr, m_pRoot);}
protected:
    Node* CreateNode(Node* pParent, value_type &elem, Ref ref){ 
        return new Node(pParent, elem, ref); 
    }
    Node* internal_insert(value_type &elem, Ref ref, LinkedValueType value, Node* pParent, Node*& rpOrigin)
    {
        if (!rpOrigin) {   
            m_size = m_size + 1;
            return (rpOrigin = CreateNode(pParent, elem, ref));
        }
        size_t branch = Compfn(elem, rpOrigin->getDataRef());
        return internal_insert(elem, ref, value, rpOrigin, rpOrigin->getChildRef(branch));
    }
    Node* copyTree(Node* pParent, Node* pNode) {
        if (!pNode) return nullptr;
        Node* newNode = new Node(pParent, pNode->getDataRef(), pNode->m_ref);
        newNode->m_pChild[0] = copyTree(newNode, pNode->getChild(0));
        newNode->m_pChild[1] = copyTree(newNode, pNode->getChild(1));
        return newNode;}

    template<typename Func, typename... FArgs>
    void inorder_variadic(Node* pNode, Func fn, FArgs&&... args) {
        if (pNode) {
            inorder_variadic(pNode->getChild(0), fn, std::forward<FArgs>(args)...);
            fn(pNode->getDataRef(), std::forward<FArgs>(args)...);
            inorder_variadic(pNode->getChild(1), fn, std::forward<FArgs>(args)...);}}

    template<typename Func, typename... FArgs>
    void preorder_variadic(Node* pNode, Func fn, FArgs&&... args) {
        if (pNode) {
            fn(pNode->getDataRef(), std::forward<FArgs>(args)...);
            preorder_variadic(pNode->getChild(0), fn, std::forward<FArgs>(args)...);
            preorder_variadic(pNode->getChild(1), fn, std::forward<FArgs>(args)...);}}

    template<typename Func, typename... FArgs>
    void postorder_variadic(Node* pNode, Func fn, FArgs&&... args) {
        if (pNode) {
            postorder_variadic(pNode->getChild(0), fn, std::forward<FArgs>(args)...);
            postorder_variadic(pNode->getChild(1), fn, std::forward<FArgs>(args)...);
            fn(pNode->getDataRef(), std::forward<FArgs>(args)...);}}

public:
    template<typename Func, typename... FArgs>
    void inorder(Func fn, FArgs&&... args) { inorder_variadic(m_pRoot, fn, std::forward<FArgs>(args)...); }
    template<typename Func, typename... FArgs>
    void preorder(Func fn, FArgs&&... args) { preorder_variadic(m_pRoot, fn, std::forward<FArgs>(args)...); }
    template<typename Func, typename... FArgs>
    void postorder(Func fn, FArgs&&... args) { postorder_variadic(m_pRoot, fn, std::forward<FArgs>(args)...); }};

template <typename Traits, typename... Args>
ostream& operator<<(ostream& os, CBinaryTree<Traits, Args...>& obj){
    os << "CBinaryTree with " << obj.size() << " elements.\n";
    obj.inorder_variadic(obj.m_pRoot, [](auto& val){ os << val << " "; });
    return os;}

template <typename Traits, typename... Args>
istream& operator>>(istream& is, CBinaryTree<Traits, Args...>& obj){
    // Ejemplo: leer secuencia de valores
    return is;}

#endif // __BINARY_TREE_H__
