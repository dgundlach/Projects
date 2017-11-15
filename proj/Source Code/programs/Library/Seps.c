#include "CSet.h"

int IsColon(char ch) {

    return (ch == ':');
}

int IsComma(char ch) {

    return (ch == ',');
}

int IsTab(char ch) {

    return (ch == '\t');
}

int IsWhiteSpace(char ch) {

    return (ch == ' ' || ch == '\t');
}

static CSet _UserCSet_;

void InitUserSet(char *vals) {

    char c;

    ClearCSet(&_UserCSet_);
    while ((c = *vals++)) {
        AddToCSet(&_UserCSet_, c);
    }
}

int InUserSet(char ch) {

    return CharInCSet(&_UserCSet_, ch);
}
