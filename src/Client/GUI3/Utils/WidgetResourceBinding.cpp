#include <Client/GUI3/Application.hpp>
#include <Client/GUI3/ResourceManager.hpp>
#include <Client/GUI3/Utils/WidgetResourceBinding.hpp>
#include <Client/GUI3/Widget.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <memory>

namespace gui3
{

WidgetResourceBinding::WidgetResourceBinding(Widget & widget) :
	widget(widget)
{
	stateCallback = widget.addStateCallback(
	    [this](StateEvent event) {
		    resourceCallback.remove();
		    if (hasSource())
		    {
			    resourceCallback = getSource().addCallback(
			        [this](::res::ResourceEvent event) {
				        fireEvent(event);
			        },
			        ::res::ResourceEvent::Any);
		    }
		    fireEvent(::res::ResourceEvent::MultipleResourcesChanged);
	    },
	    StateEvent::ParentApplicationChanged);
}

WidgetResourceBinding::~WidgetResourceBinding()
{
}

bool WidgetResourceBinding::loadResource(const std::string & resourceName, std::vector<char> & dataTarget)
{
	return hasSource() && getSource().loadResource(resourceName, dataTarget);
}

bool WidgetResourceBinding::resourceExists(const std::string & resourceName) const
{
	return hasSource() && getSource().resourceExists(resourceName);
}

std::vector<std::string> WidgetResourceBinding::getResourceList(std::string prefix, ::fs::ListFlags flags) const
{
	return hasSource() ? getSource().getResourceList(prefix, flags) : std::vector<std::string>();
}

void WidgetResourceBinding::pollChanges()
{
	// Parent application already does this for us.
}

bool WidgetResourceBinding::hasSource() const
{
	return widget.getParentApplication() && widget.getParentApplication()->getResourceManager().getSource();
}

::res::AbstractSource & WidgetResourceBinding::getSource() const
{
	return *widget.getParentApplication()->getResourceManager().getSource();
}

}
