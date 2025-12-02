#ifndef __AVL_H__
#define __AVL_H__

#include <mutex>
#include <iostream>
#include "binarytree.h"

template <typename Traits>
class CAVLNode : public CBinaryTreeNode<typename Traits>{
public:
    using value_type = typename Traits::T;
    using Node       = CBinaryTree::Node;
protected:
    int     m_balanceFactor = 0; // Balance factor for AVL tree
public:
    int getBalance() const { return m_balanceFactor; } // modification
    void setBalance(int bf) { m_balanceFactor = bf; } // modification
    Node* getLeft()  { return this->m_pChild[0]; }
    Node* getRight() { return this->m_pChild[1]; }
    void setLeft(Node* n)  { this->m_pChild[0] = n; }
    void setRight(Node* n) { this->m_pChild[1] = n; }
};};

template <typename _T>
struct AVLAscTraits{
    using value_type = _T;
    using Node       = CAVLNode<_T>;
    using CompareFn  = std::less<_T>;
};

template <typename _T>
struct AVLDescTraits{
    using value_type = _T;
    using Node       = CAVLNode<_T>;
    using CompareFn  = std::greater<_T>;
};

template <typename Traits>
class CAVLTree : public CBinaryTree<Traits> {
public:
    using Base       = CBinaryTree<Traits>;
    using Node       = typename Traits::Node;
    using value_type = typename Traits::value_type;  
    using CompareFn  = typename Traits::CompareFn;
    using Container  = CAVLTree<Traits>;
    using iterator   = binary_tree_iterator<Container>;

protected:
    // Additional members for AVL tree balancing can be added here
    // TODO: modificar la insercion para que mantenga
    //       el balance del arbo y realice las 
    //       rotaciones necesarias
    
    Node* rotateLeft(Node* y) {
        Node* x  = y->getRight();
        Node* T2 = x->getLeft();
        x->setLeft(y);
        y->setRight(T2);
        y->setBalance(getHeight(y->getRight()) - getHeight(y->getLeft()));
        x->setBalance(getHeight(x->getRight()) - getHeight(x->getLeft()));
        return x;
    }

    Node* rotateRight(Node* x) {
        Node* y  = x->getLeft();
        Node* T2 = y->getRight();
        y->setRight(x);
        x->setLeft(T2);
        x->setBalance(getHeight(x->getRight()) - getHeight(x->getLeft()));
        y->setBalance(getHeight(y->getRight()) - getHeight(y->getLeft()));
        return y;
    }

    int getHeight(Node* n) {
        if (!n) return 0;
        int lh = getHeight(n->getLeft());
        int rh = getHeight(n->getRight());
        return 1 + std::max(lh, rh);
    }

    int getBalance(Node* n) {
        if (!n) return 0;
        return getHeight(n->getRight()) - getHeight(n->getLeft());
    }
    
    Node *internal_insert(value_type &elem, Ref ref,
                          Node* pParent, Node*& rpOrigin) override
    {
        // TODO 1. insertar
        if (!rpOrigin) {
            Node* newNode = new Node(pParent, elem, ref);
            m_size = m_size + 1;
            return (rpOrigin = newNode);
        }

        CompareFn cmp;
        size_t branch = cmp(elem, rpOrigin->getDataRef()) ? 0 : 1;
        Node* child = internal_insert(elem, ref, rpOrigin, rpOrigin->m_pChild[branch]);
        rpOrigin->m_pChild[branch] = child;

        // TODO 2. verificar balance
        int bf = getBalance(rpOrigin);
        rpOrigin->setBalance(bf);

        // TODO 3. realizar rotaciones si es necesario
        if (bf > 1) { // derecha
            if (cmp(elem, rpOrigin->getRight()->getDataRef()))
                rpOrigin->setRight(rotateRight(rpOrigin->getRight()));
            return rotateLeft(rpOrigin);
        } else if (bf < -1) { // izquierda
            if (!cmp(elem, rpOrigin->getLeft()->getDataRef()))
                rpOrigin->setLeft(rotateLeft(rpOrigin->getLeft()));
            return rotateRight(rpOrigin);
        }
        return rpOrigin;
    }
    
public:
    CAVLTree() : Base() {}

    // write
    void write(std::ostream& os) {
        this->inorder([&os](auto& val){ os << val << " "; });
        os << "\n";
    }

    // read
    void read(std::istream& is) {
        value_type val;
        while (is >> val) {
            this->insert(val, nullptr);
        }
    }
};

#endif // __AVL_H__
