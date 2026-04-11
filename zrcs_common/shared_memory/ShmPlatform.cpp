#include "ShmPlatform.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <fcntl.h>
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

namespace zrcs {

// ─────────────────────────────────────────────────────────────────────────────
// Windows 实现
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32

void* platformShmOpen(const char* name, size_t size, bool create,
                      void** out_handle) noexcept
{
    HANDLE hMap;

    if (create) {
        hMap = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            static_cast<DWORD>(size >> 32),
            static_cast<DWORD>(size & 0xFFFFFFFFu),
            name);
        if (!hMap) {
            std::fprintf(stderr, "[ShmPlatform] CreateFileMappingA failed: %lu\n",
                         GetLastError());
            return nullptr;
        }
        std::fprintf(stdout, "[ShmPlatform] CreateFileMappingA('%s') ok, hMap=%p, already_existed=%s\n",
                     name, static_cast<void*>(hMap),
                     (GetLastError() == ERROR_ALREADY_EXISTS) ? "YES" : "NO");
    } else {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
        if (!hMap) {
            std::fprintf(stdout, "[ShmPlatform] OpenFileMappingA('%s') failed: err=%lu\n",
                         name, GetLastError());
            return nullptr;
        }
    }

    void* addr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);

    if (!addr) {
        std::fprintf(stderr, "[ShmPlatform] MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(hMap);
        return nullptr;
    }

    if (create && out_handle) {
        // RT 侧：调用方保存句柄，保持对象存活供其他进程 OpenFileMappingA
        *out_handle = static_cast<void*>(hMap);
    } else {
        // NRT 侧或不需要保持：视图已建立，可以关闭句柄
        // （NRT 侧 OpenFileMappingA 的 hMap 只需关闭自己那份，视图仍有效）
        CloseHandle(hMap);
    }

    return addr;
}

void platformShmClose(void* addr, size_t /*size*/, const char* /*name*/,
                      bool /*unlink*/, void* handle) noexcept
{
    if (addr) {
        UnmapViewOfFile(addr);
    }
    if (handle) {
        CloseHandle(static_cast<HANDLE>(handle));
    }
    // Windows 命名文件映射对象在所有进程关闭句柄后自动销毁，
    // 无需（也无法）显式 unlink。
}

// ─────────────────────────────────────────────────────────────────────────────
// POSIX（Linux / macOS）实现
// ─────────────────────────────────────────────────────────────────────────────
#else

void* platformShmOpen(const char* name, size_t size, bool create,
                      void** /*out_handle*/) noexcept
{
    // POSIX shm_open 名称必须以 '/' 开头
    char posix_name[256];
    if (name[0] != '/') {
        std::snprintf(posix_name, sizeof(posix_name), "/%s", name);
    } else {
        std::strncpy(posix_name, name, sizeof(posix_name) - 1);
        posix_name[sizeof(posix_name) - 1] = '\0';
    }

    int flags = create ? (O_CREAT | O_RDWR | O_TRUNC) : O_RDWR;
    int fd = shm_open(posix_name, flags, 0600);
    if (fd < 0) {
        if (!create) return nullptr;  // 段不存在，由调用方重试
        std::perror("[ShmPlatform] shm_open");
        return nullptr;
    }

    if (create) {
        if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
            std::perror("[ShmPlatform] ftruncate");
            close(fd);
            shm_unlink(posix_name);
            return nullptr;
        }
    }

    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);  // fd 关闭后映射依然有效

    if (addr == MAP_FAILED) {
        std::perror("[ShmPlatform] mmap");
        return nullptr;
    }
    return addr;
}

void platformShmClose(void* addr, size_t size, const char* name,
                      bool unlink, void* /*handle*/) noexcept
{
    if (addr) munmap(addr, size);

    if (unlink && name) {
        char posix_name[256];
        if (name[0] != '/') {
            std::snprintf(posix_name, sizeof(posix_name), "/%s", name);
        } else {
            std::strncpy(posix_name, name, sizeof(posix_name) - 1);
            posix_name[sizeof(posix_name) - 1] = '\0';
        }
        shm_unlink(posix_name);
    }
}

#endif  // _WIN32

}  // namespace zrcs
