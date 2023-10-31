#ifndef CUSTOM_CLIENT_HPP
#define CUSTOM_CLIENT_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thallium.hpp>
#include <vector>
#include <thallium/serialization/stl/string.hpp>
#include <sys/stat.h>
#include <map>
#include "custom_struct.hpp" // Include your custom_struct.hpp here

#define CONF_FILENAME "client.conf"

namespace tl = thallium;

extern std::vector<std::string> server_list;
extern tl::engine myEngine;
extern std::map<int, std::pair<char*, int>> ufd_dictionary;
extern unsigned int server_number;

std::vector<std::string> read_conf();
std::string hash_server(const char* path);
std::pair<char*, int> ufd_lookup(int ufd);
int calculate_ufd(int fd, const char* path);
int get_ufd(int fd, const char* path);
int custom_open(const char* path);
int custom_stat(const char* path, struct stat* statbuf);
int custom_unlink(const char* path);
int custom_close(int ufd);
ssize_t custom_pwrite(int ufd, const void* buf, size_t count, off_t offset);
ssize_t custom_pread(int ufd, void* buf, size_t count, off_t offset);

#endif // CUSTOM_CLIENT_HPP
