#include <iostream>
#include <thallium.hpp>
//#include <boost/asio.hpp>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <thallium/serialization/stl/string.hpp>
#include <unistd.h>
#ifndef STAT_H
#define STAT_H 
#include <sys/stat.h>
#endif
#include <custom_struct.hpp>



#define CONF_FILENAME "server.conf"

namespace tl = thallium;

// get own local ip address and port from config
std::string read_conf() {
    std::ifstream input_file(CONF_FILENAME);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open configuration file.");
    }

    std::string first_line;
    if (!std::getline(input_file, first_line)){
        throw std::runtime_error("Error reading server.conf.");
    }
    // std::cout << first_line << std::endl;
    size_t pos = first_line.find("OWN_ADDR: ");
    if (pos == std::string::npos) {
            throw std::runtime_error("Error parsing server address");
    }
    
    std::string server_addr = first_line.substr(pos + 10); // 10 is the length of "OWN_ADDR: "
    // std::cout << server_addr << std::endl;

    input_file.close();

    return server_addr;
}

void hello(const tl::request& req) {
    (void)req;
    //std::cout << "Hello World!" << std::endl;
}

void remote_open(const tl::request& req, std::string &file_name) {
    // std::cout << file_name << std::endl;
    // std::string file_name_str = file_name;
    // std::cout << file_name << std::endl;
    const char* char_arr = ("data/" + file_name).c_str();
    // std::cout << char_arr << std::endl;
    int fd = open(char_arr, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);  //TODO test with new folders & Permisssions ?
    // std::cout << fd << std::endl;
    req.respond(fd);
}

void remote_close(const tl::request& req, int fd) {
    int ret = close(fd);
    req.respond(ret);
}

void remote_unlink(const tl::request& req, std::string &file_name) {
    const char* path_char = ("data/" + file_name).c_str();
    int ret = unlink(path_char);
    req.respond(ret);
}


void remote_stat(const tl::request& req, std::string &file_name) {
    // std::cout << "custom struct after passing to server: " << serializable_struct.cst_mode << std::endl;
    const char* file_path = ("data/" + file_name).c_str();
    struct stat sb;
    int ret = stat(file_path, &sb);
    // std::cout << "read out: " << sb.st_ctim.tv_nsec << std::endl;
    custom_struct ret_struct = custom_struct(sb,ret);
    req.respond(ret_struct);
}

/*
void remote_pwrite(const tl::request& req, int fd, std::string to_write, size_t count, off_t offset) {
    //int is_open = fcntl(fd, F_GETFD);
    const void* buf =  reinterpret_cast<const void*>(to_write.c_str());
    int ret = pwrite(fd,buf,count,offset);
    if (ret == -1){
        perror("pwrite");
    }
    req.respond(ret);
}
*/


/*
void remote_pread(const tl::request& req, int fd, size_t count, off_t offset) {
    //int is_open = fcntl(fd, F_GETFD);
    char* buf;
    int ret = pread(fd,&buf,count,offset);
    //if (ret == -1){
    //    perror("pwrite");
    //}
    std::string ret_str = buf;
    req.respond(ret);
}
*/

int main() {
 
    tl::engine myEngine(read_conf(), THALLIUM_SERVER_MODE);
    std::cout << "Server running at address: " << myEngine.self() << std::endl;
    myEngine.define("h", hello).disable_response();
    myEngine.define("o", remote_open);
    myEngine.define("c", remote_close);
    myEngine.define("u", remote_unlink);
    myEngine.define("s", remote_stat);
    // pocus ;)


    std::function<void(const tl::request&, int, tl::bulk&, size_t, off_t)> remote_pwrite = 
        [&myEngine](const tl::request& req, int fd, tl::bulk& write_bulk, size_t count, off_t offset){
            //const void* buf =  reinterpret_cast<const void*>(to_write.c_str());

            // std::cout << "pwrite" << std::endl;y

            tl::endpoint ep = req.get_endpoint();
            //std::vector<char> v(count);
            auto v = std::make_unique<char[]>(count);
            std::vector<std::pair<void*,std::size_t>> segments(1);
            segments[0].first = static_cast<void*>(v.get());
            segments[0].second = count;
            tl::bulk local = myEngine.expose(segments, tl::bulk_mode::write_only);
            write_bulk.on(ep) >> local; 


            // for(auto c : v) std::cout << c; 
            // std::cout << std::endl;
            
            
            int ret = pwrite(fd,segments[0].first,count,offset);
            req.respond(ret);
            // std::cout << "pwrite done" << std::endl;
        };
    myEngine.define("w", remote_pwrite);


    std::function<void(const tl::request&, int, tl::bulk&, size_t, off_t)> remote_pread = 
    [&myEngine](const tl::request& req, int fd, tl::bulk& read_bulk, size_t count, off_t offset){    
        // std::cout << "pr" << std::endl;
        tl::endpoint ep = req.get_endpoint();

        auto buf = std::make_unique<char[]>(count);

        //char buf[count];
        int ret = pread(fd,buf.get(),count,offset);
        // std::cout << "ret " << ret << std::endl;
        // std::cout << "buf " << &buf << std::endl; 

        //std::string buffer(buf);
        // std::cout << "buffer " << buffer << std::endl;        

        std::vector<std::pair<void*,std::size_t>> segments(1);
        segments[0].first = static_cast<void*>(buf.get());
        segments[0].second = count;
        // std::cout << "segments" << std::endl;

        tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);
        // std::cout << "myBulk" << std::endl;

        read_bulk.on(ep) << myBulk;
        // std::cout << "myBulk >> read_bulk" << std::endl;

        req.respond(ret);
    };
    myEngine.define("r", remote_pread);

    return 0;
}