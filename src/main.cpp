

#include <iostream>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

namespace {

//The documents

rapidxml::xml_document<> source_doc;
rapidxml::xml_document<> inc_doc;
rapidxml::xml_document<> target_doc;

bool ignore_package(const std::string& name, const std::vector<std::string>& ignore_packages){
    for(auto& ignore : ignore_packages){
        if(std::mismatch(ignore.begin(), ignore.end(), name.begin()).first == ignore.end()){
            return true;
        }
    }

    return false;
}

rapidxml::xml_node<>* copy_package(rapidxml::xml_document<>& source_doc, rapidxml::xml_node<>* package_node){
    //Create the "package" node (rates are not copied on purpose)
    auto package_target = target_doc.allocate_node(rapidxml::node_element, "package");
    package_target->append_attribute(target_doc.allocate_attribute("name", package_node->first_attribute("name")->value()));
    package_target->append_attribute(target_doc.allocate_attribute("complexity", package_node->first_attribute("complexity")->value()));

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

    return package_target;
}

} //end of anonymous namespace

int main(int argc, char* argv[]){
    //Argument counter
    std::size_t i = 1;

    //Collect the packages to ignore
    std::vector<std::string> ignore_packages;
    for(; i < std::size_t(argc); ++i){
        std::string arg(argv[i]);

        std::string ignore("--ignore=");

        if(std::mismatch(ignore.begin(), ignore.end(), arg.begin()).first == ignore.end()){
            ignore_packages.emplace_back(arg.begin() + ignore.size(), arg.end());
        } else {
            break;
        }
    }

    if(argc - i + 1 < 4){
        std::cout << "coverage_merger: not enough arguments" << std::endl;
        return 1;
    }

    //Collect the paths
    std::string source_path(argv[i]);
    std::string inc_path(argv[i+1]);
    std::string target_path(argv[i+2]);

    std::cout << "Source file: " << source_path << std::endl;
    std::cout << "Increment file: " << inc_path << std::endl;
    std::cout << "Target file: " << target_path << std::endl;

    //Parse all documents

    rapidxml::file<> source_file(source_path.c_str());
    source_doc.parse<0>(source_file.data());

    rapidxml::file<> inc_file(inc_path.c_str());
    inc_doc.parse<0>(inc_file.data());

    std::cout << "Documents parsed" << std::endl;

    //Find the root nodes
    auto source_root = source_doc.first_node("coverage");
    auto inc_root = inc_doc.first_node("coverage");

    //Find the "packages" nodes
    auto packages_source = source_root->first_node("packages");
    auto packages_inc = inc_root->first_node("packages");

    //Create root node in target (rates attributes are not copied on purpose)
    auto* target_root = target_doc.allocate_node(rapidxml::node_element, "coverage");
    target_root->append_attribute(target_doc.allocate_attribute("version", source_root->first_attribute("version")->value()));
    target_root->append_attribute(target_doc.allocate_attribute("timestamp", source_root->first_attribute("timestamp")->value()));
    target_doc.append_node(target_root);

    //Copy directly the "sources" node
    auto* target_sources_node = source_doc.clone_node(source_root->first_node("sources"));
    target_root->append_node(target_sources_node);

    //Create the "packages" node
    auto packages_target = target_doc.allocate_node(rapidxml::node_element, "packages");
    target_root->append_node(packages_target);

    //1. Copy all relevant packages from source -> target

    for(auto* package_node = packages_source->first_node("package"); package_node; package_node = package_node->next_sibling()){
        std::string package_name(package_node->first_attribute("name")->value());
        std::string package_branch_rate(package_node->first_attribute("branch-rate")->value());
        std::string package_line_rate(package_node->first_attribute("line-rate")->value());

        //Skip any ignored package
        bool process = !ignore_package(package_name, ignore_packages);

        //Skip the entire package is no coverage
        if(package_branch_rate == "0.0" && package_line_rate == "0.0"){
            process = false;
        }

        if(process){
            //Copy the package into the target (ignoring uncovered classes)
            packages_target->append_node(copy_package(source_doc, package_node));
        }
    }

    //2. Copy all relevant packages not present in target from inc -> target

    for(auto* package_node = packages_inc->first_node("package"); package_node; package_node = package_node->next_sibling()){
        std::string package_name(package_node->first_attribute("name")->value());
        std::string package_branch_rate(package_node->first_attribute("branch-rate")->value());
        std::string package_line_rate(package_node->first_attribute("line-rate")->value());

        //Skip any ignored package
        bool process = !ignore_package(package_name, ignore_packages);

        //Skip the entire package is no coverage
        if(package_branch_rate == "0.0" && package_line_rate == "0.0"){
            process = false;
        }

        //Skip package that are already existing in target
        if(process){
            for(auto* target_package = packages_target->first_node("package"); target_package; target_package = target_package->next_sibling()){
                std::string source_package_name(target_package->first_attribute("name")->value());

                if(source_package_name == package_name){
                    process = false;
                    break;
                }
            }
        }

        if(process){
            //Copy the package into the target (ignoring uncovered classes)
            packages_target->append_node(copy_package(inc_doc, package_node));
        }
    }

    //Write the target doc
    std::ofstream target_stream(target_path);
    target_stream << target_doc;
    target_stream.close();

    std::cout << "Documents have been processed and target has been written" << std::endl;

    return 0;
}
