#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

std::atomic_bool videoStatus = false;
std::atomic_bool processBarIsSliding = false;
std::atomic_int volume = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    player = new QMediaPlayer;
    ui->startAndPauseBtn->setEnabled(false);


    connect(ui->openBtn, &QPushButton::clicked, this, [=](){
        //选择文件并存储路径
        videoStatus = false;
        selectVideo = QFileDialog::getOpenFileName(this, "选择一个视频", "C:\\", "视频 (*.mp4;*.flv;*.hevc;*.mov)");
        //若路径不为空才执行播放操作

        if(!selectVideo.isEmpty())
        {
            ProcessBarThread *procBarThd = new ProcessBarThread;  //进度条子线程
            if(!procBarThd->isFinished())
            {
                 procBarThd->terminate();
            }
            videoStatus = true;
            //播放器播放
            player->setSource(QUrl::fromLocalFile(selectVideo));
            player->setVideoOutput(ui->videoWiget);
            player->setLoops(true);
            //音频相关
            audioOut = new QAudioOutput(this);
            audioOut->setVolume(volume);
            player->setAudioOutput(audioOut);
            //播放
            player->play();

            //暂停和播放按钮状态
            ui->startAndPauseBtn->setEnabled(true);
            ui->startAndPauseBtn->setText("暂停");
            ui->timeText->setText(get_process_text(format_time(player->duration()), "0:0:0"));
            //进度条设置
            ui->processBar->setMinimum(0);
            ui->processBar->setMaximum(100);
            ui->processBar->setValue(0);
            //音量条
            ui->volumeBar->setMinimum(0);
            ui->volumeBar->setMaximum(100);
            ui->volumeBar->setValue(volume);


            procBarThd->start(); //线程启动
            connect(procBarThd, &ProcessBarThread::current_play_duration, this, [=]()
            {
                //在循环体中
               ui->timeText->setText(get_process_text(format_time(player->duration()), format_time(player->position())));
               if(!processBarIsSliding)
               {
                 ui->processBar->setValue(get_process_percent(player->position(), player->duration()));
               }
            });
            //进度条拖动处理
            connect(ui->processBar, &QSlider::sliderMoved, player, [&]()
            {
                processBarIsSliding = true;
                player->setPosition(int(((double)ui->processBar->value() / 100.00) * (double)player->duration()));
                connect(ui->processBar, &QSlider::sliderReleased, player, [&]()
                {
                    processBarIsSliding = false;
                });
            });
            //音量条拖动处理
            connect(ui->volumeBar, &QSlider::sliderMoved, player, [&]()
            {
                volume = ui->volumeBar->value();
                audioOut->setVolume(volume);
                ui->volumePercentText->setText(QString("%1%").arg(volume));
            });
        }
        else //如果地址为空，则提示
        {
            ui->startAndPauseBtn->setEnabled(false);
            videoStatus = false;

            msg = new QMessageBox;
            msg->setText("未选择任何文件");
            msg->setWindowTitle("提示");
            msg->addButton(QMessageBox::StandardButton::Ok);
            msg->show();
        }
    });

    //开始&暂停按钮事件
    connect(ui->startAndPauseBtn, &QPushButton::clicked, player, [=]()
    {
       static int clickCount = 0;
       if(clickCount % 2 == 0)
       {
           player->pause();
           ui->startAndPauseBtn->setText("播放");
           clickCount++;
       }
       else
       {
           player->play();
           ui->startAndPauseBtn->setText("暂停");
           clickCount++;
       }
    });
}

QString MainWindow::format_time(int ms)
{
    int ss = 1000;
    int mi = ss * 60;
    int hh = mi * 60;
    int dd = hh * 24;

    long day = ms / dd;
    long hour = (ms - day * dd) / hh;
    long minute = (ms - day * dd - hour * hh) / mi;
    long second = (ms - day * dd - hour * hh - minute * mi) / ss;
    long milliSecond = ms - day * dd - hour * hh - minute * mi - second * ss;

    QString h = QString::number(hour,10);
    QString m = QString::number(minute,10);
    QString s = QString::number(second,10);
    QString msec = QString::number(milliSecond,10);

    //qDebug() << "minute:" << min << "second" << sec << "ms" << msec <<endl;

    return h + ":" + m + ":" + s ;
}

QString MainWindow::get_process_text(QString inTotalDuration, QString inCurrent)
{
    return QString("%1 / %2").arg(inCurrent).arg(inTotalDuration);
}

int MainWindow::get_process_percent(qint64 inValue, qint64 inTotal)
{
    int result = 0;
    result = int((double(inValue) / (double)inTotal) * 100.00);
    return result;
}

void ProcessBarThread::run()
{
    while(videoStatus)
    {
        emit current_play_duration();
        QThread::usleep(10);
    }
}

MainWindow::~MainWindow()
{
    delete msg;
    delete player;
    delete ui;
}

