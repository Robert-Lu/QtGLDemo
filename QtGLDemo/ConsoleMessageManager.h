#pragma once

#include <QString>
#include <string>

#define TRIVIAL_MSG     0x01
#define INFO_MSG        0x02
#define DATA_MSG        0x04
#define WARNING_MSG     0x08
#define ERROR_MSG       0x10
#define DEFAULT_MSG     0x20
#define BUFFER_INFO_MSG 0x40
#define MATRIX_MSG      0x80

#define DEFAULT_MAX_LEN  25000
#define MAX_MAT_SIZE     99

class ConsoleMessageManager
{
public:
    explicit ConsoleMessageManager(std::ostream &o)
    : out(o), indent_level(0), end_with("\n"), max_string_length(DEFAULT_MAX_LEN)
    {
        // default setting;
        msg_mask = ERROR_MSG | WARNING_MSG | DATA_MSG | INFO_MSG | DEFAULT_MSG;
    }
    void enable(unsigned msg_code) { msg_mask |= msg_code; }
    void silent(unsigned msg_code) { msg_mask &= ~msg_code; }

    void log(const QString &s, unsigned msg_code = DEFAULT_MSG) const
    {
        if (msg_code & msg_mask && s.size() <= max_string_length)
        {
            preface(msg_code);
            for (int i = 0; i < indent_level; ++i)
                out << '\t';
            out << s.toStdString() << end_with;
        }
    }

    void log(const QString &s, const QString &s2, unsigned msg_code = DEFAULT_MSG) const
    {
        if (msg_code & msg_mask && s.size() <= max_string_length && s2.size() <= max_string_length)
        {
            preface(msg_code);
            for (int i = 0; i < indent_level; ++i)
                out << '\t';
            out << s.toStdString() << s2.toStdString() << end_with;
        }
    }

    void log(const std::string &s, unsigned msg_code = DEFAULT_MSG) const
    {
        if (msg_code & msg_mask && s.size() <= max_string_length)
        {
            preface(msg_code);
            for (int i = 0; i < indent_level; ++i)
                out << '\t';
            out << s << end_with;
        }
    }

    void log(const char *s, unsigned msg_code = DEFAULT_MSG) const
    {
        if (msg_code & msg_mask)
        {
            preface(msg_code);
            for (int i = 0; i < indent_level; ++i)
                out << '\t';
            out << s << end_with;
        }
    }

    void set_indent(int i)
    {
        indent_level = i;
    }

    void indent(int i)
    {
        indent_level = i;
    }

    void indent_more()
    {
        indent_level++;
    }

    void indent_less()
    {
        indent_level--;
    }

    void reset_indent()
    {
        indent_level = 0;
    }

    void set_end_with(std::string &e)
    {
        end_with = e;
    }

    ~ConsoleMessageManager() {  }

private:
    std::ostream &out;
    unsigned      msg_mask;
    int           indent_level;
    int           max_string_length;
    std::string   end_with;

    void preface(unsigned mask) const
    {
        if (mask == TRIVIAL_MSG)
            out << "[trivial] ";
        else if (mask == INFO_MSG)
            out << "[ info  ] ";
        else if (mask == DATA_MSG)
            out << "[ data  ] ";
        else if (mask == WARNING_MSG)
            out << "[warning] ";
        else if (mask == ERROR_MSG)
            out << "[ error ] ";
        else if (mask == DEFAULT_MSG)
            out << "[default] ";
        else if (mask == BUFFER_INFO_MSG)
            out << "[buffer ] ";
        else if (mask == MATRIX_MSG)
            out << "[matrix ] ";
    }
};
