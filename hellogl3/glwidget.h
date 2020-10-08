/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include "logo.h"

#include <QTimer>
#include <QStack>
#include <QVector2D>

class MainWindow;

class RecognizeCube;

class Recognizecylinder;

class Recognizecorner;

class Common;

class Identification_type;

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    static bool isTransparent() { return m_transparent; }
    static void setTransparent(bool t) { m_transparent = t; }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void loader_data();

    // 保存模型的数据
    QVector<GLfloat> cylinder_vector;

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void cleanup();

    void reviceStackDataSlot(QStack<QVector<float>> draw_stack);

    // 识别杯子
    void Recognize_cup(QStack<QVector<float>> draw_stack);

    // 识别台灯
    void Recognize_deskLamp(QStack<QVector<float>> draw_stack);

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupVertexAttribs();

    void allocate_vector();

    // 坐标转换
    void coordinate_transformation(QStack<QVector<float>> draw_stack);

    // 计算多边形的面积
    float calculateArea(QVector<QVector2D> &vec);

    // 三角化面片
    void genTriangle(QVector<float> &vec,QVector3D p0,QVector3D p1,QVector3D p2);
    void genRectangleZ(QVector<float> &vec,QVector3D p0,QVector3D p1);

    // 计算倾斜度
    void findMinMax(QVector<QVector2D> head_path, QVector2D &min,QVector2D &max);
    float mapEllipseToCircle(QVector<QVector2D> &head_path);

    // 画出模型

    // 画椭圆
    void genCylinder(QVector<float> &vec,QVector2D cylinder_center,float r,float z,QVector3D offset);
    void genCylinder(QVector<float> &vec,QVector<QVector2D> head_path,float z,QVector3D offset);
    void genCylinder(QVector<float> &vec,QVector<QVector2D> head_path, QVector<QVector2D> line_path, float height,QVector3D offset);
    void genCylinder(QVector<float> &vec,QVector<QVector2D> head_path, QVector<QVector2D> line_path_1, QVector<QVector2D> line_path_2, float height_1, float height_2, QVector3D offset);
    void genCylinder(QVector<float> &vec,float r,QVector<QVector2D> head_path, QVector<QVector2D> line_path, QVector3D offset);

    // 画花生
    void genCylinder(QVector<float> &vec, QVector<QVector2D> line_path, QVector3D offset);

    bool m_core;
    int m_xRot;
    int m_yRot;
    int m_zRot;
    // 使用整数精度在平面中定义一个点。
    QPoint m_lastPos;

    Logo m_logo;

    // OpenGL顶点数组对象
    QOpenGLVertexArrayObject m_vao;

    // 用于创建和管理OpenGL缓冲区对象的函数。
    QOpenGLBuffer m_logoVbo;

    // 允许OpenGL着色程序被链接和使用。
    QOpenGLShaderProgram *m_program;

    // load_data
    bool flag;

    QVector<GLfloat> temp;

    QVector<GLfloat> temp_onlyVertex;

    // draw cylinder
    bool appendingFlag;

    GLfloat radius,height_1,height_2;

    QStack<QVector<float>> draw_stack;

    QStack<QVector<float>> draw_coorstack;

    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_normalMatrixLoc;
    int m_lightPosLoc;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
    static bool m_transparent;

    float m_cameraZ;

    // 识别矩形
    RecognizeCube *recognizeCube;
    bool recognize_cube;

    // 识别椭圆
    Recognizecylinder *recognizecylinder;

    Recognizecorner *recognizecorner;

    Common *common;

    Identification_type *identificationtypes;

    // 偏移值
    float off_var;

    // 保存花生和直线交点的数组
    QVector<QVector2D> intersection_vector;

    // 圆柱体的中心(2D和3D一样不会改变)
    QVector2D  cylinder_center;

    float peanut_minX_3D;
};

#endif
