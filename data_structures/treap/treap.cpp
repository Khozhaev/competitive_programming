#include <iostream>
#include <random>


std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<uint64_t> dist(0ull, 1'000'000'000'000ull);
inline uint64_t Random() {
    return dist(mt);
}
template<typename TValue, bool PersistentMode = false>
struct TNode {
    uint64_t Prior;
    TNode* Left;
    TNode* Right;
    ssize_t Size;
    TValue Value;

    TNode()
        : Prior(Random())
        , Left(nullptr)
        , Right(nullptr)
        , Size(1)
        , Value()
    {}

    TNode(TValue value)
        : Prior(Random())
        , Left(nullptr)
        , Right(nullptr)
        , Size(1)
        , Value(std::move(value))
    {}

    static inline ssize_t GetSize(TNode* root) {
        if (!root) {
            return 0;
        }
        return root->Size;
    }

    static inline TValue GetValue(TNode* root) {
        if (!root) {
            return TValue();
        }
        return root->Value;
    }

    static inline TNode* Copy(TNode* root) {
        if (!root) {
            return nullptr;
        }
        return TNode(*root);
    }


    static inline void Update(TNode* root) {
        if (!root) {
            return;
        }
        root->Size = 1 + GetSize(root->Left) + GetSize(root->Right);
        root->Value.Update(GetValue(root->Left), GetValue(root->Right));
    }

    static inline TNode* Merge(TNode* rootLeft, TNode* rootRight) {
        if (!rootLeft) {
            return rootRight;
        }
        if (!rootRight) {
            return rootLeft;
        }
        if (rootLeft->Prior < rootRight->Prior) {
            if constexpr (PersistentMode) {
                rootLeft = Copy(rootLeft);
            }
            rootLeft->Right = Merge(rootLeft->Right, rootRight);
            Update(rootLeft);
            return rootLeft;
        } else {
            if constexpr (PersistentMode) {
                rootRight = Copy(rootRight);
            }
            rootRight->Left = Merge(rootLeft, rootRight->Left);
            Update(rootRight);
            return rootRight;
        }
    }

    static inline std::pair<TNode*, TNode*> SplitSz(TNode* root, ssize_t sz) {
        if (!root) {
            return {nullptr, nullptr};
        }
        if constexpr (PersistentMode) {
            root = Copy(root);
        }
        ssize_t lsz = GetSize(root->Left);
        if (lsz >= sz) {
            auto p = SplitSz(root->Left, sz);
            root->Left = p.second;
            Update(root);
            return {p.first, root};
        } else {
            auto p = SplitSz(root->Right, sz - lsz - 1);
            root->Right = p.first;
            Update(root);
            return {root, p.second};
        }
    }

    static inline std::pair<TNode*, TNode*> SplitVal(TNode* root, const TValue& value) {
        if (!root) {
            return {nullptr, nullptr};
        }
        if constexpr (PersistentMode) {
            root = Copy(root);
        }
        if (root->Value < value) {
            auto p = SplitVal(root->Right, value);
            root->right = p.first;
            Update(root);
            return {root, p.second};
        } else {
            auto p = SplitVal(root->Left, value);
            root->Left = p.second;
            Update(root);
            return {p.first, root};
        }
    }

    static inline TNode* InsertPos(TNode* root, TNode* v, ssize_t pos) {
        if (!root) {
            return v;
        }
        auto p = SplitSz(root, pos);
        root = Merge(Merge(p.first, v), p.second);
        return root;
    }

    static inline TNode* InsertVal(TNode* root, TNode* v) {
        if (!root) {
            return v;
        }
        auto p = SplitVal(root, v->Value);
        root = Merge(Merge(p.first, v), p.second);
        return root;
    }

    static inline TNode* ErasePos(TNode* root, ssize_t pos) {
        if (!root) {
            return nullptr;
        }
        ssize_t lsz = GetSize(root->Left);
        if (lsz == pos) {
            return Merge(root->Left, root->Right);
        }
        if constexpr (PersistentMode) {
            root = Copy(root);
        }
        if (lsz > pos) {
            root->Left = ErasePos(root->Left, pos);
            Update(root);
            return root;
        } else {
            root->Right = ErasePos(root->Right, pos - 1 - lsz);
            Update(root);
            return root;
        }
    }
    static inline TNode* EraseVal(TNode* root, const TValue& value) {
        if (!root) {
            return nullptr;
        }
        if (root->Value == value) { // need copy root?
            return Merge(root->Left, root->Right);
        }
        if constexpr (PersistentMode) {
            root = Copy(root);
        }
        if (root->Value > value) {
            root->Left = EraseVal(root->Left, value);
            Update(root);
            return root;
        } else {
            root->Right = EraseVal(root->Right, value);
            Update(root);
            return root;
        }
    }


    /* Применяет операцию на отрезке [l, r], нумерация с нуля.
     * Может работать медленно из-за прослойки вызова функции,
     * если нужно ускорение - заменить вызов функтора на свой код. */
    template<typename TFunctor>
    static inline TNode* DoSegment(TNode* root, ssize_t l, ssize_t r, TFunctor& functor) {
        if constexpr (PersistentMode) {
            root = Copy(root);
        }
        auto p1 = SplitSz(root,r + 1);
        auto p2 = SplitSz(p1.first, l);
        functor(p2.second);
        root = Merge(Merge(p2.first, p2.second), p1.second);
        return root;
    }
};

// todo: fast build, fast insert
template<typename TValue>
class TTreap {
public:
    using Node = TNode<TValue, false>;

    inline void InsertPos(TValue val, ssize_t pos) {
        root = Node::InsertPos(root, new Node(std::move(val)), pos);
    }
    inline void InsertVal(TValue val) {
        root = Node::InsertVal(root, new Node(std::move(val)));
    }
    inline void DeletePos(ssize_t pos) {
        root = Node::ErasePos(root, pos);
    }
    inline void DeleteVal(const TValue& val) {
        root = Node::EraseVal(root, val);
    }

    /* Применяет операцию на отрезке [l, r], нумерация с нуля.
     * Может работать медленно из-за прослойки вызова функции,
     * если нужно ускорение - заменить вызов функтора на свой код. */
    template<typename TFunctor>
    inline auto DoSegment(ssize_t l, ssize_t r, TFunctor& functor) {
        auto p1 = Node::SplitSz(root, r + 1);
        auto p2 = Node::SplitSz(p1.first, l);
        auto result = functor(p2.second);
        root = Merge(Merge(p2.first, p2.second), p1.second);
        return result;
    }

    // Don't use this
    inline Node* Get() {
        return root;
    }
private:
    Node* root = nullptr;
};

template<typename TValue>
class TPersistentTreap {
public:
    using Node = TNode<TValue, true>;
    using Treap = TPersistentTreap<TValue>;

    TPersistentTreap() = default;
    TPersistentTreap(Node* root_)
        : root(root_)
    {}

    inline Treap InsertPos(TValue val, ssize_t pos) {
        return Treap(Node::InsertPos(root, new Node(std::move(val)), pos));
    }
    inline Treap InsertVal(TValue val) {
        return Treap(Node::InsertVal(root, new Node(std::move(val))));
    }
    inline Treap DeletePos(ssize_t pos) {
        return Treap(Node::ErasePos(root, pos));
    }
    inline Treap DeleteVal(const TValue& val) {
        return Treap(Node::EraseVal(root, val));
    }

    /* Применяет операцию на отрезке [l, r], нумерация с нуля.
     * Может работать медленно из-за прослойки вызова функции,
     * если нужно ускорение - заменить вызов функтора на свой код. */
    template<typename TFunctor>
    inline Treap DoSegment(ssize_t l, ssize_t r, TFunctor& functor) {
        auto p1 = Node::SplitSz(root, r + 1);
        auto p2 = Node::SplitSz(p1.first, l);
        functor(p2.second);
        return Treap(Merge(Merge(p2.first, p2.second), p1.second));
    }

    // Don't use this
    inline Node* Get() {
        return root;
    }
private:
    Node* root = nullptr;
};
// for example
template<typename T = int>
struct SumTreap {
    T key = 0;
    T sum = 0;
    SumTreap() = default;
    SumTreap(T val)
        : key(val)
        , sum(key)
    {}

    inline bool operator<(const SumTreap<T>& other) const {
        return key < other.key;
    }
    inline bool operator==(const SumTreap<T>& other) const {
        return key == other.key;
    }
    void Update(const SumTreap<T>& left, const SumTreap& right) {
        sum = left.sum + right.sum + key;
    }
};


int main() {
    using Node = TNode<SumTreap<int>>;
    Node* root = nullptr;
    Node* v = new Node(3);
    root = Node::InsertPos(root, v, 0);
    root = Node::InsertPos(root, new Node(7), 1);
    root = Node::InsertPos(root, new Node(9), 2);
    root = Node::InsertPos(root, new Node(4), 2);
    int res = 0;
    auto f = [&](Node* root) {
        if (!root) return;
        res = root->Value.sum;
    };
    root = Node::DoSegment(root, 1, 2, f);
    std::cout << res << std::endl;
}