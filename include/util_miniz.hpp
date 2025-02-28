#include "util_std.hpp"
#include "miniz/miniz.h"

// Platform-specific includes
#ifdef _WIN32
    #include <direct.h>  // For _mkdir on Windows
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>  // For mkdir on Unix-based systems
    #include <unistd.h>
#endif

namespace vicmil {
int extract_zip(const char *zip_filename, const char *output_dir) {
    // Open the ZIP file
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, zip_filename, 0)) {
        printf("Failed to open ZIP file: %s\n", zip_filename);
        return -1;
    }

    // Extract all files to the output directory
    for (int i = 0; i < mz_zip_reader_get_num_files(&zip); i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip, i, &file_stat)) {
            printf("Failed to get file info for file %d\n", i);
            continue;
        }

        // Calculate the needed buffer size and allocate memory dynamically
        size_t path_len = strlen(output_dir) + strlen(file_stat.m_filename) + 2;  // +2 for "/" and '\0'
        char *output_file_path = (char *)malloc(path_len);

        if (output_file_path == NULL) {
            printf("Memory allocation failed for output file path\n");
            mz_zip_reader_end(&zip);
            return -1;
        }

        // Construct the full path for the output file
        snprintf(output_file_path, path_len, "%s/%s", output_dir, file_stat.m_filename);
        std::cout << "Extracting to: " << output_file_path << std::endl;

        // If the file is a directory, create it
        if (file_stat.m_is_directory) {
            // Create directory
            if (mkdir(output_file_path, 0755) != 0) {
                printf("Failed to create directory: %s\n", output_file_path);
            }
        } else {
            // Extract the file
            FILE *output_file = fopen(output_file_path, "wb");
            if (output_file == NULL) {
                printf("Failed to create file: %s\n", output_file_path);
                free(output_file_path);
                continue;
            }

            // Read the file data from the ZIP archive and write to disk
            size_t file_size = file_stat.m_uncomp_size;
            void *file_data = malloc(file_size);
            if (!mz_zip_reader_extract_to_mem(&zip, i, file_data, file_size, 0)) {
                printf("Failed to extract file %s\n", file_stat.m_filename);
                fclose(output_file);
                free(file_data);
                free(output_file_path);
                continue;
            }

            fwrite(file_data, 1, file_size, output_file);
            fclose(output_file);
            free(file_data);
        }

        // Free dynamically allocated memory
        free(output_file_path);
    }

    // Close the ZIP archive
    mz_zip_reader_end(&zip);
    return 0;
}

/*
Load files from a .zip and store them in memory as a map<file_path, file_content>
 */
std::map<std::string, std::vector<unsigned char>> load_files_from_zip(std::vector<unsigned char>& raw_zip_file_data) {
    std::map<std::string, std::vector<unsigned char>> file_map;
    
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    
    if (!mz_zip_reader_init_mem(&zip_archive, raw_zip_file_data.data(), raw_zip_file_data.size(), 0)) {
        std::cerr << "Failed to initialize ZIP reader." << std::endl;
        return file_map;
    }
    
    int file_count = mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            std::cerr << "Failed to get file info for index " << i << std::endl;
            continue;
        }
        
        std::vector<unsigned char> file_data(file_stat.m_uncomp_size);
        if (!mz_zip_reader_extract_to_mem(&zip_archive, i, file_data.data(), file_data.size(), 0)) {
            std::cerr << "Failed to extract file: " << file_stat.m_filename << std::endl;
            continue;
        }
        
        file_map[file_stat.m_filename] = std::move(file_data);
    }
    
    mz_zip_reader_end(&zip_archive);
    return file_map;
}

std::vector<std::string> filemap_files_with_extension(std::map<std::string, std::vector<unsigned char>>& file_map, std::string target_extension) {
    // target_extension such as 'obj', 'png', etc. etc.
    std::vector<std::string> file_list = {};
    for(auto my_file: file_map) {
        std::string extension = vicmil::split_string(my_file.first, '.').back();
        if(extension == target_extension) {
            file_list.push_back(my_file.first);
        }
    }
    return file_list;
}
}