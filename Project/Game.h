#pragma once
#include <chrono>
#include <thread>

#include "Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // начать игру
    int play()
    {
        // Засекаем время начала игры для замера длительности
        auto start = chrono::steady_clock::now();

        // Если это режим replay, инициализируем логику, перезагружаем конфиг и перерисовываем доску
        if (is_replay)
        {
            logic = Logic(&board, &config); // Инициализация логики
            config.reload(); // Перезагрузка конфигурации
            board.redraw(); // Перерисовка доски
        }
        else
        {
            // Иначе начинаем новую игру с отрисовки начального состояния
            board.start_draw();
        }
        is_replay = false; // Сбрасываем флаг replay

        int turn_num = -1; // Счетчик ходов (начинается с 0)
        bool is_quit = false; // Флаг для выхода из игры
        const int Max_turns = config("Game", "MaxNumTurns"); // Максимальное число ходов из конфига

        // Основной игровой цикл
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // Сбрасываем счетчик серии взятий (для шашек)

            // Находим возможные ходы для текущего игрока (0 - белые, 1 - черные)
            logic.find_turns(turn_num % 2);

            // Если ходов нет, игра завершается
            if (logic.turns.empty())
                break;

            // Устанавливаем уровень сложности бота для текущего игрока
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Если текущий игрок - человек (не бот)
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Обрабатываем ход игрока
                auto resp = player_turn(turn_num % 2);

                // Если игрок выбрал выход
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                // Если игрок выбрал повтор игры
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                // Если игрок выбрал откат хода
                else if (resp == Response::BACK)
                {
                    // Откатываем ход, если это возможно
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback(); // Откат хода
                        --turn_num; // Корректируем счетчик ходов
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback(); // Откат хода
                    --turn_num; // Корректируем счетчик ходов
                    beat_series = 0; // Сбрасываем счетчик серии взятий
                }
            }
            else
            {
                // Если текущий игрок - бот, выполняем его ход
                bot_turn(turn_num % 2);
            }
        }

        // Засекаем время окончания игры
        auto end = chrono::steady_clock::now();

        // Записываем время игры в лог-файл
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // Если выбран режим replay, запускаем игру заново
        if (is_replay)
            return play();

        // Если игрок выбрал выход, возвращаем 0
        if (is_quit)
            return 0;

        // Определяем результат игры
        int res = 2; // По умолчанию победа черных
        if (turn_num == Max_turns)
        {
            res = 0; // Ничья, если достигнуто максимальное число ходов
        }
        else if (turn_num % 2)
        {
            res = 1; // Победа белых, если последний ход был черных
        }

        // Отображаем результат игры на доске
        board.show_final(res);

        // Ожидаем реакции игрока (например, replay или выход)
        auto resp = hand.wait();

        // Если игрок выбрал повтор игры, запускаем игру заново
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }

        // Возвращаем результат игры
        return res;
    }

private:
    // Метод bot_turn отвечает за выполнение хода бота
    void bot_turn(const bool color)
    {
        // Засекаем время начала хода для замера длительности
        auto start = chrono::steady_clock::now();

        // Получаем задержку для хода бота из конфигурации
        auto delay_ms = config("Bot", "BotDelayMS");

        // Создаем отдельный поток для задержки, чтобы она не влияла на поиск хода
        thread th(SDL_Delay, delay_ms);

        // Ищем лучшие ходы для бота
        auto turns = logic.find_best_turns(color);

        // Ожидаем завершения потока с задержкой
        th.join();

        bool is_first = true; // Флаг для первого хода в серии

        // Выполняем найденные ходы
        for (auto turn : turns)
        {
            // Если это не первый ход, добавляем задержку
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false; // Сбрасываем флаг после первого хода

            // Увеличиваем счетчик серии взятий, если ход включает взятие фигуры
            beat_series += (turn.xb != -1);

            // Выполняем ход на доске
            board.move_piece(turn, beat_series);
        }

        // Засекаем время окончания хода
        auto end = chrono::steady_clock::now();

        // Записываем время выполнения хода бота в лог-файл
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Создаем список клеток, доступных для хода
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y); // Добавляем начальные позиции ходов
        }
        board.highlight_cells(cells); // Подсвечиваем доступные клетки на доске

        move_pos pos = { -1, -1, -1, -1 }; // Структура для хранения выбранного хода
        POS_T x = -1, y = -1; // Координаты выбранной клетки

        // Основной цикл для выбора хода
        while (true)
        {
            // Получаем ответ от игрока (выбор клетки или команду)
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL) // Если ответ не связан с выбором клетки
                return get<0>(resp); // Возвращаем команду (например, QUIT, BACK, REPLAY)

            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) }; // Координаты выбранной клетки

            // Проверяем, корректна ли выбранная клетка
            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                // Если клетка является начальной позицией хода
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // Если клетка является конечной позицией хода
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn; // Сохраняем выбранный ход
                    break;
                }
            }

            // Если ход выбран, выходим из цикла
            if (pos.x != -1)
                break;

            // Если клетка некорректна, сбрасываем выделение и продолжаем
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active(); // Сбрасываем активную клетку
                    board.clear_highlight(); // Сбрасываем подсветку
                    board.highlight_cells(cells); // Подсвечиваем доступные клетки заново
                }
                x = -1;
                y = -1;
                continue;
            }

            // Если клетка корректна, сохраняем её координаты
            x = cell.first;
            y = cell.second;
            board.clear_highlight(); // Сбрасываем подсветку
            board.set_active(x, y); // Устанавливаем активную клетку

            // Подсвечиваем клетки, доступные для хода из выбранной клетки
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2); // Добавляем конечные позиции ходов
                }
            }
            board.highlight_cells(cells2); // Подсвечиваем доступные клетки
        }

        // Очищаем подсветку и активную клетку
        board.clear_highlight();
        board.clear_active();

        // Выполняем ход
        board.move_piece(pos, pos.xb != -1); // Перемещаем фигуру
        if (pos.xb == -1) // Если ход без взятия, завершаем ход
            return Response::OK;

        // Продолжаем серию взятий, если возможно
        beat_series = 1; // Счетчик серии взятий
        while (true)
        {
            // Ищем возможные взятия для текущей позиции
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats) // Если взятий больше нет, завершаем
                break;

            // Подсвечиваем клетки для продолжения взятий
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2); // Добавляем конечные позиции взятий
            }
            board.highlight_cells(cells); // Подсвечиваем клетки
            board.set_active(pos.x2, pos.y2); // Устанавливаем активную клетку

            // Цикл для выбора продолжения взятия
            while (true)
            {
                auto resp = hand.get_cell(); // Получаем ответ от игрока
                if (get<0>(resp) != Response::CELL) // Если ответ не связан с выбором клетки
                    return get<0>(resp); // Возвращаем команду

                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) }; // Координаты выбранной клетки
                // Проверяем, корректна ли выбранная клетка для взятия
                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn; // Сохраняем выбранный ход
                        break;
                    }
                }
                if (!is_correct) // Если клетка некорректна, продолжаем выбор
                    continue;

                // Если клетка корректна, выполняем взятие
                board.clear_highlight(); // Сбрасываем подсветку
                board.clear_active(); // Сбрасываем активную клетку
                beat_series += 1; // Увеличиваем счетчик серии взятий
                board.move_piece(pos, beat_series); // Перемещаем фигуру
                break;
            }
        }

        // Возвращаем успешное завершение хода
        return Response::OK;
    }

private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
