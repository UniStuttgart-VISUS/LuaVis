#ifndef SRC_CLIENT_GUI3_UTILS_WIDGETRESOURCEBINDING_HPP_
#define SRC_CLIENT_GUI3_UTILS_WIDGETRESOURCEBINDING_HPP_

#include <Client/GUI3/Events/StateEvent.hpp>
#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <string>
#include <vector>

namespace gui3
{
class Widget;

/**
 * Allows the use a of gui3::Widget as a source for resources. Automatically fires change events when the widget's
 * parent application or resource manager is changed.
 */
class WidgetResourceBinding : public ::res::AbstractSource
{
public:
	WidgetResourceBinding(Widget & widget);
	virtual ~WidgetResourceBinding();

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual std::vector<std::string> getResourceList(std::string prefix, ::fs::ListFlags flags) const override;

	virtual void pollChanges() override;

private:
	Widget & widget;

	bool hasSource() const;
	::res::AbstractSource & getSource() const;

	Callback<StateEvent> stateCallback;
	Callback<::res::ResourceEvent> resourceCallback;
};

}

#endif
