#ifndef QFUNCTIONAL_H
#define QFUNCTIONAL_H

#include <functional>

//
// The small functional header for Qt.
// Dont's:
// 1) No support for other containers than QList
// 2) No in-place modification of in lists
// 3) No member function pointers
// Do's:
// 1) Handle QStringList gracefully
//

template<typename OUT, typename IN>
QList<OUT> map(const QList<IN> &list, OUT (*mapFunction)(const IN &))
{
    QList<OUT> result;
    foreach (const IN &inValue, list) {
        result.append(mapFunction(inValue));
    }
    
    return result;
}

template<typename OUT>
QList<OUT> map(const QStringList &list, OUT (*mapFunction)(const QString &))
{
    QList<OUT> result;
    foreach (const QString &inValue, list) {
        result.append(mapFunction(inValue));
    }

    return result;
}

template<typename OUT>
QList<OUT> map(const QStringList &list1, const QStringList &list2, const QStringList &list3, const QStringList &list4,
               OUT (*mapFunction)(const QString &, const QString &, const QString &, const QString &))
{
    QList<OUT> result;
    const int resultCount = qMin(qMin(qMin(list1.count(), list2.count()), list3.count()), list4.count());
    for (int i = 0; i < resultCount; ++i) {
        result.append(mapFunction(list1.at(i), list2.at(i), list3.at(i), list4.at(i)));
    }
    return result;
}


template<typename STRUCT, typename IN1, typename IN2>
QList<STRUCT> listsToStructs(const QList<IN1> &list1, const QList<IN2> &list2)
{
    QList<STRUCT> result;
    const int resultCount = qMin(list1.count(), list2.count());
    for (int i = 0; i < resultCount; ++i) {
        result.append(STRUCT(list1.at(i), list2.at(i)));
    }

    return result;
}

template<typename STRUCT, typename IN1, typename IN2, typename IN3>
QList<STRUCT> listsToStructs(const QList<IN1> &list1, const QList<IN2> &list2, const QList<IN3> &list3)
{
    QList<STRUCT> result;
    const int resultCount = qMin(qMin(list1.count(), list2.count()), list3.count());
    for (int i = 0; i < resultCount; ++i) {
        result.append(STRUCT(list1.at(i), list2.at(i), list3.at(i)));
    }

    return result;
}




#endif
