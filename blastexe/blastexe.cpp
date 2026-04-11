#include <CLI11.hpp>

int main(int argc, char** argv) {
    CLI::App app{"BLAST compiler"};
    app.set_version_flag("--version", "0.0.1");

    std::string input_file;
    app.add_option("file", input_file, "Source file to compile")->required();

    std::string output_file = "a.out";
    app.add_option("-o,--output", output_file, "Output file");

    CLI11_PARSE(app, argc, argv);
}
