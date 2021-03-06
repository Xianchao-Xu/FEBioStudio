/*This file is part of the FEBio Studio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio-Studio.txt for details.

Copyright (c) 2020 University of Utah, The Trustees of Columbia University in 
the City of New York, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "stdafx.h"
#include "RepoConnectionHandler.h"

#ifdef MODEL_REPO
#include <QEventLoop>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>
#include <QByteArray>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QUrl>
#include <QDir>
#include <QList>
#include <QSaveFile>
#include <QFileDialog>
#include <QTcpSocket>
#include "DatabasePanel.h"
#include "MainWindow.h"
#include "Document.h"
#include "LocalDatabaseHandler.h"
#include "ZipFiles.h"
#include <iostream>

class CRepoConnectionHandler::Imp
{
public:
	Imp(CDatabasePanel* dbPanel, CLocalDatabaseHandler* dbHandler, CRepoConnectionHandler* handler, CMainWindow* wnd)
		: dbPanel(dbPanel), dbHandler(dbHandler), m_wnd(wnd), uploadPermission(0)
	{
		restclient = new QNetworkAccessManager(handler);

//		QSslSocket *socket = new QSslSocket;
//		socket->addCaCertificates( "/home/sci/mherron/Desktop/pythonDaemon/cert.pem" );
//
//		conf = new QSslConfiguration;
//		conf->setCaCertificates( socket->caCertificates() );
	}

	~Imp()
	{
		delete restclient;
	}

	CDatabasePanel* dbPanel;
	CLocalDatabaseHandler* dbHandler;
	CMainWindow* m_wnd;

//	QSslConfiguration* conf;
	QNetworkAccessManager* restclient;

	QString username;
	QString token;
	int uploadPermission;

};


CRepoConnectionHandler::CRepoConnectionHandler(CDatabasePanel* dbPanel, CLocalDatabaseHandler* dbHandler, CMainWindow* wnd)
{
	imp = new Imp(dbPanel, dbHandler, this, wnd);

	connect(imp->restclient, SIGNAL(finished(QNetworkReply*)), this, SLOT(connFinished(QNetworkReply*)));
	connect(imp->restclient, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)), this, SLOT(sslErrorHandler(QNetworkReply*, const QList<QSslError>&)));
}

CRepoConnectionHandler::~CRepoConnectionHandler()
{
	delete imp;
}

void CRepoConnectionHandler::authenticate(QString userName, QString password)
{
	imp->username = userName;

	QVariantMap feed;
	feed.insert("username", userName);
	feed.insert("password", password);
	QByteArray payload=QJsonDocument::fromVariant(feed).toJson();

	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath("/modelRepo/api/v1.0/authenticate");

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	imp->restclient->setNetworkAccessible(QNetworkAccessManager::Accessible);

	if(NetworkAccessibleCheck())
	{
		imp->restclient->post(request, payload);
	}

}

void CRepoConnectionHandler::getSchema()
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath("/modelRepo/api/v1.0/schema");

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());

	if(NetworkAccessibleCheck())
	{
		imp->restclient->get(request);
	}
}

void CRepoConnectionHandler::getTables()
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath("/modelRepo/api/v1.0/tables");

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());

	if(NetworkAccessibleCheck())
	{
		imp->restclient->get(request);
	}

}

void CRepoConnectionHandler::getFile(int id, int type)
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath(QString("/modelRepo/api/v1.0/files/%1/%2").arg(type).arg(id));

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());

	if(NetworkAccessibleCheck())
	{
		QNetworkReply* reply = imp->restclient->get(request);

		QObject::connect(reply, &QNetworkReply::downloadProgress, this, &CRepoConnectionHandler::progress);

		imp->m_wnd->ShowProgress(true, "Downloading...");
	}

}

//void CRepoConnectionHandler::upload(QByteArray projectInfo)
//{
//	QUrl myurl;
//	myurl.setScheme("https");
//	myurl.setHost("omen.sci.utah.edu");
//	myurl.setPort(4433);
//	myurl.setPath("/modelRepo/api/v1.0/upload");
//
//	QNetworkRequest request;
//	request.setUrl(myurl);
//	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
//	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());
//	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//
//	if(NetworkAccessibleCheck())
//	{
//		imp->restclient->post(request, projectInfo);
//	}
//
//}

void CRepoConnectionHandler::uploadFileRequest(QByteArray projectInfo)
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath("/modelRepo/api/v1.0/uploadFileRequest");

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	if(NetworkAccessibleCheck())
	{
		imp->restclient->post(request, projectInfo);
	}
}

void CRepoConnectionHandler::uploadFile(QString fileToken)
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath("/modelRepo/api/v1.0/uploadFile");

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());
	request.setRawHeader(QByteArray("fileToken"), fileToken.toUtf8());

	string fileName = imp->m_wnd->GetDocument()->GetDocFolder() + "/.projOutForUpload.prj";

	if(NetworkAccessibleCheck())
	{
		archive(fileName.c_str(), QDir(imp->m_wnd->GetDocument()->GetDocFolder().c_str()));

		QFile* arch = new QFile(fileName.c_str());
		arch->open(QIODevice::ReadOnly);

		QNetworkReply* reply = imp->restclient->post(request, arch);
		arch->setParent(reply);
	}

}

void CRepoConnectionHandler::modifyProject(int id, QByteArray projectInfo)
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath(QString("/modelRepo/api/v1.0/projects/%1").arg(id));

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	if(NetworkAccessibleCheck())
	{
		imp->restclient->put(request, projectInfo);
	}
}

void CRepoConnectionHandler::deleteProject(int id)
{
	QUrl myurl;
	myurl.setScheme("https");
	myurl.setHost("omen.sci.utah.edu");
	myurl.setPort(4433);
	myurl.setPath(QString("/modelRepo/api/v1.0/projects/%1").arg(id));

	QNetworkRequest request;
	request.setUrl(myurl);
	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	if(NetworkAccessibleCheck())
	{
		imp->restclient->deleteResource(request);
	}
}

void CRepoConnectionHandler::connFinished(QNetworkReply *r)
{
	QString URL = r->request().url().toString();

	if(URL.contains("authenticate"))
	{
		authReply(r);
	}
//	else if(URL.contains("authCheck"))
//	{
//		authReply(r);
//	}
	//	else if(URL.contains("models"))
//	{
//		modelListReply(r);
//	}
	else if(URL.contains("schema"))
	{
		getSchemaReply(r);
	}
	else if(URL.contains("files/"))
	{
		getFileReply(r);
	}
	else if(URL.contains("uploadFileRequest"))
	{
		uploadFileRequestReply(r);
	}
	else if(URL.contains("uploadFile"))
	{
		uploadFileReply(r);
	}
//	else if(URL.contains("upload"))
//	{
//		uploadReply(r);
//	}
	else if(URL.contains("tables"))
	{
		getTablesReply(r);
	}
	else if(URL.contains("projects"))
	{
		if(r->operation() == QNetworkAccessManager::PutOperation)
		{
			modifyProjectRepy(r);
		}
		else if(r->operation() == QNetworkAccessManager::DeleteOperation)
		{
			deleteProjectRepy(r);
		}
	}

}

void CRepoConnectionHandler::sslErrorHandler(QNetworkReply *reply, const QList<QSslError> &errors)
{
	for(QSslError error : errors)
	{
		cout << error.errorString().toStdString() << endl;
	}

	reply->ignoreSslErrors();

}

void CRepoConnectionHandler::progress(qint64 bytesReceived, qint64 bytesTotal)
{
	cout << "Total: " << bytesTotal << endl;
	cout << "Received: " << bytesReceived << endl;
	cout << (float)bytesReceived/(float)bytesTotal*100 << "%" << endl;

	imp->m_wnd->UpdateProgress((float)bytesReceived/(float)bytesTotal*100);
}

bool CRepoConnectionHandler::NetworkAccessibleCheck()
{
/*	if(imp->restclient->networkAccessible() == QNetworkAccessManager::Accessible)
	{
		return true;
	}
	else
	{
		imp->dbPanel->NetworkInaccessible();

		return false;
	}
*/
	return true;
}

//bool CRepoConnectionHandler::AuthCheck()
//{
//	QUrl myurl;
//	myurl.setScheme("https");
//	myurl.setHost("omen.sci.utah.edu");
//	myurl.setPort(4433);
//	myurl.setPath("/modelRepo/api/v1.0/authCheck");
//
//	QNetworkRequest request;
//	request.setUrl(myurl);
//	request.setRawHeader(QByteArray("userName"), imp->username.toUtf8());
//	request.setRawHeader(QByteArray("token"), imp->token.toUtf8());
//	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//
//	imp->restclient->get(request);
//
//	QEventLoop loop;
//	connect( this, &CRepoConnectionHandler::authCheckDone, &loop, &QEventLoop::quit );
//	loop.exec();
//
//	return imp->authorized;
//}

void CRepoConnectionHandler::authReply(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		QJsonDocument jsonDoc = QJsonDocument::fromJson(r->readAll());
		QJsonObject obj = jsonDoc.object();

		imp->token = obj.value("authToken").toString();
		imp->uploadPermission = obj.value("uploader").toInt();

		// Store the authentication token
//		imp->token = (QString)r->readAll();

		// Send a request to get the repository schema
		getSchema();
	}
	else if(statusCode == 403)
	{
		QJsonDocument jsonDoc = QJsonDocument::fromJson(r->readAll());
		QJsonObject obj = jsonDoc.object();
		QString message = obj.value("message").toString();

		imp->dbPanel->ShowMessage(message);
	}
	else
	{
		QString message = "An unknown server error has occurred.\nHTTP Staus Code: ";
		message += std::to_string(statusCode).c_str();

		imp->dbPanel->ShowMessage(message);
	}

}

//void CRepoConnectionHandler::authCheckReply(QNetworkReply *r)
//{
//	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//
//	imp->authorized = (statusCode == 200);
//
//	cout << "authCheck response code: " << statusCode << endl;
//
//	emit authCheckDone();
//}


//void CRepoConnectionHandler::modelListReply(QNetworkReply *r)
//{
//	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//
//	std::cout << statusCode << std::endl;
////	std::cout << ((QString)r->readAll()).toStdString() << std::endl;
////	std::cout << ((QString)r->readAll()).toStdString() << std::endl;
//
//	QJsonDocument jsonDoc = QJsonDocument::fromJson(r->readAll());
//
////	imp->dbPanel->SetModelList(jsonDoc);
//
//}

void CRepoConnectionHandler::getSchemaReply(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		std::string schema = QString(r->readAll()).toStdString();

		imp->dbHandler->init(schema);

		getTables();
	}
	else if(statusCode == 403)
	{
		imp->dbPanel->LoginTimeout();
	}
	else
	{
		QString message = "An unknown server error has occurred.\nHTTP Staus Code: ";
		message += std::to_string(statusCode).c_str();

		imp->dbPanel->ShowMessage(message);
	}
}

void CRepoConnectionHandler::getTablesReply(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		QJsonDocument jsonDoc = QJsonDocument::fromJson(r->readAll());

		imp->dbHandler->update(jsonDoc);

		imp->dbPanel->SetModelList();
	}
	else if(statusCode == 403)
	{
		imp->dbPanel->LoginTimeout();
	}
	else
	{
		QString message = "An unknown server error has occurred.\nHTTP Staus Code: ";
		message += std::to_string(statusCode).c_str();

		imp->dbPanel->ShowMessage(message);
	}
}

void CRepoConnectionHandler::getFileReply(QNetworkReply *r)
{
	imp->m_wnd->ShowProgress(false);

	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		int fileID = r->rawHeader(QByteArray("fileID")).toInt();
		int IDType = r->rawHeader(QByteArray("IDType")).toInt();

		QString path = imp->dbPanel->GetRepositoryFolder() + "/";
		path += imp->dbHandler->FilePathFromID(fileID, IDType);

		QDir dir;
		dir.mkpath(path);

		QString filename = path + "/";
		filename += imp->dbHandler->FileNameFromID(fileID, IDType);

		QByteArray data = r->readAll();

		QSaveFile file(filename);
		file.open(QIODevice::WriteOnly);

		file.write(data);
		file.commit();

		imp->dbPanel->DownloadFinished(fileID, IDType);
	}
	else if(statusCode == 403)
	{
		imp->dbPanel->LoginTimeout();
	}
	else
	{
		QString message = "An unknown server error has occurred.\nHTTP Staus Code: ";
		message += std::to_string(statusCode).c_str();

		imp->dbPanel->ShowMessage(message);
	}


//	if(IDType == GLOBAL)
//	{
//		imp->m_wnd->ImportZippedProject(filename);
//	}
//	else
//	{
//		imp->m_wnd->OpenFEModel(filename);
//	}

//	QString fileName = QFileDialog::getSaveFileName(NULL, "Export", ".", "FEBio Studio Project (*.prj)");
//	if (fileName.isEmpty() == false)
//	{
//		// make sure the file has an extension
//		std::string sfile = fileName.toStdString();
//		std::size_t found = sfile.rfind(".");
//		if (found == std::string::npos) sfile.append(".prj");
//
//
//		QByteArray data = r->readAll();
//
//		QSaveFile file(sfile.c_str());
//		file.open(QIODevice::WriteOnly);
//
//		file.write(data);
//		file.commit();
//
////		imp->m_wnd->OpenFEModel("/home/sci/mherron/Desktop/test.zip");
//
//		imp->m_wnd->ImportZippedProject(sfile.c_str());
//
//	}

}

//void CRepoConnectionHandler::uploadReply(QNetworkReply *r)
//{
//	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//
//	if(statusCode == 200)
//	{
//		QString fileToken = r->readAll();
//
//		TCPUpload(fileToken);
//
//		getTables();
//
//		imp->dbPanel->SetModelList();
//	}
//	else if(statusCode == 403)
//	{
//		imp->dbPanel->LoginTimeout();
//	}
//	else
//	{
//		QString message = "An unknown server error has occurred.\nHTTP Staus Code: ";
//		message += std::to_string(statusCode).c_str();
//
//		imp->dbPanel->FailedLogin(message);
//	}
//
//}

void CRepoConnectionHandler::uploadFileRequestReply(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		QString fileToken = r->readAll();

		uploadFile(fileToken);

	}
	else if(statusCode == 403)
	{
		imp->dbPanel->LoginTimeout();
	}
	else
	{
		QString message = "An unknown server error has occurred.\nHTTP Staus Code: ";
		message += std::to_string(statusCode).c_str();

		imp->dbPanel->ShowMessage(message);
	}

}

void CRepoConnectionHandler::uploadFileReply(QNetworkReply *r)
{
	string fileName = imp->m_wnd->GetDocument()->GetDocFolder() + "/.projOutForUpload.prj";
	QFile::remove(fileName.c_str());

	getSchema();

	imp->dbPanel->SetModelList();
}

void CRepoConnectionHandler::modifyProjectRepy(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		getSchema();

		imp->dbPanel->SetModelList();
	}

	imp->dbPanel->ShowMessage(r->readAll());

}

void CRepoConnectionHandler::deleteProjectRepy(QNetworkReply *r)
{
	int statusCode = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if(statusCode == 200)
	{
		getSchema();

		imp->dbPanel->SetModelList();
	}

	imp->dbPanel->ShowMessage(r->readAll());

}

//void CRepoConnectionHandler::TCPUpload(QString fileToken)
//{
//	QTcpSocket socket;
//	socket.connectToHost("omen.sci.utah.edu", 50000);
//
//	socket.waitForReadyRead(5000);
//
//	QByteArray message = socket.readAll();
//
////	cout << message.toStdString() << endl;
//
//	socket.write(imp->token.toUtf8());
//
//	socket.waitForReadyRead(5000);
//
//	message = socket.readAll();
//
////	cout << message.toStdString() << endl;
//
//	socket.write(fileToken.toUtf8());
//
//	socket.waitForReadyRead(5000);
//
//	message = socket.readAll();
//
////	cout << message.toStdString() << endl;
//
//	string fileName = imp->m_wnd->GetDocument()->GetDocFolder() + "/.projOutForUpload.prj";
//
//	cout << "FileName: " << fileName << endl;
//
//	archive(fileName.c_str(), QDir(imp->m_wnd->GetDocument()->GetDocFolder().c_str()));
//
//	QFile arch(fileName.c_str());
//	arch.open(QIODevice::ReadOnly);
//
//	while(socket.write(arch.read(1024)))
//	{
//		socket.flush();
//	}
//
//	arch.close();
//
//	arch.remove();


//	QBuffer buff;
//	buff.open(QBuffer::ReadWrite);

//	QuaZip zip(&socket);
//	if (zip.getZipError() != 0) {
//			cout << "Zip error: " << zip.getZipError() << endl;
//		}
//
//	zip.setFileNameCodec("IBM866");
//	if (!zip.open(QuaZip::mdCreate)) {
//		cout << "Open Failed." << endl;
//		return;
//	}
//
//	QFile inFile;
//	QuaZipFile outFile(&zip);
//
//	QStringList sl;
//	QDir dir(imp->m_wnd->GetDocument()->GetDocFolder().c_str());
//	cout << "Dir: "<< imp->m_wnd->GetDocument()->GetDocFolder().c_str() << endl;
//
//	recurseAddDir(dir, sl);
//
//	QFileInfoList files;
//	foreach (QString fn, sl) files << QFileInfo(fn);
//
//	foreach(QFileInfo fileInfo, files) {
//
//		if (!fileInfo.isFile())
//			continue;
//
//		QString fileNameWithRelativePath = fileInfo.filePath().remove(0, dir.absolutePath().length() + 1);
//		cout << fileNameWithRelativePath.toStdString() << endl;
//
//		inFile.setFileName(fileInfo.filePath());
//
//		if (!inFile.open(QIODevice::ReadOnly)) {
//			cout << "Failed to open inFile" << endl;
//			return;
//		}
//
//		if(fileInfo.isSymLink())
//		{
//			if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.symLinkTarget()))) {
//				return;
//			}
//		}
//		else
//		{
//			if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileInfo.filePath()))) {
//				return;
//			}
//		}
//
//		while(outFile.write(inFile.read(1024)))
//		{
//				socket.flush();
//		}
//
//
//
////		while (inFile.getChar(&c) && outFile.putChar(c));
//
////		socket.write(buff.readAll());
////		socket.flush();
//
//		if (outFile.getZipError() != UNZ_OK) {
//			cout << "Out error: " << outFile.getZipError() << endl;
//		}
////
////		if (outFile.getZipError() != UNZ_OK) {
////			return;
////		}
//
//		outFile.close();
//
//		inFile.close();
//	}
//
////	while(socket.write(buff.read(1024)))
////	{
////		socket.flush();
////	}
//
//	zip.close();
//
//	if (zip.getZipError() != 0) {
//		cout << "Zip error: " << zip.getZipError() << endl;
//	}


//}

QString CRepoConnectionHandler::getUsername()
{
	return imp->username;
}

int CRepoConnectionHandler::getUploadPermission()
{
	return imp->uploadPermission;
}

#else

CRepoConnectionHandler::CRepoConnectionHandler(CDatabasePanel* dbPanel, CLocalDatabaseHandler* dbHandler, CMainWindow* wnd){}
CRepoConnectionHandler::~CRepoConnectionHandler(){}
void CRepoConnectionHandler::authenticate(QString userName, QString password){}
void CRepoConnectionHandler::getFile(int id, int type){}
//void CRepoConnectionHandler::upload(QByteArray projectInfo){}
void CRepoConnectionHandler::connFinished(QNetworkReply *r){}
void CRepoConnectionHandler::authReply(QNetworkReply *r){}
void CRepoConnectionHandler::getFileReply(QNetworkReply *r){}
//void CRepoConnectionHandler::uploadReply(QNetworkReply *r){}
//void CRepoConnectionHandler::TCPUpload(QString fileToken){}
void CRepoConnectionHandler::sslErrorHandler(QNetworkReply *reply, const QList<QSslError> &errors) {}
void CRepoConnectionHandler::progress(qint64 bytesReceived, qint64 bytesTotal) {}

#endif
