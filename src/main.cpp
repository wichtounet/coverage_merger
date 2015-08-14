

#include <iostream>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"

int main(int argc, char* argv[]){
    if(argc < 4){
        std::cout << "coverage_merger: not enough arguments" << std::endl;
        return 1;
    }

    std::string source_path(argv[1]);
    std::string inc_path(argv[2]);
    std::string target_path(argv[3]);

    std::vector<std::string> ignore_packages;
    for(std::size_t i = 4; i < argc; ++i){
        ignore_packages.emplace_back(argv[i]);
    }

    std::cout << "Source file: " << source_path << std::endl;
    std::cout << "Increment file: " << inc_path << std::endl;
    std::cout << "Target file: " << target_path << std::endl;

    rapidxml::file<> source_file(source_path.c_str());
    rapidxml::xml_document<> source_doc;
    source_doc.parse<0>(source_file.data());

    rapidxml::file<> inc_file(inc_path.c_str());
    rapidxml::xml_document<> inc_doc;
    inc_doc.parse<0>(inc_file.data());

    rapidxml::xml_document<> target_doc;

    std::cout << "Documents parsed" << std::endl;

    auto root_node = source_doc.first_node("coverage");
    auto packages_node = root_node->first_node("packages");

    for(auto* package_node = packages_node->first_node("package"); package_node; package_node = package_node->next_sibling()){
        std::string package_name(package_node->first_attribute("name")->value());

        bool process = true;

        for(auto& ignore : ignore_packages){
            if(std::mismatch(ignore.begin(), ignore.end(), package_name.begin()).first == ignore.end()){
                process = false;
                break;
            }
        }

        if(process){
            std::cout << package_name  << std::endl;
        }
    }

    return 0;
}
