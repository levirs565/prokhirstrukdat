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
    NodeType *root = nullptr;
    K isLess;

    RBTree(K comparer) : isLess(comparer) {}

    void insert(T&& value)
    {
        NodeType *node = new NodeType();
        node->value = std::move(value);

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

                if (isLess(node->value, current->value))
                {
                    current = current->left;
                }
                else if (isLess(current->value, node->value))
                {
                    current = current->right;
                }
                else
                {
                    throw std::domain_error("Value has been added");
                }
            }

            if (isLess(node->value, mostClose->value))
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
            else
            {
                if (node->parent == node->parent->parent->right)
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
        }

        root->isRed = false;
    }
};