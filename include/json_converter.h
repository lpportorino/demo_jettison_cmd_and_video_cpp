// json_converter.h - Convert protobuf commands to JSON format

#ifndef JSON_CONVERTER_H
#define JSON_CONVERTER_H

#include <string>

#include "jon_shared_cmd.pb.h"

namespace jettison
{

/**
 * @brief Convert protobuf command messages to JSON format
 *
 * Uses Google Protocol Buffers' JSON serialization to convert
 * binary protobuf messages to human-readable JSON.
 */
class JsonConverter
{
public:
  JsonConverter () = default;

  /**
   * @brief Convert a cmd::Root message to JSON string
   * @param cmd The protobuf command message
   * @param pretty If true, format with indentation
   * @return JSON string representation
   */
  std::string to_json (const cmd::Root &cmd, bool pretty = true);
};

} // namespace jettison

#endif // JSON_CONVERTER_H
