#ifndef RECOGNIZECYLINDER_H
#define RECOGNIZECYLINDER_H

#include <QStack>

class Identification_relation;

class Identification_type;

class Recognizecylinder
{
public:
    Recognizecylinder();

    bool recognize_cylinder_shape(QStack<QVector<float>> draw_coorstack);

    QStack<QVector<float>> draw_coorstack;

    float radius;
    float height_1;
    float height_2;

    // 记录类型
    QString str_1, str_2;

    // 记录关系的表结构
    QVector<QVector<QString>> vec;

    // 保存类型
    QVector<QString> type_vec;

private:
    Identification_relation *identification_relation;
    Identification_type *identification_type;

};

#endif // RECOGNIZECYLINDER_H
