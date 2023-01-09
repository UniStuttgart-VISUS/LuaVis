#ifndef SRC_SHARED_CONTENT_SOURCEAGGREGATOR_HPP_
#define SRC_SHARED_CONTENT_SOURCEAGGREGATOR_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Utils/Event/CallbackManager.hpp>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace res
{

class SourceAggregator : public AbstractSource
{
public:
	SourceAggregator();
	virtual ~SourceAggregator();

	void addSource(std::shared_ptr<AbstractSource> source, int order, std::string label = "");
	void removeSource(AbstractSource & source);
	bool hasSource(AbstractSource & source) const;
	void clearSources();

	void setSourceOrder(AbstractSource & source, int order);
	int getSourceOrder(AbstractSource & source) const;

	void setSourceLabel(AbstractSource & source, std::string label);
	std::string getSourceLabel(AbstractSource & source) const;

	std::shared_ptr<AbstractSource> lookUpSourceByLabel(const std::string & label) const;
	std::vector<std::shared_ptr<AbstractSource>> getSources() const;

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual std::unique_ptr<Stream> openStream(const std::string & resourceName) override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual std::vector<std::string> getResourceList(std::string prefix, fs::ListFlags flags) const override;
	virtual std::string resolveToFileName(const std::string & resourceName) const override;

	virtual void pollChanges() override;

private:
	struct SourceEntry
	{
		SourceEntry() :
			source(nullptr),
			order(0),
			label(),
			callbackHandle()
		{
		}

		SourceEntry(std::shared_ptr<AbstractSource> source, int order, std::string label,
		            CallbackHandle<ResourceEvent> callbackHandle) :
			source(source),
			order(order),
			label(label),
			callbackHandle(std::move(callbackHandle))
		{
		}

		std::shared_ptr<AbstractSource> source;
		int order;
		std::string label;
		Callback<ResourceEvent> callbackHandle;
	};

	std::size_t findSource(AbstractSource & source) const;
	void sort();

	std::vector<SourceEntry> sourceEntries;
};

}

#endif
