#include "wzconfig.h"

#include <cassert>

void WzConfig::setVector3f(const QString &name, const Vector3f &v)
{
    QStringList l;
    l.push_back(QString::number(v.x));
    l.push_back(QString::number(v.y));
    l.push_back(QString::number(v.z));
    setValue(name, l);
}

Vector3f WzConfig::vector3f(const QString &name)
{
    Vector3f r(0.0, 0.0, 0.0);
    if (!contains(name)) return r;
    QStringList v = value(name).toStringList();
    assert(v.size() == 3);
    r.x = v[0].toDouble();
    r.y = v[1].toDouble();
    r.z = v[2].toDouble();
    return r;
}

void WzConfig::setVector3i(const QString &name, const Vector3i &v)
{
    QStringList l;
    l.push_back(QString::number(v.x));
    l.push_back(QString::number(v.y));
    l.push_back(QString::number(v.z));
    setValue(name, l);
}

Vector3i WzConfig::vector3i(const QString &name)
{
    Vector3i r(0, 0, 0);
    if (!contains(name)) return r;
    QStringList v = value(name).toStringList();
    assert(v.size() == 3);
    r.x = v[0].toInt();
    r.y = v[1].toInt();
    r.z = v[2].toInt();
    return r;
}

void WzConfig::setVector2i(const QString &name, const Vector2i &v)
{
    QStringList l;
    l.push_back(QString::number(v.x));
    l.push_back(QString::number(v.y));
    setValue(name, l);
}

Vector2i WzConfig::vector2i(const QString &name)
{
    Vector2i r(0, 0);
    if (!contains(name)) return r;
    QStringList v = value(name).toStringList();
    assert(v.size() == 2);
    r.x = v[0].toInt();
    r.y = v[1].toInt();
    return r;
}