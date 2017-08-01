#pragma once

#include <QString>
#include <string>
#include <map>

class TextConfigLoader
{
public:
    TextConfigLoader(const QString &filename);
    TextConfigLoader(const std::string &filename);
    TextConfigLoader(const char *filename);
    ~TextConfigLoader();
    void reload();
    QString get_string(const QString &str)
    {
        if (!check(str))
            throw "no " + str.toStdString() + " in config.";
        return config_map_[str];
    }
    float get_float(const QString &str)
    {
        if (!check(str))
            throw "no " + str.toStdString() + " in config.";
        return config_map_[str].toFloat();
    }
    int get_int(const QString &str)
    {
        if (!check(str))
            throw "no " + str.toStdString() + " in config.";
        return config_map_[str].toInt();
    }
    bool get_bool(const QString &str)
    {
        if (!check(str))
            throw "no " + str.toStdString() + " in config.";
        return config_map_[str] == "True" || config_map_[str] == "true";
    }

    QString get_string(const QString &str, const QString &default)
    {
        if (!check(str))
            return default;
        return config_map_[str];
    }
    float get_float(const QString &str, const float &default)
    {
        if (!check(str))
            return default;
        return config_map_[str].toFloat();
    }
    int get_int(const QString &str, const int &default)
    {
        if (!check(str))
            return default;
        return config_map_[str].toInt();
    }
    bool get_bool(const QString &str, const bool &default)
    {
        if (!check(str))
            return default;
        return config_map_[str] == "True" || config_map_[str] == "true";
    }

private:
    QString filename_;
    std::map<QString, QString> config_map_;
    void read();
    bool check(const QString &str);
};