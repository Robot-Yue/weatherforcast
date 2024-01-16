#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "weatherdata.h"
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDebug>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QList>
#include <QPainter>
#include <QPen>
#include <QPoint>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    // 重写父类方法
    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

    // 获取天气数据
    void getWeatherInfo(QString cityCode);
    // 解析天气数据
    void parseJson(QByteArray &byteArray);

    //天气类型
    void weatherType();

    // 更新 UI
    void updateUI();

    // 重写父类的 eventFilter 方法
    bool eventFilter(QObject *watched, QEvent *event);

    // 绘制高低温曲线
    void paintHighCurve();
    void paintLowCurve();

private slots:
    // 用于处理 HTTP 服务返回数据的槽函数
    void onReplied(QNetworkReply *reply);
    // 城市搜索按钮
    void on_btnSearch_clicked();
    // 判断文本框中是否发生回车事件，回车即搜索
    void on_leCity_returnPressed();

private:
    Ui::MainWindow* ui;

    QMenu* mExitMenu;   // 右键退出的菜单
    QAction* mExitAct;  // 退出的行为
    QPoint mOffset;     // 窗口移动时, 鼠标与窗口左上角的偏移

    // 声明用于 HTTP 通信的指针对象
    QNetworkAccessManager *mNetAccessManager;

    // 当天和未来 6 天的天气
    Today mToday;
    Day mDay[6];

    // 控件数组，用于更新 UI
    // 星期和日期
    QList<QLabel*> mWeekList;
    QList<QLabel*> mDateList;
    // 天气和天气图标
    QList<QLabel*> mTypeList;
    QList<QLabel*> mTypeIconList;
    // 天气污染指数
    QList<QLabel*> mAqiList;
    // 风力和风向
    QList<QLabel*> mFxList;
    QList<QLabel*> mFlList;
    // 天气对应的图标
    QMap<QString, QString> mTypeMap;
};
#endif  // MAINWINDOW_H
