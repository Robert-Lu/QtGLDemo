#include "stdafx.h"

#include "TextConfigLoader.h"
#include <QFile>
#include <iostream>

TextConfigLoader::TextConfigLoader(const QString& filename) : filename_(filename)
{
    read();
}

TextConfigLoader::TextConfigLoader(const std::string& filename)
{
    filename_ = QString::fromStdString(filename);
    read();
}

TextConfigLoader::TextConfigLoader(const char* filename)
{
    filename_ = QString(filename);
    read();
}

TextConfigLoader::~TextConfigLoader() {  }

void TextConfigLoader::read()
{
    // Read shader source code from files.
    QFile config_file{ filename_ };
    config_file.open(QFile::ReadOnly | QFile::Text);
    QTextStream tsv{ &config_file };
    QString line;
    while (!tsv.atEnd())
    {
        line = tsv.readLine();
        QTextStream ls{ &line };

        // blank line all whole line comment.
        if (line.isEmpty() || line.startsWith(';'))
            continue;

        QString key, temp;
        bool key_end = false;
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
                temp.push_back(c);
            // switch in_string state;
            else if (c == '\"')
            {
                if (in_string && !key_end)
                {
                    key = temp;
                    temp.clear();
                    key_end = true;
                }
                in_string = !in_string;
            }
            // inner space
            else if (c.isSpace() && !key_end)
            {
                key = temp;
                temp.clear();
                key_end = true;
                in_space = true;
            }
            else
            {
                std::cerr << "Config Parse Fail at line:\n\t"
                    << line.toStdString() << std::endl;
                ok = false;
            }
        }
        if (ok)
            config_map_[key] = temp;
    }
    config_file.close();
}
