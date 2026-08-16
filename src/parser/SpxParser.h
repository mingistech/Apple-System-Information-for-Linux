#pragma once

#include "parser/PlistValue.h"

#include <QString>

class QIODevice;

class SpxParser
{
public:
    struct Result {
        bool ok = false;
        PlistValue root;
        QString error;
        int line = 0;
        int column = 0;
    };

    static Result parseFile(const QString &path);
    static Result parseDevice(QIODevice *device);

    static bool looksLikeSystemReport(const PlistValue &root);

private:
    static PlistValue parseValue(class QXmlStreamReader &xml);
    static PlistValue parseDict(class QXmlStreamReader &xml);
    static PlistValue parseArray(class QXmlStreamReader &xml);
};
