#pragma once
#include <tuple>

#include "Move.h"
#include "Response.h"
#include "Board.h"

// Класс Hand отвечает за обработку пользовательского ввода (мышь, события окна)
class Hand
{
public:
    // Конструктор, принимающий указатель на объект Board
    Hand(Board* board) : board(board)
    {
    }

    // Метод для получения выбранной клетки или команды от пользователя
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // Событие SDL
        Response resp = Response::OK; // Ответ по умолчанию
        int x = -1, y = -1; // Координаты мыши
        int xc = -1, yc = -1; // Координаты клетки на доске

        // Основной цикл обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие события
            {
                switch (windowEvent.type) // Обрабатываем тип события
                {
                case SDL_QUIT: // Если событие - закрытие окна
                    resp = Response::QUIT; // Устанавливаем ответ QUIT
                    break;

                case SDL_MOUSEBUTTONDOWN: // Если событие - нажатие кнопки мыши
                    x = windowEvent.motion.x; // Получаем координату X мыши
                    y = windowEvent.motion.y; // Получаем координату Y мыши

                    // Преобразуем координаты мыши в координаты клетки на доске
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // Обработка специальных кнопок (например, "назад" и "повторить")
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Если нажата кнопка "назад"
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Если нажата кнопка "повторить"
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Если выбрана клетка на доске
                    }
                    else
                    {
                        xc = -1; // Некорректные координаты
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT: // Если событие - изменение размера окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Сбрасываем размер окна
                        break;
                    }
                }

                // Если получен ответ, отличный от OK, выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }

        // Возвращаем ответ и координаты клетки
        return { resp, xc, yc };
    }

    // Метод для ожидания действия пользователя (например, нажатия кнопки)
    Response wait() const
    {
        SDL_Event windowEvent; // Событие SDL
        Response resp = Response::OK; // Ответ по умолчанию

        // Основной цикл обработки событий
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие события
            {
                switch (windowEvent.type) // Обрабатываем тип события
                {
                case SDL_QUIT: // Если событие - закрытие окна
                    resp = Response::QUIT; // Устанавливаем ответ QUIT
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED: // Если событие - изменение размера окна
                    board->reset_window_size(); // Сбрасываем размер окна
                    break;

                case SDL_MOUSEBUTTONDOWN: // Если событие - нажатие кнопки мыши
                {
                    int x = windowEvent.motion.x; // Получаем координату X мыши
                    int y = windowEvent.motion.y; // Получаем координату Y мыши
                    // Преобразуем координаты мыши в координаты клетки на доске
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);

                    // Если нажата кнопка "повторить"
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                break;
                }

                // Если получен ответ, отличный от OK, выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }

        // Возвращаем ответ
        return resp;
    }

private:
    Board* board; // Указатель на объект Board для взаимодействия с доской
};