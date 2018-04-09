/*
 * mail.cpp
 *
 *  Created on: Feb 25, 2018
 *      Author: marc
 */
#include "MessageBus.h"
#include "mail.h"
#include <thread>
#include <cstring>





mail::mail(MessageBus* messageBus): MarelabBusNode(messageBus) {
	this->mailadr = "marchammermann@googlemail.com";
}

mail::~mail() {
	// TODO Auto-generated destructor stub
}


void mail::onNotify(Message message)
{
	Json::Value json;
	if ((message.getMsgFor()=="*")||(message.getMsgFor()=="MAIL")){

		try{
        json = message.getRootJson();
        string command = json["command"].asString();
        if (command == "SendMail"){
        	string subject = json["subject"].asString();
        	string mailtext = json["mailtext"].asString();
        	SendMail(subject,  mailtext);
        	cout << "MSG->SENDMAIL " << endl;
        }
		}catch(...){
			cout << "ERROR NOTIFY MAIL" << endl;
		}
	}

}

int mail::sendEmail(string to, string subject, string body) {
  int p2cFd[2];

  int ret = pipe(p2cFd);
  if (ret) {
    return ret;
  }

  pid_t child_pid = fork();
  if (child_pid < 0) {
    close(p2cFd[READEND]);
    close(p2cFd[WRITEEND]);

    return child_pid;
  }
  else if (!child_pid) {
    dup2(p2cFd[READEND], READEND);
    close(p2cFd[READEND]);
    close(p2cFd[WRITEEND]);

    execlp("mail", "mail", "-s", subject.c_str(), to.c_str(), NULL);

    exit(EXIT_FAILURE);
  }

  close(p2cFd[READEND]);

  ret = write(p2cFd[WRITEEND], body.c_str(), body.size());
  if (ret < 0) {
    return ret;
  }

  close(p2cFd[WRITEEND]);

  if (waitpid(child_pid, &ret, 0) == -1) {
    return ret;
  }

  return 0;
}

void mail::SendMail(string subject, string texttosend) {

	try{
	string adr = "marchammermann@googlemail.com";
    char cmd[100];  // to hold the command.
    //char body[] = "SO rocks";    // email body.
    char tempFile[100];     // name of tempfile.
    string mailSubjectText = "Subject: " + subject+"\n\n";
    string mailText = mailSubjectText +texttosend+"\n\n";
    //string mailText = texttosend+"\n\n";
    string filename = config->getPathOsmoseLinux() + "mail/osmose.mail";



    strcpy(tempFile,filename.c_str()); // generate temp file name.

    FILE *fp = fopen(tempFile,"w"); // open it for writing.
    fprintf(fp,"%s\n",mailText.c_str());        // write body to it.
    fclose(fp);             // close it.

    sprintf(cmd,"mail %s < %s &",adr.c_str(),tempFile); // prepare command.
    	system(cmd);     // execute it.
    	//sendEmail(adr, mailSubjectText, mailText);
    }catch(...){
    	cout << "Mail ERROR thread" << endl;

    }
}


