#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "im2d.h"
#include "RgaUtils.h"
#include "drmrga.h"

typedef unsigned long long __u64;
typedef unsigned int __u32;
struct dma_heap_allocation_data {
    __u64 len;
    __u32 fd;
    __u32 fd_flags;
    __u64 heap_flags;
};
#define DMA_HEAP_IOC_MAGIC 'H'
#define DMA_HEAP_IOCTL_ALLOC _IOWR(DMA_HEAP_IOC_MAGIC, 0x0, struct dma_heap_allocation_data)

int main() {
    const int W = 64, H = 64;
    const size_t buf_size = static_cast<size_t>(W) * H * 3;

    // Allocate a DMA buffer from the board's uncached heap.
    int heap_fd = open("/dev/dma_heap/system-uncached", O_RDWR);
    if (heap_fd < 0) { printf("OPEN_HEAP_FAIL\n"); return 1; }
    struct dma_heap_allocation_data data;
    std::memset(&data, 0, sizeof(data));
    data.len = buf_size;
    data.fd_flags = O_CLOEXEC | O_RDWR;
    if (ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &data) < 0) { printf("ALLOC_HEAP_FAIL\n"); return 1; }
    close(heap_fd);

    void* src_va = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, data.fd, 0);
    if (src_va == MAP_FAILED) { printf("MMAP_FAIL\n"); return 1; }
    std::memset(src_va, 0x80, buf_size);

    // Import the dma-buf fd into RGA.
    rga_buffer_t src = wrapbuffer_fd_t(data.fd, W, H, W, H, RK_FORMAT_RGB_888);
    rga_buffer_t dst = src;  // same buffer as scratch

    int ret = imresize(src, dst, 2.0, 2.0, 0);
    if (ret != IM_STATUS_SUCCESS) {
        printf("IMRESIZE_FAIL ret=%d err=%s\n", ret, imStrError((IM_STATUS)ret));
        return 2;
    }

    munmap(src_va, buf_size);
    close(data.fd);
    printf("RGA_SMOKE_OK\n");
    return 0;
}


