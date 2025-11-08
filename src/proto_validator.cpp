// proto_validator.cpp - Protobuf command validator implementation

#include "proto_validator.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <iostream>

namespace jettison
{

ProtoValidator::ProtoValidator ()
{
  // Initialize the validator factory
  auto factory_or = buf::validate::ValidatorFactory::New ();
  if (!factory_or.ok ())
    {
      std::cerr << "⚠ Failed to create ValidatorFactory: "
                << factory_or.status ().message () << "\n";
      std::cerr << "  Validation will be disabled\n";
    }
  else
    {
      validator_factory_ = std::move (*factory_or);
    }
}

ValidationResult
ProtoValidator::validate (const cmd::Root &cmd)
{
  ValidationResult result;
  result.is_valid = true;

  // If protovalidate is not available, skip validation
  if (!validator_factory_)
    {
      result.warnings.push_back (
          "Validation disabled (ValidatorFactory not available)");
      return result;
    }

  // Create a validator for this validation
  auto validator = validator_factory_->NewValidator (&arena_, false);

  // Validate the message
  auto validation_result = validator.Validate (cmd);

  if (!validation_result.ok ())
    {
      result.errors.push_back ("Validation error: "
                               + std::string (validation_result.status ().message ()));
      result.is_valid = false;
      return result;
    }

  // Check for violations
  if (!validation_result->success ())
    {
      result.is_valid = false;
      for (int i = 0; i < validation_result->violations_size (); ++i)
        {
          const auto &violation = validation_result->violations (i);
          const auto &proto = violation.proto ();
          std::string error_msg = "Field '";

          // Build field path from protobuf Violation
          if (proto.has_field ())
            {
              // Convert FieldPath to string
              const auto &field_path = proto.field ();
              std::string path_str;
              for (const auto &element : field_path.elements ())
                {
                  if (!path_str.empty ())
                    path_str += ".";
                  path_str += element.field_name ();
                }
              error_msg += path_str;
            }
          else
            {
              error_msg += "<root>";
            }

          error_msg += "': " + proto.message ();

          if (!proto.rule_id ().empty ())
            {
              error_msg += " (rule: " + proto.rule_id () + ")";
            }

          result.errors.push_back (error_msg);
        }
    }

  return result;
}

} // namespace jettison
