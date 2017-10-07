#include "stdafx.h"
#include "RenderingWidget.h"
#include "ScriptLoader.h"

void RenderingWidget::_PopScriptHistory()
{
    if (!script_history.empty() && !script_history_linecnt.empty() && !script_history_type.empty())
    {
        script_history.pop_back();
        script_history_linecnt.pop_back();
        script_history_type.pop_back();
    }
}

void RenderingWidget::_InsertScriptHistory(const QString& str, ScriptHistoryType type)
{
    int linecnt = str.count("\n") + 1;
    script_history.push_front(str);
    script_history_linecnt.push_front(linecnt);
    script_history_type.push_front(type);

    if (script_history.size() > 9)
    {
        script_history.pop_back();
        script_history_linecnt.pop_back();
        script_history_type.pop_back();
    }
    else if (gui_config.get_bool("Script_Expire", true))
    {
        QTimer::singleShot(gui_config.get_int("Script_Expire_Time", 7500),
            this, [this]() {
            _PopScriptHistory();
        });
    }
}

void RenderingWidget::_RunScriptLine(std::vector<QString> &script, int recursion_limit)
{
    if (script.empty())
        return;

    // root
    auto cmd = script[0];
    if (cmd == "read") // read
    {
        if (script.size() != 3)
        {
            auto m = QString("read require 3 segments but get %0.").arg(script.size());
            msg.log(m, ERROR_MSG);
            _InsertScriptHistory(m, ErrorType);
            return;
        }

        auto type = script[1];
        auto file = script[2];

        if (type == "mesh_inner")
        {
            ReadMeshFromFile(file, mesh_inner);
        }
        else if (type == "pc" || type == "point cloud")
        {
            ReadPointCloudFromFile(file);
        }
        else if (type == "field")
        {
            ReadDistanceFieldFromFile(file);
        }
        else
        {
            auto m = QString("unknown segment %0.").arg(type);
            msg.log(m, ERROR_MSG);
            _InsertScriptHistory(m, ErrorType);
            return;
        }
    }
    else if (cmd == "cs") // clear screen
    {
        if (script.size() != 1)
        {
            auto m = QString("cs require 1 segment but get %0.").arg(script.size());
            msg.log(m, ERROR_MSG);
            _InsertScriptHistory(m, ErrorType);
            return;
        }

        for (int i = 0; i < 10; i++)
        {
            _InsertScriptHistory("", VoidType);
        }
    }
    else if (cmd == "clear") // clear components
    {
        if (script.size() != 2 && script.size() != 1)
        {
            auto m = QString("clear require 1 or 2 segments but get %0.").arg(script.size());
            msg.log(m, ERROR_MSG);
            _InsertScriptHistory(m, ErrorType);
            return;
        }

        if (script.size() == 1)
        {
            ClearMesh();
            ClearPointCloud();
            ClearDistanceField();
        }
        else
        {
            auto type = script[1];

            if (type == "mesh")
            {
                ClearMesh();
            }
            else if (type == "pc" || type == "point cloud")
            {
                ClearPointCloud();
            }
            else if (type == "field")
            {
                ClearDistanceField();
            }
            else
            {
                auto m = QString("unknown segment %0.").arg(type);
                msg.log(m, ERROR_MSG);
                _InsertScriptHistory(m, ErrorType);
                return;
            }
        }
    }
    else if (!cmd.isEmpty() && cmd[0] == '$')
    {
        cmd = cmd.right(cmd.size() - 1);

        ScriptLoader sl{ QString("script/%0.script").arg(cmd) };
        if (sl.empty())
        {
            sl = ScriptLoader{ QString("%0.script").arg(cmd) };
            if (sl.empty())
            {
                auto m = QString("cannot find script %0.").arg(script[0]);
                msg.log(m, ERROR_MSG);
                _InsertScriptHistory(m, ErrorType);
                return;
            }
        }

        // reach here IFF script reading success.
        while (!sl.end())
        {
            auto line = sl.next();
            _RunScriptLine(line, recursion_limit - 1);
        }
    }
    else
    {
        auto m = QString("unknown command %0.").arg(script[0]);
        msg.log(m, ERROR_MSG);
        _InsertScriptHistory(m, ErrorType);
        return;
    }
}

void RenderingWidget::RunScript()
{
    if (mode != ScriptMode || script_lineedit == nullptr)
        return;

    _InsertScriptHistory(script_lineedit->text(), ScriptType);
    auto script = ScriptLoader::parse(script_lineedit->text());
    script_lineedit->clear();

    // main
    _RunScriptLine(script);
}
