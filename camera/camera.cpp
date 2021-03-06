﻿/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "camera.h"
#include "ui_camera.h"
#include "videosettings.h"
#include "imagesettings.h"
#include "choosedistancetime.h"
#include "setdiff.h"
#include "autoplayspeed.h"

#include <iostream>
#include <QMediaService>
#include <QMediaRecorder>
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QMediaMetaData>

#include <QMessageBox>
#include <QPalette>

#include <QAbstractVideoSurface>

#include <QtWidgets>
#include <qmath.h>
#include <limits.h>
Q_DECLARE_METATYPE(QCameraInfo)

Camera::Camera() : ui(new Ui::Camera)
{
    ui->setupUi(this);

    image_num = 0;
    distancetime = 0;
    diff = 0;

    ui->ImagePreviewLabel->setStyleSheet("border: 2px solid red");
    ui->ImagePreviewLabel->setScaledContents(true);

    ui->videosurface->setStyleSheet("border: 2px solid red");
    ui->videosurface->setScaledContents(true);

    //Camera devices:

    QActionGroup *videoDevicesGroup = new QActionGroup(this);
    videoDevicesGroup->setExclusive(true);
    const QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : availableCameras) {
        QAction *videoDeviceAction = new QAction(cameraInfo.description(), videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(cameraInfo));
        if (cameraInfo == QCameraInfo::defaultCamera())
            videoDeviceAction->setChecked(true);

        ui->menuDevices->addAction(videoDeviceAction);
    }

    connect(videoDevicesGroup, &QActionGroup::triggered, this, &Camera::updateCameraDevice);
    connect(ui->captureWidget, &QTabWidget::currentChanged, this, &Camera::updateCaptureMode);

    timer = new QTimer();
    connect(timer,&QTimer::timeout,this,&Camera::takeImage);

    timer_speed = new QTimer();
    connect(timer_speed,&QTimer::timeout,this,&Camera::on_rearButton_clicked);


    setCamera(QCameraInfo::defaultCamera());
}

void Camera::setCamera(const QCameraInfo &cameraInfo)
{
    m_camera.reset(new QCamera(cameraInfo));

    connect(m_camera.data(), &QCamera::stateChanged, this, &Camera::updateCameraState);
    connect(m_camera.data(), QOverload<QCamera::Error>::of(&QCamera::error), this, &Camera::displayCameraError);

    m_mediaRecorder.reset(new QMediaRecorder(m_camera.data()));
    connect(m_mediaRecorder.data(), &QMediaRecorder::stateChanged, this, &Camera::updateRecorderState);

    m_imageCapture.reset(new QCameraImageCapture(m_camera.data()));

    connect(m_mediaRecorder.data(), &QMediaRecorder::durationChanged, this, &Camera::updateRecordTime);
    connect(m_mediaRecorder.data(), QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error),
            this, &Camera::displayRecorderError);

    m_mediaRecorder->setMetaData(QMediaMetaData::Title, QVariant(QLatin1String("Test Title")));

    m_camera->setViewfinder(ui->viewfinder);

    updateCameraState(m_camera->state());
    updateLockStatus(m_camera->lockStatus(), QCamera::UserRequest);
    updateRecorderState(m_mediaRecorder->state());

    // signals：
    // 1. readyForCaptureChanged： 相机准备捕获
    // 2. imageCaptured: 将用户截图在 label 上显示给用户。
    connect(m_imageCapture.data(), &QCameraImageCapture::readyForCaptureChanged, this, &Camera::readyForCapture);
    connect(m_imageCapture.data(), &QCameraImageCapture::imageCaptured, this, &Camera::processCapturedImage);
    connect(m_imageCapture.data(), &QCameraImageCapture::imageSaved, this, &Camera::imageSaved);
    connect(m_imageCapture.data(), QOverload<int, QCameraImageCapture::Error, const QString &>::of(&QCameraImageCapture::error),
            this, &Camera::displayCaptureError);

    connect(m_camera.data(), QOverload<QCamera::LockStatus, QCamera::LockChangeReason>::of(&QCamera::lockStatusChanged),
            this, &Camera::updateLockStatus);

    ui->captureWidget->setTabEnabled(0, (m_camera->isCaptureModeSupported(QCamera::CaptureStillImage)));
    ui->captureWidget->setTabEnabled(1, (m_camera->isCaptureModeSupported(QCamera::CaptureVideo)));

    updateCaptureMode();

    // add

    m_camera->start();
}

void Camera::keyPressEvent(QKeyEvent * event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        displayViewfinder();
        m_camera->searchAndLock();
        event->accept();
        break;
    case Qt::Key_Camera:
        if (m_camera->captureMode() == QCamera::CaptureStillImage) {
            takeImage();
        } else {
            if (m_mediaRecorder->state() == QMediaRecorder::RecordingState)
                stop();
            else
                record();
        }
        event->accept();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void Camera::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;
    switch (event->key()) {
    case Qt::Key_CameraFocus:
        m_camera->unlock();
        break;
    default:
        QMainWindow::keyReleaseEvent(event);
    }
}

void Camera::updateRecordTime()
{
    QString str = QString("Recorded %1 sec").arg(m_mediaRecorder->duration()/1000);
    ui->statusbar->showMessage(str);
}

void Camera::processCapturedImage(int requestId, const QImage& img)
{
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(ui->viewfinder->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);

    // 在 label 上显示图片
    ui->lastImagePreviewLabel->setPixmap(QPixmap::fromImage(scaledImage));

    // Display captured image for 1 seconds.
    displayCapturedImage();
    QTimer::singleShot(1000, this, &Camera::displayViewfinder);
}

void Camera::configureCaptureSettings()
{
    switch (m_camera->captureMode()) {
    case QCamera::CaptureStillImage:
        configureImageSettings();
        break;
    case QCamera::CaptureVideo:
        configureVideoSettings();
        break;
    default:
        break;
    }
}

void Camera::configureVideoSettings()
{
    VideoSettings settingsDialog(m_mediaRecorder.data());
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    settingsDialog.setAudioSettings(m_audioSettings);
    settingsDialog.setVideoSettings(m_videoSettings);
    settingsDialog.setFormat(m_videoContainerFormat);

    if (settingsDialog.exec()) {
        m_audioSettings = settingsDialog.audioSettings();
        m_videoSettings = settingsDialog.videoSettings();
        m_videoContainerFormat = settingsDialog.format();

        m_mediaRecorder->setEncodingSettings(
                    m_audioSettings,
                    m_videoSettings,
                    m_videoContainerFormat);

        m_camera->unload();
        m_camera->start();
    }
}

void Camera::configureImageSettings()
{
    ImageSettings settingsDialog(m_imageCapture.data());
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    settingsDialog.setImageSettings(m_imageSettings);

    if (settingsDialog.exec()) {
        m_imageSettings = settingsDialog.imageSettings();
        m_imageCapture->setEncodingSettings(m_imageSettings);
    }
}

void Camera::record()
{
    m_mediaRecorder->record();
    updateRecordTime();
}

void Camera::pause()
{
    m_mediaRecorder->pause();
}

void Camera::stop()
{
    m_mediaRecorder->stop();
}

void Camera::setMuted(bool muted)
{
    m_mediaRecorder->setMuted(muted);
}

void Camera::toggleLock()
{
    switch (m_camera->lockStatus()) {
    case QCamera::Searching:
    case QCamera::Locked:
        m_camera->unlock();
        break;
    case QCamera::Unlocked:
        m_camera->searchAndLock();
    }
}

void Camera::updateLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    QColor indicationColor = Qt::black;

    switch (status) {
    case QCamera::Searching:
        indicationColor = Qt::yellow;
        ui->statusbar->showMessage(tr("Focusing..."));
        ui->lockButton->setText(tr("Focusing..."));
        break;
    case QCamera::Locked:
        indicationColor = Qt::darkGreen;
        ui->lockButton->setText(tr("Unlock"));
        ui->statusbar->showMessage(tr("Focused"), 2000);
        break;
    case QCamera::Unlocked:
        indicationColor = reason == QCamera::LockFailed ? Qt::red : Qt::black;
        ui->lockButton->setText(tr("Focus"));
        if (reason == QCamera::LockFailed)
            ui->statusbar->showMessage(tr("Focus Failed"), 2000);
    }

    QPalette palette = ui->lockButton->palette();
    palette.setColor(QPalette::ButtonText, indicationColor);
    ui->lockButton->setPalette(palette);
}

void Camera::takeImage()
{
    if(filePath.isEmpty())
    {
        QMessageBox::information(NULL, "Warring", "请您选择保存路径，再截图！");
        return;
    }

    m_isCapturingImage = true;
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("/yyyy_MM_dd_hh.mm.ss.zzz");

    QString fileName = filePath + current_date + ".jpg";

    std::cout<<"filePath: "<<filePath.toStdString()<<std::endl;

    std::cout<<"fileName: "<<fileName.toStdString()<<std::endl;

    first_date_time = current_date_time;

    base_image_str = fileName;

    m_imageCapture->capture(fileName);
}

void Camera::displayCaptureError(int id, const QCameraImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    QMessageBox::warning(this, tr("Image Capture Error"), errorString);
    m_isCapturingImage = false;
}

void Camera::startCamera()
{
    m_camera->start();
}

void Camera::stopCamera()
{
    m_camera->stop();

    ui->radioButton->setCheckable(false);

    ui->radioButton->setCheckable(true);

    ui->radioButton_2->setCheckable(false);

    ui->radioButton_2->setCheckable(true);

    if(timer->isActive())
        timer->stop();

//    m_camera->setViewfinder(ui->viewfinder);
}

void Camera::updateCaptureMode()
{
    int tabIndex = ui->captureWidget->currentIndex();
    QCamera::CaptureModes captureMode = tabIndex == 0 ? QCamera::CaptureStillImage : QCamera::CaptureVideo;

    if (m_camera->isCaptureModeSupported(captureMode))
        m_camera->setCaptureMode(captureMode);
}

void Camera::updateCameraState(QCamera::State state)
{
    switch (state) {
    case QCamera::ActiveState:
        ui->actionStartCamera->setEnabled(false);
        ui->actionStopCamera->setEnabled(true);
        ui->captureWidget->setEnabled(true);
        ui->actionSettings->setEnabled(true);
        break;
    case QCamera::UnloadedState:
    case QCamera::LoadedState:
        ui->actionStartCamera->setEnabled(true);
        ui->actionStopCamera->setEnabled(false);
        ui->captureWidget->setEnabled(false);
        ui->actionSettings->setEnabled(false);
    }
}

void Camera::updateRecorderState(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::StoppedState:
        ui->recordButton->setEnabled(true);
        ui->pauseButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setEnabled(true);
        ui->pauseButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        break;
    case QMediaRecorder::RecordingState:
        ui->recordButton->setEnabled(false);
        ui->pauseButton->setEnabled(true);
        ui->stopButton->setEnabled(true);
        break;
    }
}

void Camera::setExposureCompensation(int index)
{
    m_camera->exposure()->setExposureCompensation(index*0.5);
}

void Camera::displayRecorderError()
{
    QMessageBox::warning(this, tr("Capture Error"), m_mediaRecorder->errorString());
}

void Camera::displayCameraError()
{
    QMessageBox::warning(this, tr("Camera Error"), m_camera->errorString());
}

void Camera::updateCameraDevice(QAction *action)
{
    setCamera(qvariant_cast<QCameraInfo>(action->data()));
}

void Camera::displayViewfinder()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Camera::displayCapturedImage()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Camera::readyForCapture(bool ready)
{
    ui->takeImageButton->setEnabled(ready);
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    ui->statusbar->showMessage(tr("Captured \"%1\"").arg(QDir::toNativeSeparators(fileName)));

    m_isCapturingImage = false;
    if (m_applicationExiting)
        close();
}

void Camera::closeEvent(QCloseEvent *event)
{
    if (m_isCapturingImage) {
        setEnabled(false);
        m_applicationExiting = true;
        event->ignore();
    } else {
        event->accept();
    }
}

void Camera::on_savePathButton_clicked()
{
    filePath = QFileDialog::getExistingDirectory(
                this,
                ("选择文件保存路径"),
                "C:\\Users\\MoreDrinkHotWater\\Pictures",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

void Camera::Camear_handleFrame(QImage image)
{
    current_image = image.copy();

    // 读取基准图片
    QPixmap base_image;

    if(!base_image.load(base_image_str))
    {
        std::cout<<"==========load failed==========="<<std::endl;
    }

    QMatrix leftmatrix;

    // 顺时针旋转 180 度
    current_image = current_image.transformed(leftmatrix.rotate(180),Qt::SmoothTransformation);

    // 水平翻转
    current_image =  current_image.mirrored(true, false);

    // 对比基准图片和实时图片的灰度差异， 如果较大的话， 则保存图片
    float current_diff = searchMinDiff(last_image, current_image,0.2f,2);
    std::cout<<"current_diff: "<< current_diff<<std::endl;

    if(current_diff > diff)
    {
        // 保存图片
        QDateTime current_date_time =QDateTime::currentDateTime();
        QString current_date =current_date_time.toString("/yyyy_MM_dd_hh.mm.ss.zzz");

        // 计算时间差
        QString time = QDateTime::fromMSecsSinceEpoch(QDateTime::currentDateTime().toMSecsSinceEpoch() - first_date_time.toMSecsSinceEpoch()).toUTC().toString("_hh.mm.ss");

        QString fileName = filePath + current_date + time + ".jpg";

        current_image.save(fileName);
    }

    last_image = current_image;

    ui->ImagePreviewLabel->setPixmap(QPixmap::fromImage(current_image));
}

// 监控
void Camera::on_radioButton_toggled(bool checked)
{
    if(filePath.isEmpty())
    {
        QMessageBox::information(NULL, "Warring", "请您选择保存路径，再进行操作！");

        if(checked)
        {
            // 重置 radioButton
            ui->radioButton->setCheckable(false);

            ui->radioButton->setCheckable(true);
        }

        return;
    }

    // 显示子窗口
    chooseDistanceTime *distanceTimeWidget = new chooseDistanceTime;
    connect(distanceTimeWidget, &chooseDistanceTime::send_distanceTime, this, &Camera::receive_distanceTime);

    distanceTimeWidget->show();
}

void Camera::receive_distanceTime(int distanceTime)
{
    this->distancetime = distanceTime;

    std::cout<<"distanceTime: "<<distanceTime<<std::endl;

    if(distancetime != 0)
    {
        takeImage();

        timer->start(distancetime);
    }
    else
        std::cout<<"the distanceTime = 0!"<<std::endl;
}

// 检异
void Camera::on_radioButton_2_toggled(bool checked)
{
    if(filePath.isEmpty())
    {
        QMessageBox::information(NULL, "Warring", "请您选择保存路径，再进行操作！");

        if(checked)
        {
            ui->radioButton_2->setCheckable(false);

            ui->radioButton_2->setCheckable(true);
        }

        return;
    }

    if(timer->isActive())
        timer->stop();

    // 显示子窗口
    setDiff *setdiffWidget = new setDiff;
    connect(setdiffWidget, &setDiff::send_Diff, this, &Camera::receive_diff);
    setdiffWidget->show();
}

void Camera::receive_diff(float diff)
{
    this->diff = diff;

    std::cout<<"diff: "<<diff<<std::endl;

    CameraFrameGrabber *_cameraFrameGrabber = new CameraFrameGrabber(this);
    // 设置取景器
    m_camera->setViewfinder(_cameraFrameGrabber);
    connect(_cameraFrameGrabber, SIGNAL(frameAvailable(QImage)), this, SLOT(Camear_handleFrame(QImage)));

    m_camera->start();
}

CameraFrameGrabber::CameraFrameGrabber(QObject *parent) :
    QAbstractVideoSurface(parent)
{

}

QList<QVideoFrame::PixelFormat> CameraFrameGrabber::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle)
    {
        return QList<QVideoFrame::PixelFormat>()<< QVideoFrame::Format_RGB32<< QVideoFrame::Format_ARGB32<< QVideoFrame::Format_ARGB32_Premultiplied<< QVideoFrame::Format_RGB565<< QVideoFrame::Format_RGB555;
           /*
        return QList<QVideoFrame::PixelFormat>()
                  <<QVideoFrame::Format_RGB32
                  <<QVideoFrame::Format_YUV420P
                  <<QVideoFrame::Format_YUYV
                  <<QVideoFrame::Format_ARGB32
                  <<QVideoFrame::Format_ARGB32_Premultiplied
                  <<QVideoFrame::Format_RGB565
                  <<QVideoFrame::Format_Jpeg
                  <<QVideoFrame::Format_RGB555;*/
       }
       else
       {
          qDebug()<<"无效格式2."<<QList<QVideoFrame::PixelFormat>();
          return  QList<QVideoFrame::PixelFormat>();
       }
}

// 获得实时的帧
bool CameraFrameGrabber::present(const QVideoFrame &frame)
{
    if (frame.isValid()) {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);

        const QImage image(cloneFrame.bits(),
                           cloneFrame.width(),
                           cloneFrame.height(),
                           QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat()));

        emit frameAvailable(image);


        cloneFrame.unmap();
        return true;
    }
    return false;
}

// 读取
void Camera::on_readButton_clicked()
{

    files = QFileDialog::getOpenFileNames(
                this,
                tr("Select one or more files to open"),
                "C:\\Users\\MoreDrinkHotWater\\Pictures",
                tr("Image files(*.bmp *.jpg *.pbm *.pgm *.png *.ppm *.xbm *.xpm);;All files (*.*)"));

    if(files.isEmpty())
    {
        QMessageBox mesg;
        mesg.warning(this,"警告","打开图片失败!");
        return;
    }

    std::cout<<"files size: "<<files.size()<<std::endl;

    ui->num1->setNum(1);

    ui->num3->setNum(files.size());

    ui->horizontalSlider->setMinimum(0);  // 最小值
    ui->horizontalSlider->setMaximum(files.size() - 1);  // 最大值
    ui->horizontalSlider->setSingleStep(1);  // 步长

    std::cout<<"files[0]: "<<files[image_num].toStdString()<<std::endl;

    // 显示图片信息
    statusBar()->setStyleSheet("color:green");
    statusBar()->showMessage(files[image_num]);

    QPixmap pix = files[image_num];

    if(pix.isNull())
    {
        std::cout<<"=============the pix is null================"<<std::endl;
    }

    ui->ImagePreviewLabel->setPixmap(pix);

}

// 滑动条
void Camera::on_horizontalSlider_sliderMoved(int position)
{
    ui->num1->setNum(position + 1);

    QPixmap pix = files[position];

    // 显示图片信息
    statusBar()->setStyleSheet("color:green");
    statusBar()->showMessage(files[position]);

    ui->ImagePreviewLabel->setPixmap(pix);
}

// 读取上一张
void Camera::on_frontButton_clicked()
{
    if(image_num > 0)
    {
        image_num -= 1;

        if(image_num == 0)
            ui->num1->setNum(1);
        else
            ui->num1->setNum(image_num + 1);

        QPixmap pix = files[image_num];

        // 显示图片信息
        statusBar()->setStyleSheet("color:green");
        statusBar()->showMessage(files[image_num]);

        ui->ImagePreviewLabel->setPixmap(pix);

        // 设置滑块
        ui->horizontalSlider->setSliderPosition(image_num);

        ui->horizontalSlider->setValue(image_num);
    }
}

// 读取下一张
void Camera::on_rearButton_clicked()
{
//    if(speed == 0)
//        speed = 1;

    if(image_num < files.size() - 1)
    {
        image_num += 1;

        ui->num1->setNum(image_num + 1);

        QPixmap pix = files[image_num];

        // 显示图片信息
        statusBar()->setStyleSheet("color:green");
        statusBar()->showMessage(files[image_num]);

        ui->ImagePreviewLabel->setPixmap(pix);

        // 设置滑块的位置
        ui->horizontalSlider->setSliderPosition(image_num);

        ui->horizontalSlider->setValue(image_num);
    }
    else
    {
        image_num = files.size() - 1;

        ui->num1->setNum(image_num + 1);

        QPixmap pix = files[image_num];

        // 显示图片信息
        statusBar()->setStyleSheet("color:green");
        statusBar()->showMessage(files[image_num]);

        ui->ImagePreviewLabel->setPixmap(pix);

        // 设置滑块的位置
        ui->horizontalSlider->setSliderPosition(image_num);

        ui->horizontalSlider->setValue(image_num);

        timer_speed->stop();

        // 重置
        image_num = 0;
    }
}

// 自动播放
void Camera::on_autoplayButton_clicked()
{ 
    if(image_num == 0)
    {
        autoplaySpeed *autoplaySpeedWidget = new autoplaySpeed;
        connect(autoplaySpeedWidget, &autoplaySpeed::seed_speed, this, &Camera::receive_speed);
        autoplaySpeedWidget->show();
    }
    else
        receive_speed(speed);
}


void Camera::receive_speed(int speed)
{
    this->speed = speed;

    timer_speed->start(speed * 50);
}

// 暂停播放
void Camera::on_stopPlayButton_clicked()
{
    timer_speed->stop();
}

float Camera::searchMinDiff(const QImage &img1,const QImage &img2,float scale,int searchStep)
{
    if(img1.width() != img2.width() || img1.height() != img2.height())
    {
        qDebug() << "resolution diff:" << img1.width() << img2.width() << img1.height() << img2.height();
        return 0;
    }
    QImage im1 = (scale == 1.0f)?img1:img1.scaled(img1.width()*scale,img1.height()*scale);
    QImage im2 = (scale == 1.0f)?img2:img2.scaled(img2.width()*scale,img2.height()*scale);
    int w1 = im1.width(),w2 = im2.width();
    int h1 = im1.height(),h2 = im2.height();


    if(w1 == 0 || h1 == 0){
        qDebug() << "empty Image";
        return 0;
    }

    QPoint offset(0,0);
    float minDiff = 1000000;
    QPoint minOffset(0,0);
    while(1){
        float localMinDiff = 100000;

        for(int m = -1; m <= 1; m++){
            for(int n = -1; n <= 1; n++){
//                if(m == 0 && n == 0)
//                    continue;
                float diff = 0;
                QPoint offfset_ = offset + searchStep*QPoint(m,n);
                for(int y = 0; y < h1; y++){
                    for(int x = 0; x < w1; x++){
                        int x1 = x + offfset_.x();
                        int y1 = y + offfset_.y();
                        if(x1 >= 0 && x1 < w2 && y1 >= 0 && y1 < h2){
                            QRgb rgb = im1.pixel(x,y);
                            int gray = qGray(rgb);
                            QRgb rgb1 = im2.pixel(x1,y1);
                            int gray1 = qGray(rgb1);
                            diff += qAbs(gray - gray1)/255.0;
                        }
                    }
                }
                diff /= (w1*h1);
                if(localMinDiff > diff){
                    localMinDiff = diff;
                    minOffset = offfset_;
                }
            }
        }
        if(localMinDiff < minDiff){
            offset = minOffset;
            minDiff = localMinDiff;
        }else{
            break;
        }
    }
    qDebug() << "min offset:" << offset;
    return minDiff;

}

float Camera::compareImage(QImage &img1,QImage &img2,float scale = 1.0f,QPoint offset = QPoint(0,0)){
    QImage im1 = (scale == 1.0f)?img1:img1.scaled(img1.width()*scale,img1.height()*scale);
    QImage im2 = (scale == 1.0f)?img2:img2.scaled(img2.width()*scale,img2.height()*scale);
    int w1 = im1.width(),w2 = im2.width();
    int h1 = im1.height(),h2 = im2.height();
    if(w1 == 0 || h1 == 0){
        qDebug() << "empty Image";
        return 0;
    }
    float diff = 0;
    for(int y = 0; y < h1; y++){
        for(int x = 0; x < w1; x++){
            int x1 = x + offset.x();
            int y1 = y + offset.y();
            if(x1 >= 0 && x1 < w2 && y1 >= 0 && y1 < h2){
                QRgb rgb = im1.pixel(x,y);
                int gray = qGray(rgb);
                QRgb rgb1 = im2.pixel(x1,y1);
                int gray1 = qGray(rgb1);
                diff += qAbs(gray - gray1)/255.0;
            }
        }
    }
    return diff/(w1*h1);
}
