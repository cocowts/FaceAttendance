#include "QLoginDialog.h"

#include <QFont>
#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>
#include <QTime>

QLoginDialog::QLoginDialog(QWidget* parent) : QDialog (parent, Qt::Drawer | Qt::Desktop)
{
    initControl();
    initSlots();

    //QRandomGenerator::global()->seed(static_cast<uint>((QTime::currentTime().second() * 1000 + QTime::currentTime().msec())));

    m_colors = getColor();
    m_captcha = getCaptcha();

    m_timer.setParent(this);
    m_timer.start(100);

    setObjectName("loginDialog");
}

void QLoginDialog::initControl()
{
    m_userEdit.setObjectName("loginUserEdit");
    m_userEdit.setParent(this);
    m_userEdit.move(10, 10);
    m_userEdit.resize(160, 20);
    m_userEdit.setMaxLength(12);
    m_userEdit.setPlaceholderText(" 手机/邮箱、用户名");

    m_passwordEdit.setObjectName("loginPasswordEdit");
    m_passwordEdit.setParent(this);
    m_passwordEdit.move(10, 37);
    m_passwordEdit.resize(160, 20);
    m_passwordEdit.setEchoMode(QLineEdit::Password);
    m_passwordEdit.setMaxLength(8);
    m_passwordEdit.setPlaceholderText(" 密码");

    m_captchaEdit.setObjectName("loginCaptchaEdit");
    m_captchaEdit.setParent(this);
    m_captchaEdit.move(10, 67);
    m_captchaEdit.resize(57, 20);
    m_captchaEdit.setMaxLength(4);
    m_captchaEdit.setPlaceholderText(" 验证码");

    m_cancelBtn.setObjectName("loginCancelBtn");
    m_cancelBtn.setParent(this);
    m_cancelBtn.setText("取消");
    m_cancelBtn.move(10, 103);
    m_cancelBtn.resize(70, 22);

    m_loginBtn.setObjectName("loginLoginBtn");
    m_loginBtn.setParent(this);
    m_loginBtn.setText("登陆");
    m_loginBtn.move(100, 103);
    m_loginBtn.resize(70, 22);

    setFixedSize(180, 135);
    setWindowTitle("账号密码登录");
}

void QLoginDialog::initSlots()
{
    connect(&m_cancelBtn, SIGNAL(clicked()), this, SLOT(onCancelBtnClicked()));
    connect(&m_loginBtn, SIGNAL(clicked()), this, SLOT(onLoginBtnClicked()));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(Timer_TimeOut()));
}

QString QLoginDialog::getUser()
{
    return m_user;
}

QString QLoginDialog::getPassword()
{
    return m_password;
}

QString QLoginDialog::getCaptcha()
{
    QString ret = "";

    for(int i=0; i<4; i++)
    {
        int c = (QRandomGenerator::global()->generate() % 2) ? 'a' : 'A';
        ret += static_cast<QChar>(c + QRandomGenerator::global()->generate() % 26);
    }

    return ret;
}

Qt::GlobalColor* QLoginDialog::getColor()
{
    static Qt::GlobalColor color[4];

    for(int i=0; i<4; i++)
    {
        color[i] = static_cast<Qt::GlobalColor>((2 + QRandomGenerator::global()->generate() % 16));
    }

    return color;
}

//------------------------------------
void QLoginDialog::onCancelBtnClicked()
{
    done(QDialog::Rejected);
}

void QLoginDialog::onLoginBtnClicked()
{
    if( m_captcha.toLower() == m_captchaEdit.text().toLower() )
    {
        m_user = m_userEdit.text().trimmed();
        m_password = m_passwordEdit.text();

        if( m_user != "" && m_password != "")
        {
            done(QDialog::Accepted);
        }
        else
        {
            QMessageBox(QMessageBox::Critical, "错误", "用户名或密码输入有误", QMessageBox::Ok, this, Qt::Drawer).exec();
        }
    }
    else
    {
        QMessageBox(QMessageBox::Critical, "错误", "验证码输入有误", QMessageBox::Ok, this, Qt::Drawer).exec();

        //  QRandomGenerator::global()->seed(QTime::currentTime().second() * 1000 + QTime::currentTime().msec());

        m_captcha = getCaptcha();
    }
}

void QLoginDialog::Timer_TimeOut()
{
    m_colors = getColor();

    update();
}

//---------------------------------------
void QLoginDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.setFont(QFont("consolas", 12));
    painter.fillRect(90, 67, 70, 20, Qt::white);

    for(int i=0; i<150; i++)
    {
        painter.setPen(m_colors[i % 4]);
        painter.drawPoint(90 + QRandomGenerator::global()->generate() % 70, 67 + QRandomGenerator::global()->generate() % 20);
    }

    for(int i=0; i<4; i++)
    {
        painter.setPen(m_colors[i]);
        painter.drawText(90 + i*17, 67, 17, 20, Qt::AlignCenter, m_captcha.at(i));
    }
}
