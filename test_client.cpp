#include <kommclient.h>
#include <thallium.hpp>
#include <mpi.h>
#include <chrono>


void bench1(int bench_rounds, int argc, char** argv) {
    double total_time = 0.0;
        
    MPI_Init(&argc, &argv);

    int my_rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::chrono::high_resolution_clock::time_point start_t, end_t;
    unsigned int tsum = 0, loop_rank = my_rank * 16777216;
    struct stat ts;

    // START THREAD TIME MEASSUREMENT
    MPI_Barrier(MPI_COMM_WORLD);
    start_t = std::chrono::high_resolution_clock::now();

    // 4 OPS PER LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        int tufd = komm_open(tf);
        komm_close(tufd);
        komm_stat(tf, &ts);
        komm_unlink(tf);
    }

    //STOP THREAD TIME MEASSUREMENT
    end_t = std::chrono::high_resolution_clock::now();
    MPI_Barrier(MPI_COMM_WORLD);

    std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
    double elapsed_time_int = elapsed_time.count();

    MPI_Reduce(&elapsed_time_int, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Finalize();
        
    std::cout << "THREAD: " << my_rank << ", THREAD TIME: " << elapsed_time.count() << std::endl;

    if (my_rank == 0) {
        std::cout << "TOTAL TIME: " << total_time << std::endl;
        std::cout << "TOTAL OPS: " << bench_rounds * size * 4 << std::endl; 
        std::cout << "CALC SPEED: " << bench_rounds * size * 4 / total_time << " ops/s" << std::endl;
    }
}


void bench2(int transfer_size_in_kib, int argc, char** argv) {
    double total_time = 0.0;
            
    MPI_Init(&argc, &argv);

    int my_rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // CREATE WRITE BULK
    std::string s_write_bulk_content(transfer_size_in_kib * 1024, 'A');
    const char* write_bulk_content = s_write_bulk_content.c_str();
    std::size_t write_bulk_size = std::strlen(write_bulk_content);

    // CREATE READ BULK
    void* read_bulk_content = new char[transfer_size_in_kib * 1024];
    
    // SETUP
    std::chrono::high_resolution_clock::time_point start_t, end_t;
    const char* f_path = (std::string("tft" + std::to_string(my_rank) + "c")).c_str();
    int ufd = komm_open(f_path);
    std::cout << f_path << std::endl;
    std::cout << ufd << std::endl;

    // START THREAD TIME MEASSUREMENT
    MPI_Barrier(MPI_COMM_WORLD);
    start_t = std::chrono::high_resolution_clock::now();  

    // WRITE BULK
    int write_ret = komm_pwrite(ufd, write_bulk_content, write_bulk_size, 0);

    
    // READ BULK
    int read_ret = komm_pread(ufd, read_bulk_content, write_bulk_size, 0);

    // STOP THREAD TIME MEASSUREMENT
    end_t = std::chrono::high_resolution_clock::now();
    MPI_Barrier(MPI_COMM_WORLD);
    
    // CLOSE, UNLINK 
    int close_ret = komm_close(ufd);
    int unlink_ret = komm_unlink(f_path);
    
    // COMPARE READ WRITE CONTENT SIZE
    if (write_ret == read_ret) {
        std::cout << "T" << my_rank << ": READ WRITE SIZE MATCH" << std::endl;
    }
    else {
        std::cout << "T" << my_rank << ": READ WRITE SIZE MISMATCH" << std::endl;
        std::cout << "T" << my_rank << ": READ SIZE " << read_ret << std::endl;
        std::cout << "T" << my_rank << ": WRITE SIZE " << write_ret << std::endl;
    }

    // COMPARE READ WRITE CONTENT
    
    if (std::strcmp(write_bulk_content, static_cast<const char*>(read_bulk_content)) == 0) {
        std::cout << "T" << my_rank << ": READ WRITE CONTENT MATCH" << std::endl;
    }
    else {
        std::cout << "T" << my_rank << ": READ WRITE CONTENT MISMATCH" << std::endl;
        std::cout << "T" << my_rank << ": WRITE LAST 100 CHAR: " << write_bulk_content + (std::strlen(write_bulk_content) - 100) << std::endl;
        const char* read_bulk_convserion = static_cast<const char*>(read_bulk_content);
        std::cout << "T" << my_rank << ": READ LAST 100 CHAR: " << read_bulk_convserion + (std::strlen(read_bulk_convserion) - 100) << std::endl;
    }

    // MiB CALC
    std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
    double elapsed_time_int = elapsed_time.count();
    
    MPI_Reduce(&elapsed_time_int, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    MPI_Finalize();

    if (my_rank == 0) {
        std::cout << "TOTAL TIME: " << total_time << " s" << std::endl;
        std::cout << "TOTAL MiB: " << transfer_size_in_kib * 2.00 * size / 1024 << std::endl; 
        std::cout << "TRANSFER SPEED: " << transfer_size_in_kib * 2 * size / 1024 / total_time << " MiB/s" << std::endl;
    }
    
}


void bench3(int bench_rounds, int argc, char** argv) {
    double total_time = 0.0;
            
    MPI_Init(&argc, &argv);

    int my_rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::chrono::high_resolution_clock::time_point start_t, end_t;
    unsigned int tsum = 0, loop_rank = my_rank * 16777216;
    int ufds[bench_rounds];
    const char* paths[bench_rounds];
    struct stat ts;

    // START THREAD TIME MEASSUREMENT
    MPI_Barrier(MPI_COMM_WORLD);
    start_t = std::chrono::high_resolution_clock::now();  

    // OPEN LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        ufds[i] = komm_open(tf);
        paths[i] = tf;
    }

    //STOP THREAD TIME MEASSUREMENT
    end_t = std::chrono::high_resolution_clock::now();
    MPI_Barrier(MPI_COMM_WORLD);
    

    // CLOSE, DEL LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        komm_close(ufds[i]);
        komm_unlink(paths[i]);
    }


    std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
    double elapsed_time_int = elapsed_time.count();

    MPI_Reduce(&elapsed_time_int, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    
    std::cout << "THREAD: " << my_rank << ", THREAD TIME: " << elapsed_time.count() << std::endl;

    if (my_rank == 0) {
        std::cout << "TOTAL TIME: " << total_time << std::endl;
        std::cout << "TOTAL OPS: " << bench_rounds * size << std::endl; 
        std::cout << "CALC SPEED: " << bench_rounds * size / total_time << " ops/s" << std::endl;
    }
}


void bench4(int bench_rounds, int argc, char** argv) {
    double total_time = 0.0;
            
    MPI_Init(&argc, &argv);

    int my_rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::chrono::high_resolution_clock::time_point start_t, end_t;
    unsigned int tsum = 0, loop_rank = my_rank * 16777216;
    int ufds[bench_rounds];
    const char* paths[bench_rounds];
    struct stat ts;

    

    // OPEN LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        ufds[i] = komm_open(tf);
        paths[i] = tf;
    }

    // START THREAD TIME MEASSUREMENT
    MPI_Barrier(MPI_COMM_WORLD);
    start_t = std::chrono::high_resolution_clock::now();
    

    // CLOSE LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        komm_close(ufds[i]);
    }

    //STOP THREAD TIME MEASSUREMENT
    end_t = std::chrono::high_resolution_clock::now();
    MPI_Barrier(MPI_COMM_WORLD);

    // DEL LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        komm_unlink(paths[i]);
    }


    std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
    double elapsed_time_int = elapsed_time.count();

    MPI_Reduce(&elapsed_time_int, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    
    std::cout << "THREAD: " << my_rank << ", THREAD TIME: " << elapsed_time.count() << std::endl;

    if (my_rank == 0) {
        std::cout << "TOTAL TIME: " << total_time << std::endl;
        std::cout << "TOTAL OPS: " << bench_rounds * size << std::endl; 
        std::cout << "CALC SPEED: " << bench_rounds * size / total_time << " ops/s" << std::endl;
    }
}


void bench5(int bench_rounds, int argc, char** argv) {
    double total_time = 0.0;
            
    MPI_Init(&argc, &argv);

    int my_rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::chrono::high_resolution_clock::time_point start_t, end_t;
    unsigned int tsum = 0, loop_rank = my_rank * 16777216;
    const char* paths[bench_rounds];
    struct stat ts;

    

    // OPEN LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        int tufd = komm_open(tf);
        komm_close(tufd);
        paths[i] = tf;
    }

    // START THREAD TIME MEASSUREMENT
    MPI_Barrier(MPI_COMM_WORLD);
    start_t = std::chrono::high_resolution_clock::now();
    

    // STAT LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        komm_stat(paths[i], &ts);
    }

    //STOP THREAD TIME MEASSUREMENT
    end_t = std::chrono::high_resolution_clock::now();
    MPI_Barrier(MPI_COMM_WORLD);

    // DEL LOOP
    for (int i = 0; i < bench_rounds; i++) {
        const char* tf = (std::string("tf" + std::to_string(loop_rank+ i))).c_str();
        komm_unlink(paths[i]);
    }


    std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_t - start_t);
    double elapsed_time_int = elapsed_time.count();

    MPI_Reduce(&elapsed_time_int, &total_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    
    std::cout << "THREAD: " << my_rank << ", THREAD TIME: " << elapsed_time.count() << std::endl;

    if (my_rank == 0) {
        std::cout << "TOTAL TIME: " << total_time << std::endl;
        std::cout << "TOTAL OPS: " << bench_rounds * size << std::endl; 
        std::cout << "CALC SPEED: " << bench_rounds * size / total_time << " ops/s" << std::endl;
    }
}



int main(int argc, char** argv) {

    // SERVER SETUP
    server_list = read_conf();
    tl::endpoint server = myEngine.lookup(server_list[0]);

    // BENCHMARKs
    if (argc == 3) {

        //OPEN, CLOSE, STAT, DEL LOOP: bench1
        if (std::strcmp(argv[1], "bench1") == 0) {        
            bench1(std::atoi(argv[2]), argc, argv);
        }

        //READ, WRITE LOOP: bench2
        if (std::strcmp(argv[1], "bench2") == 0) {
            bench2(std::atoi(argv[2]), argc, argv);            
        }

        //OPEN LOOP: bench3
        if (std::strcmp(argv[1], "bench3") == 0) {
            bench3(std::atoi(argv[2]), argc, argv);
        }

        //CLOSE LOOP: bench4
        if (std::strcmp(argv[1], "bench4") == 0) {
            bench4(std::atoi(argv[2]), argc, argv);
        }

        //STAT LOOP: bench5
        if (std::strcmp(argv[1], "bench5") == 0) {
            bench5(std::atoi(argv[2]), argc, argv);
        }
    }
    return 0;
}
