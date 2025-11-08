// proto_validator.h - Protobuf command validator with buf.validate

#ifndef PROTO_VALIDATOR_H
#define PROTO_VALIDATOR_H

#include <memory>
#include <string>
#include <vector>

#include "buf/validate/validator.h"
#include "jon_shared_cmd.pb.h"

namespace jettison
{

/**
 * @brief Validation result for a protobuf command
 */
struct ValidationResult
{
  bool is_valid;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
};

/**
 * @brief Validator for Jettison command protobuf messages
 *
 * Validates command messages according to buf.validate constraints
 * embedded in the proto definitions.
 */
class ProtoValidator
{
public:
  ProtoValidator ();

  /**
   * @brief Validate a command message
   * @param cmd The command to validate
   * @return Validation result
   */
  ValidationResult validate (const cmd::Root &cmd);

private:
  std::unique_ptr<buf::validate::ValidatorFactory> validator_factory_;
  google::protobuf::Arena arena_;
};

} // namespace jettison

#endif // PROTO_VALIDATOR_H
