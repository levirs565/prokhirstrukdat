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

    void setLeft(RBNode *child)
    {
        left = child;
        if (child != nullptr)
        {
            child->parent = this;
        }
    }

    void setRight(RBNode *child)
    {
        right = child;
        if (child != nullptr)
        {
            child->parent = this;
        }
    }
};

template <typename T, typename K>
struct RBTree
{
    using NodeType = RBNode<T>;
    using KeyType = std::invoke_result_t<K, const T&>;
    NodeType *root = nullptr;
    K getKey;

    RBTree(K keyGetter) : getKey(keyGetter) {}

    void insert(T value)
    {
        NodeType *node = new NodeType();
        node->value = value;

        if (root == nullptr)
        {
            root = node;
            root->isRed = false;
        }
        else
        {
            NodeType *current = root;
            NodeType *mostClose = nullptr;

            while (current != nullptr)
            {
                mostClose = current;

                if (getKey(value) == getKey(current->value))
                {
                    throw std::domain_error("Value has been added");
                }
                else if (getKey(value) < getKey(current->value))
                {
                    current = current->left;
                }
                else
                {
                    current = current->right;
                }
            }

            if (getKey(value) < getKey(mostClose->value))
            {
                mostClose->setLeft(node);
            }
            else
            {
                mostClose->setRight(node);
            }

            insertFixUp(node);
        }
    }

    void internalFindBetween(
        NodeType *node,
        const KeyType &from,
        const KeyType &to,
        const std::function<void(NodeType *)> &visitor)
    {
        if (node == nullptr)
        {
            return;
        }

        if (getKey(node->value) >= from && getKey(node->value) <= to)
        {
            internalFindBetween(node->left, from, to, visitor);
            visitor(node);
            internalFindBetween(node->right, from, to, visitor);
        }
        else if (getKey(node->value) < from)
        {
            internalFindBetween(node->right, from, to, visitor);
        }
        else
        {
            internalFindBetween(node->left, from, to, visitor);
        }
    }

    void findBetween(
        const KeyType &from,
        const KeyType &to,
        const std::function<void(NodeType *)> &visitor)
    {
        internalFindBetween(root, from, to, visitor);
    }

    void inorder(NodeType *node, const std::function<void(NodeType *)> &visit)
    {
        if (node == nullptr)
        {
            return;
        }
        inorder(node->left, visit);
        visit(node);
        inorder(node->right, visit);
    }

    int maxLevel(NodeType *node, int current)
    {
        if (node == nullptr)
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

    void rotateRight(NodeType *node)
    {
        NodeType *other = node->left;
        NodeType *newLeft = other->right;

        swapParent(node, other);
        other->setRight(node);
        node->setLeft(newLeft);
    }

    void rotateLeft(NodeType *node)
    {
        NodeType *other = node->right;
        NodeType *newRight = other->left;

        swapParent(node, other);
        other->setLeft(node);
        node->setRight(newRight);
    }

    void insertFixUp(NodeType *node)
    {
        while (node != root && node->parent->isRed)
        {
            NodeType *grandParent = node->parent->parent;
            NodeType *uncle = nullptr;

            assert(grandParent != nullptr);

            if (grandParent->left == node->parent)
            {
                uncle = grandParent->right;
            }
            else
            {
                uncle = grandParent->left;
            }

            // assert(uncle != nullptr);

            if (uncle && uncle->isRed)
            {
                node->parent->isRed = false;
                uncle->isRed = false;
                grandParent->isRed = true;
                node = grandParent;
            }
            else
            {
                if (node->parent == grandParent->right && node == node->parent->right)
                {
                    rotateLeft(grandParent);

                    node->parent->isRed = false;
                    grandParent->isRed = true;
                }

                if (node->parent == grandParent->left && node == node->parent->left)
                {
                    rotateRight(grandParent);

                    node->parent->isRed = false;
                    grandParent->isRed = true;
                }

                if (node->parent == grandParent->right && node == node->parent->left)
                {
                    rotateRight(node->parent);
                    rotateLeft(grandParent);

                    node->isRed = false;
                    grandParent->isRed = true;
                }

                if (node->parent == grandParent->left && node == node->parent->right)
                {
                    rotateLeft(node->parent);
                    rotateRight(grandParent);

                    node->isRed = false;
                    grandParent->isRed = true;
                }
            }
        }

        root->isRed = false;
    }
};