#pragma once
#include <stdlib.h>
#include <cstdint>

// Определение типа POS_T как 8-битного целого числа (int8_t)
typedef int8_t POS_T;

// Структура move_pos описывает ход на игровой доске
struct move_pos
{
    POS_T x, y;             // Координаты начальной позиции (откуда)
    POS_T x2, y2;           // Координаты конечной позиции (куда)
    POS_T xb = -1, yb = -1; // Координаты позиции взятой фигуры (если есть)

    // Конструктор для хода без взятия фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Конструктор для хода с взятием фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Перегрузка оператора равенства для сравнения двух ходов
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Перегрузка оператора неравенства для сравнения двух ходов
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};