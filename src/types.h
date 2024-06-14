#pragma once

#include <sw/helpers/common.h>
#include <sw/target_properties.h>
#include <sw/command/command.h>
#include <sw/package_id.h>

namespace sw {

struct git {
    std::string dl_url;

    git(auto &&url, auto &&version_tag) {
        dl_url = std::format("{}/archive/refs/tags/{}", url, version_tag);
    }
};
struct github : git {
    github(auto &&user, auto &&repo, auto &&version_tag)
        : git{"https://github.com/"s + user + "/" + repo,version_tag} {
    }
    github(auto &&user_repo, auto &&version_tag)
        : git{"https://github.com/"s + user_repo, version_tag} {
    }
};

std::string replace_all(std::string s, const std::string &pattern, const std::string &repl) {
    size_t pos;
    while ((pos = s.find(pattern)) != -1) {
        s = s.substr(0, pos) + repl + s.substr(pos + pattern.size());
    }
    return s;
}

struct file_regex_op {
    enum {
        add,
        remove,
    };
    file_regex r;
    int operation{add};
};
using file_regex_ops = std::vector<file_regex_op>;

void unpack(const path &arch, const path &dir) {
    fs::create_directories(dir);

    raw_command c;
    c.working_directory = dir;
    c += resolve_executable("tar"), "-xvf", arch;
    log_info("extracting {}", arch);
    c.run();
}


template <typename Source, typename Version>
struct package {
    std::string name;
    Source source;
    Version version;

    //
    path source_dir;
    std::set<path> files;
    path arch;

    ~package() {
        //std::error_code ec;
        fs::remove(arch);
    }
    void download(auto &&wdir) {
        auto ext = ".zip";
        arch = wdir / (name + ext);
        fs::create_directories(wdir);
        {
            raw_command c;
            // c += resolve_executable("git"), "clone", url, "1";
            auto url = source.dl_url + ext;
            url = replace_all(url, "{v}", version);
            url = replace_all(url, "{M}", std::to_string(version[0]));
            url = replace_all(url, "{m}", std::to_string(version[1]));
            url = replace_all(url, "{p}", std::to_string(version[2]));
            if (version[2]) {
                url = replace_all(url, "{po}", "."s + std::to_string(version[2]));
            } else {
                url = replace_all(url, "{po}", {});
            }
            c += resolve_executable("curl"), "-fail", "-L", url, "-o", arch;
            log_info("downloading {}", url);
            c.run();
        }
        scope_exit se{[&] {
            fs::remove(arch);
        }};
        unpack(arch, wdir);
        se.disable();
        fs::remove(arch);
        {
            int ndir{}, nfile{};
            for (auto &&p : fs::directory_iterator{wdir}) {
                if (p.is_directory()) {
                    source_dir = p;
                    ++ndir;
                } else {
                    ++nfile;
                }
            }
            if (ndir == 1 && nfile == 0) {
                //source_dir = wdir / source_dir;
            } else {
                SW_UNIMPLEMENTED;
            }
        }
        if (source_dir.empty()) {
            throw std::runtime_error{"cannot detect source dir"};
        }
    }
    void make_archive() {
        {
            raw_command c;
            c.working_directory = source_dir;
            c += resolve_executable("tar"), "-c", "-f", arch;
            c += files;
            log_info("creating archive {}", arch);
            c.run();
        }
        fs::remove_all(source_dir);
    }

    void operator+=(auto &&v) {
        add(v);
    }
    void add(const file_regex_op &r) {
        switch (r.operation) {
        case file_regex_op::add: add(r.r); break;
        //case file_regex_op::remove: add(r.r); break;
        }
    }
    void add(const file_regex &r) {
        r(source_dir, [&](auto &&iter) {
            for (auto &&e : iter) {
                if (fs::is_regular_file(e)) {
                    //add(e);
                    files.insert(e.path().lexically_relative(source_dir));
                }
            }
        });
    }
};

template <typename Ops>
struct github_package {
    using type = Ops;

    std::string name;
    sw::package_path prefix;
    std::string user_repo;
    std::string version_tag;
    std::vector<sw::package_version> versions;
    file_regex_ops files;

    void download(auto &&swctx, auto &&tmpdir) {
        for (auto &&v : versions)
            download(swctx, tmpdir, v);
    }
    void download(auto &&swctx, auto &&tmpdir, auto &&version) {
        auto dst = swctx.mirror_fn(name, version);
        if (fs::exists(dst)) {
            return;
        }

        auto source = github(user_repo, version_tag);
        auto p = package{name, source, version};
        p.download(tmpdir);
        for (auto &&op : files) {
            p += op;
        }
        p.make_archive();

        fs::create_directories(dst.parent_path());
        fs::copy_file(p.arch, dst);
    }
};

}
