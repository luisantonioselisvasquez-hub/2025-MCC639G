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
       //typedef ObjectInfo iterator;
       // TODO replace thius functions by foreach
       typedef typename BTNode::lpfnForEach2    lpfnForEach2;
       typedef typename BTNode::lpfnForEach3    lpfnForEach3;
       typedef typename BTNode::lpfnFirstThat2  lpfnFirstThat2;
       typedef typename BTNode::lpfnFirstThat3  lpfnFirstThat3;
       typedef typename BTNode::ObjectInfo      ObjectInfo;

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
		   std::lock_guard<std::mutex> lk(other.m_Mutex);
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
           std::lock_guard<std::mutex> lk1(m_Mutex);
           std::lock_guard<std::mutex> lk2(other.m_Mutex);
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
       //int           Open (char * name, int mode);
       //int           Create (char * name, int mode);
       //int           Close ();
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

       // ========== ADDED: generalized foreach (templated) ==========
       // Uses CBTreePage's existing lpfnForEach2/3 signatures via adapters.
       // Example usage:
       //    tree.foreach([](const auto &oi){ std::cout << oi.key << "\n"; });
       template<typename Fn>
       void foreach(Fn&& fn)
       {
           struct Wrapper { typename std::decay<Fn>::type f; Wrapper(Fn&& g): f(std::forward<Fn>(g)) {} };
           Wrapper* p = new Wrapper(std::forward<Fn>(fn));
           // adapter to lpfnForEach2: void(ObjectInfo&, size_t, void*)
           auto adapter = +[](ObjectInfo& oi, size_t /*level*/, void* extra){
               Wrapper* w = static_cast<Wrapper*>(extra);
               w->f(oi);
           };
           {
               std::lock_guard<std::mutex> lk(m_Mutex);
               m_Root.ForEach(reinterpret_cast<lpfnForEach2>(adapter), 0, static_cast<void*>(p));
           }
           delete p;
       }

       // overload with extra parameter passed to functor (2-arg form)
       template<typename Fn, typename E1>
       void foreach(Fn&& fn, E1* extra1)
       {
           using Pair = std::pair<typename std::decay<Fn>::type, E1*>;
           Pair* p = new Pair(std::forward<Fn>(fn), extra1);
           auto adapter3 = +[](ObjectInfo& oi, size_t /*level*/, void* e1, void* extra){
               Pair* wp = static_cast<Pair*>(extra);
               wp->first(oi, wp->second);
           };
           {
               std::lock_guard<std::mutex> lk(m_Mutex);
               m_Root.ForEach(reinterpret_cast<lpfnForEach3>(adapter3), 0, nullptr, static_cast<void*>(p));
           }
           delete p;
       }

       // ========== ADDED: generalized firstThat (templated) ==========
       // Returns pointer into the page (same semantics as existing FirstThat)
       template<typename Pred>
       ObjectInfo* firstThat(Pred&& pred)
       {
           struct PWrapper { typename std::decay<Pred>::type p; PWrapper(Pred&& pr): p(std::forward<Pred>(pr)) {} };
           PWrapper* pw = new PWrapper(std::forward<Pred>(pred));
           auto adapter = +[](ObjectInfo& oi, size_t /*level*/, void* extra)->ObjectInfo* {
               PWrapper* w = static_cast<PWrapper*>(extra);
               return w->p(oi) ? &oi : nullptr;
           };
           ObjectInfo* res = nullptr;
           {
               std::lock_guard<std::mutex> lk(m_Mutex);
               res = m_Root.FirstThat(reinterpret_cast<lpfnFirstThat2>(adapter), 0, static_cast<void*>(pw));
           }
           delete pw;
           return res;
       }

       template<typename Pred, typename E1>
       ObjectInfo* firstThat(Pred&& pred, E1* extra1)
       {
           using Pair = std::pair<typename std::decay<Pred>::type, E1*>;
           Pair* p = new Pair(std::forward<Pred>(pred), extra1);
           auto adapter3 = +[](ObjectInfo& oi, size_t /*level*/, void* e1, void* extra)->ObjectInfo* {
               Pair* wp = static_cast<Pair*>(extra);
               return wp->first(oi, wp->second) ? &oi : nullptr;
           };
           ObjectInfo* res = nullptr;
           {
               std::lock_guard<std::mutex> lk(m_Mutex);
               res = m_Root.FirstThat(reinterpret_cast<lpfnFirstThat3>(adapter3), 0, nullptr, static_cast<void*>(p));
           }
           delete p;
           return res;
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
           std::vector<ObjectInfo> snapshot;
           foreach([&](const ObjectInfo& oi){ snapshot.push_back(oi); });
           return iterator(std::move(snapshot), 0);
       }
       iterator end() const {
           std::vector<ObjectInfo> snapshot;
           foreach([&](const ObjectInfo& oi){ snapshot.push_back(oi); });
           return iterator(std::move(snapshot), snapshot.size());
       }

       reverse_iterator rbegin() const {
           std::vector<ObjectInfo> snapshot;
           foreach([&](const ObjectInfo& oi){ snapshot.push_back(oi); });
           std::reverse(snapshot.begin(), snapshot.end());
           return reverse_iterator(std::move(snapshot), 0);
       }
       reverse_iterator rend() const {
           std::vector<ObjectInfo> snapshot;
           foreach([&](const ObjectInfo& oi){ snapshot.push_back(oi); });
           std::reverse(snapshot.begin(), snapshot.end());
           return reverse_iterator(std::move(snapshot), snapshot.size());
       }

       // Read/Write
       void write(std::ostream& os) const {
           std::lock_guard<std::mutex> lk(m_Mutex);
           auto writer = [&](const ObjectInfo& oi){ os << oi.key << " " << oi.ObjID << "\n"; };
           auto w = writer;
           const_cast<BTree*>(this)->foreach(w);
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
