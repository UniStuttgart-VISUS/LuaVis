#include <Client/GUI3/Widgets/Panels/FillPanel.hpp>

namespace gui3
{

FillPanel::FillPanel()
{
	myFillCallback = addStateCallback(
	    [this](StateEvent event) {
		    updateSize();
	    },
	    StateEvent::ParentChanged | StateEvent::ParentBoundsChanged);
}

FillPanel::FillPanel(Ptr<Widget> widget) :
	FillPanel()
{
	add(widget);
}

FillPanel::~FillPanel()
{
}

void FillPanel::add(Ptr<Widget> widget)
{
	own(widget);
}

void FillPanel::remove(Widget & widget)
{
	disown(widget);
}

void FillPanel::updateSize()
{
	if (getParent())
	{
		sf::FloatRect rect = getParent()->getContainerBoundingBox();
		rect = getCustomTransform().getInverse().transformRect(rect);
		setRect(rect);
	}

	for (Widget * widget : getContainedWidgets())
	{
		updateWidgetSize(*widget);
	}
}

void FillPanel::updateWidgetSize(Widget & widget)
{
	widget.setRect(getContainerBoundingBox());
}

}
