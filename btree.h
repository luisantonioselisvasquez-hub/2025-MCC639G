#ifndef __BTREE_H__
#define __BTREE_H__

#include <iostream>
#include <mutex>
#include <functional>
#include <stack>
#include <vector>
#include <utility>
#include <fstream>
#include <algorithm>
#include <memory>
#include "btreepage.h"
#define DEFAULT_BTREE_ORDER 3

const size_t MaxHeight = 5; //??

template <typename _keyType, typename _ObjIDType>
struct BTreeTrait
{
       using keyType = _keyType;
       using ObjIDType = _ObjIDType;
};

template <typename Trait>
class BTree // this is the full version of the BTree
{
       typedef typename Trait::keyType    keyType;
       typedef typename Trait::ObjIDType    ObjIDType;
       typedef CBTreePage <Trait> BTNode;// useful shorthand

public:
    using ObjectInfo = typename BTNode::ObjectInfo;

protected:
    using lpfnForEach2   = typename BTNode::lpfnForEach2;
    using lpfnForEach3   = typename BTNode::lpfnForEach3;
    using lpfnFirstThat2 = typename BTNode::lpfnFirstThat2;
    using lpfnFirstThat3 = typename BTNode::lpfnFirstThat3;

public:
       BTree(size_t order = DEFAULT_BTREE_ORDER, bool unique = true)
              : m_Order(order),
                m_Root(2 * order  + 1, unique),
                m_Unique(unique),
                m_NumKeys(0)
       {
              m_Root.SetMaxKeysForChilds(order);
              m_Height = 1;
       }
       // Move constructor
       BTree(BTree&& other) noexcept
       {
		   std::scoped_lock lock(m_Mutex, other.m_Mutex);
           m_Root = std::move(other.m_Root);
           m_Order = other.m_Order;
           m_Height = other.m_Height;
           m_NumKeys = other.m_NumKeys;
           m_Unique = other.m_Unique;
           other.m_Order = 0;
           other.m_Height = 0;
           other.m_NumKeys = 0;
           other.m_Unique = true;
       }
       BTree& operator=(BTree&& other) noexcept
       {
           if (this == &other) return *this;
           std::scoped_lock lock(m_Mutex, other.m_Mutex);
           m_Root = std::move(other.m_Root);
           m_Order = other.m_Order;
           m_Height = other.m_Height;
           m_NumKeys = other.m_NumKeys;
           m_Unique = other.m_Unique;
           other.m_Order = 0;
           other.m_Height = 0;
           other.m_NumKeys = 0;
           other.m_Unique = true;
           return *this;
       }
       
       ~BTree() {}
       bool            Insert (const keyType key, const long ObjID);
       bool            Remove (const keyType key, const long ObjID);
       ObjIDType       Search (const keyType key)
       {      ObjIDType ObjID = -1;
              m_Root.Search(key, ObjID);
              return ObjID;
       }
       size_t            size()  { std::lock_guard<std::mutex> lk(m_Mutex); return m_NumKeys; }
       size_t            height() { std::lock_guard<std::mutex> lk(m_Mutex); return m_Height; }
       size_t            GetOrder() { std::lock_guard<std::mutex> lk(m_Mutex); return m_Order; }

       void            Print (ostream &os)
       {               std::lock_guard<std::mutex> lk(m_Mutex); m_Root.Print(os); }
       void            ForEach( lpfnForEach2 lpfn, void *pExtra1 )
       {               std::lock_guard<std::mutex> lk(m_Mutex); m_Root.ForEach(lpfn, 0, pExtra1);}
       void            ForEach( lpfnForEach3 lpfn, void *pExtra1, void *pExtra2)
       {               std::lock_guard<std::mutex> lk(m_Mutex); m_Root.ForEach(lpfn, 0, pExtra1, pExtra2);}
       ObjectInfo*     FirstThat( lpfnFirstThat2 lpfn, void *pExtra1 )
       {               std::lock_guard<std::mutex> lk(m_Mutex); return m_Root.FirstThat(lpfn, 0, pExtra1);}
       ObjectInfo*     FirstThat( lpfnFirstThat3 lpfn, void *pExtra1, void *pExtra2)
       {               std::lock_guard<std::mutex> lk(m_Mutex); return m_Root.FirstThat(lpfn, 0, pExtra1, pExtra2);}
       //typedef               ObjectInfo iterator;

       // foreach template
       template <typename Fn, typename... Args>
       void foreach(Fn&& fn, Args&&... args)
       {
           std::lock_guard<std::mutex> lk(m_Mutex);
           using callback_t = std::function<void(ObjectInfo&, Args...)>;
           callback_t callback = std::forward<Fn>(fn);
           m_Root.ForEach(
               [&](ObjectInfo& oi, size_t) {
            		callback(static_cast<const ObjectInfo&>(oi), std::forward<Args>(args)...);}, 0, std::forward<Args>(args)...
           );
       }
		
		// firstThat template
       template <typename Pred, typename... Args>
       ObjectInfo* firstThat(Pred&& pred, Args&&... args)
       {
           std::lock_guard<std::mutex> lk(m_Mutex);
           using callback_t = std::function<bool(ObjectInfo&, Args...)>;
           callback_t callback = std::forward<Pred>(pred);
           return m_Root.FirstThat(
               [&](ObjectInfo& oi, size_t /*level*/) -> ObjectInfo* {
            return callback(static_cast<const ObjectInfo&>(oi), std::forward<Args>(args)...) ? &oi : nullptr;}, 0, std::forward<Args>(args)...
           );
       }

       // Iterator
       class iterator {
       public:
           using value_type = ObjectInfo;
           using reference = const ObjectInfo&;
           using pointer = const ObjectInfo*;
           iterator() : idx(0) {}
           iterator(std::vector<ObjectInfo>&& items_, size_t start = 0) : items(std::move(items_)), idx(start) {}
           reference operator*() const { return items[idx]; }
           pointer operator->() const { return &items[idx]; }
           iterator& operator++() { ++idx; return *this; }
           iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
           bool operator==(const iterator& o) const { return idx == o.idx && items.data() == o.items.data(); }
           bool operator!=(const iterator& o) const { return !(*this == o); }
       private:
           std::vector<ObjectInfo> items;
           size_t idx = 0;
           friend class BTree;
       };

		// Backward iterator
       class reverse_iterator {
       public:
           using value_type = ObjectInfo;
           using reference = const ObjectInfo&;
           using pointer = const ObjectInfo*;
           reverse_iterator() : idx(0) {}
           reverse_iterator(std::vector<ObjectInfo>&& items_, size_t start = 0) : items(std::move(items_)), idx(start) {}
           reference operator*() const { return items[idx]; }
           pointer operator->() const { return &items[idx]; }
           reverse_iterator& operator++() { ++idx; return *this; }
           reverse_iterator operator++(int) { reverse_iterator tmp = *this; ++(*this); return tmp; }
           bool operator==(const reverse_iterator& o) const { return idx == o.idx && items.data() == o.items.data(); }
           bool operator!=(const reverse_iterator& o) const { return !(*this == o); }

       private:
           std::vector<ObjectInfo> items;
           size_t idx = 0;
           friend class BTree;
       };

       iterator begin() const {
           std::vector<ObjectInfo> s;
           m_Root->ForEach([&](const ObjectInfo& oi, size_t){ s.push_back(oi); }, 0);
           return iterator(std::move(s), 0);
       }
       iterator end() const {
           std::vector<ObjectInfo> s;
           m_Root->ForEach([&](const ObjectInfo& oi, size_t){ s.push_back(oi); }, 0);
           return iterator(std::move(s), s.size());
       }

       reverse_iterator rbegin() const {
           std::vector<ObjectInfo> s;
           m_Root->ForEachReverse([&](const ObjectInfo& oi, size_t){ s.push_back(oi); }, 0);;
           return reverse_iterator(std::move(s), 0);
       }
       reverse_iterator rend() const {
           std::vector<ObjectInfo> s;
           m_Root->ForEachReverse([&](const ObjectInfo& oi, size_t){ s.push_back(oi); }, 0);
           return reverse_iterator(std::move(s), s.size());
       }

       // Read/Write
       void write(std::ostream& os) const {
           std::lock_guard<std::mutex> lk(m_Mutex);
           auto writer = [&](const ObjectInfo& oi){ os << oi.key << " " << oi.ObjID << "\n"; };
           const_cast<BTree*>(this)->foreach(writer);
       }

       void read(std::istream& is) {
           std::lock_guard<std::mutex> lk(m_Mutex);
           keyType k;
           ObjIDType id;
           while (is >> k >> id) {
               m_Root.Insert(k, static_cast<long>(id));
               ++m_NumKeys;
           }
       }

       friend std::ostream& operator<<(std::ostream& os, const BTree& bt) {
           bt.write(os);
           return os;
       }

       friend std::istream& operator>>(std::istream& is, BTree& bt) {
           bt.read(is);
           return is;
       }

protected:
       BTNode          m_Root;
       size_t          m_Height;  // height of tree
       size_t          m_Order;   // order of tree
       size_t          m_NumKeys; // number of keys
       bool            m_Unique;  // Accept the elements only once ?
private:
       mutable std::mutex m_Mutex;
};     

// TODO change ObjID by LinkedValueType value
template <typename Trait>
bool BTree<Trait>::Insert(const keyType key, const long ObjID){
       bt_ErrorCode error = m_Root.Insert(key, ObjID);
       if( error == bt_duplicate )
               return false;
       m_NumKeys++;
       if( error == bt_overflow )
       {
               m_Root.SplitRoot();
               m_Height++;
       }
       return true;
}

template <typename Trait>
bool BTree<Trait>::Remove (const keyType key, const long ObjID)
{
       bt_ErrorCode error = m_Root.Remove(key, ObjID);
       if( error == bt_duplicate || error == bt_nofound )
               return false;
       m_NumKeys--;

       if( error == bt_rootmerged )
               m_Height--;
       return true;
}

#endif
