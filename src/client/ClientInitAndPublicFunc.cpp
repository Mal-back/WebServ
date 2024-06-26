/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientInitAndPublicFunc.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcutte <rcutte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/05 19:47:21 by purmerinos        #+#    #+#             */
/*   Updated: 2024/06/17 17:03:15 by rcutte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "PortListener.hpp"
#include <Client.hpp>
#include <ctime>

Client::Client(int fd, PortListener& owner, EventLoop& eventLoop): _owner(owner)
	 ,_mainEventLoop(eventLoop), _connectionEntry(fd) {
	memset(_buffer, 0, BUFFER_SIZE);
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
	_requestIsHandledByCgi = false;
	_cgiIsRunning = false;
	_cgiBinPath = "";
	_locationBlockForTheRequest = NULL;
	_configServer = NULL;
	_lastInteractionTime = time(NULL);
	return ;
}

Client::~Client( void ) {
	if (_cgiInfilePath != "") {
		remove(_cgiInfilePath.c_str());
	}
	if (_cgiOutFilePath != "") {
		remove(_cgiOutFilePath.c_str());
	}
}

bool	Client::isCLientTimeout( void ) {
	const int timeSinceLastAction = time(NULL) - _lastInteractionTime;
	bool	ret = false;

	if (timeSinceLastAction >= TIMEOUT) {
		if (_cgiIsRunning == true) {
			_killCgi();
			_lastInteractionTime = time(NULL);
		} else {
			ret = true;
		}
	}
	return (ret);
}

void Client::manageNewEvent( void ) {
	if (_cgiIsRunning == true) {
		_checkCgiStatus();
		return ;
	}
	if (_responseIsReady == true) {
		_sendAnswer();
	} else {
		_readRequest();
	}
	time(&_lastInteractionTime);
}
