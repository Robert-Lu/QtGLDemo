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
    QString get_string(const QString &str) { return config_map_[str]; }
    float get_float(const QString &str) { return config_map_[str].toFloat(); }
    int get_int(const QString &str) { return config_map_[str].toInt(); }
    bool get_bool(const QString &str) { return config_map_[str] == "True" || config_map_[str] == "true"; }

private:
    QString filename_;
    std::map<QString, QString> config_map_;
    void read();
};