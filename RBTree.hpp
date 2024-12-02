#pragma once

#include <cassert>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <functional>

template <typename T>
struct RBNode
{
    RBNode<T> *left = nullptr, *right = nullptr, *parent = nullptr;
    bool isRed = true;
    T value;
};

template <typename T, typename K>
struct RBTree
{
    using NodeType = RBNode<T>;
    NodeType *root = nullptr;
    size_t count = 0;
    K isLess;
    NodeType *nil;

    RBTree(K comparer) : isLess(comparer)
    {
        nil = new NodeType();
        nil->left = nullptr;
        nil->right = nullptr;
        nil->parent = nullptr;
        nil->isRed = false;
        root = nil;
    }

    void insert(T &&value)
    {
        NodeType *z = new NodeType();
        z->left = nil;
        z->right = nil;
        z->value = std::move(value);

        if (root == nil)
        {
            root = z;
            root->isRed = false;
        }
        else
        {
            NodeType *x = root;
            NodeType *y = nullptr;

            while (x != nil)
            {
                y = x;

                if (isLess(z->value, x->value))
                {
                    x = x->left;
                }
                else if (isLess(x->value, z->value))
                {
                    x = x->right;
                }
                else
                {
                    throw std::domain_error("Value has been added");
                }
            }

            z->parent = y;
            if (isLess(z->value, y->value))
            {
                y->left = z;
            }
            else
            {
                y->right = z;
            }

            count++;
            insertFixUp(z);
        }
    }

    NodeType *findNode(const T &value)
    {
        NodeType *current = root;

        while (current != nil && current->value != value)
        {
            if (isLess(value, current->value))
            {
                current = current->left;
            }
            else
            {
                current = current->right;
            }
        }

        return current;
    }

    void transplant(NodeType *u, NodeType *v)
    {
        if (u->parent == nullptr)
        {
            root = v;
        }
        else if (u == u->parent->left)
        {
            u->parent->left = v;
        }
        else
        {
            u->parent->right = v;
        }
        v->parent = u->parent;
    }

    NodeType *minimum(NodeType *x)
    {
        while (x->left != nil)
            x = x->left;
        return x;
    }

    bool remove(const T &value)
    {
        NodeType *node = findNode(value);

        if (node == nil)
        {
            return false;
        }

        removeNode(node);
        return true;
    }

    void removeNode(NodeType *z)
    {
        NodeType *y = z;
        bool yOrigRed = y->isRed;

        NodeType *x = nullptr;
        if (z->left == nil)
        {
            x = z->right;
            transplant(z, z->right);
        }
        else if (z->right == nil)
        {
            x = z->left;
            transplant(z, z->left);
        }
        else
        {
            y = minimum(z->right);
            yOrigRed = y->isRed;
            x = y->right;

            if (y->parent == z)
                x->parent = y;
            else
            {
                transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }

            transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->isRed = z->isRed;
        }

        delete z;
        if (!yOrigRed)
            removeFixUp(x);
    }

    void removeFixUp(NodeType *x)
    {
        while (x != root && !x->isRed)
        {
            if (x == x->parent->left)
            {
                NodeType *w = x->parent->right;

                if (w->isRed)
                {
                    w->isRed = false;
                    x->parent->isRed = true;
                    rotateLeft(x->parent);
                    w = x->parent->right;
                }
                if (!w->left->isRed && !w->right->isRed)
                {
                    w->isRed = true;
                    x = x->parent;
                }
                else
                {
                    if (!w->right->isRed)
                    {
                        w->left->isRed = false;
                        w->isRed = true;
                        rotateRight(w);
                        w = x->parent->right;
                    }

                    w->isRed = x->parent->isRed;
                    x->parent->isRed = false;
                    w->right->isRed = false;
                    rotateLeft(x->parent);
                    x = root;
                }
            }
            else
            {
                NodeType *w = x->parent->left;

                if (w->isRed)
                {
                    w->isRed = false;
                    x->parent->isRed = true;
                    rotateRight(x->parent);
                    w = x->parent->left;
                }

                if (!w->right->isRed && !w->left->isRed)
                {
                    w->isRed = true;
                    x = x->parent;
                }
                else
                {
                    if (!w->left->isRed)
                    {
                        w->right->isRed = false;
                        w->isRed = true;
                        rotateLeft(w);
                        w = x->parent->left;
                    }

                    w->isRed = x->parent->isRed;
                    x->parent->isRed = false;
                    w->left->isRed = false;
                    rotateRight(x->parent);
                    x = root;
                }
            }
        }

        x->isRed = false;
    }

    void internalFindBetween(
        NodeType *node,
        const T &from,
        const T &to,
        const std::function<void(NodeType *)> &visitor)
    {
        if (node == nullptr)
        {
            return;
        }

        // node->value >= from          node->value <= to
        // !(node->value < from)        !(node->value > to)
        //                              !(to < node->value)
        if (!isLess(node->value, from) && !isLess(to, node->value))
        {
            internalFindBetween(node->left, from, to, visitor);
            visitor(node);
            internalFindBetween(node->right, from, to, visitor);
        }
        else if (isLess(node->value, from))
        {
            internalFindBetween(node->right, from, to, visitor);
        }
        else
        {
            internalFindBetween(node->left, from, to, visitor);
        }
    }

    void findBetween(
        const T &from,
        const T &to,
        const std::function<void(NodeType *)> &visitor)
    {
        internalFindBetween(root, from, to, visitor);
    }

    void inorder(NodeType *node, const std::function<void(NodeType *)> &visit)
    {
        if (node == nil)
        {
            return;
        }
        inorder(node->left, visit);
        visit(node);
        inorder(node->right, visit);
    }

    int maxLevel(NodeType *node, int current)
    {
        if (node == nil)
            return current;

        return std::max(maxLevel(node->left, current + 1), maxLevel(node->right, current + 1));
    }

    void swapParent(NodeType *node, NodeType *other)
    {
        if (node->parent == nullptr)
        {
            other->parent = nullptr;
            root = other;
        }
        else if (node->parent->left == node)
        {
            node->parent->setLeft(other);
        }
        else
        {
            node->parent->setRight(other);
        }
    }

    void rotateRight(NodeType *x)
    {
        NodeType *y = x->left;
        x->left = y->right;

        if (y->right != nil)
            y->right->parent = x;

        y->parent = x->parent;

        if (x->parent == nullptr)
        {
            root = y;
        }
        else if (x == x->parent->right)
        {
            x->parent->right = y;
        }
        else
        {
            x->parent->left = y;
        }

        y->right = x;
        x->parent = y;
    }

    void rotateLeft(NodeType *x)
    {
        NodeType *y = x->right;
        x->right = y->left;

        if (y->left != nil)
            y->left->parent = x;

        y->parent = x->parent;
        if (x->parent == nullptr)
        {
            root = y;
        }
        else if (x == x->parent->left)
        {
            x->parent->left = y;
        }
        else
        {
            x->parent->right = y;
        }

        y->left = x;
        x->parent = y;
    }

    void insertFixUp(NodeType *node)
    {
        while (node != root && node->parent->isRed)
        {
            NodeType *uncle = nullptr;

            if (node->parent->parent->left == node->parent)
            {
                uncle = node->parent->parent->right;
            }
            else
            {
                uncle = node->parent->parent->left;
            }

            if (uncle && uncle->isRed)
            {
                node->parent->isRed = false;
                uncle->isRed = false;
                node->parent->parent->isRed = true;
                node = node->parent->parent;
            }
            else if (node->parent == node->parent->parent->right)
            {
                if (node == node->parent->left)
                {
                    node = node->parent;
                    rotateRight(node);
                }

                node->parent->isRed = false;
                node->parent->parent->isRed = true;

                rotateLeft(node->parent->parent);
            }
            else if (node->parent == node->parent->parent->left)
            {
                if (node == node->parent->right)
                {
                    node = node->parent;
                    rotateLeft(node);
                }
                node->parent->isRed = false;
                node->parent->parent->isRed = true;

                rotateRight(node->parent->parent);
            }
        }

        root->isRed = false;
    }
};