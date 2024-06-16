/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientBuildResponse.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcutte <rcutte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/12 12:57:31 by purmerinos        #+#    #+#             */
/*   Updated: 2024/06/14 19:48:20 by rcutte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Server.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdlib>

void Client::_generateContentExtension(string& path) {
	path.erase(0, 1);
	if (path == "" || path.find("/") != path.npos) {
		path = "text/plain";
	} else if (path == "jpg" || path == "jpeg" || path == "png"
			|| path == "avif" || path == "webp" ) {
		path.insert(0, "image/");
	} else {
		path.insert(0, "text/");
	}
	_responseHeader.insert(pair<string, string>("Content-type: ", path));
}

bool	Client::_checkExtensionMatch(const string& extension) {
	const map<string, string>::const_iterator it = _headerFields.find("accept");
	if (it == _headerFields.end()) {
		if (extension != "text/plain" && extension != "text/html") {
			return false;
		} else {
			return true;
		}
	}
	// cout << it->second << endl;
	if (it->second.find("*/*") != it->second.npos || it->second.find(extension) != it->second.npos) {
		return true;
	} else {
		return false;
	}
}

void Client::_noBodyResponseDriver(const int status, const string& optionalBody, bool isFatal) {
	switch (status) {
		case 201 :
			_buildNoBodyResponse("201", " Created", "Data Successefully Uploaded", isFatal);
			break ;
		case 204 :
			_buildNoBodyResponse("204", " No content", "No content to display", isFatal);
			break ;
		case 400 :
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", isFatal);
			break ;
		case 403 :
			_buildNoBodyResponse("403", " Forbidden", "Access to the ressource is forbidden", isFatal);
			break ;
		case 404 :
			_buildNoBodyResponse("404", " Not Found", "Oops ! It seems there's nothing available here ...", isFatal);
			break ;
		case 406 :
			_buildNoBodyResponse("406", " Not Acceptable", optionalBody, isFatal);
			break ;
		case 409 :
			_buildNoBodyResponse("409", " Conflict", "Conflict between the current state of the ressource and the asked one", isFatal);
			break ;
		case 413 :
			_buildNoBodyResponse("413", " Content Too Large", "Message Body is too large for the server configuration", isFatal);
			break ;
		case 414 :
			_buildNoBodyResponse("414", " Uri Too Loong", "Uri exceed max size", isFatal);
			break;
		case 415 :
			_buildNoBodyResponse("415", " Unsupported Media Type", "Only chunked encoding is allowed", isFatal);
			break ;
		case 426 :
			_buildNoBodyResponse("426", " Upgrade Required", "This service require use of HTTP/1.1 protocol", isFatal);	
		case 500 :
			_buildNoBodyResponse("500", " Internal Server Error", "Sorry, it looks like something went wrong\
on our side ... Maybe try refresh the page ?", isFatal);
			break ;
		case 501 :
			_buildNoBodyResponse("501", " Not Implemented", "Requested method isn't implemented", isFatal);
		case 504 :
			_buildNoBodyResponse("504", "Gateway Timeout", "Timeout occurs while executing CGI", isFatal);
			break ;
		case 505 :
			_buildNoBodyResponse("505", " HTTP Protocol not supported", "Server protocol is HTTP/1.1", isFatal);
			break ;
		default :
			break;
	}
}

void Client::_buildNoBodyResponse(string status, string info, string body, bool isFatal) {
	bool		customPageIsPresent = false;
	if (_configServer != NULL) {
		const string	customPage = _configServer->get_error_page(strtol(status.c_str(), NULL, 10));
		if (customPage != "") {
			customPageIsPresent = _loadCustomStatusPage(customPage);
		}
	}
	if (customPageIsPresent == false) {
		_bodyStream << "<!doctype html><title>" << status << info << "</title><h1>"
			<< info << "</h1><p>" << body << "</p>";
		stringstream size;
		size << _bodyStream.str().size();
		_responseHeader.insert(pair<string, string>("Content-length: ", size.str()));
		_responseHeader.insert(pair<string, string>("Content-type: ", "text/html"));
	}
	_responseHeader.insert(pair<string, string>("Date: ", getDate()));
	_fillResponse(status + info, isFatal);
}

bool Client::_loadCustomStatusPage(string path) {
	struct stat buf;
	if (stat(path.c_str(), &buf) != 0) {
		return (0);
	}
	else if (!S_ISREG(buf.st_mode) == false) {
		return (0);
	}
	ifstream customPage;
	customPage.open(path.c_str());
	if (customPage.fail()) {
		return (0);
	}
	string extension = path.substr(path.find_last_of("/", path.npos));
	_generateContentExtension(extension);
	if (_checkExtensionMatch(extension) == false) {
		return (0);
	}
	_bodyStream << customPage.rdbuf();
	customPage.close();
	stringstream size;
	size << buf.st_size;
	_responseHeader.insert(pair<string, string>("Content-length: ", size.str()));
	return (1);
}

void	Client::_fillResponse( string status, bool shouldClose ) {
	cout << "In fill response" << endl;
	// for (map<string, string>::iterator it = _headerFields.begin(); it != _headerFields.end(); ++it) {
	// 	cout << "New Header : " << it->first << ":" << it->second << endl;
	// }
	const map<string, string>::const_iterator it = _headerFields.find("connection");
	if (it == _responseHeader.end()) {
		shouldClose = true;
	} else if (it->second.compare("keep-alive") != 0) {
		shouldClose = true;
	}
	if (_responseHeader.find("Connection") == _responseHeader.end()) {
		if (shouldClose == true) {
			_responseHeader.insert(pair<string, string>("Connection: ", "close"));
		} else {
			_responseHeader.insert(pair<string, string>("Connection: ", "Keep-Alive"));
			_responseHeader.insert(pair<string, string>("Keep-Alive: ", "timeout=5, max=1"));
		}
	}
	_response << "HTTP/1.1 " << status << "\r\n";
	if (_configServer != NULL) {
		_response << "Server: " << _configServer->get_name() << "\r\n";
	}
	for (map<string, string>::iterator it = _responseHeader.begin(); it != _responseHeader.end(); ++it) {
		_response << it->first << it->second << "\r\n";
	} _response << "\r\n";
	_response << _bodyStream.rdbuf();
	_response << "\r\n";
	_responseIsReady = true;
	_connectionShouldBeClosed = shouldClose;
	_status = WRITING;
	_mainEventLoop.modifyFdOfInterest(_connectionEntry, EPOLLOUT);
}

void	Client::_sendAnswer( void ) {
	const size_t writeValue = write(_connectionEntry, _response.str().c_str(), _response.str().size());
	if (writeValue != _response.str().size() || _connectionShouldBeClosed == true) {
		// cout << writeValue << "   " << _response.str().size() << boolalpha << _connectionShouldBeClosed << endl;
		// cout << "gneuuuuu" << endl;
		throw CloseMeException();
	}
	_requestLine.cgiQuery.clear();
	_requestLine.filePath.clear();
	_requestLine.method.clear();
	_header.clear();
	_body.clear();
	_headerFields.clear();
	_response.clear();
	_bodyStream.clear();
	_responseHeader.clear();
	_requestLine.protocol = 0.;
	_status = IDLE;
	_bytesReadFromBody = 0;
	_responseIsReady = false;
	_connectionShouldBeClosed = true;
	_headerIsFullyRed = false;
	_bodyIsFullyRed = false;
	_bodyIsPresent = false;
	_requestIsChunked = false;
	_contentLength = 0;
	_cgiIsRunning = false;
	_requestIsHandledByCgi = false;
	_cgiBinPath = "";
	_locationBlockForTheRequest = NULL;
	if (_cgiInfilePath != "") {
		remove(_cgiInfilePath.c_str());
		_cgiInfilePath = "";
	}
	if (_cgiOutFilePath != "") {
		remove(_cgiOutFilePath.c_str());
		_cgiOutFilePath = "";
	}
}