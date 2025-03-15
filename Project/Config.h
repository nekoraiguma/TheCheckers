#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Project_path.h"

class Config
{
    /*Оператор () перегружен для удобного доступа к значениям в JSON-конфигурации.
    Он позволяет получить значение настройки по её "директории" (разделу) и имени.*/
    /*Функция reload отвечает за загрузку конфигурационных данных из JSON-файла (settings.json)
    и их сохранение в приватном поле config типа json (из библиотеки nlohmann/json).*/
public:
    Config()
    {
        reload();
    }
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    auto operator()(const string& setting_dir, const string& setting_name) const
    {

        return config[setting_dir][setting_name];

    }

private:

    json config;

};
