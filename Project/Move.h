#pragma once
#include <stdlib.h>
#include <cstdint>

// ќпределение типа POS_T как 8-битного целого числа (int8_t)
typedef int8_t POS_T;

// —труктура move_pos описывает ход на игровой доске
struct move_pos
{
    POS_T x, y;             //  оординаты начальной позиции (откуда)
    POS_T x2, y2;           //  оординаты конечной позиции (куда)
    POS_T xb = -1, yb = -1; //  оординаты позиции вз€той фигуры (если есть)

    //  онструктор дл€ хода без вз€ти€ фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    //  онструктор дл€ хода с вз€тием фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // ѕерегрузка оператора равенства дл€ сравнени€ двух ходов
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // ѕерегрузка оператора неравенства дл€ сравнени€ двух ходов
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};