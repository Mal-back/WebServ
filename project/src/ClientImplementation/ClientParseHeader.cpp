/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientParseHeader.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: purmerinos <purmerinos@protonmail.com>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/12 13:07:16 by purmerinos        #+#    #+#             */
/*   Updated: 2024/06/12 13:07:19 by purmerinos       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "utils.hpp"

void	Client::_parseHeader( void ) {
	string header_line, field_value, field_content;
	while (_header.size() != 0) {
		header_line = _header.substr(0, _header.find("\r\n"));
		_header.erase(0, _header.find("\r\n") + 2);
		substituteSpaces(header_line);
		if (header_line.find(":") == header_line.npos) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
			return ;
		}
		field_value = header_line.substr(0, header_line.find_first_of(":"));
		field_content = header_line.substr(header_line.find_first_of(":") + 1, header_line.npos);
		if (field_value.find_first_not_of(" ") != 0) {
			continue;
		}
		transform(field_value.begin(), field_value.end(), field_value.begin(), ::tolower);
		if (fieldValueHasForbiddenChar(field_value) == true) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
			return ;
		}
		field_content.erase(0, field_content.find_first_not_of(" "));
		field_content.erase(field_content.find_last_not_of(" ") + 1, field_content.npos);
		if (fieldContentHasForbiddenChar(field_content) == true) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
		}
		normalizeStr(field_content);
		_headerFields.insert(pair<string, string>(field_value, field_content));
	}
}

void	Client::_checkForChunkedRequest( void ) {
	const map<string, string>::const_iterator it = _headerFields.find("content-encoding"); 
	if (it == _headerFields.end()) {
		_requestIsChunked = false;
		return ;
	}
	if (it->second != "chunked") {
		_buildNoBodyResponse("415", " Unsupported Media Type", "Only chunked encoding is allowed", true);
	} else {
		_requestIsChunked = true;
	} return ;
}

void	Client::_checkContentLength( void ) {
	const map<string, string>::const_iterator it = _headerFields.find("content-length");
	if (it == _headerFields.end()) {
		_contentLength = 0;
		return ;
	} else if (_requestIsChunked == true) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
	}
	else if (it->second.size() > 11) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
	}
	char *endptr;
	_contentLength = strtol(it->second.c_str(), &endptr, 10);
	if (*endptr != '\0' || _contentLength < 0) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
	}// else if (_contentLength >= _configServer->getMaxBodySize) {
	// 		_buildNoBodyResponse("413", " Content Too Large", "Message Body is too large for the server configuration", true);
	// }
}

void Client::_parseChunkedRequest(string requestPart) {
	string	chunk_size, chunk_content;
	size_t	num_size;
	char		*endptr;
	while (requestPart.size() != 0) {
		chunk_size = requestPart.substr(0, requestPart.find("\r\n"));
		if (requestPart.size() > 8) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
			break ;
		}
		requestPart.erase(0, chunk_size.size() + 2);
		num_size = strtol(chunk_size.c_str(), &endptr, 16);
		// if (num_size >= _configServer->getMaxBodySize()) {
		// 	_buildNoBodyResponse("413", " Content Too Large", "Message Body is too large for the server configuration", true);
		// 	break ;
	//	}
		if (*endptr != '\0') {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
			break ;
		} else if (num_size == 0) {
			if (requestPart.find("\r\n\r\n") != 0) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
			} else {
				_bodyIsFullyRed = true;
				break ;
			}
		}
		if (requestPart.find("\r\n") != num_size) {
			_buildNoBodyResponse("400", " BadRequest", "Syntax error or ambiguous request", true);
		}
		_body += requestPart.substr(0, num_size);
		requestPart.erase(0, num_size + 2);
	}
	return ;
}
