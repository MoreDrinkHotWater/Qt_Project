#include "identification_type.h"
#include "common.h"

#include <QVector>
#include <math.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <QDebug>

std::random_device rd;  //Will be used to obtain a seed for the random number engine
std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()

Identification_type *Identification_type::getInstance(){
    static Identification_type _instance;
    return &_instance;
}

Identification_type::Identification_type()
{
    common =  Common::getInstance();
}

int  Identification_type::jundge_cylinder(QVector<QVector2D> head_circle)
{

    // 1.判断椭圆！（若为椭圆，找出长、短半轴的长度。为了方便我们默认长半轴为半径）
    QVector<float> step_vector;
    std::uniform_int_distribution<> dist_int(1, head_circle.size()-1);

    // 取50组测试数据
    for (int i = 0; i < 50; i++) {

        QVector<QVector2D> OriginPoints_vector;
        QVector<int> rand_vector;

        // 随机取四个点数据
        for (int i = 0; i < 4; i++) {
            int rand = dist_int(gen);
            rand_vector.push_back(rand);
        }

        // 对随机数据排序
        std::sort(rand_vector.begin(), rand_vector.end());

        for (int j = 0; j < 4; j++)
            OriginPoints_vector.push_back(QVector2D(head_circle[rand_vector[j]]));


        float step = 1.0;

        // 托勒密定理 判断 四点共圆 (四边形要是有序的！)
        auto left = [](QVector<QVector2D> OriginPoints_vector,float step, bool flag){
            float AB = 0,CD = 0,BC = 0,AD = 0,AC = 0,BD = 0;

            //            std::cout<<"step: "<<step<<std::endl;

            if(flag)
            {
                AB =  sqrt(pow(OriginPoints_vector[1].y() * step - OriginPoints_vector[0].y() * step, 2) + pow(OriginPoints_vector[1].x() - OriginPoints_vector[0].x(), 2));
                CD =  sqrt(pow(OriginPoints_vector[3].y() * step - OriginPoints_vector[2].y() * step,2) + pow(OriginPoints_vector[3].x() - OriginPoints_vector[2].x(),2));
                BC =  sqrt(pow(OriginPoints_vector[2].y() * step - OriginPoints_vector[1].y() * step,2) + pow(OriginPoints_vector[2].x() - OriginPoints_vector[1].x(),2));
                AD =  sqrt(pow(OriginPoints_vector[3].y() * step - OriginPoints_vector[0].y() * step,2) + pow(OriginPoints_vector[3].x() - OriginPoints_vector[0].x(),2));
                AC =  sqrt(pow(OriginPoints_vector[2].y() * step - OriginPoints_vector[0].y() * step,2) + pow(OriginPoints_vector[2].x() - OriginPoints_vector[0].x(),2));
                BD =  sqrt(pow(OriginPoints_vector[3].y() * step - OriginPoints_vector[1].y() * step,2) + pow(OriginPoints_vector[3].x() - OriginPoints_vector[1].x(),2));
            }
            else
            {
                AB =  sqrt(pow(OriginPoints_vector[1].y()  - OriginPoints_vector[0].y(),2) + pow(OriginPoints_vector[1].x() * step - OriginPoints_vector[0].x() * step,2));
                CD =  sqrt(pow(OriginPoints_vector[3].y()  - OriginPoints_vector[2].y(),2) + pow(OriginPoints_vector[3].x() * step - OriginPoints_vector[2].x() * step,2));
                BC =  sqrt(pow(OriginPoints_vector[2].y()  - OriginPoints_vector[1].y(),2) + pow(OriginPoints_vector[2].x() * step - OriginPoints_vector[1].x() * step,2));
                AD =  sqrt(pow(OriginPoints_vector[3].y()  - OriginPoints_vector[0].y(),2) + pow(OriginPoints_vector[3].x() * step - OriginPoints_vector[0].x() * step,2));
                AC =  sqrt(pow(OriginPoints_vector[2].y()  - OriginPoints_vector[0].y(),2) + pow(OriginPoints_vector[2].x() * step - OriginPoints_vector[0].x() * step,2));
                BD =  sqrt(pow(OriginPoints_vector[3].y()  - OriginPoints_vector[1].y(),2) + pow(OriginPoints_vector[3].x() * step - OriginPoints_vector[1].x() * step,2));
            }

            if(AB*CD + BC*AD == AC *BD)
                return false;

            return true;
        };

        bool flag;

        // 拉伸 y 值
        if(circle_width>circle_height)
        {
            flag=true;
            do
            {
                step+=0.0001;
            }while(left(OriginPoints_vector, step, flag) && step <= 5);
        }
        // 拉伸 x 值
        else
        {
            flag = false;
            do
            {
                step+=0.0001;
            }while(left(OriginPoints_vector,step,flag) && step <= 5);
        }

        step_vector.push_back(step);

    }

    // 记录无效的数组数
    int flag = 0;

    // 匹配标准？
    std::sort(step_vector.begin(), step_vector.end());

    for(int i = 0 ; i < step_vector.size(); i++)
    {
        if(abs(step_vector[i]-5.00007) < 0.001)
        {
            flag+=1;
        }
    }

    return flag;
}

bool Identification_type::recognize_cylinder(QVector<float> vec)
{
    QVector<QVector2D> head_circle;

    for (int i = 0; i < vec.size(); i+=2) {
        QVector2D temp(vec[i],vec[i+1]);
        head_circle.push_back(temp);

        xcoor_vector.push_back(vec[i]);
        ycoor_vector.push_back(vec[i+1]);
    }

    QVector2D min,max;

    common->findMinMax(head_circle, min, max);

    // 记录长，短轴的距离
    circle_width = (max.x() - min.x()) / 2;
    circle_height = (max.y() - min.y()) / 2;

    int flag = jundge_cylinder(head_circle);

    std::uniform_int_distribution<> dist_int(1, head_circle.size()-1);

    if(flag>30)
        return false;
    else
    {
        float length = sqrt(pow(head_circle[0].x() - head_circle[head_circle.size()-1].x(),2) + pow(head_circle[0].y() - head_circle[head_circle.size()-1].y(),2));
        // 是椭圆，而且相连
        if(length < 0.1)
        {   
            // 随机取20个点
            QVector<int> arb_vec;
            for(int i = 0; i < 50; i++)
            {
                int arb = dist_int(gen);
                arb_vec.push_back(arb);
            }

            std::sort(arb_vec.begin(), arb_vec.end());

            int flag = 0;

            for(int i = 0; i < arb_vec.size() - 1; i++)
            {
                float x = 0, y = 0;

                int num = 0;

                for(auto it : head_circle)
                {
                    if(it.x() == head_circle[arb_vec[i]].x())
                    {
                        num += 1;
                    }
                }

                for(auto it : head_circle)
                {
                    if(it.x() == head_circle[arb_vec[i]].x())
                    {

                        if(x == 0 && y == 0)
                        {
                            x = it.x();
                            y = it.y();
                        }
                        else
                            if(abs(abs(y - (min.y() + max.y())/2) - abs(it.y() - (min.y() + max.y())/2)) < 0.001 && x == it.x())
                            {

                                flag += 1;
                                x = 0;
                                y = 0;
                            }
                    }
                }
            }

            if(flag >= 2)
            {
                // 默认长半轴为半径 (椭圆近似平行于X轴)
                radius = (max.x()-min.x())/2;
            }
            else
            {
                // 椭圆具有倾斜角度
                radius = sqrt(pow(max.y()-min.y(),2) + pow(max.x()-min.x(),2))/2;
            }

            return true;
        }
        else
        {
            return false;
        }
    }
}

bool Identification_type::recognize_straightLine(QVector<float> vec)
{
    // line
    QVector<QVector2D> line_vector;

    xcoor_vector.clear();
    ycoor_vector.clear();

    for (int var = 0; var < vec.size(); var+=2) {
        QVector2D temp(vec[var],vec[var+1]);
        xcoor_vector.push_back(vec[var]);
        ycoor_vector.push_back(vec[var+1]);

        line_vector.push_back(temp);
    }

    int num = 100;

    common->calculate_meanK(line_vector, num);

    // 1. 计算斜率不存在的情况 && 计算方差 (垂直X轴)3. 计算斜率的方差
    if( (num < 80 && common->variance(xcoor_vector) < 0.1) || (common->calculate_k(line_vector) < 0.2))
        return true;
    else
        return false;

}

bool Identification_type::recognize_curveLine(QVector<float> vec)
{
    QVector<QVector2D> head_circle;

    xcoor_vector.clear();
    ycoor_vector.clear();

    for (int i = 0; i < vec.size(); i+=2) {
        QVector2D temp(vec[i],vec[i+1]);
        head_circle.push_back(temp);

        xcoor_vector.push_back(vec[i]);
        ycoor_vector.push_back(vec[i+1]);
    }

    int flag = jundge_cylinder(head_circle);

//    std::cout<<"flag: "<<flag<<std::endl;

//    std::cout<<"common->variance(xcoor_vector): "<<common->variance(xcoor_vector)<<std::endl;

//    std::cout<<"common->variance(ycoor_vector): "<<common->variance(ycoor_vector)<<std::endl;

    int num = 100;

    common->calculate_meanK(head_circle, num);

    // 排除竖直直线
    if((num < 80 && common->variance(xcoor_vector) < 0.1))
        return false;
    // 排除水平直线
    if((common->calculate_k(head_circle) < 0.2))
        return false;
    // 竖直方向的曲线
    else if(common->variance(xcoor_vector) < 0.1 && common->variance(ycoor_vector) > 0.1 && flag < 35)
        return true;
    // 无效的大于 35, 说明不满足椭圆的条件
    else if(flag > 35)
        return false;
    // 水平方向的曲线
    else if(common->variance(ycoor_vector) < 0.1
            && common->variance(xcoor_vector) > 0.1
            && sqrt(pow(head_circle[0].x() - head_circle[head_circle.size()-1].x(),2) + pow(head_circle[0].y() - head_circle[head_circle.size()-1].y(),2)) > 0.2)
        return true;
    else
        return false;

}

bool Identification_type::recognize_wavyLine(QVector<float> vec)
{
    // line
    QVector<QVector2D> line_vector;

    xcoor_vector.clear();
    ycoor_vector.clear();

    for (int var = 0; var < vec.size(); var+=2) {
        QVector2D temp(vec[var],vec[var+1]);
        line_vector.push_back(temp);

        xcoor_vector.push_back(vec[var]);
        ycoor_vector.push_back(vec[var+1]);

    }

    int num = 100;

    common->calculate_meanK(line_vector, num);

    int flag = jundge_cylinder(line_vector);

    // 排除水平曲线
    if(common->variance(ycoor_vector) < 0.1 && common->variance(xcoor_vector) > 0.1 && flag < 35)
        return false;
    // 排除竖直曲线
    else if(common->variance(ycoor_vector) > 0.1 && common->variance(xcoor_vector) < 0.1 && flag < 35)
        return false;
    // 1. 计算斜率不存在的情况(垂直X轴)  2.计算斜率的方差。为区分椭圆 判断首尾的距离
    else if( num > 80 && common->calculate_k(line_vector) > 0.2 && sqrt(pow(line_vector[0].x() - line_vector[line_vector.size()-1].x(),2) + pow(line_vector[0].y() - line_vector[line_vector.size()-1].y(),2)) > 0.2)
        return true;
    else
        return false;
}

bool Identification_type::recognize_corner(QVector<float> vec)
{
    // line
    QVector<QVector2D> line_vector;

    for (int var = 0; var < vec.size(); var+=2) {
        QVector2D temp(vec[var],vec[var+1]);
        line_vector.push_back(temp);
    }

    /* 二. 基于距离变换的方法

    通过定位距离物体边界最远的一组点来确定物体的中心线，通常采用欧式距离，可以理解为一系列最大内切球的球心构成了物体的中心线，有代表性的是基于边界的距离变换和基于源点的距离变换*/

    return true;
}

bool Identification_type::recognize_peanut(QVector<float> vec)
{
    // line
    QVector<QVector2D> line_vector;

    for (int var = 0; var < vec.size(); var+=2) {
        QVector2D temp(vec[var],vec[var+1]);
        line_vector.push_back(temp);
    }

    if(QVector2D(line_vector[line_vector.size() - 1] - line_vector[0]).length() < 0.1)
        return true;
    else
        return false;
}


