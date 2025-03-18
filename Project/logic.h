#pragma once
#include <random>
#include <vector>

#include "Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9; // Определение бесконечности для оценки

class Logic
{
public:
    // Конструктор класса Logic, инициализирует доску и конфигурацию, а также настраивает генератор случайных чисел
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0); // Инициализация генератора случайных чисел
        scoring_mode = (*config)("Bot", "BotScoringType"); // Получение режима оценки ходов
        optimization = (*config)("Bot", "Optimization"); // Получение параметров оптимизации
    }

    // Метод для поиска лучшего хода для текущего игрока
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear(); // Очистка предыдущих состояний
        next_move.clear(); // Очистка предыдущих ходов

        find_first_best_turn(board->get_board(), color, -1, -1, 0); // Поиск лучшего хода

        int cur_state = 0;
        vector<move_pos> res;
        do
        {
            res.push_back(next_move[cur_state]); // Добавление лучшего хода в результат
            cur_state = next_best_state[cur_state]; // Переход к следующему состоянию
        } while (cur_state != -1 && next_move[cur_state].x != -1); // Пока есть следующие ходы
        return res;
    }

private:
    // Метод для выполнения хода на доске
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0; // Удаление фигуры, если это взятие
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // Превращение пешки в дамку
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещение фигуры
        mtx[turn.x][turn.y] = 0; // Очистка старой позиции
        return mtx;
    }

    // Метод для расчета оценки текущего состояния доски
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - кто является максимизирующим игроком
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // Подсчет белых пешек
                wq += (mtx[i][j] == 3); // Подсчет белых дамок
                b += (mtx[i][j] == 2); // Подсчет черных пешек
                bq += (mtx[i][j] == 4); // Подсчет черных дамок
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Учет потенциала белых пешек
                    b += 0.05 * (mtx[i][j] == 2) * (i); // Учет потенциала черных пешек
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w); // Обмен значений для противоположного цвета
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF; // Если белых фигур нет, возвращаем бесконечность
        if (b + bq == 0)
            return 0; // Если черных фигур нет, возвращаем 0
        int q_coef = 4; // Коэффициент для дамок
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5; // Изменение коэффициента для дамок
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Возвращаем оценку
    }

    // Рекурсивный метод для поиска лучшего хода
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        next_best_state.push_back(-1); // Добавление нового состояния
        next_move.emplace_back(-1, -1, -1, -1); // Добавление нового хода
        double best_score = -1; // Инициализация лучшей оценки
        if (state != 0)
            find_turns(x, y, mtx); // Поиск возможных ходов
        auto turns_now = turns; // Получение текущих ходов
        bool have_beats_now = have_beats; // Проверка наличия взятий

        if (!have_beats_now && state != 0)
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha); // Рекурсивный поиск лучшего хода
        }

        vector<move_pos> best_moves; // Лучшие ходы
        vector<int> best_states; // Лучшие состояния

        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;
            if (have_beats_now)
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score); // Рекурсивный поиск с взятием
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score); // Рекурсивный поиск без взятия
            }
            if (score > best_score)
            {
                best_score = score; // Обновление лучшей оценки
                next_best_state[state] = (have_beats_now ? int(next_state) : -1); // Обновление лучшего состояния
                next_move[state] = turn; // Обновление лучшего хода
            }
        }
        return best_score; // Возвращаем лучшую оценку
    }

    // Рекурсивный метод для поиска лучшего хода с использованием алгоритма минимакс и альфа-бета отсечения
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        // Если достигнута максимальная глубина рекурсии, возвращаем оценку текущего состояния доски
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color));
        }

        // Если указаны координаты фигуры, ищем возможные ходы для неё
        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        else // Иначе ищем ходы для всех фигур текущего цвета
        {
            find_turns(color, mtx);
        }

        auto turns_now = turns; // Сохраняем текущие ходы
        bool have_beats_now = have_beats; // Сохраняем информацию о наличии взятий

        // Если нет взятий и указаны координаты фигуры, переходим к следующему ходу
        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // Если нет возможных ходов, возвращаем оценку в зависимости от глубины
        if (turns.empty())
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1; // Минимальная оценка для минимизирующего игрока
        double max_score = -1; // Максимальная оценка для максимизирующего игрока

        // Перебираем все возможные ходы
        for (auto turn : turns_now)
        {
            double score = 0.0;
            // Если нет взятий и не указаны координаты фигуры, рекурсивно ищем лучший ход
            if (!have_beats_now && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else // Иначе продолжаем поиск с текущими координатами
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            // Обновляем минимальную и максимальную оценки
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // Альфа-бета отсечение
            if (depth % 2)
                alpha = max(alpha, max_score); // Обновляем альфа для максимизирующего игрока
            else
                beta = min(beta, min_score); // Обновляем бета для минимизирующего игрока

            // Если альфа больше или равна бета, прекращаем поиск (отсечение)
            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }

        // Возвращаем оценку в зависимости от глубины
        return (depth % 2 ? max_score : min_score);
    }

    // Метод для поиска всех возможных ходов для текущего цвета
public:
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    // Метод для поиска всех возможных ходов для конкретной фигуры
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

    // Приватный метод для поиска всех возможных ходов для текущего цвета на заданной доске
private:
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns; // Вектор для хранения всех возможных ходов
        bool have_beats_before = false; // Флаг для проверки наличия взятий

        // Перебираем все клетки доски
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // Если клетка содержит фигуру противоположного цвета, ищем ходы для неё
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    // Если найдены взятия, очищаем предыдущие ходы и обновляем флаг
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    // Если есть взятия или их не было ранее, добавляем ходы в результат
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }

        turns = res_turns; // Сохраняем найденные ходы
        shuffle(turns.begin(), turns.end(), rand_eng); // Перемешиваем ходы для случайности
        have_beats = have_beats_before; // Обновляем флаг наличия взятий
    }

    // Метод для поиска всех возможных ходов для конкретной фигуры на заданной доске
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear(); // Очищаем предыдущие ходы
        have_beats = false; // Сбрасываем флаг наличия взятий
        POS_T type = mtx[x][y]; // Тип фигуры

        // Проверка на взятия
        switch (type)
        {
        case 1:
        case 2:
            // Проверка для пешек
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb); // Добавляем ход с взятием
                }
            }
            break;
        default:
            // Проверка для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb); // Добавляем ход с взятием
                        }
                    }
                }
            }
            break;
        }

        // Если есть взятия, завершаем поиск
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }

        // Проверка на обычные ходы (без взятий)
        switch (type)
        {
        case 1:
        case 2:
            // Проверка для пешек
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j); // Добавляем обычный ход
            }
            break;
        }
        default:
            // Проверка для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2); // Добавляем обычный ход
                    }
                }
            }
            break;
        }
    }

    // Публичные поля и методы
public:
    vector<move_pos> turns; // Вектор для хранения возможных ходов
    bool have_beats; // Флаг наличия взятий
    int Max_depth; // Максимальная глубина рекурсии

    // Приватные поля
private:
    default_random_engine rand_eng; // Генератор случайных чисел
    string scoring_mode; // Режим оценки ходов
    string optimization; // Параметры оптимизации
    vector<move_pos> next_move; // Вектор для хранения следующего хода
    vector<int> next_best_state; // Вектор для хранения следующего лучшего состояния
    Board* board; // Указатель на доску
    Config* config; // Указатель на конфигурацию
};