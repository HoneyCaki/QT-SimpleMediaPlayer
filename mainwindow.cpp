#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

std::atomic_bool videoStatus = false;
std::atomic_bool processBarIsSliding = false;
std::atomic_int volume = 50;
int startAndStopClickCount = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("播放器");
    player = new QMediaPlayer;
    //控件初始化
    ui->startAndPauseBtn->setEnabled(false);
    //点击焦点设置
    ui->fullScrBtn->setFocusPolicy(Qt::NoFocus);
    ui->startAndPauseBtn->setFocusPolicy(Qt::NoFocus);
    ui->openBtn->setFocusPolicy(Qt::NoFocus);
    //音频播放设置
    audioOut = new QAudioOutput(this);
    audioOut->setVolume(volume);
    ui->volumeBar->setValue(volume);
    ui->volumePercentText->setText(QString("%1%").arg(volume));
    //音量条拖动处理
    connect(ui->volumeBar, &QSlider::sliderMoved, player, [&]()
    {
        //BUG↓
        if(ui->volumeBar->value() == 1)
            ui->volumeBar->setValue(0);

        volume = ui->volumeBar->value();
        audioOut->setVolume((float)volume / 100.0f);
        ui->volumePercentText->setText(QString("%1%").arg(volume));
    });
    //打开按钮
    connect(ui->openBtn, &QPushButton::clicked, this, [=](){
        //选择文件并存储路径
        videoStatus = false;
        selectVideo = QFileDialog::getOpenFileName(this, "选择一个视频", "C:\\", "视频 (*.mp4;*.flv;*.hevc;*.mov;*.avi)");
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
            //键盘监听事件
            ui->videoWidget->installEventFilter(this); //为videoWidget安装事件过滤器
            this->installEventFilter(this);
            //全屏按钮
            connect(ui->fullScrBtn, &QPushButton::clicked, [=]()
            {
                ui->videoWidget->setFullScreen(true);
            });
            //初始化播放器
            init_player();
            //设置播放器
            player->setSource(QUrl::fromLocalFile(selectVideo));
            player->setVideoOutput(ui->videoWidget);
            player->setLoops(true);
            player->setAudioOutput(audioOut);
            //开始播放
            player->play();

            //此处复杂了，可不用多线程
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
                setPlayerStatus(false);
                player->setPosition(int(((double)ui->processBar->value() / 100.00) * (double)player->duration()));
                connect(ui->processBar, &QSlider::sliderReleased, player, [&]()
                {
                    processBarIsSliding = false;
                    setPlayerStatus(true);
                });
            });
            //视频播放速度选择框处理
            connect(ui->speedComboBox, &QComboBox::currentIndexChanged, [=](int index)
            {
                if(index == 0)
                    player->setPlaybackRate(1);
                else if(index == 1)
                    player->setPlaybackRate(1.25);
                else if(index == 2)
                    player->setPlaybackRate(1.5);
                else if(index == 3)
                    player->setPlaybackRate(2);
                else if(index == 4)
                    player->setPlaybackRate(3);
                else
                    player->setPlaybackRate(4);
            });
        }
        else //如果地址为空，则提示
        {
            if(!player->hasVideo())
            {
                ui->startAndPauseBtn->setEnabled(false);
            }

            videoStatus = false;

            msg = new QMessageBox;
            msg->information(this, "提示", "未选择任何文件", QMessageBox::StandardButton::Ok);
            delete msg;
        }
    });

    //开始&暂停按钮事件
    connect(ui->startAndPauseBtn, &QPushButton::clicked, player, [=]()
    {
       if(startAndStopClickCount % 2 == 0)
       {
           setPlayerStatus(true);
       }
       else
       {
           setPlayerStatus(false);
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

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if(e->type() == QEvent::KeyRelease)
    {
        //全屏状态
        if(obj == ui->videoWidget)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            //禁止重复按键
            if(!keyEvent->isAutoRepeat())
            {
                //全屏退出
                if(keyEvent->key() == Qt::Key_Escape || keyEvent->key() == Qt::Key_F12)
                {
                    ui->videoWidget->setFullScreen(false);
                }
                //暂停与播放
                if(keyEvent->key() == Qt::Key_Space)
                {
                   startAndStopClickCount % 2 == 0 ? setPlayerStatus(true) : setPlayerStatus(false);
                }
            }
        }
        //窗口状态
        if(obj == this)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            //禁止重复按键
            if(!keyEvent->isAutoRepeat())
            {
                //打开全屏
                if(keyEvent->key() == Qt::Key_F11)
                {
                    ui->videoWidget->setFullScreen(true);
                }

                //暂停与播放
                if(keyEvent->key() == Qt::Key_Space)
                {
                    startAndStopClickCount % 2 == 0 ? setPlayerStatus(true) : setPlayerStatus(false);
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj,e);
}

void MainWindow::setPlayerStatus(bool playStatus)
{
    if(playStatus)
    {
        player->play();
        ui->startAndPauseBtn->setText("暂停");
        startAndStopClickCount++;
    }
    else
    {
        player->pause();
        ui->startAndPauseBtn->setText("播放");
        startAndStopClickCount++;
    }
}

void MainWindow::init_player()
{
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

}



MainWindow::~MainWindow()
{
    delete player;
    delete audioOut;
    delete ui;
}

void ProcessBarThread::run()
{
    while(videoStatus)
    {
        emit current_play_duration();
        QThread::usleep(10);
    }
}



