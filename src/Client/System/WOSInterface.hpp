#ifndef WOS_INTERFACE_HPP
#define WOS_INTERFACE_HPP

#include <Client/GUI3/Interface.hpp>
#include <Client/System/WOSRenderer.hpp>

class WOSClient;

class WOSInterface : public gui3::Interface
{
public:
	WOSInterface(WOSClient * parentApplication);
	virtual ~WOSInterface();

private:
	WOSRenderer myRenderer;
};

#endif
