#pragma once
#include <QStringList>

struct AudioRoute {
    QString id;
    bool followDefault=true;
    bool fellBack=false;
};

inline AudioRoute resolveAudioRoute(const QStringList &available, const QString &systemDefault, const QString &selected) {
    if(!selected.isEmpty() && available.contains(selected))return {selected,false,false};
    const auto target=available.contains(systemDefault)?systemDefault:available.value(0);
    return {target,true,!selected.isEmpty()};
}
