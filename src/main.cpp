

#include <iostream>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

int main(int argc, char* argv[]){
    if(argc < 4){
        std::cout << "coverage_merger: not enough arguments" << std::endl;
        return 1;
    }

    std::string source_path(argv[1]);
    std::string inc_path(argv[2]);
    std::string target_path(argv[3]);

    std::vector<std::string> ignore_packages;
    for(std::size_t i = 4; i < std::size_t(argc); ++i){
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



    auto source_root_node = source_doc.first_node("coverage");

    //Create root node in target
    auto* target_root_node = target_doc.allocate_node(rapidxml::node_element, "coverage");
    //TODO Copy attributes from source
    target_doc.append_node(target_root_node);

    //Copy directly the "sources" node
    auto* target_sources_node = source_doc.clone_node(source_root_node->first_node("sources"));
    target_root_node->append_node(target_sources_node);

    //Create the "packages" node
    auto packages_target = target_doc.allocate_node(rapidxml::node_element, "packages");
    target_root_node->append_node(packages_target);

    auto packages_source = source_root_node->first_node("packages");

    for(auto* package_node = packages_source->first_node("package"); package_node; package_node = package_node->next_sibling()){
        std::string package_name(package_node->first_attribute("name")->value());
        std::string package_branch_rate(package_node->first_attribute("branch-rate")->value());
        std::string package_line_rate(package_node->first_attribute("line-rate")->value());

        bool process = true;

        //Skip any ignored package
        for(auto& ignore : ignore_packages){
            if(std::mismatch(ignore.begin(), ignore.end(), package_name.begin()).first == ignore.end()){
                process = false;
                break;
            }
        }

        //Skip the entire package is no coverage
        if(package_branch_rate == "0.0" && package_line_rate == "0.0"){
            process = false;
        }

        if(process){
            //Create the "package" node
            auto package_target = target_doc.allocate_node(rapidxml::node_element, "package");
            package_target->append_attribute(target_doc.allocate_attribute("name", package_node->first_attribute("name")->value()));
            //TODO Copy all attributes
            packages_target->append_node(package_target);

            //Create the "classes" node
            auto classes_target = target_doc.allocate_node(rapidxml::node_element, "classes");
            package_target->append_node(classes_target);

            auto classes_sources = package_node->first_node("classes");

            for(auto* class_source = classes_sources->first_node("class"); class_source; class_source = class_source->next_sibling()){
                std::string class_name(class_source->first_attribute("name")->value());
                std::string class_branch_rate(class_source->first_attribute("branch-rate")->value());
                std::string class_line_rate(class_source->first_attribute("line-rate")->value());

                if(!(class_branch_rate == "0.0" && class_line_rate == "0.0")){
                    //Copy "class" node into target
                    auto* class_target = source_doc.clone_node(class_source);
                    classes_target->append_node(class_target);
                }
            }
        }
    }

    //Write the target doc
    std::ofstream target_stream(target_path);
    target_stream << target_doc;
    target_stream.close();

    std::cout << "Documents have been processed and target has been written" << std::endl;

    return 0;
}
