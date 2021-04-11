#include "waitdialog.h"

#include <QMovie>
#include <QVBoxLayout>

WaitDialog::WaitDialog(QWidget *parent) : QDialog(parent, Qt::FramelessWindowHint)
{
    QVBoxLayout *pLayout = new QVBoxLayout;
    pLayout->setMargin(0);
    pLayout->addWidget(&m_label);
    setLayout(pLayout);

    setAttribute(Qt::WA_TranslucentBackground);
    m_label.setAttribute(Qt::WA_TranslucentBackground);

    setModal(true);
    hide();
}

void WaitDialog::show()
{
    QDialog::show();

    QWidget* pw = static_cast<QWidget*>(this->parent());

    QPoint centerPoint(pw->x() + 0.5 * (pw->width() - width()), pw->y() + 0.5 * (pw->height() - height()));
    move(centerPoint);

    QMovie *movie = m_label.movie();
    if (movie != nullptr)
    {
        movie->start();
    }
}

void WaitDialog::hide()
{
    QDialog::hide();

    QMovie *movie = m_label.movie();
    if (movie != nullptr)
    {
        movie->stop();
    }
}

void WaitDialog::clear()
{
    m_label.clear();
}

void WaitDialog::setMovie(QMovie *movie)
{
    m_label.setMovie(movie);
}

void WaitDialog::setNum(double num)
{
    m_label.setNum(num);
}

void WaitDialog::setNum(int num)
{
    m_label.setNum(num);
}

void WaitDialog::setPicture(const QPicture &picture)
{
    m_label.setPicture(picture);
}

void WaitDialog::setPixmap(const QPixmap &pixmap)
{
    m_label.setPixmap(pixmap);
}

void WaitDialog::setText(const QString &text)
{
    m_label.setText(text);
}

void WaitDialog::setScaledContents(bool scaked)
{
    m_label.setScaledContents(scaked);
}
