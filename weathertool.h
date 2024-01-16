#ifndef WEATHERTOOL_H
#define WEATHERTOOL_H
#include <QString>
#include <QMap>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QIODevice>
#include <QByteArray>

class WeatherTool {
private:
    static QMap<QString, QString> mCityMap;

    // 初始化城市名和城市编码的 map
    static void initCityMap() {
        // 1. 读取文件
        QString filePath = "D:/BaiduNetdiskDownload/weatherforcast/weatherforcast/citycode.json";  // 城市编码文件
        QFile file(filePath);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray json = file.readAll();
        file.close();

        // 2. 解析并写入到 map 中
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(json, &err);

        if (err.error != QJsonParseError::NoError) {
            return;
        }

        if (!doc.isArray()) {
            return;
        }

        QJsonArray cities = doc.array();
        for (int i = 0; i < cities.size(); i++) {
            QString city = cities[i].toObject().value("city_name").toString();
            QString code = cities[i].toObject().value("city_code").toString();

            if (code.size() > 0) {
                mCityMap.insert(city, code);
            }
        }
    }

public:
    // 输入城市名，得到城市编码
    static QString getCityCode(QString cityName) {
        if (mCityMap.isEmpty()) {
            initCityMap();
        }

        QMap<QString, QString>::iterator it = mCityMap.find(cityName);
        // 包含两种模式 北京/北京市
        if (it == mCityMap.end()) {
            it = mCityMap.find(cityName + u8"市");
        }
        if (it != mCityMap.end()) {
            return it.value();
        }
        return "";
    }
};

// 静态成员变量，类内声明，类外初始化
QMap<QString, QString> WeatherTool::mCityMap = {};

#endif // WEATHERTOOL_H
