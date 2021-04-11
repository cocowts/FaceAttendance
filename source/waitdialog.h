#ifndef WAITDIALOG_H
#define WAITDIALOG_H

#include <QDialog>
#include <QLabel>

class WaitDialog : public QDialog
{
    Q_OBJECT
public:
    WaitDialog(QWidget *parent = nullptr);

    void show();
    void hide();

    void clear();
    void setMovie(QMovie *movie);
    void setNum(double num);
    void setNum(int num);
    void setPicture(const QPicture &picture);
    void setPixmap(const QPixmap &);
    void setText(const QString &);
    void setScaledContents(bool);

private:
    QLabel m_label;
};

#endif // WAITDIALOG_H
