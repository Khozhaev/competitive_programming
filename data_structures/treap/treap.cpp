#include <iostream>



inline uint64_t Random() {
    return rand();
}
template<typename TValue>
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

    static inline void Update(TNode* root) {
        if (!root) {
            return;
        }
        root->Size = 1 + GetSize(root->Left) + GetSize(root->Right);
        root->Value.Upd(GetValue(root->Left), GetValue(root->Right));
    }

    static inline TNode* Merge(TNode* rootLeft, TNode* rootRight) {
        if (!rootLeft) {
            return rootRight;
        }
        if (!rootRight) {
            return rootLeft;
        }
        if (rootLeft->Prior < rootRight->Prior) {
            rootLeft->Right = Merge(rootLeft->Right, rootRight);
            Update(rootLeft);
            return rootLeft;
        } else {
            rootRight->Left = Merge(rootLeft, rootRight->Left);
            Update(rootRight);
            return rootRight;
        }
    }

    static inline std::pair<TNode*, TNode*> SplitSz(TNode* root, ssize_t sz) {
        if (!root) {
            return {nullptr, nullptr};
        }
        ssize_t lsz = GetSize(root->Left);
        if (lsz >= sz) {
            auto p = SplitSz(root->Left, sz);
            root->Left = p.second;
            Update(root);
            return {p.first, root};
        } else {
            auto p = SplitSz(root->right, sz - lsz - 1);
            root->Right = p.first;
            Update(root);
            return {root, p.second};
        }
    }

    static inline std::pair<TNode*, TNode*> SplitVal(TNode* root, const TValue& value) {
        if (!root) {
            return {nullptr, nullptr};
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
        } else if (lsz > pos) {
            root = ErasePos(root->Left, pos);
            Update(root);
            return root;
        } else {
            root = ErasePos(root->Right, pos - 1 - lsz);
            Update(root);
            return root;
        }
    }
    static inline TNode* EraseVal(TNode* root, const TValue& value) {
        if (!root) {
            return nullptr;
        }
        if (root->Value == value) {
            return Merge(root->Left, root->Right);
        } else if (root->Value > value) {
            root = EraseVal(root->Left, value);
            Update(root);
            return root;
        } else {
            root = EraseVal(root->Right, value);
            Update(root);
            return root;
        }
    }


    /* Применяет операцию на отрезке [l, r], нумерация с нуля.
     * Может работать медленно из-за прослойки вызова функции,
     * если нужно ускорение - заменить вызов функтора на свой код. */
    template<typename TFunctor>
    static inline TNode* DoSegment(TNode* root, ssize_t l, ssize_t r, TFunctor& functor) {
        auto p1 = SplitSz(root,r + 1);
        auto p2 = SplitSz(p1.first, l);
        functor(p2);
        root = Merge(Merge(p2.first, p2.second), p1.second);
        return root;
    }
 };