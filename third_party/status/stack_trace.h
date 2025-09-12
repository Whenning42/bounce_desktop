#ifndef REAPER_STACK_TRACE_H_
#define REAPER_STACK_TRACE_H_

#include <string>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <unistd.h>
#include <climits>
#include <format>

inline const char* _st_exe_path() {
    static char path[PATH_MAX];
    static bool init = false;
    if (!init) {
        ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (n >= 0) path[n] = '\0';
        else std::snprintf(path, sizeof(path), "%s", "/proc/self/exe");
        init = true;
    }
    return path;
}

inline std::string _st_addr2line(void* addr) {
    // -C (demangle), -f (function), -p (pretty), -e exe
    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "addr2line -Cfpe %s %p", _st_exe_path(),
                  (void*)( (uintptr_t)addr - 1 )); // -1 gets into the call site
    FILE* fp = popen(cmd, "r");
    if (!fp) return {};
    char buf[1024];
    std::string out;
    while (std::fgets(buf, sizeof(buf), fp)) out += buf;
    pclose(fp);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

inline std::string get_backtrace(int skip_frames = 1, bool with_file_lines = true,
                            int max_frames = 64) {
    std::string bt = "";

    if (max_frames > 256) max_frames = 256;
    void* buffer[256];
    int n = ::backtrace(buffer, max_frames);

    bt += "Stack trace (most recent call first):\n";
    for (int i = skip_frames; i < n; ++i) {
        void* addr = buffer[i];
        Dl_info info{};
        const char* sym = nullptr;
        const char* image = nullptr;
        uintptr_t offset = 0;

        if (::dladdr(addr, &info) && info.dli_sname) {
            int status = 0;
            char* dem = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            sym = (status == 0 && dem) ? dem : info.dli_sname;
            image = info.dli_fname ? info.dli_fname : "?";
            offset = (uintptr_t)addr - (uintptr_t)info.dli_saddr;
            bt += std::format("  #{:02d} {:p} {} + 0x{:x} ({})\n",
                             i - skip_frames, addr, sym, (size_t)offset, image);
            std::free(dem);
        } else {
            bt += std::format("  #{:02d} {:p} (no symbol)\n", i - skip_frames, addr);
        }

        if (with_file_lines) {
            std::string loc = _st_addr2line(addr);
            if (!loc.empty()) bt += std::format("        at {}\n", loc);
        }
    }

    return bt;
}

#endif
