#ifndef NEWSTUDYDIALOG_H
#define NEWSTUDYDIALOG_H

#include <QDialog>
#include "../share/studyrecord.h"

namespace Ui {
class NewStudyDialog;
}

class NewStudyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewStudyDialog(const StudyRecord &studyRec, QWidget *parent = 0);
    ~NewStudyDialog();

    const StudyRecord& getStudyRecord() const { return study; }

public slots:
    void onOk();

    void updateAge(const QDate &date, bool force = false);
    void updateBirth();

private:
    void init();
    Ui::NewStudyDialog *ui;

    StudyRecord study;
};

#endif // NEWSTUDYDIALOG_H
