#pragma once
#include <generator>
#include <pugixml.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace zimm
{

inline std::generator<std::pair<std::string, std::string>> parse_cli_args(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (auto eq = arg.find('='); eq != std::string_view::npos)
            co_yield {std::string{arg.substr(0, eq)}, std::string{arg.substr(eq + 1)}};
        else
            throw std::invalid_argument(std::format("Argument '{}' is not in key=value form", arg));
    }
}

inline std::generator<std::pair<std::string, std::string>> parse_xml_args(std::string path)
{
    pugi::xml_document doc;
    auto result = doc.load_file(path.c_str());
    if (!result)
        throw std::invalid_argument(
            std::format("args_xml: failed to parse '{}': {}", path, result.description()));

    for (pugi::xml_node node = doc.first_child().first_child(); node; node = node.next_sibling())
    {
        if (node.type() != pugi::node_element) continue;
        co_yield {node.name(), node.child_value()};
    }
}

inline std::generator<std::pair<std::string, std::string>> parse_yaml_args(std::string path)
{
    YAML::Node root = YAML::LoadFile(path);
    if (!root || !root.IsMap())
        throw std::invalid_argument(
            std::format("args_yaml: '{}' does not contain a top-level mapping", path));

    for (const auto &kv : root)
    {
        std::string key = kv.first.as<std::string>();
        std::string value =
            kv.second.IsScalar() ? kv.second.as<std::string>() : YAML::Dump(kv.second);
        co_yield {std::move(key), std::move(value)};
    }
}

} // namespace zimm
