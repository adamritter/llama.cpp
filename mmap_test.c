#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#define TWENTY_GB (20ULL * 1024 * 1024 * 1024)

#define NUM_THREADS 12

// Thread argument structure
typedef struct {
    const unsigned char *buf;
    size_t start;
    size_t end;
    uint64_t result;
} thread_arg_t;

// Thread function for summing bytes
void* sum_bytes_thread(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    uint64_t sum = 0;

    for (size_t i = targ->start; i < targ->end; i += 1024) {
        for (int j = 0; j < 1024; j += 8) {
            if (i + j < targ->end) {
                sum += targ->buf[i + j];
            }
        }
    }

    targ->result = sum;
    return NULL;
}

uint64_t sum_bytes(const unsigned char *buf, size_t len) {
    pthread_t threads[NUM_THREADS];
    thread_arg_t thread_args[NUM_THREADS];

    // Calculate chunk size for each thread
    size_t chunk_size = (len + NUM_THREADS - 1) / NUM_THREADS;

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].buf = buf;
        thread_args[i].start = i * chunk_size;
        thread_args[i].end = (i + 1) * chunk_size;
        if (thread_args[i].end > len) {
            thread_args[i].end = len;
        }
        thread_args[i].result = 0;

        if (pthread_create(&threads[i], NULL, sum_bytes_thread, &thread_args[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    // Join threads and sum results
    uint64_t total_sum = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            exit(1);
        }
        total_sum += thread_args[i].result;
    }

    return total_sum;
}

// Function to map a file into memory
void* mmap_file(const char *filename, off_t offset, size_t *size_mapped) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        exit(1);
    }

    *size_mapped = (sb.st_size - offset < TWENTY_GB) ?
                   (sb.st_size - offset) : TWENTY_GB;

    printf("Mapping %s, size: %.2fGB, offset: %.2fGB\n",
           filename,
           (double)*size_mapped / (1ULL * 1024 * 1024 * 1024),
           (double)offset / (1ULL * 1024 * 1024 * 1024));

    void *mapped_data = mmap(NULL, *size_mapped, PROT_READ, MAP_PRIVATE, fd, offset);
    if (mapped_data == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(1);
    }

    // // Tell the kernel we will need this memory
    // if (madvise(mapped_data, *size_mapped, MADV_WILLNEED) == -1) {
    //     perror("Error with madvise");
    // }

    close(fd);
    return mapped_data;
}

// Function to unmap a memory-mapped file
void unmap_file(void *mapped_data, size_t size) {
    madvise(mapped_data, size, MADV_DONTNEED);
    if (munmap(mapped_data, size) == -1) {
        perror("Error unmapping file");
    }
}

// Function to process a file region and print timing information
uint64_t process_file_region(const void *data, size_t size, const char *label, int lock) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    if (lock) {
        mlock(data, size);
    }

    uint64_t sum = sum_bytes((const unsigned char *)data, size);

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Print timing information
    double speed_gbps = (size / (1024.0 * 1024 * 1024)) / elapsed;
    printf("%s: %.6f seconds, Speed: %.2f GB/s, sum: %016llx\n", label, elapsed, speed_gbps, sum);

    return sum;
}

int main() {
    const char *filenames[] = {
        "/Users/adamritter/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf",
        "/Users/adamritter/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00002-of-00003.gguf",
        "/Users/adamritter/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00003-of-00003.gguf"
    };
    void *mapped_datas[6];

    for (int i = 0; i < 3; i++) {
        size_t size_mapped;
        void *mapped_data = mmap_file(filenames[i], TWENTY_GB, &size_mapped);
        mapped_datas[i] = mapped_data;

    }
    for (int i = 0; i < 3; i++) {
        size_t size_mapped;
        void *mapped_data = mmap_file(filenames[i], 0, &size_mapped);
        mapped_datas[i + 3] = mapped_data;
    }

    for (int i = 0; i < 6; i++) {
        char str[200];
        sprintf(str, "Read of %d", i);
        process_file_region(mapped_datas[i], TWENTY_GB, str, i < 3);
    }

    for (int i = 0; i < 2; i++) {
            char str[200];
            sprintf(str, "Read of %d", i);
            process_file_region(mapped_datas[i], TWENTY_GB, str, 0);
        }
    return 0;
}
