/**
 * @file JsonFile.hpp
 * @brief JsonFile class definition, this is a tool class for json file
 * operation coming from
 * https://www.boost.org/doc/libs/1_84_0/libs/json/doc/html/json/examples.html#json.examples.pretty
 *
 */
#include "JsonFile.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <boost/json.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <filesystem>

void pretty_print(std::ostream& os, boost::json::value const& jv,
                  std::string* indent) {
  std::string indent_;
  if (!indent) indent = &indent_;
  switch (jv.kind()) {
    case boost::json::kind::object: {
      os << "{\n";
      indent->append(4, ' ');
      auto const& obj = jv.get_object();
      if (!obj.empty()) {
        auto it = obj.begin();
        for (;;) {
          os << *indent << boost::json::serialize(it->key()) << " : ";
          pretty_print(os, it->value(), indent);
          if (++it == obj.end()) break;
          os << ",\n";
        }
      }
      os << "\n";
      indent->resize(indent->size() - 4);
      os << *indent << "}";
      break;
    }

    case boost::json::kind::array: {
      os << "[\n";
      indent->append(4, ' ');
      auto const& arr = jv.get_array();
      if (!arr.empty()) {
        auto it = arr.begin();
        for (;;) {
          os << *indent;
          pretty_print(os, *it, indent);
          if (++it == arr.end()) break;
          os << ",\n";
        }
      }
      os << "\n";
      indent->resize(indent->size() - 4);
      os << *indent << "]";
      break;
    }

    case boost::json::kind::string: {
      os << boost::json::serialize(jv.get_string());
      break;
    }

    case boost::json::kind::uint64:
    case boost::json::kind::int64:
    case boost::json::kind::double_:
      os << jv;
      break;

    case boost::json::kind::bool_:
      if (jv.get_bool())
        os << "true";
      else
        os << "false";
      break;

    case boost::json::kind::null:
      os << "null";
      break;
  }

  if (indent->empty()) os << "\n";
}

/**
 * @brief while create a json file named by now time and uuid, the json file will be in the json_dir_path,
 * and return the log file path which is related to the json file
 * 
 * @param json_dir_path  the path of json file
 * @param jv  the json value
 * 
 * @return std::string  the log file path
 * 
 */
  // judge if the path exists, if not, create it
std::string logJson(std::string const& json_dir_path, boost::json::value const& jv) {
  if (!std::filesystem::exists(json_dir_path)) {
    std::filesystem::create_directory(json_dir_path);
  }
  // generate a uuid
  boost::uuids::random_generator gen;
  boost::uuids::uuid id = gen();
  std::string uuid_str = boost::uuids::to_string(id);
  // generate a time_str as the format of "YYYYMMDDHHMMSS"
  std::string time_str = genTimeStr();
  std::string file_name = time_str +"_"+ uuid_str;
  std::string json_file_name = file_name + ".json";
  std::string log_file_name = file_name + ".csv";
  // generate a json file path
  std::string json_file_path = json_dir_path + "/" + json_file_name;
  std::string log_file_path = json_dir_path + "/" + log_file_name;
  // add the log file path to json object
  boost::json::object obj = jv.get_object();
  obj["data"] = log_file_path;
  // write json string to json file
  std::ofstream ofs(json_file_path);
  pretty_print(ofs,obj);
  return log_file_path;
}

/**
 * @brief generate a time string as the format of "YYYYMMDDHHMMSS"
 * 
 * @return std::string 
 */
std::string genTimeStr() {
  auto now = std::chrono::system_clock::now();
  time_t now_c = std::chrono::system_clock::to_time_t(now);
  tm* now_tm = localtime(&now_c);
  std::stringstream ss;
  ss << std::put_time(now_tm, "%Y%m%d%H%M%S");
  return ss.str();
}