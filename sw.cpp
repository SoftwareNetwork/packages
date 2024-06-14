void build(Solution &s) {
    auto &create_package_storage = s.addExecutable("package_storage");
    {
        auto &t = create_package_storage;
        t.SwDefinitions = true;
        t.PackageDefinitions = true;
        t += cpp23;
        t += "src/.*"_rr;
        t += "org.sw.sw2.sw"_dep;
    }
}
