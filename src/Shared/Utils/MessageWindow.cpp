#include <Shared/External/TinyFileDialogs/tinyfiledialogs.h>
#include <Shared/Utils/MessageWindow.hpp>

void MessageWindow::showErrorMessage(std::string title, std::string text)
{
	tinyfd_messageBox(title.c_str(), text.c_str(), "ok", "error", 0);
}

MessageWindow::MessageWindow()
{
}
