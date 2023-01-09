#ifndef SRC_SHARED_UTILS_ERROR_HPP_
#define SRC_SHARED_UTILS_ERROR_HPP_

#include <Shared/Utils/Debug/StackTrace.hpp>

class Error : public std::exception
{
public:
	Error() noexcept;
	Error(std::string errorText) noexcept;
	virtual ~Error() noexcept;

	const std::string & getErrorText() const noexcept;
	const StackTrace & getStackTrace() const noexcept;

	virtual const char * what() const noexcept override;

private:
	StackTrace stackTrace;
	std::string errorText;
	mutable std::string fullMessage;
};

#endif
