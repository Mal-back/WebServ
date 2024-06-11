/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PortListener.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: purmerinos <purmerinos@protonmail.com>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/04 14:07:54 by purmerinos        #+#    #+#             */
/*   Updated: 2024/06/04 14:07:55 by purmerinos       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"
#include <PortListener.hpp>
#include <EventLoop.hpp>
#include <Client.hpp>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>

PortListener::PortListener( void ) {
	return;
}

PortListener::~PortListener( void ) {
	for(map<string, Server *>::iterator it = _serverMap.begin();
			it != _serverMap.end(); ++it) {
		delete it->second;
	}
	for(map<int, Client *>::iterator it = _clientMap.begin();
			it != _clientMap.end(); ++it) {
		delete it->second;
	}
	return ;
}

int PortListener::getSocketFd( void ) const {
	return(_socketFd);
}

const string& PortListener::getListeningPort( void ) const {
	return(_listeningPort);
}

void PortListener::setMainEventLoop(EventLoop* ptr) {
	this->_mainEventLoop = ptr;
	return ;
}

void PortListener::initSocket( void ) {
	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd < 0) {
		throw runtime_error(strerror(errno));
		return;
	}

	struct addrinfo		hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo	*res;
	int	gai_error = getaddrinfo(NULL, _listeningPort.c_str(), &hints, &res);

	if (gai_error != 0) {
		throw runtime_error(gai_strerror(gai_error));
		return;
	}
	// This is only so kernel don't block the port for this socket only.
	int reUse = 1;
	setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &reUse, sizeof(reUse));

	// Assign the socket to the address and port we define with the structure.
	for (_address = res; _address != NULL; _address = _address->ai_next) {
		if (bind(_socketFd, _address->ai_addr, _address->ai_addrlen) == 0) {
			break;
		}
	}
	if (_address == NULL) {
		freeaddrinfo(res);
		res = NULL;
		close(_socketFd);
		throw runtime_error(strerror(errno));
	}

	// Reserve the port and start listening to it.
	if (listen(_socketFd, 4096) < 0) {
		throw runtime_error(strerror(errno));
		close(_socketFd);
		return;
	}	
}

void	PortListener::_acceptConnection( void ) {

	int clientFd = accept(this->_socketFd, this->_address->ai_addr,
			(socklen_t *)&this->_address->ai_addrlen);
	if (clientFd < 0) {
		cerr << "Failed connection to port" << _listeningPort << ":" << strerror(errno) << endl;
	}
	try {
		_mainEventLoop->addFdOfInterest(clientFd, this, EPOLLIN);
	} catch (runtime_error& e) {
		cerr << "Epoll_ctl_failure : " << e.what() << " closing correspondant connection" << endl;
		close (clientFd);
		return ;
	}
	if (clientFd > 1000) {
		_writeMinimalAnswer(clientFd, "503", " Too Busy", "Server is too busy at the moment. try again later.");
	}
	Client* newClient;
	try {
		 newClient = new Client(clientFd, *this, *_mainEventLoop);	
			_clientMap.insert(pair<int, Client *>(clientFd, newClient));
	} catch (bad_alloc& e) {
		_writeMinimalAnswer(clientFd, "500", " Internal Server Error",
			"An internal Server error occured. Maybe you should try refresh the page ...");
	} catch (exception& e) {
		_writeMinimalAnswer(clientFd, "500", " Internal Server Error",
			"An internal Server error occured. Maybe you should try refresh the page ...");
		delete newClient;
	}
}

void PortListener::_writeMinimalAnswer( int fd, string status,
		string info , string body ) {
	string messageBody = "<!doctype html><title>" + status + info + "</title><h1>"
			+ info + "</h1><p>" + body + "</p>\r\n";	
	stringstream response;
	response << "HTTP/1.1 " << status << info << "\r\n";
	response << "Date : " << getDate() << "\r\n";
	response << "Content-Type: text/html" << "\r\n";
	response << "Content-Length: " << messageBody.size() << "\r\n";
	response << "Connection: close\r\n\r\n";
	response << messageBody;
	_immediateResponse.insert(pair<int, string>(fd, response.str()));
	_mainEventLoop->addFdOfInterest(fd, this, EPOLLOUT);
}

void	PortListener::_closeConnection(int fd) {
	map<int, Client *>::iterator it;
	this->_mainEventLoop->deleteFdOfInterest(fd);
	if ((it = _clientMap.find(fd)) != _clientMap.end()) {
		delete it->second;
	}
	_clientMap.erase(fd);
	close (fd);
}

const string*	PortListener::_thisNeedToSendAnswer( int fd ) {
	const map<int, string>::const_iterator it = _immediateResponse.find(fd);
	if (it == _immediateResponse.end()) {
		return (NULL);
	} else {
		return &(it->second);
	}
}

void	PortListener::_sendMinimalAnswer(int fd, string answer) {
	write(fd, answer.c_str(), answer.size());
	close(fd);
	_immediateResponse.erase(fd);
	_mainEventLoop->deleteFdOfInterest(fd);
}

void	PortListener::manageEvent(int fd) {
	map<int, Client*>::iterator it;
	const string*								immediateResponse;
	if (fd == _socketFd) {
		try {
			_acceptConnection();
		} catch (runtime_error& e) {
			cerr << "Connection Refused: " << e.what() << endl;
		}
	} else if ((immediateResponse = _thisNeedToSendAnswer(fd)) != NULL) {
		_sendMinimalAnswer(fd, *immediateResponse);
	} else {
		try {
			it->second->manageNewEvent();
		} catch (exception& e) {
			cerr << e.what() << endl;
			_closeConnection(fd);
		}
	}
	return ;
}

Server*	PortListener::getServer(const string& name) const {
	map<string, Server*>::const_iterator it = _serverMap.find(name);
	if (it == _serverMap.end()) {
		return (_serverMap.find(_defaultServer)->second);
	} return (it->second);
}

void PortListener::getTimeout(void) {
	time_t current;

	time(&current);
	for (map<int, Client*>::iterator it = _clientMap.begin();
			it != _clientMap.end(); ++it) {
		if (current - it->second->getLastInteractionTime() >= TIMEOUT) {	
			_closeConnection(it->first);
		}
	}
}
