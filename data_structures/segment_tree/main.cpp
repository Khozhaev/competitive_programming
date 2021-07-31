#include <iostream>
#include <vector>

template<typename TNode, bool NeedGroupUpdate = false>
class TSegmentTree {
public:
    using TVal = typename TNode::TValueType;

    TSegmentTree(size_t n) {
        Size = n;
        Tree.resize(Size << 2u);
    }
    inline void Build(const std::vector<TVal>& a) {
        if constexpr (NeedGroupUpdate) {
            RecursiveBuildImpl(1, 0, Size - 1, a);
        } else {
            IterativeBuildImpl(a);
        }
    }

    // Если не нужна TNode или TNode::operator+ реализует тяжелую логику, необходимую только для построения,
    // следует в GetImpl возвращать не TNode, и не использовать operator+ для объединения нижних уровней
    inline TNode Get(size_t l, size_t r) {
        if constexpr (NeedGroupUpdate) {
            RecursiveGetImpl(1, 0, Size - 1, l, r);
        } else {
            IterativeGetImpl(l, r + 1);
        }
    }
    inline void Update(size_t pos, TVal val) {
        if constexpr (NeedGroupUpdate) {
            RecursiveUpdateImpl(1, 0, Size - 1, pos, std::move(val));
        } else {
            IterativeUpdateImpl(pos, std::move(val));
        }
    }

    template<typename T>
    inline void GroupUpdate(size_t l, size_t r, T val) {
        static_assert(NeedGroupUpdate, "Can't use Group Operation with NeedGroupUpdate==false");
        GroupUpdateImpl(1, 0, Size - 1, l, r, val);
    }
private:
    inline void Push(size_t v) {
        if constexpr (NeedGroupUpdate) {
            // for group set
            // if (!Tree[v].HasPromise) return;
            SetPromise(v << 1u, Tree[v].Promise);
            SetPromise((v<<1u) | 1u, Tree[v].Promise);
            Tree[v].Promise = {};
        }
    }
    template<typename TProm>
    inline void SetPromise(size_t v, TProm prom) {
        if constexpr (NeedGroupUpdate) {
            if (v < Tree.size()) {
                Tree[v].AddPromise(prom);
            }
        }
    }
    inline void IterativeBuildImpl(const std::vector<TVal>& a) {
        for (size_t i = 0; i < Size; ++i) {
            Tree[Size + i] = TNode(a[i]);
        }
        for (size_t i = Size - 1; i > 0; --i) {
            Tree[i] = Tree[i << 1u] + Tree[(i << 1u) | 1u];
        }
    }

    //[l, r)
    inline TNode IterativeGetImpl(size_t l, size_t r) {
        TNode res = {};
        for (l += Size, r += Size; l < r; l >>= 1u, r >>= 1u) {
            if (l&1u) res = res + Tree[l++];
            if (r&1u) res = res + Tree[--r];
        }
        return res;
    }
    inline void IterativeUpdateImpl(size_t pos, TVal val) {
        for (Tree[pos += Size] = Tree[val]; pos > 1u; pos >>= 1u) {
            Tree[pos>>1u] = Tree[pos] + Tree[pos^1u];
        }
    }
    inline void RecursiveBuildImpl(size_t v, size_t tl, size_t tr, const std::vector<TVal>& a) {
        if (tl == tr) {
            Tree[v] = TNode(a[tl]);
            return;
        }
        Push(v);
        size_t tm = (tl + tr) >> 1u;
        RecursiveBuildImpl((v << 1u), tl, tm, a);
        RecursiveBuildImpl((v << 1u) | 1u, tm + 1, tr, a);
        Tree[v] = Tree[v << 1u] + Tree[(v << 1u) | 1u];
    }
    inline TNode RecursiveGetImpl(size_t v, size_t tl, size_t tr, size_t l, size_t r) {
        if (tl > r || tr < l) {
            return {};
        }
        Push(v);
        if (tl >= l && tr <= r) {
            return Tree[v];
        }
        size_t tm = (tl + tr) >> 1u;
        return RecursiveGetImpl(v << 1u, tl, tm, l, r)
                + RecursiveGetImpl((v << 1u) | 1u, tm + 1, tr, l, r);
    }

    inline void RecursiveUpdateImpl(size_t v, size_t tl, size_t tr, size_t pos, TVal val) {
        Push(v);
        if (tl == tr) {
            Tree[v] = TNode(val);
            return;
        }
        size_t tm = (tl + tr) >> 1u;
        if (tm >= pos) {
            RecursiveUpdateImpl(v << 1u, tl, tm, pos, std::move(val));
        } else {
            RecursiveUpdateImpl((v << 1u) | 1u, tm + 1, tr, pos, std::move(val));
        }
        Tree[v] = Tree[v << 1u] + Tree[(v << 1u) | 1u];
    }

    template<typename T>
    inline void GroupUpdateImpl(size_t v, size_t tl, size_t tr, size_t l, size_t r, T val) {
        if (tl > r || tr < l) {
            return;
        }
        Push(v);
        if (tl >= l && tr <= r) {
            SetPromise(v, val);
            return;
        }
        size_t tm = (tl + tr) >> 1u;
        GroupUpdateImpl(v << 1u, tl, tm, l, r, val);
        GroupUpdateImpl((v << 1u) | 1u, tm + 1, tr, l, r, val);
        Tree[v] = Tree[v << 1u] + Tree[(v << 1u) | 1u];
    }

private:
    std::vector<TNode> Tree;
    size_t Size;
};

template<typename T>
struct NodeSum {
    using TValueType = T;
    NodeSum() = default;
    NodeSum(T val)
        : Result(std::move(val))
    {}
    inline NodeSum operator+(const NodeSum<T>& other) const {
        return NodeSum(Result + other.Result);
    }
    T Result = T();
};

template<typename T>
struct NodeMin {
    using TValueType = T;
    NodeMin() = default;
    NodeMin(T val)
        : Result(std::move(val))
    {}
    inline NodeMin operator+(const NodeMin<T>& other) const {
        return NodeMin(std::min(Result, other.Result));
    }
    T Result = 1e9 + 7;
};

template<typename T>
struct NodeMax {
    using TValueType = T;
    NodeMax() = default;
    NodeMax(T val)
        : Result(std::move(val))
    {}
    inline NodeMax operator+(const NodeMax<T>& other) const {
        return NodeMax(std::max(Result, other.Result));
    }
    T Result = -(1e9 + 7);
};

template<typename T>
struct NodeGcd {
    using TValueType = T;
    NodeGcd() = default;
    NodeGcd(T val)
        : Result(std::move(val))
    {}
    inline NodeGcd operator+(const NodeGcd<T>& other) const {
        return NodeGcd(gcd(Result, other.Result));
    }
    T Result = T();
};

template <typename T>
struct NodeSumWithGroupAdd {
    using TValueType = T;
    NodeSumWithGroupAdd() = default;
    NodeSumWithGroupAdd(T val)
        : Result(std::move(val))
    {}
    NodeSumWithGroupAdd(T val, size_t sz)
        : Result(std::move(val))
        , Sz(sz)
    {}
    inline NodeSumWithGroupAdd operator+(const NodeSumWithGroupAdd<T>& other) const {
        return NodeSumWithGroupAdd(Result + other.Result, Sz + other.Sz);
    }
    void AddPromise(T prom) {
        Promise += prom;
        Result += Sz * prom;
    }

    T Result = T();
    size_t Sz = 1;
    T Promise = T();
};


