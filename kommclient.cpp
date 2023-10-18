
#include <kommclient.h>


namespace tl = thallium;

std::vector<std::string> server_list;
tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);

std::map<int,std::pair<char*, int>> ufd_dictonary; // ufd, (path, fd)
unsigned int server_number;


std::vector<std::string> read_conf() {
    std::ifstream input_file(CONF_FILENAME);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open configuration file.");
    }

    server_number = 0;
    std::string first_line;
    if (!(std::getline(input_file, first_line) &&
          (std::sscanf(first_line.c_str(), "NUMBER_OF_SERVERS = %u", &server_number) == 1))) {
        throw std::runtime_error("Error parsing NUMBER_OF_SERVERS.");
    }

    std::cout << "Server Number: " << server_number << std::endl;
    std::vector<std::string> server_list;

    for (unsigned int i = 0; i < server_number; i++) {
        std::string current_line;
        if (!std::getline(input_file, current_line)) {
            throw std::runtime_error("Error reading server address from configuration file.");
        }

        // Assuming the format is "ADDR: server_address"
        size_t pos = current_line.find("ADDR: ");
        if (pos == std::string::npos) {
            throw std::runtime_error("Error parsing server address in line " + std::to_string(i + 2));
        }

        std::string server_address = current_line.substr(pos + 6); // 6 is the length of "ADDR: "
        server_list.push_back(server_address);
        std::cout << "Server Address: " << server_address << std::endl;
    }

    input_file.close();

    return server_list;
}





std::string hash_server(const char* path){ 
    int hash = 0;
    while(*path){
        hash += *path++;
    }
    return server_list[hash%server_number];
}


std::pair<char*, int> ufd_lookup(int ufd){
    auto it = ufd_dictonary.find(ufd);
    if (it != ufd_dictonary.end()) {
        const std::pair<char*, int>& data = it->second;
        return std::make_pair(data.first, data.second);
    }
    else {
        perror("ufd matching error");
        return std::make_pair(const_cast<char*>(""),-1);
    }
}

int calculate_ufd(int fd, const char* path) {
    std::string ufd_str = std::to_string(fd) + path;
    int ufd = std::hash<std::string>{}(ufd_str);
    return ufd;
}

int get_ufd(int fd, const char* path){
    unsigned int ufd = calculate_ufd(fd, path);
    ufd_dictonary[ufd] = std::make_pair(const_cast<char*>(path), fd);
    return ufd;
}



int komm_open(const char* path){
    std::string s = hash_server(path);
    tl::endpoint server = myEngine.lookup(s);
    // std::cout << "server ip " << s << std::endl;
    tl::remote_procedure open = myEngine.define("o");
    // std::cout << "o" << std::endl;
    int ret_fd = open.on(server)((std::string) path);
    // std::cout << "open.on" << std::endl;
    int ufd = get_ufd(ret_fd, path);
    // std::cout << "ufd: " << ufd << std::endl;
    return ufd;
}

int komm_stat(const char* path, struct stat* statbuf){
    tl::endpoint server = myEngine.lookup(hash_server(path));
    tl::remote_procedure stat = myEngine.define("s");
    // receive struct stat from server (not int anymore)
    custom_struct ret_stat = stat.on(server)((std::string) path);
    struct stat temp_stat = ret_stat.get_struct();
    statbuf = &temp_stat;
    return ret_stat.return_value;
}

int komm_unlink(const char* path){
    tl::endpoint server = myEngine.lookup(hash_server(path));
    tl::remote_procedure unlink = myEngine.define("u");
    int ret_unlink = unlink.on(server)((std::string) path);
    return ret_unlink;
}

int komm_close(int ufd){
    std::pair<char*, int> sercon = ufd_lookup(ufd);
    tl::endpoint server = myEngine.lookup(hash_server(sercon.first));
    tl::remote_procedure close = myEngine.define("c");
    int ret_close = close.on(server)(sercon.second);
    return ret_close;
}

ssize_t komm_pwrite(int ufd, const void* buf, size_t count, off_t offset) {
    std::pair<char*, int> sercon = ufd_lookup(ufd);
    tl::endpoint server = myEngine.lookup(hash_server(sercon.first));
    auto buffer = const_cast<void*>(buf);
    std::vector<std::pair<void*,std::size_t>> segments(1);
    segments[0].first = buffer;
    segments[0].second = count;

    tl::remote_procedure pwrite = myEngine.define("w");
    tl::bulk myBulk = myEngine.expose(segments, tl::bulk_mode::read_only);
    int ret_pwrite = pwrite.on(server)(sercon.second, myBulk, count, offset);
    return ret_pwrite;
}

ssize_t komm_pread(int ufd, void* buf, size_t count, off_t offset) {
    std::pair<char*, int> sercon = ufd_lookup(ufd);
    tl::endpoint server = myEngine.lookup(hash_server(sercon.first));
    std::vector<std::pair<void*,std::size_t>> segments2(1);
    segments2[0].first = buf;
    segments2[0].second = count;
    tl::bulk read_bulk = myEngine.expose(segments2, tl::bulk_mode::write_only);

    tl::remote_procedure pread = myEngine.define("r");
    int ret_pread = pread.on(server)(sercon.second, read_bulk, count, offset);

    return ret_pread;
}



