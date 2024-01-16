#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#endif // WEATHERDATA_H

#include <QString>

class Today {
public:
    Today() {
        date = "2023-12-01";
        city = u8"广州";

        ganmao = u8"感冒指数";

        wendu = "0";
        shidu = "0%";
        pm25 = 0;
        quality = u8"无数据";

        type = u8"多云";

        fl = u8"2级";
        fx = u8"南风";

        high = 30;
        low = 18;
    }

    QString date;
    QString city;

    QString ganmao;

    QString wendu;
    QString shidu;
    int pm25;
    QString quality;

    QString type;

    QString fl;
    QString fx;

    int high;
    int low;
};

class Day {
public:
    Day() {
        date = "2023-12-01";
        week = u8"周五";

        type = u8"多云";

        high = 0;
        low = 0;

        fx = u8"南风";
        fl = u8"2级";

        aqi = 0;
    }

    QString date;
    QString week;

    QString type;

    int high;
    int low;

    QString fx;
    QString fl;

    int aqi; // 空气污染系数
};
