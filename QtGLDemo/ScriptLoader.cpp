#include "stdafx.h"
#include "ScriptLoader.h"
#include <QFile>
#include <iostream>

ScriptLoader::ScriptLoader(const QString& filename)
    : curr_line_(0), filename_(filename)
{
    // Read shader source code from files.
    QFile script_file{ filename_ };
    script_file.open(QFile::ReadOnly | QFile::Text);
    QTextStream tsv{ &script_file };
    QString line;

    while (!tsv.atEnd())
    {
        line = tsv.readLine();
        QTextStream ls{ &line };

        // blank line all whole line comment.
        if (line.isEmpty() || line.startsWith(';'))
            continue;

        QString temp;
        bool in_string = false;
        bool in_space = false;
        bool ok = true;

        lines_.push_back(std::vector<QString>());

        for (auto c : line)
        {
            // if state is in_string, put into temp anyway.
            if (in_string && c != '\"')
            {
                temp.push_back(c);
                continue;
            }

            if (in_space && c.isSpace())
            {
                continue;
            }

            // throw remaining characters.
            if (c == ';')
                break;
            // put into temp for non-space and not '\"'.
            else if (!c.isSpace() && c != '\"')
            {
                temp.push_back(c);
                in_space = false;
            }
            // switch in_string state;
            else if (c == '\"')
            {
                //if (in_string)
                //{
                //    //lines_.back().push_back(temp);
                //    //temp.clear();
                //}
                in_string = !in_string;
                in_space = false;
            }
            // inner space
            else if (c.isSpace())
            {
                lines_.back().push_back(temp);
                temp.clear();
                in_space = true;
            }
            else
            {
                std::cerr << "Config Parse Fail at line:\n\t"
                    << line.toStdString() << std::endl;
                ok = false;
            }
        }

        if (ok && !temp.isEmpty())
            lines_.back().push_back(temp);

        if (ok && lines_.back().empty())
            lines_.pop_back();
            
    }
    script_file.close();
}

std::vector<QString> ScriptLoader::parse(const QString& line)
{
    std::vector<QString> script;

    QString temp;
    bool in_string = false;
    bool in_space = false;
    bool ok = true;

    for (auto c : line)
    {
        // if state is in_string, put into temp anyway.
        if (in_string && c != '\"')
        {
            temp.push_back(c);
            continue;
        }

        if (in_space && c.isSpace())
        {
            continue;
        }

        // throw remaining characters.
        if (c == ';')
            break;
        // put into temp for non-space and not '\"'.
        else if (!c.isSpace() && c != '\"')
        {
            temp.push_back(c);
            in_space = false;
        }
        // switch in_string state;
        else if (c == '\"')
        {
            in_string = !in_string;
            in_space = false;
        }
        // inner space
        else if (c.isSpace())
        {
            script.push_back(temp);
            temp.clear();
            in_space = true;
        }
        else
        {
            std::cerr << "Script Parse Fail at line:\n\t"
                << line.toStdString() << std::endl;
            ok = false;
        }
    }

    if (ok && !temp.isEmpty())
        script.push_back(temp);

    return script;
}

ScriptLoader::~ScriptLoader()
{
}
