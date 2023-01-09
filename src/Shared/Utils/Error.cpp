#include <Shared/Utils/Error.hpp>

Error::Error() noexcept
{
	stackTrace = StackTrace::generate();
	errorText = "Error";
}

Error::Error(std::string errorText) noexcept
{
	stackTrace = StackTrace::generate();
	this->errorText = errorText;
}

Error::~Error() noexcept
{
}

const StackTrace & Error::getStackTrace() const noexcept
{
	return stackTrace;
}

const std::string & Error::getErrorText() const noexcept
{
	return errorText;
}

const char * Error::what() const noexcept
{
	if (fullMessage.empty())
	{
		if (errorText.empty())
		{
			fullMessage = stackTrace.toString();
		}
		else
		{
			fullMessage = errorText + ":\n" + stackTrace.toString();
		}
	}
	return fullMessage.c_str();
}
