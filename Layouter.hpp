#pragma once

#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>

constexpr int spacing = 5;

struct Layouter {
    int x, y, w, h;
    int nextX, nextY;
    
    int maxW = 0, maxH = 0;

    Layouter(HWND hwnd, int paddingBotom = 0) {
        x = spacing;
        y = spacing;

        RECT rect;
        GetClientRect(hwnd, &rect);

        w = rect.right - rect.left;
        h = rect.bottom - rect.top - paddingBotom;
    }

    void nextRow() {
        if (maxH == 0)
            return;

        x = spacing;
        y += maxH + spacing;
        maxH = 0;
    }

    void nextColumn() {
        if (maxW == 0) 
            return;

        x = nextX + spacing;
        maxW = 0;
    }

    POINT pos(int spaceX = 0, int spaceY = 0) {
        x += spaceX;
        y += spaceY;
        return {x, y};
    }

    SIZE component(int w, int h) {
        maxW = std::max(maxW, w);
        maxH = std::max(maxH, h);

        nextX = x + w;
        nextY = y + h;

        return {w, h};
    }

    SIZE button(int width = 75) {
        return component(width, 23);
    }    

    SIZE edit(int width = 75) {
        return component(width, 21);
    }

    SIZE fill() {
        return component(w - x - spacing, h - y - spacing);
    }
};