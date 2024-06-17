#pragma once

#include "types.h"

//#include "packages/png.h"
//#include "packages/png_1.0.5.h"
//#include "packages/zlib.h"

namespace sw {

// common sources

template <typename Source>
struct package_description {
    string name;
};

struct repository {
    // add name?
    path dir;

    void init(auto &&swctx) {
        init1(swctx);
    }
    void init1(auto &&swctx) {
        auto sl = std::source_location::current();
        if (!fs::exists(sl.file_name())) {
            // unpack self
            SW_UNIMPLEMENTED;
        } else {
            // we are in dev mode, so ok
        }

        auto add_package = [](auto &&p) {
            p.make_archive();
        };

        file_regex_op c_files{".*\\.[hc]"_rr};
        file_regex_op c_files_no_recusive{".*\\.[hc]"_r};

        /*auto github_package = [&](auto &&name, auto &&user_repo, auto &&version_tag, auto &&version,
                                  const file_regex_ops &ops = {}) {
            auto source = github(user_repo, version_tag);
            auto p = package{name, source, version};
            download_package(p);
            for (auto &&op : ops) {
                p += op;
            }
            add_package(p);
            return p;
        };*/

        // demo = non official package provided by sw project
        // original authors should not use it when making an sw package
        #define DEMO_PREFIX "org.sw.demo."

        auto add_sources = [&](auto &&pkg) {
            add(swctx, pkg);
        };
        auto add_binary = [&](auto &&pkg) {
            using Builder = std::decay_t<decltype(pkg)>::type;

            for (auto &&v : pkg.versions) {
                auto fn = swctx.mirror_fn(pkg.name, v);
                auto root = swctx.pkg_root(pkg.name, v) / "sdir";
                if (!fs::exists(root)) {
                    unpack(fn, root);
                }
                //b.build();

                /*auto s = make_solution();
                input_with_settings is;
                is.ep.build = [](auto &&s) {
                    Builder b;
                    b.build(s);
                };
                is.ep.source_dir = root;
                auto dbs = default_build_settings();
                //dbs.build_type = build_type::debug{};
                is.settings.insert(dbs);
                s.add_input(is);
                s.load_inputs();
                command_line_parser cl;
                s.build(cl);*/
            }
        };



        /*github_package<zlib> zlib{"zlib", DEMO_PREFIX "zlib", "madler/zlib", "v{M}.{m}{po}", {
            "1.2.13"_ver,
            "1.3.0"_ver,
            "1.3.1"_ver,
        }};
        zlib.files.emplace_back(c_files_no_recusive);
        add_sources(zlib);
        add_binary(zlib);

        // check png 1.0.5
        github_package<png_1_0_5> png_1_0_5{"png", DEMO_PREFIX "png", "glennrp/libpng", "v{v}", {
            "1.0.5"_ver,
        }};
        png_1_0_5.files.emplace_back(c_files);
        add_sources(png_1_0_5);

        github_package<png> png{"png", DEMO_PREFIX "png", "glennrp/libpng", "v{v}", {
            "1.6.39"_ver,
            "1.6.40"_ver,
            "1.6.41"_ver,
            "1.6.42"_ver,
        }};
        png.files.emplace_back(c_files);
        add_sources(png);*/

        /*github_package<sw::self_build::sw_build> sw2{"sw2", "org.sw.sw", "SoftwareNetwork/sw2"};
        sw2.files.emplace_back("src/.*"_rr);
        add_sources(sw2);*/

        #undef DEMO_PREFIX
    }

private:
    void add(auto &&swctx, auto &&pkg) {
        pkg.download(swctx, swctx.temp_dir / "dl");
    }
};

}
