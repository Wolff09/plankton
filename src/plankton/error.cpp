#include "plankton/error.hpp"

#include <sstream>
#include "cola/util.hpp"

using namespace cola;
using namespace plankton;


UnsupportedConstructError::UnsupportedConstructError(std::string construct) : VerificationError(
	"Unsupported construct: " + std::move(construct) + ".") {}

inline std::string mk_assert_msg(const Assert& cmd) {
	std::stringstream msg;
	msg << "Assertion error: ";
	cola::print(cmd, msg);
	return msg.str();
}

AssertionError::AssertionError(const Assert& cmd) : VerificationError(mk_assert_msg(cmd)) {}

inline std::string mk_deref_msg(const Expression& expr, const Command* cmd=nullptr) {
	std::stringstream msg;
	msg << "Unsafe dereference: '";
	cola::print(expr, msg);
	msg << "' might be 'NULL'";
	if (cmd) {
		msg << " in command '";
		cola::print(*cmd, msg);
		msg << "'";
	}
	msg << ".";
	return msg.str();
}

UnsafeDereferenceError::UnsafeDereferenceError(const Expression& expr) : VerificationError(mk_deref_msg(expr)) {}

UnsafeDereferenceError::UnsafeDereferenceError(const Expression& expr, const Command& cmd) : VerificationError(mk_deref_msg(expr, &cmd)) {}

SolvingError::SolvingError(std::string cause) : VerificationError(std::move(cause)) {}

EncodingError::EncodingError(std::string cause) : VerificationError(std::move(cause)) {}
