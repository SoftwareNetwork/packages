#include <sw/runtime/main.h>
#include <sw/crypto/sha3.h>
#include <sw/package.h>
#include <sw/command/command.h>
#include <algorithm>
#include <ranges>

using namespace std::literals;

//#include "repository.h"

/*
When you write software, often you want to make it available to others or your other projects.
We call this 'publishing'.
*/

using path = sw::path;
using sw::replace_all;
namespace fs = sw::fs;

struct git_like_storage {
    using hash_type = crypto::sha3<256>;
};

struct git_remote {
    struct tag_ref {
        std::string_view line;
        std::string_view commit;
    };

    std::string refs;
    std::map<std::string_view, std::string_view> tag_commits;

    auto tag_hash(this auto &&obj, const std::string &tag) {
        obj.read_refs();

        auto it = obj.tag_commits.find(tag);
        if (it == obj.tag_commits.end()) {
            throw std::runtime_error{"tag not found"};
        }
        return std::string{it->second};
    }
    bool has_tag(this auto &&obj, const std::string &tag) {
        obj.read_refs();
        return obj.tag_commits.contains(tag);
    }
    void read_refs(this auto &&obj) {
        if (!obj.refs.empty()) {
            return;
        }

        constexpr auto ref_suffix = "^{}"sv;
        // also 'git ls-remote origin'
        auto u = obj.url() + "/info/refs?service=git-upload-pack"s;
        sw::raw_command c;
        c += sw::resolve_executable("curl"), "-s", "-L", u, "-o-";
        c.out = std::string{};
        c.run();
        obj.refs = c.out.get<std::string>();
        if (!obj.refs.starts_with("001e#"sv)) {
            throw std::runtime_error{"bad git response"};
        }
        for (auto &&[b,e] : std::views::split(obj.refs, "\n"sv) | std::views::drop(2)) {
            std::string_view sv{b,e};
            auto s = " refs/tags/"sv;
            auto p = sv.find(s);
            if (p != -1) {
                auto tag = sv.substr(p + s.size());
                auto tail = sv.ends_with(ref_suffix);
                if (tail || !obj.tag_commits.contains(tag)) {
                    if (tail) {
                        tag.remove_suffix(ref_suffix.size());
                    }
                    obj.tag_commits[tag] = sv.substr(4, 40); // sha1 has 40 hex digits
                }
            }
        }
    }
};
struct commit {
    std::string value;
};
struct tag {
    std::string value;

    auto operator()(const sw::package_version &version) const {
        auto v = value;
        v = replace_all(v, "{v}", version);
        v = replace_all(v, "{M}", std::to_string(version[0]));
        v = replace_all(v, "{m}", std::to_string(version[1]));
        v = replace_all(v, "{p}", std::to_string(version[2]));
        if (version[2]) {
            v = replace_all(v, "{po}", "."s + std::to_string(version[2]));
        } else {
            v = replace_all(v, "{po}", {});
        }
        if (version.size() > 3) {
            v = replace_all(v, "{t}", std::to_string(version[3]));
            if (version[3]) {
                v = replace_all(v, "{to}", "."s + std::to_string(version[3]));
            } else {
                v = replace_all(v, "{to}", {});
            }
        }
        if (version.size() > 4) {
            v = replace_all(v, "{v5}", std::to_string(version[4]));
        }
        if (version.size() > 5) {
            v = replace_all(v, "{v6}", std::to_string(version[5]));
        }
        return tag{v};
    }
};

// cgit https://git.savannah.gnu.org/gitweb/?p=gnulib.git;a=snapshot;h=d4ec02b3cc70cddaaa5183cc5a45814e0afb2292;sf=tgz

struct github_repository : git_remote {
    std::string user_repo;

    github_repository(auto &&user_repo) : user_repo{user_repo} {
    }
    github_repository(auto &&user, auto &&repo) {
        user_repo += user;
        user_repo += "/";
        user_repo += repo;
    }
    auto url() {
        return std::format("https://github.com/{}", user_repo);
    }

    auto operator()(const commit &t) {
        struct x {
            std::string value;
            auto url() const {
                return value;
            }
        };
        return x{url() + std::format("/archive/{}.tar.gz", t.value)};
    }
    auto operator()(const tag &t) {
        return operator()(commit{tag_hash(t.value)});
        //return x{url() + std::format("/archive/refs/tags/{}.tar.gz", t.value)};
    }
};

struct software {
    // source
};

struct repository {
    path root;

    repository(const path &root) : root{root} {
        fs::create_directories(root / "tmp");
    }
    void add(auto &&remote) {
        auto url = remote.url();

        int a = 5;
        a++;
    }
};

struct versioned_url {

};

struct default_version_incrementer {
    std::set<sw::package_version> versions;

    default_version_incrementer(sw::package_version base) {
        for (auto &&e : std::get<sw::package_version::number_version>(base.version).elements.value | std::views::reverse) {
            ++e;
            versions.insert(base);
            e = 0;
        }
    }
};

struct version_list {
    std::set<sw::package_version> versions;
    sw::package_version last_ver;

    auto begin() {return versions.begin();}
    auto end() {return versions.end();}
    void add(const std::initializer_list<int> &v) {
        versions.insert(v);
        last_ver = v;
    }
    bool check_next(auto &&repo, auto &&tag, int recursion = 0, std::source_location sl = std::source_location::current()) {
        bool result{};
        default_version_incrementer i{last_ver};

        // use reverse and add only latest versions for now
        for (auto &&v : i.versions | std::views::reverse) {
            if (!versions.contains(v) && repo.has_tag(tag(v).value)) {
                versions.insert(v);
                last_ver = v;
                result = true;
                if (check_next(repo, tag, recursion + 1, sl)) {
                }
                // unconditionally
                break;
            }
        }
        if (recursion) {
            return result;
        }
        if (!result) {
            return result;
        }
        static int added_lines = 0;
        auto f = sw::read_file(sl.file_name());
        auto [b,e] = *(std::views::split(f, "\n"sv) | std::views::drop(sl.line() - 1 + added_lines++)).begin();
        auto line = std::string_view{b, e};
        auto pos = &line[0] - &f[0];
        auto non_space = f.find_first_not_of(' ', pos);
        auto spaces = non_space - pos;
        std::string space(spaces, ' ');
        f = f.substr(0, pos) + std::format("{}vl.add({});\n", space, last_ver.to_string_initializer_list()) + f.substr(pos);
        sw::write_file(sl.file_name(), f);
        // we dont need to rebuild and run the file again because
        // new version is in the version list now,
        // so it will be downloaded
        return result;
    }
};

int main1(int argc, char *argv[]) {
    if (argc == 1) {
        std::cerr << std::format("usage: {} storage_dir\n", argv[0]);
    }
    repository repo{argv[1]};

    tag vv{"v{v}"};
    tag vmmpo{"v{M}.{m}{po}"};

    //repo.add_software();

    {
        github_repository ghr{"madler/zlib"};
        version_list vl;
        vl.add({1,2,3});
        vl.add({1,3,1});
        vl.check_next(ghr, vmmpo);
        for (auto &&v : vl)
        {
            //madler_zlib.download(vmmpo(v));
            //madler_zlib(vmmpo({1,3}));
            //auto pkg = repo.add(madler_zlib(vmmpo({1,2,13})));
            //repo.add(madler_zlib(vmmpo({1,3,1})));
        }
    }
    {
        github_repository ghr{"glennrp/libpng"};
        version_list vl;
        //vl.add({1,0,5}); separate cfg in sw
        vl.add({1,6,35});
        vl.add({1,6,43});
        vl.check_next(ghr, vv);
        for (auto &&v : vl)
        {
            //madler_zlib.download(vmmpo(v));
            //madler_zlib(vmmpo({1,3}));
            //auto pkg = repo.add(madler_zlib(vmmpo({1,2,13})));
            //repo.add(madler_zlib(vmmpo({1,3,1})));
        }
    }

    return 0;
}
