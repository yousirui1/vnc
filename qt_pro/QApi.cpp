#include <QApplication>
#include "LoginDialog.h"
#include "QApi.h"


int client_port = 0;
char *client_ip = NULL;

int create_login_dialog()
{
	int argc = 0;
	char **argv = NULL;

	QApplication a(argc, argv);
    LoginDialog login;
	login.show();
	

	return a.exec();	
}

#if 0
int main(int argc, char *argv[])
{
    int port = 0;



}
#endif
