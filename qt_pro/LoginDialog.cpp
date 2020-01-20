#include <QtGui>
#include "LoginDialog.h"
#include "QApi.h"


LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent)

{
	hostLabel = new QLabel(tr("Server host:"));	
	portLabel = new QLabel(tr("Server port:"));

	
	hostLineEdit = new QLineEdit();	
	portLineEdit = new QLineEdit();

	portLineEdit->setValidator(new QIntValidator(1, 65545, this));
	
	statusLabel = new QLabel(tr("This YZY VNC Debug Programe"
								"connect to Client."));
		
	connectButton = new QPushButton(tr("Connect"));
	connectButton->setDefault(true);
	connectButton->setEnabled(false);
	
	quitButton = new QPushButton(tr("Quit"));
	
	buttonBox = new QDialogButtonBox;
	buttonBox->addButton(connectButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);		
	
	connect(hostLineEdit, SIGNAL(textChanged(QString)),
			this, SLOT(enableConnectButton()));
		
	connect(portLineEdit, SIGNAL(textChanged(QString)),
			this, SLOT(enableConnectButton()));
	
	connect(connectButton, SIGNAL(clicked()),
			this, SLOT(connectClient()));

	connect(connectButton, SIGNAL(clicked()),
			this, SLOT(close()));
	

	QGridLayout *mainLayout = new QGridLayout;
	mainLayout->addWidget(statusLabel, 0, 0, 1, 2);
	mainLayout->addWidget(hostLabel, 1, 0);
	mainLayout->addWidget(hostLineEdit, 1, 1);
	mainLayout->addWidget(portLabel, 2, 0);
	mainLayout->addWidget(portLineEdit, 2, 1);
	mainLayout->addWidget(buttonBox, 3, 0, 1, 2);
	setLayout(mainLayout);

	setWindowTitle(tr("Connect Client"));
	portLineEdit->setFocus();
}

void LoginDialog::connectClient()
{
    //close();
}

void LoginDialog::enableConnectButton()
{
    connectButton->setEnabled(!hostLineEdit->text().isEmpty() &&
                                 !portLineEdit->text().isEmpty());
}


