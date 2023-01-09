#ifndef SRC_SHARED_UTILS_MESSAGEWINDOW_HPP_
#define SRC_SHARED_UTILS_MESSAGEWINDOW_HPP_

#include <string>

class MessageWindow
{
public:
	static void showErrorMessage(std::string title, std::string text);

private:
	MessageWindow();
};

#endif
