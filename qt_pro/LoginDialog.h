#ifndef __Q_WINDOW_H__
#define __Q_WINDOW_H__

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE


class LoginDialog : public QDialog
{
	Q_OBJECT

public:
	LoginDialog(QWidget *parent = 0);

private slots:
	void connectClient();
	void enableConnectButton();

private:
	QLabel *hostLabel;
	QLabel *portLabel;
	QLineEdit *hostLineEdit;
	QLineEdit *portLineEdit;
	QLabel *statusLabel;
	QPushButton *connectButton;
	QPushButton *quitButton;
	QDialogButtonBox *buttonBox;


public:
    int port;
    char *ip;

};



#endif
