// json_converter.cpp - Convert protobuf commands to JSON

#include "json_converter.h"

#include <google/protobuf/util/json_util.h>

namespace jettison
{

std::string
JsonConverter::to_json (const cmd::Root &cmd, bool pretty)
{
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = pretty;
  // Note: always_print_primitive_fields removed in Protobuf 29.x
  // Default behavior now includes primitive fields
  options.preserve_proto_field_names = true;

  std::string json_string;
  auto status
      = google::protobuf::util::MessageToJsonString (cmd, &json_string, options);

  if (!status.ok ())
    {
      return "{\"error\": \"Failed to convert to JSON: "
             + std::string (status.message ()) + "\"}";
    }

  return json_string;
}

} // namespace jettison
