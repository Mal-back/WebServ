/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcutte <rcutte@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/11 12:12:31 by rcutte            #+#    #+#             */
/*   Updated: 2024/06/14 19:18:31 by rcutte           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_H_
# define SERVER_H_

# include <iostream>
# include <fstream>
# include <iomanip>
# include <stdexcept>
# include <algorithm>
# include <string>
# include <vector>
# include <list>
# include <map>

# include "color.h"
# include "PortListener.hpp"

using namespace std;

struct already_seen_location {
  bool redirect_;
  bool root_;
  bool index_;
  bool autoindex_;
  bool limit_except_;
  bool upload_path_;
  map<string,bool> cgi_;
};

struct access_seen {
  bool get_;
  bool post_;
  bool delete_;
};

struct already_seen_server {
  bool        name_;
  bool        port_;
  bool        max_client_body_size_;
  map<int,bool> error_pages_;
};

struct location {
  // Path to the location
  std::string path_;
  // vector(method) - GET by default (Limited to: GET, POST, DELETE)
  std::vector<std::string>            limit_except_;

  // Redirections
    // redirection false by default
  bool        redirect_;
  std::string redirect_path_;

  // Path infos

  std::string                         root_;
  std::string                         index_;
    // autoindex - directory listing false by default
  bool                                autoindex_;
  std::string                         upload_path_;
    // map(extension, absolue_path_to_bin)
  std::map<std::string, std::string>  cgi_;
};

class Server
{
private:
  /* Core values */

  std::string name_;
  int         port_;

  /* Additional values */

  size_t                              max_client_body_size_;
    // map(error_code, error_page_absolute_path)
  std::map<int, std::string>          error_pages_;

  /* Locations values */
    // map(path, location struct)
  std::map<std::string, location> locations_;

public:
  Server();
  ~Server();

  // Getters

  std::string const &get_name() const;
  int const         &get_port() const;
  size_t const      &get_max_client_body_size() const;
  
  std::map<std::string, location> const &get_locations() const;
  location const    *get_location(std::string const &path) const;
  map<int,string> const &get_error_pages() const;
  string const      get_error_page(int const &error_code) const;

  bool              is_redirect(std::string const &path) const;
  string const      get_redirect_path(std::string const &path) const;

  string const      get_root(std::string const &path) const;
  string const      get_index(std::string const &path) const;
  bool              is_autoindex(std::string const &path) const;
  string const      get_upload_path(std::string const &path) const;
  bool              is_allowed_method(std::string const &path, std::string const &method) const;
  vector<string>    get_limit_except(std::string const &path) const;
  bool              is_cgi_extension(std::string const &path, std::string const &extension) const;
  string const      get_cgi_path(std::string const &path, std::string const &extension) const;


  // Setters

  void set_name(std::string const &name);
  void set_port(int const &port);
  void set_max_client_body_size(int const &port);

  // Functions

  location SearchLocation(std::string const &path) const;
  void AddLocation(std::string const &path, location const &loc);
  void AddError_page(int const &error_code, std::string const &error_page);
};

std::ostream& operator<<(std::ostream& os, const Server& obj);

std::ostream& operator<<(std::ostream& os, const location& obj);

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  os << "[";
  for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
    if (it != vec.begin()) os << ", ";
    os << *it;
  }
  os << "]";
  return os;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& map);

#endif
