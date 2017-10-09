#pragma once
#include <QTime>

class QTiming
{
public:
    static QTime T;
    static void Tic() { T.start(); }
    static int Toc() { return T.elapsed(); }
};

