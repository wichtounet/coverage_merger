

#include <iostream>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

namespace {

//The documents

rapidxml::xml_document<> source_doc;
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
    bool verbose = false;

    //Argument counter
    std::size_t i = 1;

    //Collect the packages to ignore
    std::vector<std::string> ignore_packages;
    for(; i < std::size_t(argc); ++i){
        std::string arg(argv[i]);

        std::string ignore("--ignore=");

        if(std::mismatch(ignore.begin(), ignore.end(), arg.begin()).first == ignore.end()){
            ignore_packages.emplace_back(arg.begin() + ignore.size(), arg.end());
        } else if(arg == "--verbose"){
            verbose = true;
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
    std::string target_path(argv[argc - 1]);

    if(verbose){
        std::cout << "Source file: " << source_path << std::endl;
        std::cout << "Target file: " << target_path << std::endl;
    }

    std::vector<std::string> inc_paths;

    for(i = i + 1; i < std::size_t(argc) - 1; ++i){
        if(verbose){
            std::cout << "Increment file: " << argv[i] << std::endl;
        }

        inc_paths.emplace_back(argv[i]);
    }

    //Parse all documents

    rapidxml::file<> source_file(source_path.c_str());
    source_doc.parse<0>(source_file.data());

    std::vector<rapidxml::xml_document<>> inc_docs(inc_paths.size());
    std::vector<rapidxml::file<>> inc_files;

    for(std::size_t i = 0; i < inc_paths.size(); ++i){
        auto& inc_path = inc_paths[i];

        inc_files.emplace_back(inc_path.c_str());

        inc_docs[i].parse<0>(inc_files.back().data());
    }

    std::cout << "Documents parsed" << std::endl;

    //Find the root nodes
    auto source_root = source_doc.first_node("coverage");

    //Find the "packages" nodes
    auto packages_source = source_root->first_node("packages");

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
            if(verbose){
                std::cout << "Copy package " << package_name << " from source to target" << std::endl;
            }

            //Copy the package into the target (ignoring uncovered classes)
            packages_target->append_node(copy_package(source_doc, package_node));
        }
    }

    //2. Copy all relevant packages not present in target from inc -> target

    for(auto& inc_doc : inc_docs){
        auto inc_root = inc_doc.first_node("coverage");
        auto packages_inc = inc_root->first_node("packages");

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

            //Skip package if there is a better package candidate in the incs
            if(process){
                for(auto& other_inc_doc : inc_docs){
                    if(&inc_doc != &other_inc_doc){
                        auto other_inc_root = other_inc_doc.first_node("coverage");
                        auto other_packages_inc = other_inc_root->first_node("packages");

                        for(auto* opi = other_packages_inc->first_node("package"); opi; opi = opi->next_sibling()){
                            std::string other_package_name(opi->first_attribute("name")->value());
                            std::string other_package_line_rate(opi->first_attribute("line-rate")->value());

                            if(other_package_name == package_name){
                                if(std::stod(package_line_rate) < std::stod(other_package_line_rate)){
                                    process = false;
                                    break;
                                }
                            }
                        }

                        if(!process){
                            break;
                        }
                    }
                }
            }

            if(process){
                if(verbose){
                    std::cout << "Copy package " << package_name << " from inc to target" << std::endl;
                }

                //Copy the package into the target (ignoring uncovered classes)
                packages_target->append_node(copy_package(inc_doc, package_node));
            }
        }
    }

    //3. Copy all relevant classes not present in target from inc -> target

    for(auto& inc_doc : inc_docs){
        auto inc_root = inc_doc.first_node("coverage");
        auto packages_inc = inc_root->first_node("packages");

        for(auto* package_inc = packages_inc->first_node("package"); package_inc; package_inc = package_inc->next_sibling()){
            std::string package_name(package_inc->first_attribute("name")->value());

            rapidxml::xml_node<>* package_target = nullptr;

            for(auto* target_package = packages_target->first_node("package"); target_package; target_package = target_package->next_sibling()){
                std::string source_package_name(target_package->first_attribute("name")->value());

                if(source_package_name == package_name){
                    package_target = target_package;

                    break;
                }
            }

            //If the package is not found, it means it has been filtered in the previous step
            if(!package_target){
                continue;
            }

            auto classes_inc = package_inc->first_node("classes");
            auto classes_target = package_target->first_node("classes");

            for(auto* class_inc = classes_inc->first_node("class"); class_inc; class_inc = class_inc->next_sibling()){
                std::string class_name(class_inc->first_attribute("name")->value());
                std::string class_branch_rate(class_inc->first_attribute("branch-rate")->value());
                std::string class_line_rate(class_inc->first_attribute("line-rate")->value());

                if(class_branch_rate == "0.0" && class_line_rate == "0.0"){
                    continue;
                }

                bool found = false;
                for(auto* class_target = classes_target->first_node("class"); class_target; class_target = class_target->next_sibling()){
                    if(class_name == class_target->first_attribute("name")->value()){
                        found = true;
                        break;
                    }
                }

                if(!found){
                    bool better = false;

                    //Check if there is a better candidate
                    for(auto& other_inc_doc : inc_docs){
                        if(&inc_doc != &other_inc_doc){
                            auto other_inc_root = other_inc_doc.first_node("coverage");
                            auto other_packages_inc = other_inc_root->first_node("packages");

                            for(auto* opi = other_packages_inc->first_node("package"); opi; opi = opi->next_sibling()){
                                std::string other_package_name(opi->first_attribute("name")->value());

                                if(other_package_name == package_name){
                                    auto other_classes_inc = opi->first_node("classes");

                                    for(auto* oci = other_classes_inc->first_node("class"); oci; oci = oci->next_sibling()){
                                        std::string other_class_name(oci->first_attribute("name")->value());
                                        std::string other_class_line_rate(oci->first_attribute("line-rate")->value());

                                        if(other_class_name == class_name){
                                            if(std::stod(class_line_rate) < std::stod(other_class_line_rate)){
                                                better = true;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if(better){
                                    break;
                                }
                            }
                        }

                        if(better){
                            break;
                        }
                    }

                    if(!better){
                        if(verbose){
                            std::cout << "Copy class " << class_name << " from inc to target" << std::endl;
                        }

                        classes_target->append_node(inc_doc.clone_node(class_inc));
                    }
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
