#ifndef STUDYRECORD_H
#define STUDYRECORD_H

#include <QList>
#include <QString>
#include <QDateTime>

class StudyRecord;
class ImageRecord
{
public:
    ImageRecord(const QString &uid = QString()):
        imageUid(uid) {}

    QString imageUid;
    QString sopClassUid;
    QString seriesUid;
    QString studyUid;
    QString refImageUid;
    QString imageNo;
    QDateTime imageTime;
    QString bodyPart;
    QString imageDesc;
    QString imageFile;
};

class ReportRecord
{
public:
    ReportRecord(const QString &uid = QString()):
        reportUid(uid),isCompleted(false),isVerified(false) {}

    QString reportUid;
    QDateTime createTime;
    QDateTime contentTime;
    QString reportFile;
    QString seriesUid;
    QString studyUid;
    bool isCompleted;
    bool isVerified;
};

class StudyRecord
{
public:
    StudyRecord(const QString &uid = QString()) :
        studyUid(uid) {}
    ~StudyRecord() { qDeleteAll(imageList); qDeleteAll(reportList); }

    QString studyUid;
    QString accNumber;
    QString patientId;
    QString patientName;
    QString patientSex;
    QDate patientBirth;
    QString patientAge;
    QString medicalAlert;
    QString patientAddr;
    QString patientPhone;
    QString patientWeight;
    QString patientSize;
    QDateTime studyTime;
    QString modality;
    QString studyDesc;
    QString procId;
    QString reqPhysician;
    QString perPhysician;
    QString institution;
    QString status;
    QList<ImageRecord*> imageList;
    QList<ReportRecord*> reportList;
};

#endif // STUDYRECORD_H
