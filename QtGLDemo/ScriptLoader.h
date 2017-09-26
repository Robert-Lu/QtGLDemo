#pragma once

#include <QString>
#include <string>
#include <vector>

class ScriptLoader
{
public:
    ScriptLoader(const QString &filename);
    bool end()
    {
        return curr_line_ == lines_.size();
    }
    std::vector<QString> next()
    {
        return lines_[curr_line_++];
    }
    static std::vector<QString> parse(const QString &str);
    bool empty() const { return lines_.empty(); }
    ~ScriptLoader();

private:
    int curr_line_;
    std::vector<std::vector<QString>> lines_;
    QString filename_;
};

