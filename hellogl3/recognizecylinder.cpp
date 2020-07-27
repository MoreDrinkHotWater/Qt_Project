#include "recognizecylinder.h"

#include <iostream>
#include <QVector2D>
#include <QVector>

#include <math.h>

#include <random>
#include<algorithm>

Recognizecylinder::Recognizecylinder()
{

}

// 计算方差
float Recognizecylinder::variance (QVector<float> vector){

    double sum = std::accumulate(std::begin(vector) + 1, std::end(vector) - 1, 0.0);
    double mean =  sum / vector.size(); //均值

    double accum  = 0.0;
    std::for_each (std::begin(vector), std::end(vector), [&](const double d) {
        accum  += (d - mean)*(d - mean);
    });

    float stdev = sqrt(accum/(vector.size()-1)); //方差

    return stdev;
};

bool Recognizecylinder::recognize_cylinder_shape(QStack<QVector<float>> draw_coorstack)
{
    for(int var = 0; var < draw_coorstack.size(); var++)
    {
        // 椭圆
        if(recognize_cylinder(draw_coorstack[var]))
        {
            std::cout<<"第"<<var<<"条线段是: 椭圆"<<std::endl;
            continue;
        }
        // 直线
        else if(recognize_straightLine(draw_coorstack[var]))
        {
            std::cout<<"第"<<var<<"条线段是: 直线"<<std::endl;
            continue;
        }
        // 曲线
        else if(recognize_curveLine(draw_coorstack[var]))
        {
            std::cout<<"第"<<var<<"条线段是: 曲线"<<std::endl;
            continue;
        }

    }

    return true;
}

bool Recognizecylinder::recognize_cylinder(QVector<float> vec)
{
    QVector<QVector2D> head_circle;

    for (int i = 0; i < vec.size(); i+=2) {
        QVector2D temp(vec[i],vec[i+1]);
        head_circle.push_back(temp);
    }

    std::cout<<"head_circle size: "<<head_circle.size()<<std::endl;

    float maxX = head_circle[0].x(), minX = head_circle[0].x(), maxY = head_circle[0].y(), minY = head_circle[0].y();

    for(auto it = head_circle.begin(); it != head_circle.end(); it++)
    {
        if(it ->x() > maxX)
            maxX = it->x();

        if(it ->x() < minX)
            minX = it->x();

        if(it ->y() > maxY)
            maxY = it->y();

        if(it ->y() < minY)
            minY = it->y();
    }

    std::cout<<"maxX: "<<maxX<<" minX:"<<minX<<" maxY: "<<maxY<<" minY: "<<minY<<std::endl;

    // 记录长，短轴的距离
    float circle_width = (maxX - minX) / 2;
    float circle_height = (maxY - minY) / 2;

    std::cout<<"circle_width: "<<circle_width<<" circle_height:"<<circle_height<<std::endl;

    // 记录椭圆的左右结点
    cylinder_left = QVector2D(minX, (maxY + minY) / 2);
    cylinder_right = QVector2D(maxX, (maxY + minY) / 2);

    // 1.判断椭圆！（若为椭圆，找出长、短半轴的长度。为了方便我们默认长半轴为半径）
    QVector<float> step_vector;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dist_int(1, head_circle.size()-1);

    int num = 50;
    // 取50组测试数据
    for (int i = 0; i < num; i++) {

        QVector<QVector2D> OriginPoints_vector;
        QVector<int> rand_vector;

        // 随机取四个点数据
        for (int i = 0; i < 4; i++) {
            int rand = dist_int(gen);
            rand_vector.push_back(rand);
        }

        // 对随机数据排序
        std::sort(rand_vector.begin(), rand_vector.end());

        for (int j = 0; j < 4; j++) {
            QVector2D temp;
            temp.setX(head_circle[rand_vector[j]].x());
            temp.setY(head_circle[rand_vector[j]].y());
            OriginPoints_vector.push_back(temp);
        }

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
                //            if(abs((AB*CD + BC*AD) - AC *BD) < 0.001)
            {
                //                                std::cout<<"find the step"<<std::endl;
                return false;
            }

            return true;
        };

        bool flag;

        // 拉伸 y 值
        if(circle_width>circle_height)
        {
            flag=true;
            do
            {
                if(step == 5.0)
                {
                    //                                        std::cout<<"can't find the step "<<std::endl;
                }

                step+=0.0001;

            }while(left(OriginPoints_vector, step, flag) && step <= 5);
        }
        // 拉伸 x 值
        else
        {
            flag = false;
            do
            {
                if(step == 5.0)
                {
                    //                                        std::cout<<"can't find the step "<<std::endl;
                }

                step+=0.0001;

            }while(left(OriginPoints_vector,step,flag) && step <= 5);
        }

        step_vector.push_back(step);

        //        std::cout<<"the final step = "<<step<<std::endl;

    }

    // 匹配标准？
    std::sort(step_vector.begin(), step_vector.end());

    for(int i = 0 ; i < step_vector.size(); i++)
    {
        std::cout<<"step_vector: "<<step_vector[i]<<std::endl;
    }

    std::cout<<"step_vector的方差: "<< variance(step_vector) <<std::endl;

    // 默认长半轴为半径
    radius = (maxX-minX)/2;

    std::cout<<"radius: "<<radius<<std::endl;

    return true;
}

bool Recognizecylinder::recognize_straightLine(QVector<float> vec)
{
    // 2.判断线段是否垂直上面的椭圆！（若垂直，求出高度 height）
    // 注：由于构建数据时，为了在三维中表现出二维。 我们记录的 right_max 和 r_line 的二维坐标相同。
    // 下面的方法不适用测试数据！

    QVector<QVector2D> l_line_vector;

    std::cout<<"vec.size(): "<<vec.size()<<std::endl;

    QVector<float> xcoor_vector;
    QVector2D l_line;

    for (int var = 0; var < vec.size(); var+=2) {
        QVector2D temp(vec[var],vec[var+1]);
        xcoor_vector.push_back(vec[var]);
        l_line_vector.push_back(temp);

        if(var == vec.size() - 2)
        {
            l_line = temp;
        }
    }

    std::cout<<"l_line_vector size: "<<l_line_vector.size()<<std::endl;

    // 计算 x 坐标的方差。看是不是直线
    std::cout<<"xcoor_vector的方差: "<< variance(xcoor_vector) <<std::endl;

    if(variance(xcoor_vector) < 0.03)
    {
        std::cout<<"l_line is straight line! "<<std::endl;
    }

    //    std::cout<<"right_max_x: "<<right_max.x()<<" left_min_x:"<<left_min.x()<<" maxY: "<<maxY<<" minY: "<<minY<<std::endl;

    // 向量
    float min_max_x = cylinder_right.x() - cylinder_left.x();
    float min_max_y = cylinder_right.y() - cylinder_left.y();

    float min_lline_x = l_line.x() - cylinder_left.x();
    float min_lline_y = l_line.y() - cylinder_left.y();

    float point_multi = min_max_x * min_lline_x + min_max_y * min_lline_y;

    std::cout<<"point_multi: "<<point_multi<<std::endl;

    // 以 75度 为标准
    if(point_multi < 0.25882)
    {
        std::cout<<"the l_line is right!"<<std::endl;
    }

//    height = min_lline_y;

//    std::cout<<"height: "<<height<<std::endl;
    return true;
}

bool Recognizecylinder::recognize_curveLine(QVector<float> vec)
{
    return true;
}

