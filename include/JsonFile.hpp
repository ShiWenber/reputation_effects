#ifndef JSONFILE_HPP
#define JSONFILE_HPP

#include <boost/json.hpp>
#include <string>

void pretty_print(std::ostream& os, boost::json::value const& jv,
                  std::string* indent = nullptr);
std::string logJson(std::string const& json_dir_path, boost::json::value const& jv);
std::string genTimeStr();

#endif // !JSONFILE_HPP