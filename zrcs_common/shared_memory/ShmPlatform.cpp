#include "ShmPlatform.h"

#include <boost/interprocess/mapped_region.hpp>

#ifdef _WIN32
#   include <boost/interprocess/windows_shared_memory.hpp>
#else
#   include <boost/interprocess/shared_memory_object.hpp>
#endif

#include <cstdio>
#include <cstring>

namespace bip = boost::interprocess;

namespace zrcs {

// ─────────────────────────────────────────────────────────────────────────────
// Windows：使用 windows_shared_memory（底层 CreateFileMappingA 内核对象）
// POSIX ：使用 shared_memory_object（底层 shm_open + mmap）
//
// 注意：boost::interprocess::shared_memory_object 在 Windows 上使用磁盘临时文件
// 而非内核命名对象，跨进程 open 会报"系统找不到指定的文件"。
// windows_shared_memory 才是 CreateFileMappingA(INVALID_HANDLE_VALUE, ...) 的包装。
// ─────────────────────────────────────────────────────────────────────────────

#ifdef _WIN32

// Windows 句柄包装
struct ShmHandle {
    bip::windows_shared_memory shm;
    bip::mapped_region         region;
};

void* platformShmOpen(const char* name, size_t size, bool create,
                      void** out_handle) noexcept
{
    try {
        if (create) {
            bip::windows_shared_memory shm_obj(
                bip::create_only, name, bip::read_write,
                static_cast<bip::offset_t>(size));
            bip::mapped_region region(shm_obj, bip::read_write);

            std::memset(region.get_address(), 0, region.get_size());

            void* addr = region.get_address();
            std::fprintf(stdout,
                "[ShmPlatform] windows_shared_memory create('%s') ok, addr=%p, size=%zu\n",
                name, addr, region.get_size());

            auto* h = new ShmHandle{std::move(shm_obj), std::move(region)};
            if (out_handle) {
                *out_handle = static_cast<void*>(h);
            } else {
                delete h;
                return nullptr;
            }
            return addr;
        } else {
            bip::windows_shared_memory shm_obj(
                bip::open_only, name, bip::read_write);
            bip::mapped_region region(shm_obj, bip::read_write);

            void* addr = region.get_address();

            auto* h = new ShmHandle{std::move(shm_obj), std::move(region)};
            if (out_handle) {
                *out_handle = static_cast<void*>(h);
            } else {
                delete h;
                return nullptr;
            }
            return addr;
        }
    } catch (const bip::interprocess_exception& ex) {
        if (create) {
            std::fprintf(stderr,
                "[ShmPlatform] windows_shared_memory create('%s') failed: %s\n",
                name, ex.what());
        }
        // open 失败不打印——NRT 启动时段尚未创建是正常时序，由调用方重试日志告知用户
        return nullptr;
    }
}

void platformShmClose(void* /*addr*/, size_t /*size*/, const char* /*name*/,
                      bool /*unlink*/, void* handle) noexcept
{
    // 析构 ShmHandle：自动 UnmapViewOfFile + CloseHandle
    if (handle) {
        delete static_cast<ShmHandle*>(handle);
    }
    // Windows 内核命名对象在所有句柄关闭后自动销毁，无需显式 unlink
}

// ─────────────────────────────────────────────────────────────────────────────
// POSIX（Linux / macOS）
// ─────────────────────────────────────────────────────────────────────────────
#else

struct ShmHandle {
    bip::shared_memory_object shm;
    bip::mapped_region        region;
};

void* platformShmOpen(const char* name, size_t size, bool create,
                      void** out_handle) noexcept
{
    try {
        if (create) {
            bip::shared_memory_object::remove(name);

            bip::shared_memory_object shm_obj(bip::create_only, name, bip::read_write);
            shm_obj.truncate(static_cast<bip::offset_t>(size));
            bip::mapped_region region(shm_obj, bip::read_write);

            std::memset(region.get_address(), 0, region.get_size());

            void* addr = region.get_address();
            std::fprintf(stdout,
                "[ShmPlatform] boost::interprocess create('%s') ok, addr=%p, size=%zu\n",
                name, addr, region.get_size());

            auto* h = new ShmHandle{std::move(shm_obj), std::move(region)};
            if (out_handle) {
                *out_handle = static_cast<void*>(h);
            } else {
                delete h;
                return nullptr;
            }
            return addr;
        } else {
            bip::shared_memory_object shm_obj(bip::open_only, name, bip::read_write);
            bip::mapped_region region(shm_obj, bip::read_write);

            void* addr = region.get_address();

            auto* h = new ShmHandle{std::move(shm_obj), std::move(region)};
            if (out_handle) {
                *out_handle = static_cast<void*>(h);
            } else {
                delete h;
                return nullptr;
            }
            return addr;
        }
    } catch (const bip::interprocess_exception& ex) {
        if (create) {
            std::fprintf(stderr,
                "[ShmPlatform] boost::interprocess create('%s') failed: %s\n",
                name, ex.what());
        }
        return nullptr;
    }
}

void platformShmClose(void* /*addr*/, size_t /*size*/, const char* name,
                      bool unlink, void* handle) noexcept
{
    if (handle) {
        delete static_cast<ShmHandle*>(handle);
    }
    if (unlink && name) {
        bip::shared_memory_object::remove(name);
    }
}

#endif  // _WIN32

}  // namespace zrcs
