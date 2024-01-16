#include "mainwindow.h"

#include "ui_mainwindow.h"
// weathertool.h 有初始化静态成员变量，故只能放在此处，头文件中不可以放变量的定义或初始化静态成员变量
// 如果把定义放到头文件的话，就不能避免多次定义变量，C++ 不允许多次定义变量
// https://blog.csdn.net/u012422524/article/details/127645265
#include "weathertool.h"

#define INCREMENT 1.2     // 温度每升高/降低 1°，y 坐标的增量
#define POINT_RADIUS 3    // 曲线描点的大小
#define TEXT_OFFSET_X 12
#define TEXT_OFFSET_Y 12

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    //设置窗口属性
    setWindowFlag(Qt::FramelessWindowHint);  // 无边框
    setFixedSize(width(), height());         // 固定窗口大小

    // 右键菜单：退出程序
    mExitMenu = new QMenu(this);
    mExitAct = new QAction();
    mExitAct->setText(tr("Exit"));
    mExitAct->setIcon(QIcon(":/res/close.png"));
    mExitMenu->addAction(mExitAct);

    connect(mExitAct, &QAction::triggered, this, [=]() {
        qApp->exit(0);
    });

    // 天气类型
    weatherType();

    mNetAccessManager = new QNetworkAccessManager(this);
    connect(mNetAccessManager, &QNetworkAccessManager::finished, this, &MainWindow::onReplied);

    // 直接在构造中请求天气数据
    //getWeatherInfo("101010100");  // 101010100 表示北京城市编码
    getWeatherInfo(u8"北京");

    // 给标签添加事件过滤器
    // 事件过滤器是接收发送到该对象的所有事件的对象，过滤器可以停止事件或将其转发到此对象（this）
    ui->lblHighCurve->installEventFilter(this);
    ui->lblLowCurve->installEventFilter(this);
}

MainWindow::~MainWindow() { delete ui; }

// 重写父类的虚函数
// 父类中默认的实现是忽略右键菜单事件，重写之后，就可以处理右键菜单
void MainWindow::contextMenuEvent(QContextMenuEvent *event) {
    // 弹出右键菜单（在鼠标右键单击的位置）
    mExitMenu->exec(QCursor::pos());
    // 调用 accept 表示，这个事件已经处理，不需要向上传递
    event->accept();
}

// 监听鼠标点击事件（显示鼠标点击处坐标）
void MainWindow::mousePressEvent(QMouseEvent* event) {
    qDebug() << u8"窗口左上角：" << this->pos() << u8", 鼠标坐标点：" << event->globalPos();
    mOffset = event->globalPos() - this->pos();
}

// 监听鼠标移动事件（拖动窗口）
void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    this->move(event->globalPos() - mOffset);
}

// 发送一个 GET 请求
void MainWindow::getWeatherInfo(QString cityName) {
    QString cityCode = WeatherTool::getCityCode(cityName);

    if (cityCode.isEmpty()) {
        QMessageBox::warning(this, u8"天气", u8"请检查输入是否正确！", QMessageBox::Ok);
        return;
    }

    QUrl url("http://t.weather.itboy.net/api/weather/city/" + cityCode);
    mNetAccessManager->get(QNetworkRequest(url));
}

// 解析天气数据并更新 UI
void MainWindow::parseJson(QByteArray &byteArray) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(byteArray, &err);
    if (err.error != QJsonParseError::NoError) {
        return;
    }

    QJsonObject rootObj = doc.object();
    qDebug() << rootObj.value("message").toString();

    /****** 1. 解析日期和城市 ******/
    mToday.date = rootObj.value("date").toString();
    mToday.city = rootObj.value("cityInfo").toObject().value("city").toString();

    /****** 2. 解析 yesterday ******/
    QJsonObject objData = rootObj.value("data").toObject();
    QJsonObject objYesterday = objData.value("yesterday").toObject();

    mDay[0].week = objYesterday.value("week").toString();
    mDay[0].date = objYesterday.value("ymd").toString();

    mDay[0].type = objYesterday.value("type").toString();

    // 解析 高、低温 数据
    QString s;
    s = objYesterday.value("high").toString().split(" ").at(1);
    s = s.left(s.length() - 1);
    mDay[0].high = s.toInt();

    s = objYesterday.value("low").toString().split(" ").at(1);
    s = s.left(s.length() - 1);
    mDay[0].low = s.toInt();

    // 解析 风向、风力 数据
    mDay[0].fx = objYesterday.value("fx").toString();
    mDay[0].fl = objYesterday.value("fl").toString();

    // 解析 污染指数 aqi 数据
    mDay[0].aqi = objYesterday.value("aqi").toDouble();

    /****** 3. 解析预测 5 天的数据 ******/
    QJsonArray forecastArr = objData.value("forecast").toArray();

    for (int i = 0; i < 5; i++) {
        QJsonObject objForecast = forecastArr[i].toObject();
        mDay[i + 1].week = objForecast.value("week").toString();
        mDay[i + 1].date = objForecast.value("ymd").toString();

        // 天气类型
        mDay[i + 1].type = objForecast.value("type").toString();

        // 高温、低温
        QString s;
        s = objForecast.value("high").toString().split(" ").at(1);
        s = s.left(s.length() - 1);
        mDay[i + 1].high = s.toInt();

        s = objForecast.value("low").toString().split(" ").at(1);
        s = s.left(s.length() - 1);
        mDay[i + 1].low = s.toInt();

        // 风向、风力 数据
        mDay[i + 1].fx = objForecast.value("fx").toString();
        mDay[i + 1].fl = objForecast.value("fl").toString();

        // 污染指数 aqi 数据
        mDay[i + 1].aqi = objForecast.value("aqi").toDouble();
    }

    /****** 4. 解析今天的数据 ******/
    mToday.ganmao = objData.value("ganmao").toString();
    mToday.wendu = objData.value("wendu").toString();
    mToday.shidu = objData.value("shidu").toString();
    mToday.pm25 = objData.value("pm25").toInt();
    mToday.quality = objData.value("quality").toString();

    /****** 5. forecast 中第一个数组元素，也是今天的数据 ******/
    mToday.type = mDay[1].type;

    mToday.fx = mDay[1].fx;
    mToday.fl = mDay[1].fl;

    mToday.high = mDay[1].high;
    mToday.low = mDay[1].low;

    /****** 6. 更新 UI ******/
    updateUI();

    /****** 7. 更新温度曲线图 ******/
    ui->lblHighCurve->update();
    ui->lblLowCurve->update();
}

void MainWindow::weatherType() {
    // 将控件添加到控件数组
    // 星期和日期
    mWeekList << ui->lblWeek0 << ui->lblWeek1 << ui->lblWeek2 <<
        ui->lblWeek3 << ui->lblWeek4 << ui->lblWeek5;
    mDateList << ui->lblDate0 << ui->lblDate1 << ui->lblDate2 <<
        ui->lblDate3 << ui->lblDate4 << ui->lblDate5;

    // 天气和天气图标
    mTypeList << ui->lblType0 << ui->lblType1 << ui->lblType2 <<
        ui->lblType3 << ui->lblType4 << ui->lblType5;
    mTypeIconList << ui->lblTypeIcon0 << ui->lblTypeIcon1 << ui->lblTypeIcon2 <<
        ui->lblTypeIcon3 << ui->lblTypeIcon4 << ui->lblTypeIcon5;

    // 天气指数
    mAqiList << ui->lblQuality0 << ui->lblQuality1 << ui->lblQuality2 <<
        ui->lblQuality3 << ui->lblQuality4 << ui->lblQuality5;

    // 风向和风力
    mFlList << ui->lblFl0 << ui->lblFl1 << ui->lblFl2 << ui->lblFl3 <<
        ui->lblFl4<<ui->lblFl5;
    mFxList << ui->lblFx0 << ui->lblFx1 << ui->lblFx2 << ui->lblFx3 <<
        ui->lblFx4 << ui->lblFx5;

    // 天气对应的图标
    mTypeMap.insert(u8"暴雪", ":/res/type/BaoXue.png");
    mTypeMap.insert(u8"暴雨", ":/res/type/BaoYu.png");
    mTypeMap.insert(u8"暴雨到暴雪", ":/res/type/BaoYuDaoDaBaoYu.png");
    mTypeMap.insert(u8"大暴雨", ":/res/type/DaBaoYu.png");
    mTypeMap.insert(u8"大暴雨到大暴雪", ":/res/type/DaBaoYuDaoTeDaBaoYu.png");
    mTypeMap.insert(u8"大到暴雪", ":/res/type/DaDaoBaoXue.png");
    mTypeMap.insert(u8"大到暴雨", ":/res/type/DaDaoBaoYu.png");
    mTypeMap.insert(u8"大雪", ":/res/type/DaXue.png");
    mTypeMap.insert(u8"大雨", ":/res/type/DaYu.png");
    mTypeMap.insert(u8"冻雨", ":/res/type/DongYu.png");
    mTypeMap.insert(u8"多云", ":/res/type/DuoYun.png");
    mTypeMap.insert(u8"浮尘", ":/res/type/FuChen.png");
    mTypeMap.insert(u8"雷阵雨", ":/res/type/LeiZhenYu.png");
    mTypeMap.insert(u8"雷阵雨伴有冰雹", ":/res/type/LeiZhenYuBanYouBingBao.png");
    mTypeMap.insert(u8"霾", ":/res/type/Mai.png");
    mTypeMap.insert(u8"强沙尘暴", ":/res/type/QiangShaChenBao.png");
    mTypeMap.insert(u8"晴", ":/res/type/Qing.png");
    mTypeMap.insert(u8"沙尘暴", ":/res/type/ShaChenBao.png");
    mTypeMap.insert(u8"特大暴雨", ":/res/type/TeDaBaoYu.png");
    mTypeMap.insert(u8"雾", ":/res/type/Wu.png");
    mTypeMap.insert(u8"小到中雨", ":/res/type/XiaoDaoZhongYu.png");
    mTypeMap.insert(u8"小到中雪", ":/res/type/XiaoDaoZhongXue.png");
    mTypeMap.insert(u8"小雪", ":/res/type/XiaoXue.png");
    mTypeMap.insert(u8"小雨", ":/res/type/XiaoYu.png");
    mTypeMap.insert(u8"雪", ":/res/type/Xue.png");
    mTypeMap.insert(u8"扬沙", ":/res/type/YangSha.png");
    mTypeMap.insert(u8"阴", ":/res/type/Yin.png");
    mTypeMap.insert(u8"雨", ":/res/type/Yu.png");
    mTypeMap.insert(u8"雨夹雪", ":/res/type/YuJiaXue.png");
    mTypeMap.insert(u8"阵雨", ":/res/type/ZhenYu.png");
    mTypeMap.insert(u8"阵雪", ":/res/type/ZhenXue.png");
    mTypeMap.insert(u8"中雨", ":/res/type/ZhongYu.png");
    mTypeMap.insert(u8"中雪", ":/res/type/ZhongXue.png");
}

// 更新 UI
void MainWindow::updateUI() {
    // 1. 更新日期和城市
    ui->lblDate->setText(QDateTime::fromString(mToday.date, "yyyyMMdd").toString("yyyy/MM/dd") + " " + mDay[1].week);
    ui->lblCity->setText(mToday.city);

    // 2. 更新今天的数据
    ui->lblTypeIcon->setPixmap(mTypeMap[mToday.type]);
    ui->lblTemp->setText(mToday.wendu + u8"°C");
    //ui->lblTemp->setText(mToday.type);
    ui->lblLowHigh->setText(QString::number(mToday.low) + "~" + QString::number(mToday.high) + u8"°C");

    ui->lblGanMao->setText(u8"感冒指数: " + mToday.ganmao);
    ui->lblWindFx->setText(mToday.fx);
    ui->lblWindFl->setText(mToday.fl);

    ui->lblPM25->setText(QString::number(mToday.pm25));

    ui->lblShiDu->setText(mToday.shidu);
    ui->lblQuality->setText(mToday.quality);

    // 3. 更新六天的数据
    for (int i = 0; i < 6; i++) {
       // 3.1 更新日期和时间
       mWeekList[i]->setText(u8"周" + mDay[i].week.right(1));
       ui->lblWeek0->setText(u8"昨天");
       ui->lblWeek1->setText(u8"今天");
       ui->lblWeek2->setText(u8"明天");

       QStringList ymdList = mDay[i].date.split("-");
       mDateList[i]->setText(ymdList[1] + "/" + ymdList[2]);

       // 3.2 更新天气类型
       mTypeList[i]->setText(mDay[i].type);
       mTypeIconList[i]->setPixmap(mTypeMap[mDay[i].type]);

       // 3.3 更新空气质量
       if (mDay[i].aqi >= 0 && mDay[i].aqi <= 50) {
           mAqiList[i]->setText(u8"优");
           mAqiList[i]->setStyleSheet("background-color: rgb(121, 184, 0);");
       } else if (mDay[i].aqi >= 50 && mDay[i].aqi <= 100) {
           mAqiList[i]->setText(u8"良");
           mAqiList[i]->setStyleSheet("background-color: rgb(255, 187, 23);");
       } else if (mDay[i].aqi >= 100 && mDay[i].aqi <= 150) {
           mAqiList[i]->setText(u8"轻度");
           mAqiList[i]->setStyleSheet("background-color: rgb(255, 87, 97);");
       } else if (mDay[i].aqi >= 150 && mDay[i].aqi <= 200) {
           mAqiList[i]->setText(u8"中度");
           mAqiList[i]->setStyleSheet("background-color: rgb(235, 17, 27);");
       } else if (mDay[i].aqi >= 200 && mDay[i].aqi <= 250) {
           mAqiList[i]->setText(u8"重度");
           mAqiList[i]->setStyleSheet("background-color: rgb(170, 0, 0);");
       } else {
           mAqiList[i]->setText(u8"严重");
           mAqiList[i]->setStyleSheet("background-color: rgb(110, 0, 0);");
       }

       // 3.4 更新风力、风向
       mFxList[i]->setText(mDay[i].fx);
       mFlList[i]->setText(mDay[i].fl);
    }
}

// 重写父类事件过滤器方法
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->lblHighCurve && event->type() == QEvent::Paint) {
       paintHighCurve();
    }

    if (watched == ui->lblLowCurve && event->type() == QEvent::Paint) {
       paintLowCurve();
    }

    return QWidget::eventFilter(watched, event);
}

void MainWindow::paintHighCurve() {
    QPainter painter(ui->lblHighCurve);

    // 抗锯齿
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. 获取 x 坐标
    int pointX[6] = {0};
    for (int i = 0; i < 6; i++) {
       // 每个控件的中心点就是曲线的 x 坐标
       pointX[i] = mWeekList[i]->pos().x() + mWeekList[i]->width() / 2;
    }

    // 2. 获取 y 坐标
    int tempSum = 0;
    int tempAverage = 0;
    for(int i = 0; i < 6; i++) {
       tempSum += mDay[i].high;
    }
    tempAverage = tempSum / 6;  // 六天最高温的平均值
    // 计算 y 坐标
    int pointY[6] = {0};
    int yCenter = ui->lblHighCurve->height() / 2;
    for (int i = 0; i < 6; i++) {
       pointY[i] = yCenter - ((mDay[i].high - tempAverage) * INCREMENT);
    }

    // 3. 开始绘制
    // 3.1 初始化画笔
    QPen pen = painter.pen();
    pen.setWidth(1);                    // 设置画笔的宽度
    pen.setColor(QColor(255, 170, 0));  // 设置画笔的颜色

    painter.setPen(pen);
    painter.setBrush(QColor(255, 170, 0));

    // 3.2 画点、写文本
    for (int i = 0; i < 6; i++) {
       // 显示圆点
       painter.drawEllipse(QPoint(pointX[i], pointY[i]), POINT_RADIUS, POINT_RADIUS);

       // 显示温度文本
       painter.drawText(pointX[i] - TEXT_OFFSET_X, pointY[i] - TEXT_OFFSET_Y, QString::number(mDay[i].high) + u8"°C");
    }

    // 3.3 画线
    for (int i = 0; i < 5; i++) {
       if (i == 0) {
           pen.setStyle(Qt::DotLine);
           painter.setPen(pen);
       } else {
           pen.setStyle(Qt::SolidLine);
           painter.setPen(pen);
       }

       painter.drawLine(pointX[i], pointY[i], pointX[i+1], pointY[i+1]);
    }
}

void MainWindow::paintLowCurve() {
    QPainter painter(ui->lblLowCurve);

    //抗锯齿
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. 获取 x 坐标
    int pointX[6] = {0};
    for (int i = 0; i < 6; i++) {
       // 每个控件的中心点就是曲线的 x 坐标
       pointX[i] = mWeekList[i]->pos().x() + mWeekList[i]->width() / 2;
    }

    // 2. 获取 y 坐标
    int tempSum = 0;
    int tempAverage = 0;
    for(int i = 0; i < 6; i++) {
       tempSum += mDay[i].low;
    }
    tempAverage = tempSum / 6;  // 六天最高温的平均值
    // 计算 y 坐标
    int pointY[6] = {0};
    int yCenter = ui->lblLowCurve->height() / 2;
    for (int i = 0; i < 6; i++) {
       pointY[i] = yCenter - ((mDay[i].low - tempAverage) * INCREMENT);
    }

    // 3. 开始绘制
    // 3.1 初始化画笔
    QPen pen = painter.pen();
    pen.setWidth(1);                    // 设置画笔的宽度
    pen.setColor(QColor(0, 255, 255));  // 设置画笔的颜色

    painter.setPen(pen);
    painter.setBrush(QColor(0, 255, 255));

    // 3.2 画点、写文本
    for (int i = 0; i < 6; i++) {
       // 显示圆点
       painter.drawEllipse(QPoint(pointX[i],pointY[i]), POINT_RADIUS, POINT_RADIUS);

       // 显示温度文本
       painter.drawText(pointX[i] - TEXT_OFFSET_X, pointY[i] - TEXT_OFFSET_Y, QString::number(mDay[i].low) + u8"°C");
    }

    // 3.3 画线
    for (int i = 0; i < 5; i++) {
       if (i == 0) {
           pen.setStyle(Qt::DotLine);
           painter.setPen(pen);
       } else {
           pen.setStyle(Qt::SolidLine);
           painter.setPen(pen);
       }

       painter.drawLine(pointX[i], pointY[i], pointX[i+1], pointY[i+1]);
    }
}

// 接收服务端数据
// 当 GET 请求完毕，服务器返回数据时 mNetAccessManager 会发射 finished 信号，进而调用 onReplied()
void MainWindow::onReplied(QNetworkReply *reply) {
    // 响应的状态码为 200，表示请求成功
    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    qDebug() << "operation:" << reply->operation();          // 请求方式
    qDebug() << "status code:" << status_code;               // 状态码
    qDebug() << "url:" << reply->url();                      // url
    qDebug() << "raw header:" << reply->rawHeaderList();     // header

    // 如果指定的城市编码不存在，就会报错
    if (reply->error() != QNetworkReply::NoError || status_code != 200) {
        QMessageBox::warning(this, u8"提示", u8"请求数据失败！", QMessageBox::Ok);
    } else {
        // 获取响应信息
        QByteArray reply_data = reply->readAll();
        QByteArray byteArray = QString(reply_data).toUtf8();
        qDebug() << "read all:" << byteArray.data();

        parseJson(byteArray);
    }

    reply->deleteLater();
}

// 城市搜索按钮
void MainWindow::on_btnSearch_clicked() {
    QString cityName = ui->leCity->text();
    getWeatherInfo(cityName);
    ui->leCity->clear();
}

// 判断文本框中是否发生回车事件
void MainWindow::on_leCity_returnPressed() {
    QString cityName = ui->leCity->text();
    getWeatherInfo(cityName);
    ui->leCity->clear();
}

